/*
 * heap.c — alocador del heap (F2).
 *
 * Layout de cada objeto (HEAP_LAYOUT.md §2):
 *   header_addr ──┐
 *                 ├─ tag (u32 BE) : MARK_BIT | FREE_BIT | (type<<24) | reservados
 *                 ├─ length (u32 BE) : nº de elementos (arrays) o class_ptr (objetos)
 *                 ├─ payload (alineado a 4 bytes)
 *                 └─ ...
 *
 *   user_ref que ve el código BP = header_addr + 4
 *     (es decir, el "length" está en el slot [0] del payload visible).
 *
 * F2 v1 usa bump allocator simple desde `heap_start` hacia arriba. Cuando
 * el bump se queda sin espacio, corre un mark-sweep conservativo y
 * reintenta. Si tras GC sigue sin caber, error.
 *
 * F4 añade locking (synchronized(vmLock) equivalente). F2 es single-thread.
 */

#include "bpvm_internal.h"
#include "bpvm_nativa.h"   /* V7/H5 P2b.3: las natives paradas en su ejecutor, y el GC que piden */
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <setjmp.h>   /* #302 p3: volcar registros al escanear la pila de C del native */
#ifdef BPVM_GUI
#include "bpvm_gui.h"   /* #302: bpvm_gui_visit_roots (raíces GC del GUI) */
#endif

static uint32_t align4(uint32_t v) {
    return (v + 3) & ~3u;
}

/* Devuelve el tamaño total del bloque (header + payload alineado) según
 * el tag del header. Sirve tanto para objetos vivos como para libres. */
static uint32_t block_total_size(const bpvm_t* vm, uint32_t header_addr) {
    uint32_t tag    = bpvm_read_u32_be(vm->memory + header_addr);
    /* H3: bloque libre → su tamaño total (header incluido) está en +4, lo
     * escribe add_to_free_list. No recalcular desde type/length. */
    if (tag & BPVM_TAG_FREE_BIT) {
        return bpvm_read_u32_be(vm->memory + header_addr + 4);
    }
    uint32_t length = bpvm_read_u32_be(vm->memory + header_addr + 4);
    int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
    uint32_t payload;
    switch (type) {
    case BPVM_TYPE_ARRAY_I8:  payload = length;          break;
    case BPVM_TYPE_ARRAY_I16: payload = length * 2;      break;
    case BPVM_TYPE_ARRAY_I32: payload = length * 4;      break;
    case BPVM_TYPE_ARRAY_I64: payload = length * 8;      break;   /* H1.2 (V2) */
    case BPVM_TYPE_ARRAY_REF: payload = length * 8;      break;   /* H1.2a (V4): ref plana = 8 bytes */
    case BPVM_TYPE_OBJECT: {
        /* length = class_ptr absoluto. Leemos num_fields del descriptor. */
        uint16_t num_fields = bpvm_read_u16_be(vm->memory + length
                                                + BPVM_CLS_OFF_NUM_FIELDS);
        payload = (uint32_t) num_fields * 4;
        break;
    }
    default:
        /* Tipo desconocido: asumimos 0 payload para no progresar erróneamente. */
        payload = 0;
        break;
    }
    uint32_t total = align4(BPVM_OBJ_HEADER_SIZE + payload);
    if (total < BPVM_MIN_FREE_BLOCK) total = BPVM_MIN_FREE_BLOCK;
    return total;
}

/* V7/H7 #593 — lo mismo, para las pruebas que estropean una cabecera a la medida de ESTA VM (test_heapdescarrilado). */
uint32_t bpvm_heap_bloque_tam(const bpvm_t* vm, uint32_t header_addr) { return block_total_size(vm, header_addr); }

/* #355 — RETRATO DEL BLOQUE QUE MIENTE.
 *
 * Todo el recorrido del heap se apoya en que cada bloque diga la verdad sobre su
 * tamaño, y hay UN camino en block_total_size() que devuelve la palabra de +4 tal
 * cual, sin clamp ni validación: el de los bloques LIBRES. Si esa palabra se
 * corrompe, el recorrido salta a cualquier parte y todo lo que viene después —
 * mapa, marcado, barrido — es ruido.
 *
 * En la Pico el barrido sumó 3.272.343.552 bytes liberados sobre un heap de
 * 268 KB. Esa cifra dice QUE pasó pero no DÓNDE. Esto lo dice: la dirección, lo
 * que hay en la cabecera, y el bloque ANTERIOR — que es el principal sospechoso,
 * porque el que pisa una cabecera casi siempre es su vecino de la izquierda. */
static void bpvm_diag_bloque_mentiroso(const bpvm_t* vm, const char* quien,
                                       uint32_t cur, uint32_t prev, uint32_t total) {
    uint32_t t = bpvm_read_u32_be(vm->memory + cur);
    uint32_t l = bpvm_read_u32_be(vm->memory + cur + 4);
    bpvm_diag_urgente("[gc] !! BLOQUE MENTIROSO (%s) en %u: dice medir %u B y el heap entero "
              "son %u. tag=0x%08x len/size=%u tipo=%u libre=%u marcado=%u",
              quien, (unsigned) cur, (unsigned) total,
              (unsigned)(vm->heap_top - vm->heap_start),
              (unsigned) t, (unsigned) l,
              (unsigned)((t & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT),
              (unsigned)((t & BPVM_TAG_FREE_BIT) ? 1u : 0u),
              (unsigned)((t & BPVM_TAG_MARK_BIT) ? 1u : 0u));
    if (prev != 0) {
        uint32_t pt = bpvm_read_u32_be(vm->memory + prev);
        uint32_t pl = bpvm_read_u32_be(vm->memory + prev + 4);
        bpvm_diag("[gc]    el vecino anterior estaba en %u (%u B por delante): "
                  "tag=0x%08x len/size=%u tipo=%u libre=%u marcado=%u",
                  (unsigned) prev, (unsigned)(cur - prev), (unsigned) pt, (unsigned) pl,
                  (unsigned)((pt & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT),
                  (unsigned)((pt & BPVM_TAG_FREE_BIT) ? 1u : 0u),
                  (unsigned)((pt & BPVM_TAG_MARK_BIT) ? 1u : 0u));
    }
}

/* --- GC mark-sweep conservativo ---
 *
 * Mark phase:
 *   1. Para cada thread, escanea su stack [stack_base, sp) por palabras
 *      de 4 bytes que caigan en [heap_start, heap_next). Trata como ref
 *      potencial y marca el header.
 *   2. Trace recursivo para arrays REF y objetos.
 *
 * Sweep phase:
 *   3. Recorre el heap. Los headers sin MARK_BIT pasan a FREE.
 *      Quita MARK_BIT de los vivos.
 *
 * F2 v1: no compacta (no mueve objetos), no maneja free-list (los
 * bloques libres NO se reusan, sólo "perdidos" entre los vivos). Eso
 * es OK para el smoke; F2.b añadirá la free-list.
 */

/* --- Camino 1 (H-008, v3.0.1): validación de cabeceras reales ---------------
 *
 * El scan conservativo trata cualquier palabra de la pila como posible ref. Sin
 * validar, un ENTERO cuyo valor cae en el rango del heap se toma por puntero
 * (falsa raíz) y, como el MARK_BIT se escribe EN BANDA en la cabecera, PISA
 * datos vivos → corrupción no determinista. Espejo del set `valid` de la VM-Java
 * (`valid.contains(headerAddr)`): un bitmap de "esto es inicio de cabecera real"
 * reconstruido al empezar cada mark. Subsume GC-1: la cabecera de un
 * long[]/double[] real ENTRA en el bitmap (block_total_size la recorre) → se
 * reconoce; un entero a mitad de objeto NO entra → se rechaza. */
static int is_valid_header(const bpvm_t* vm, uint32_t header_addr);   /* abajo; la usa la comprobación de #593 */

static void build_gc_valid_map(bpvm_t* vm) {
    /* Dimensiona el bitmap por el heap USADO [heap_start, heap_next), NO por la
     * capacidad total (stack_base - heap_start): is_valid_header solo consulta
     * direcciones < heap_next, y build recorre hasta heap_next. Sizearlo por la
     * capacidad hacía un calloc de ~256 KB en el heap de 8 MB (PSRAM) del Metro
     * que COLGABA el firmware en cada gc() (regresión de Camino 1). El bitmap
     * crece con el uso real (el chequeo gc_valid_map_size<bytes solo re-aloca al
     * crecer). Verificado: Pico 128 KB no colgaba; Metro 8 MB sí. */
    vm->gc_descarrilo = 0;   /* V7 #494: lo levanta el guardián de abajo */
    uint32_t span  = vm->heap_next - vm->heap_start;       /* heap USADO */
    uint32_t words = span / 4u;                            /* nº de palabras de 4B */
    size_t   bytes = ((size_t) words + 7u) / 8u + 1u;      /* 1 bit por palabra + pad */
    if (vm->gc_valid_map == NULL || vm->gc_valid_map_size < bytes) {
        bpvm_free(vm->gc_valid_map);
        vm->gc_valid_map = (uint8_t*) bpvm_calloc(1, bytes);
        vm->gc_valid_map_size = vm->gc_valid_map ? bytes : 0;
    } else {
        memset(vm->gc_valid_map, 0, vm->gc_valid_map_size);
    }
    /* V7/H7 #598 — SIN MEMORIA PARA EL MAPA. Aquí ponía «is_heap_ref rechaza todo (conservador)», y era lo
     * contrario: sin mapa no se reconoce ninguna raíz, no se marca nada, y lo que venía después —cerrar los File
     * inalcanzables, matar los handles de lo no marcado, barrer— se llevaba el heap ENTERO, vivo incluido. En el PC
     * el malloc no falla; en una placa es el margen pequeño de la plataforma (la revisión de #595). Ahora es lo
     * mismo que un recorrido roto (#494): este GC no marca ni barre, y la reserva que lo pidió lanza la
     * prefabricada. El siguiente GC lo vuelve a intentar: si ya hay memoria, colecta como siempre. */
    if (vm->gc_valid_map == NULL) {
        vm->gc_descarrilo = 1;
        bpvm_diag_urgente("[gc] !! SIN MEMORIA para el mapa de cabeceras del GC (%u B): este GC no marca ni barre "
                          "(sin mapa lo barreria TODO) y la reserva que lo pidio lanza la excepcion del heap.",
                          (unsigned) bytes);
        return;
    }
    uint32_t cur = vm->heap_start;
    uint32_t prev = 0;
    uint32_t vistos = 0;   /* V7/H7 #593: los bloques no libres que ve el recorrido */
    while (cur < vm->heap_next) {
        uint32_t total = block_total_size(vm, cur);
        if (total == 0) break;
        /* #355 — EL AGUJERO DEL GUARDIÁN DE ABAJO. Ese sólo mira DÓNDE acaba el
         * recorrido, y `cur += total` es aritmética de 32 bits: un bloque que
         * declare un tamaño gigantesco hace que `cur` DÉ LA VUELTA y siga
         * paseando por el heap desde el principio. Si tras el rodeo aterriza por
         * casualidad en heap_next, el guardián se calla con el mapa hecho polvo.
         * Por eso llevamos dos tandas viéndolo mudo. Aquí se caza el tamaño
         * imposible EN EL BLOQUE, antes de que la suma tape el rastro. */
        if (total > vm->heap_top - vm->heap_start) {
            bpvm_diag_bloque_mentiroso(vm, "mapa", cur, prev, total);
            break;
        }
        uint32_t word = (cur - vm->heap_start) / 4u;
        vm->gc_valid_map[word / 8u] |= (uint8_t)(1u << (word % 8u));
        if ((bpvm_read_u32_be(vm->memory + cur) & BPVM_TAG_FREE_BIT) == 0u) vistos++;
        prev = cur;
        cur += total;
    }
    /* GUARDIÁN DEL INVARIANTE (permanente; coste = UNA comparación por GC).
     * El heap es una tira contigua de bloques y NINGUNO guarda su tamaño: se
     * RECALCULA con block_total_size desde type/length. Luego, si todos miden lo
     * que ese recálculo dice, este recorrido aterriza EXACTAMENTE en heap_next.
     *
     * Si no, ya se ha desincronizado: sigue leyendo payload como si fuera
     * cabecera, el mapa sale truncado → is_heap_ref rechaza objetos VIVOS → el
     * barrido se los lleva → "use-after-free" cientos de asignaciones más tarde
     * y sin rastro del origen (así se fueron varios días con MemT4: el
     * descarrilamiento era SILENCIOSO — ojo, el `break` de arriba NO lo cazaba,
     * block_total_size nunca devuelve 0 porque clampa a MIN_FREE_BLOCK).
     * Que grite aquí, en el sitio y el instante del destrozo.
     *
     * No abortamos: el mensaje ya identifica la causa, y matar la VM en placa
     * sería peor que dejar que el error normal de la VM siga su curso.
     *
     * V7 #494 (27-sep, Eduardo) — pero tampoco se sigue como si nada, que era
     * barrer con el mapa truncado: UNA EXCEPCIÓN. Se marca `gc_descarrilo`, este
     * GC no marca ni barre (gc_mark_phase, bpvm_gc_cuerpo) y la reserva que lo
     * pidió lanza la prefabricada «heap descarrilado» (bpvm_heap_alloc →
     * bpvm_throw_runtime_error). El detalle, este aviso. */
    if (cur != vm->heap_next) {
        vm->gc_descarrilo = 1;
        /* #355 — POR bpvm_diag, NO por stderr. Este guardián existía y describía
         * EXACTAMENTE el fallo que hemos perseguido todo el día... pero gritaba
         * por stderr, que en la placa no llega a ninguna parte. Llevaba horas
         * avisando al vacío mientras nosotros lanzábamos hipótesis.
         * (Otro de los 29 fprintf de #353, y el que más caro ha salido.)
         *
         * Se añade el ÚLTIMO bloque recorrido y lo que se leyó en él: sin eso el
         * aviso dice que hay un descarrilamiento pero no dónde, y el heap tiene
         * miles de bloques. */
        uint32_t tag = (cur + 8 <= vm->heap_top) ? bpvm_read_u32_be(vm->memory + cur) : 0;
        uint32_t len = (cur + 8 <= vm->heap_top) ? bpvm_read_u32_be(vm->memory + cur + 4) : 0;
        bpvm_diag_urgente("[gc] !! HEAP DESCARRILADO: el recorrido de cabeceras acabo en %u, "
                  "no en heap_next=%u. En %u hay tag=0x%08x len=%u (tipo=%u, libre=%u). "
                  "Un bloque mide distinto de lo que dice block_total_size() -> el mapa "
                  "sale truncado, is_heap_ref rechaza objetos VIVOS y el barrido se los lleva.",
                  (unsigned) cur, (unsigned) vm->heap_next, (unsigned) cur,
                  (unsigned) tag, (unsigned) len,
                  (unsigned)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT),
                  (unsigned)((tag & BPVM_TAG_FREE_BIT) ? 1u : 0u));
    }
    /* V7/H7 #593 (29-sep) — LO QUE EL RECORRIDO NO VE SOLO. Acabar en heap_next no basta: si un bloque miente y el
     * salto cae justo en el borde de otro, el recorrido se recompone y acaba donde debe, con lo saltado fuera del mapa
     * (medido al hacer #494; por ahí se coló #597 una vez de cada tres). Dos comprobaciones más, con lo que ya existe:
     * (1) la CUENTA —los bloques no libres que ve, contra los que lleva la reserva: bpvm_heap_nuevo suma, soltar un
     * owner resta y el barrido deja los que quedan—; contar bytes no valdría (el que miente se traga justo los bytes
     * de los que se salta); y (2) los HANDLES: todo handle vivo que apunta dentro del heap cae en una cabecera vista y
     * no libre (desde #593 todo bloque nace con el suyo). Si no, heap descarrilado (#494): la excepción, sin corregir
     * nada —Eduardo: «si se detecta en un bloque vivo y un programa funcionando se debería producir una excepción»—.
     * Espejo de buildValidObjectsSet (miVM). */
    if (!vm->gc_descarrilo) {
        if (vistos != vm->heap_bloques) {
            vm->gc_descarrilo = 1;
            bpvm_diag_urgente("[gc] !! HEAP DESCARRILADO: el recorrido ve %u bloques y la reserva lleva %u (#593): "
                              "se ha saltado bloques o se ha inventado alguno.",
                              (unsigned) vistos, (unsigned) vm->heap_bloques);
        } else if (vm->handles.addr != NULL) {
            for (uint32_t idx = 1u; idx < vm->handles.next; idx++) {
                uint32_t a = vm->handles.addr[idx];
                if (a == 0u) continue;                                    /* slot libre */
                uint32_t hh = a - 4u;
                if (hh < vm->heap_start || hh >= vm->heap_next) continue;   /* fuera del heap: una constante */
                if (!is_valid_header(vm, hh)
                        || (bpvm_read_u32_be(vm->memory + hh) & BPVM_TAG_FREE_BIT) != 0u) {
                    vm->gc_descarrilo = 1;
                    bpvm_diag_urgente("[gc] !! HEAP DESCARRILADO: el handle %u apunta a %u, donde el recorrido no "
                                      "vio ningún bloque vivo (#593).", (unsigned) idx, (unsigned) hh);
                    break;
                }
            }
        }
    }
}

/* ¿`header_addr` es el inicio de una cabecera de bloque real? Usa el bitmap
 * fresco del GC en curso. Rechaza desalineadas o fuera de rango. */
static int is_valid_header(const bpvm_t* vm, uint32_t header_addr) {
    if (vm->gc_valid_map == NULL) return 0;
    if (header_addr < vm->heap_start || header_addr >= vm->heap_next) return 0;
    uint32_t off = header_addr - vm->heap_start;
    if ((off & 3u) != 0) return 0;          /* no alineada a 4 → no es cabecera */
    uint32_t word = off / 4u;
    return (vm->gc_valid_map[word / 8u] >> (word % 8u)) & 1u;
}

static int is_heap_ref(const bpvm_t* vm, uint32_t v) {
    if (v < vm->heap_start || v >= vm->heap_next) return 0;
    if (v < vm->heap_start + 4) return 0;
    /* Camino 1 (H-008): un ref real apunta a user_ref = cabecera + 4. Validamos
     * que (v-4) sea una cabecera REAL, no la vieja heurística de tipo (que
     * aceptaba enteros a mitad de objeto como falsas raíces y los pisaba). */
    return is_valid_header(vm, v - 4);
}

/* ═══ #648 (4-oct) — EL MARCADO SIN RECURSIÓN ═════════════════════════════════
 *
 * Marcaba llamándose a sí mismo por cada referencia: una lista enlazada de N nodos bajaba N niveles, 32 B de pila
 * cada uno en la Pico, y una de ~360 desbordaba la tarea de la VM —STACK OVERFLOW y la placa colgada, sin decir nada
 * (medido el 4-oct, en la revisión de H12)—. Ahora, una PILA DE MARCADO explícita y acotada: marcar un bloque es
 * ponerle la marca y apilarlo; vaciar la pila es escanear los hijos de lo apilado (que se marcan y se apilan a su vez).
 * Una lista no pasa nunca de un elemento: la pila sólo crece con bloques ANCHOS (un array de referencias). Si se
 * llena, lo que no cabe queda marcado y sin escanear, se apunta, y al final de la fase de marcado se REPASA el heap:
 * por cada bloque marcado, sus hijos —y otra vuelta si se volvió a llenar—. La pila, estática: el GC para a todos
 * (stop-the-world) y no hay dos a la vez. Espejo de markObject en la VM-Java (allí la pila es de Java y no se llena). */
#define GC_PILA_N 256u
static uint32_t s_gc_pila[GC_PILA_N];   /* user_ref de bloques marcados con los hijos por escanear */
static uint32_t s_gc_pila_n = 0;
static int      s_gc_desbordada = 0;     /* alguno no cupo: sus hijos, en el repaso */

/* `ref_word` es una PALABRA de referencia tal cual vive en pila/heap: un HANDLE
 * tageado (idx|TAG en la palabra baja) o una dirección cruda/constante sin tag.
 * bpref_deref la resuelve a user_ref (tabla si tageada, identidad si no) — espejo del
 * refDeref del scanRegion de miVM. Sin esto, un handle (idx|TAG > heap_next) se
 * rechazaría como no-heap y NADA vivo se marcaría. Marca el bloque (si es uno real, vivo y sin marcar) y lo apila. */
static void gc_marca_uno(bpvm_t* vm, uint32_t ref_word) {
    bpref_t rr; rr.v = ref_word;
    uint32_t user_ref = bpref_deref(vm, rr);
    if (!is_heap_ref(vm, user_ref)) return;
    uint32_t header_addr = user_ref - 4;
    uint32_t tag = bpvm_read_u32_be(vm->memory + header_addr);
    if (tag & BPVM_TAG_MARK_BIT) return;   /* ya marcado */
    if (tag & BPVM_TAG_FREE_BIT) return;   /* libre, no marcar */
    bpvm_write_u32_be(vm->memory + header_addr, tag | BPVM_TAG_MARK_BIT);
    if (s_gc_pila_n < GC_PILA_N) s_gc_pila[s_gc_pila_n++] = user_ref;
    else s_gc_desbordada = 1;              /* marcado, y sus hijos en el repaso */
}

/* Los hijos de un bloque YA marcado: lo que antes era la recursión. */
static void gc_hijos(bpvm_t* vm, uint32_t user_ref) {
    uint32_t tag = bpvm_read_u32_be(vm->memory + user_ref - 4);
    int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
    uint32_t length = bpvm_read_u32_be(vm->memory + user_ref);

    if (type == BPVM_TYPE_ARRAY_REF) {
        for (uint32_t i = 0; i < length; i++) {
            /* H1.2a (V4): elemento ref = 8 bytes (stride 8); la dirección va en la
             * palabra baja (big-endian) → read_i64 y (uint32_t) toma los 32 bajos. */
            uint32_t slot = (uint32_t) bpvm_read_i64_be(vm->memory + user_ref + 4 + i * 8);
            gc_marca_uno(vm, slot);
        }
    } else if (type == BPVM_TYPE_OBJECT) {
        /* length = class_ptr. Lee field_bitmap del descriptor y traza
         * cada field marcado como ref. Sube por parent_offset para
         * incluir los heredados (sus bits viven en el bitmap del propio
         * descriptor también porque ModWriter los copia al heredar; pero
         * para descriptors cross-module L2 v3 con bitmap propio sólo,
         * tendríamos que subir aquí. F2 v1 hace sólo el bitmap directo
         * — F3 generaliza). */
        uint32_t cls_ptr = length;
        uint16_t num_fields = bpvm_read_u16_be(vm->memory + cls_ptr
                                                + BPVM_CLS_OFF_NUM_FIELDS);
        uint16_t bw = bpvm_read_u16_be(vm->memory + cls_ptr
                                        + BPVM_CLS_OFF_BITMAP_WORDS);
        uint32_t fbm_base = cls_ptr + BPVM_CLS_OFF_FIELD_BITMAP;
        for (uint32_t i = 0; i < num_fields; i++) {
            uint32_t word = bpvm_read_u32_be(vm->memory + fbm_base + (i / 32) * 4);
            if (word & (1u << (i & 31))) {
                /* H1.2a (V4): campo ref = 8 bytes (2 slots, bit en el slot base);
                 * dirección en la palabra baja → read_i64 + (uint32_t) low32. */
                uint32_t slot = (uint32_t) bpvm_read_i64_be(vm->memory + user_ref + 4 + i * 4);
                gc_marca_uno(vm, slot);
            }
        }
        (void) bw;
    }
}

/* Escanea lo apilado hasta que no quede nada (lo que se va marcando entra por arriba). */
static void gc_vacia(bpvm_t* vm) {
    while (s_gc_pila_n > 0) gc_hijos(vm, s_gc_pila[--s_gc_pila_n]);
}

/* Una RAÍZ: se marca y se escanea todo lo que alcanza. Conserva el nombre de siempre —lo usan como función las raíces
 * de la GUI y de los eventos—, pero ya no recurre (#648). */
static void mark_recursive(bpvm_t* vm, uint32_t ref_word) {
    gc_marca_uno(vm, ref_word);
    gc_vacia(vm);
}

/* #648 — EL REPASO, si la pila se llenó: por cada bloque marcado del heap, sus hijos (los que estén sin marcar se
 * marcan y se escanean); otra vuelta si se volvió a llenar. Recorre el heap como build_gc_valid_map, que ya lo validó
 * en este mismo GC. */
static void gc_repasa(bpvm_t* vm) {
    while (s_gc_desbordada) {
        s_gc_desbordada = 0;
        uint32_t cur = vm->heap_start;
        while (cur < vm->heap_next) {
            uint32_t total = block_total_size(vm, cur);
            if (total == 0 || total > vm->heap_top - vm->heap_start) break;   /* el mapa ya lo habría dicho */
            uint32_t tag = bpvm_read_u32_be(vm->memory + cur);
            if ((tag & BPVM_TAG_MARK_BIT) && !(tag & BPVM_TAG_FREE_BIT)) {
                gc_hijos(vm, cur + 4);
                gc_vacia(vm);
            }
            cur += total;
        }
    }
}

#ifdef BPVM_GUI
/* #302 — adaptador visitor→mark: gui.c enumera sus objptr (opacos para él) y
 * aquí los marcamos. Así gui.c sigue sin saber nada de la VM y mark_recursive
 * sigue siendo static. */
static void gui_mark_visit(void* ctx, uint32_t objptr) {
    mark_recursive((bpvm_t*) ctx, objptr);
}
#endif

static void gc_mark_phase(bpvm_t* vm) {
    /* Camino 1 (H-008): (re)construir el set de cabeceras reales ANTES de marcar,
     * para que el scan conservativo no tome enteros por punteros. */
    build_gc_valid_map(vm);
    /* V7 #494 — con el recorrido descarrilado el mapa está truncado: marcar con él (y luego barrer) se
     * llevaría objetos VIVOS. Este GC no hace nada más; la reserva que lo pidió lanza la excepción. */
    if (vm->gc_descarrilo) return;
    s_gc_pila_n = 0;          /* #648: la pila de marcado, vacía (un GC anterior la deja así, pero no se supone) */
    s_gc_desbordada = 0;
    /* 1. Stacks de threads (V7/H12, etapa 2: los de la cola, una vuelta). */
    for (const bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        int t = tc->id;
        uint32_t lo = tc->stack_base;
        uint32_t hi = tc->sp;
        /* #355 — QUE SE VEA LO QUE SE ESCANEA. El marcado daba 44 bytes vivos con
         * cuatro cadenas en uso, asi que o no mira donde debe, o mira bien y no
         * reconoce lo que hay. Estas dos cifras lo separan: cuantas PALABRAS
         * recorre y cuantas resultan ser referencias al heap. Si recorre 0, el
         * rango esta mal; si recorre miles y reconoce 0, el problema es el
         * reconocimiento. */
        uint32_t palabras = 0, refs = 0;
        for (uint32_t addr = lo; addr + 4 <= hi; addr += 4) {
            uint32_t v = bpvm_read_u32_be(vm->memory + addr);
            palabras++;
            {   /* ¿esta palabra apunta a un bloque real del heap? */
                bpref_t rr; rr.v = v;
                uint32_t ur = bpref_deref(vm, rr);
                if (is_heap_ref(vm, ur)) refs++;
            }
            mark_recursive(vm, v);
        }
        bpvm_diag("[gc] pila t%d: rango [%u..%u) = %u palabras, %u son refs al heap",
                  t, (unsigned) lo, (unsigned) hi, (unsigned) palabras, (unsigned) refs);
    }
    /* 2. allocAnchor por thread: objeto anclado durante el unwind de un
     *    RuntimeError (F5), raíz mientras no esté en la pila (H-001/GC-2). */
    for (const bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        uint32_t anchor = (uint32_t) tc->alloc_anchor;
        if (anchor != 0) mark_recursive(vm, anchor);
    }
    /* 2a-bis. #430 — la excepcion PREFABRICADA del OOM (idea de Eduardo):
     *     construida en el prologo del RUN, sin mas referencia que esta raiz.
     *     Tiene que sobrevivir TODAS las colectas para que lanzarla, cuando
     *     no queda memoria ni para contar el error, no aloje nada. */
    if (vm->oom_exc.v != 0) mark_recursive(vm, (uint32_t) vm->oom_exc.v);
    if (vm->heap_exc.v != 0) mark_recursive(vm, (uint32_t) vm->heap_exc.v);   /* V7 #494: la del heap descarrilado */
    /* 2b. #302 — raíces del GUI: los objptr ligados a widgets (bind_click) y la
     *     cola de eventos viven en GLOBALS C (gui.c), FUERA de vm->memory → el
     *     scan conservador no los ve. Sin esto, un objeto BP cuyo único holder
     *     es el widget se recolecta EN VIVO y el siguiente clic es un UAF real.
     *     Además, mantenerlo vivo hace sólido el regen del despacho: el slot del
     *     handle nunca se recicla mientras el widget lo retenga. */
#ifdef BPVM_GUI
    bpvm_gui_visit_roots(gui_mark_visit, vm);
#endif
    /* 2c. H5.c — la cola de eventos vive en el struct de la VM, fuera de
     *     vm->memory: el receptor y los argumentos-referencia de un evento
     *     pendiente no los ve el scan conservativo. Sin esto, un objeto cuyo
     *     único holder es un evento en cola se recolecta EN VIVO. */
    bpvm_event_mark_roots(vm, mark_recursive);
    /* 2d. #302 paso 3 — LA PILA DE C DEL NATIVE COMPILADO (idea de Eduardo:
     *     que el GC mire donde el native ya tiene sus handles, en vez de
     *     obligar al native a apartarlos a un shadow stack).
     *
     *     Si esta colecta la disparó una alocación hecha DESDE un thunk AOT
     *     (un helper como string_concat), los handles que el native retiene
     *     están en sus locales de C y en registros — y hasta hoy no los veía
     *     nadie: `make test-aotgc` demostró que un intermedio de
     *     `"valor " + intToString(n)` se reciclaba en mitad de la expresión,
     *     imprimiendo NULs con status=OK. Corrupción MUDA, y en host.
     *
     *     El rango: desde este frame (el GC corre más hondo en la MISMA pila:
     *     native → helper → alloc → gc) hasta el techo que apuntó
     *     `aot_call_guarded` al entrar al thunk más externo. Se recorre
     *     palabra a palabra dándoselo a `mark_recursive`, que ya valida — es
     *     EXACTAMENTE lo que el paso 1 hace con la pila BP, que también es
     *     conservadora. Un falso positivo retiene de más una colecta;
     *     este GC no compacta, así que retener nunca corrompe. (Heap.compact
     *     sí mueve, #595, pero nunca con una native a medias: la rechaza.)
     *
     *     El `setjmp` no es un salto: es el truco clásico (Boehm) para VOLCAR
     *     los registros preservados a la pila. Un handle puede vivir SOLO en
     *     un registro que nadie del camino native→GC haya tocado; setjmp los
     *     guarda todos en `regs`, que es un local de este frame y cae dentro
     *     del rango escaneado.
     *
     *     Con el callctx sin thunk activo (tc==NULL), cero trabajo: los
     *     programas sin AOT no pagan nada. Y en SMP el callctx es TLS por
     *     worker, y el STW garantiza que los demás workers están aparcados en
     *     frontera de quantum — nunca a mitad de un native. */
    /* 2e. V7/H5 P2b.1.c / P2b.3 — LAS NATIVES PARADAS EN SU EJECUTOR: suspendidas, o esperando a que el
     *     worker corra este mismo GC por ellas. Su estado de C —sus variables, y los registros, que volcó un
     *     setjmp antes de pasar el testigo— está en la pila del ejecutor, desde donde se paró hasta el techo
     *     que apuntó su native más externa. Nadie más lo ve: la native no corre, así que el escaneo de abajo
     *     (el del native en curso) no la cubre. Palabra a palabra y conservador, como ese. */
    for (const bpvm_thread_t* th = bpvm_hilos_primero(vm); th; th = bpvm_hilos_siguiente(vm, th)) {
        int t = th->id;
        const struct bpvm_nativa* nv = th->nativa;
        if (nv == NULL || (nv->estado != BPVM_NATIVA_SUSPENDIDA && nv->estado != BPVM_NATIVA_PIDE_GC)) continue;
        if (nv->cc_nativa.cstack_hi == 0) {
            /* Sin marcos de native vivos: pide el GC tras salir de ella (construyendo la excepción con la que
             * lanzó, ya fuera del longjmp). No hay pila que escanear. */
            bpvm_diag("[gc] el ejecutor del hilo %d pide este GC sin native viva (lanzando su excepción)", t);
            continue;
        }
        {
            uintptr_t lo = (nv->sp_parado + 3u) & ~(uintptr_t) 3u;
            uintptr_t hi = nv->cc_nativa.cstack_hi;
            uint32_t palabras = 0;
            for (uintptr_t a = lo; a + 4u <= hi; a += 4u) {
                uint32_t v;
                memcpy(&v, (const void*) a, 4);
                palabras++;
                mark_recursive(vm, v);
            }
            bpvm_diag("[gc] pila C de la native %s del hilo %d (en su ejecutor): [%p..%p) = %u palabras",
                      nv->estado == BPVM_NATIVA_PIDE_GC ? "que pide este GC" : "SUSPENDIDA",
                      t, (void*) lo, (void*) hi, (unsigned) palabras);
        }
    }
    {
        bpvm_aot_callctx_t* cc = bpvm_aot_callctx();
        if (cc->tc != NULL && cc->cstack_hi != 0) {
            jmp_buf regs;
            if (setjmp(regs) == 0) { /* volcado de registros, no un salto */ }
            uintptr_t lo = (uintptr_t) &regs;
            uintptr_t marca = (uintptr_t) &cc;         /* otro local: el menor manda */
            if (marca < lo) lo = marca;
            lo = (lo + 3u) & ~(uintptr_t) 3u;
            uintptr_t hi = cc->cstack_hi;
            uint32_t palabras = 0, refs = 0;
            for (uintptr_t a = lo; a + 4u <= hi; a += 4u) {
                uint32_t v;
                memcpy(&v, (const void*) a, 4);        /* palabra NATIVA, no BE */
                palabras++;
                {   bpref_t rr; rr.v = v;
                    uint32_t ur = bpref_deref(vm, rr);
                    if (is_heap_ref(vm, ur)) refs++;
                }
                mark_recursive(vm, v);
            }
            /* V7/H5 P2b.3 — aquí sólo llega una native que corre en la pila del WORKER (sin ejecutor, por
             * falta de memoria): las de un ejecutor le piden el GC al worker y se escanean arriba (2e). Que lo
             * diga: es la forma de saber, en el artefacto, por qué pila corre una native. */
            bpvm_diag("[gc] pila C del native (en la del worker, sin ejecutor): [%p..%p) = %u palabras, %u refs",
                      (void*) lo, (void*) hi, (unsigned) palabras, (unsigned) refs);
        }
    }
    /* 3. GC-2: data blocks de módulo. Consts + globales de módulo viven en
     *    [data_start, code_start) (crecen hacia atrás desde CS). Un global que
     *    apunte a heap es una RAÍZ; sin escanearlo se recolecta en vivo (UAF).
     *    Conservador, como las pilas. Espejo del getDataRegions de la VM-Java. */
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        uint32_t lo = m->data_start;
        uint32_t hi = m->data_start + m->data_size;
        for (uint32_t addr = lo; addr + 4 <= hi; addr += 4) {
            uint32_t v = bpvm_read_u32_be(vm->memory + addr);
            mark_recursive(vm, v);
        }
    }
    /* #648 — si la pila de marcado se llenó con alguna raíz, lo marcado y sin escanear: el repaso. */
    gc_repasa(vm);
}

/* #355 — ¿SE FUSIONAN DE VERDAD? (pregunta de Eduardo). Lo escribí, no lo probé.
 * OJO AL INTERPRETAR: durante el BARRIDO casi no debe haber fusiones — ese ya
 * junta los huecos seguidos en su propio recorrido, y los que da de alta están
 * separados por bloques VIVOS. Donde tiene que verse es en las liberaciones de
 * FUERA del GC (owner-free / OP_FREE_REF) y en el split del alocador. */
static uint32_t g_fus_izq = 0, g_fus_der = 0, g_altas = 0;

/* Añade un bloque libre [tag FREE][size@+4][next@+8] a la lista, ORDENADO POR
 * DIRECCIÓN y FUSIONANDO con los vecinos contiguos.
 *
 * ── #355 · propuesta de Eduardo, 2-ago ──────────────────────────────────────
 *
 * ANTES se insertaba en la CABEZA. Y el barrido recorre el heap de menor a mayor
 * dirección, así que la lista acababa ordenada AL REVÉS: de mayor a menor. Como
 * try_allocate_inner hace first-fit desde la cabeza, eso significaba servir
 * SIEMPRE desde el bloque más alto — el gran bloque menguante pegado al techo,
 * que es justo el que no hay que tocar.
 *
 * El daño real no era el desperdicio, era este:
 *
 *     if (pend_start != 0) vm->heap_next = pend_start;   // en gc_sweep_phase
 *
 * el bump SÓLO retrocede si el último hueco llega hasta heap_next. Sirviendo
 * desde arriba dejábamos objetos vivos pegados al techo en cada vuelta → la cola
 * nunca quedaba libre → `heap_next` NO RETROCEDÍA JAMÁS. De ahí el "recupera
 * 0 B" que salía en todos los logs de la placa, colecta tras colecta, con 48
 * bytes vivos.
 *
 * AHORA la lista va de menor a mayor. El first-fit coge de abajo, los objetos se
 * empaquetan en la parte baja, y el gran bloque queda al final como último
 * recurso — que es exactamente lo que hay que hacer para que la cola del heap
 * quede libre y el bump pueda volver atrás.
 *
 * La FUSIÓN viene de propina y hace falta igual: sin ella, dos huecos contiguos
 * liberados en momentos distintos (OP_FREE_REF, owner-free) se quedan como dos
 * entradas que ninguna petición mediana puede usar, aunque juntas sobren. El
 * barrido ya fusiona lo que encuentra seguido en UNA pasada; esto lo fusiona
 * también entre pasadas. Coste: recorrer la lista, que es corta por construcción
 * (una entrada por hueco, y fusionar las reduce). */
static void add_to_free_list(bpvm_t* vm, uint32_t addr, uint32_t size) {
    uint8_t* mem = vm->memory;
    /* #355 — ÚLTIMA PUERTA. Esta palabra de +4 es la ÚNICA que block_total_size()
     * devuelve tal cual, sin clamp ni recálculo (camino de bloque libre). Todo el
     * recorrido del heap se apoya en ella, así que un valor imposible aquí es un
     * descarrilamiento garantizado unas cuantas reservas más tarde, ya sin rastro
     * de quién lo escribió. Que se cace EN LA ESCRITURA, no en la lectura. */
    if (size < BPVM_MIN_FREE_BLOCK || addr + size > vm->heap_top) {
        bpvm_diag_urgente("[gc] !! ALTA IMPOSIBLE en la lista de libres: bloque en %u de %u B "
                  "(minimo %u, y el heap acaba en %u). No se da de alta: dejarlo entrar "
                  "descarrilaria el recorrido del heap en el proximo GC.",
                  (unsigned) addr, (unsigned) size,
                  (unsigned) BPVM_MIN_FREE_BLOCK, (unsigned) vm->heap_top);
        return;
    }
    g_altas++;
    bpvm_write_u32_be(mem + addr,     BPVM_TAG_FREE_BIT);
    bpvm_write_u32_be(mem + addr + 4, size);

    /* Busca el sitio: el primer nodo con dirección MAYOR que la nuestra. */
    uint32_t prev = 0, cur = vm->free_list_head;
    while (cur != 0u && cur < addr) {
        prev = cur;
        cur  = bpvm_read_u32_be(mem + cur + 8);
    }

    /* Fusión con el vecino de la IZQUIERDA si es contiguo: crece prev y ya está
     * (no entra un nodo nuevo). Hay que releer su tamaño, que es el que manda. */
    if (prev != 0u) {
        uint32_t psize = bpvm_read_u32_be(mem + prev + 4);
        if (prev + psize == addr) {
            g_fus_izq++;
            bpvm_write_u32_be(mem + prev + 4, psize + size);
            addr = prev;                 /* el bloque fusionado es el de la izquierda */
            size = psize + size;
            /* y sigue siendo el mismo nodo de la lista: prev no cambia de sitio */
        } else {
            bpvm_write_u32_be(mem + prev + 8, addr);
            bpvm_write_u32_be(mem + addr + 8, cur);
        }
    } else {
        bpvm_write_u32_be(mem + addr + 8, cur);
        vm->free_list_head = addr;
    }

    /* Fusión con el vecino de la DERECHA si es contiguo: se traga `cur` y este
     * desaparece de la lista. Se hace DESPUÉS de la izquierda a propósito, para
     * que tres huecos seguidos acaben en uno solo en una sola llamada. */
    if (cur != 0u && addr + size == cur) {
        g_fus_der++;
        uint32_t csize = bpvm_read_u32_be(mem + cur + 4);
        uint32_t cnext = bpvm_read_u32_be(mem + cur + 8);
        bpvm_write_u32_be(mem + addr + 4, size + csize);
        bpvm_write_u32_be(mem + addr + 8, cnext);
    }
}

/* H-010 (v3.0.1): libera un bloque de objeto dejándolo CONSISTENTE (espejo del
 * freeOwnedObject de la VM-Java). Antes, OP_FREE_REF/OP_SET_FIELD_OWNER solo
 * ponían FREE_BIT sin el tamaño en +4 → block_total_size leía el class_ptr como
 * tamaño y DESINCRONIZABA el recorrido del heap (el sweep y el build_gc_valid_map
 * de Camino 1). Aquí calculamos el tamaño ANTES de tocar la cabecera y lo
 * escribimos vía add_to_free_list ([FREE_BIT][size@+4][next]) → bloque caminable
 * y reutilizable de inmediato. */
/* #355 — SUELTA LA RESERVA DE EMERGENCIA (idea de Eduardo).
 *
 * La llama el camino del THROW, no el de la reserva normal, y esa distincion es
 * TODO el diseno: si se soltara al fallar una reserva cualquiera y se reintentara,
 * se la quedaria el programa —la peticion que fallo cabria y seguiria como si
 * nada— y cuando de verdad hiciera falta construir el error ya no habria nada.
 * La reserva existe para PODER AVISAR, no para estirar el heap un poco mas.
 *
 * Se gasta una vez por RUN. Devuelve los bytes soltados (0 si ya no habia). */
uint32_t bpvm_heap_release_reserve(bpvm_t* vm) {
    uint32_t r = vm->heap_reserve;
    if (r != 0u) {
        vm->heap_reserve = 0u;
        bpvm_diag("[bpvm] heap lleno: se suelta la reserva de emergencia (%u B) "
                  "para poder construir el error", (unsigned) r);
    }
    return r;
}

void bpvm_heap_free_block(bpvm_t* vm, uint32_t header_addr) {
    /* V7/H7 #603 (29-sep) — BAJO EL CERROJO: la lista de libres es de todos los workers. Aquí llega el FREE_REF de
     * un `var owner` (bpvm_free_owned, interp.c), que corre SIN cerrojo, mientras el alocador de otro worker recorre
     * la misma lista bajo él (bpvm_heap_alloc): con --smp=2, dos hilos soltando owners rompían el heap 10 de cada 10
     * veces (samples/OwnerSmp.bp). El barrido y la compactación también la tocan, pero con el mundo parado y el
     * cerrojo ya tomado; ésta es la única entrada desde el bytecode. Con un worker, el cerrojo no hace nada. */
    bpvm_smp_lock(vm);
    uint32_t size = block_total_size(vm, header_addr);
    if (size != 0) {   /* defensivo: 0 = no sabemos el tamaño, no tocar */
        add_to_free_list(vm, header_addr, size);
        vm->heap_bloques--;   /* V7/H7 #593: un bloque menos */
    }
    bpvm_smp_unlock(vm);
}

/* H3 (V2): sweep que RECONSTRUYE la free-list coalesciendo runs de bloques
 * libres/muertos adyacentes, y RETROCEDE heap_next si el run final lo toca
 * (devuelve memoria al bump sin compactar). Espejo del gcLocked de la VM-Java. */
/* #355 — ¿SE REUTILIZA LA MEMORIA LIBERADA, O NO?
 *
 * En la placa el GC libera 33.724 B a la lista de libres... y acto seguido
 * `heap_next` sube otros 33.768 B, que es clavado el umbral de la siguiente
 * colecta. Con 48 bytes vivos. La lectura de esos dos numeros es que el
 * recolector SI libera y el alocador NO recoge — pero es una lectura, no una
 * medida, y ya nos ha pasado hoy dar por bueno un razonamiento asi.
 *
 * Estos tres contadores lo contestan sin interpretar nada: cuantas reservas
 * salen de la lista de libres, cuantas del bump, y cuantas RECHAZAN un bloque
 * suficientemente grande porque el sobrante seria una astilla no representable
 * (la regla que arreglo el UAF de #352 y que, si muerde mucho, deja la lista
 * llena de huecos que nadie puede usar). Se imprimen en la linea del GC. */
static uint32_t g_alloc_de_lista = 0;   /* servidas desde la lista de libres */
static uint32_t g_alloc_de_bump  = 0;   /* servidas ampliando el heap */
static uint32_t g_alloc_astilla  = 0;   /* bloques descartados por sobrante no representable */

static void gc_sweep_phase(bpvm_t* vm) {
    uint8_t* mem = vm->memory;
    vm->free_list_head = 0;
    uint32_t cur = vm->heap_start;
    uint32_t freed = 0, kept = 0;
    uint32_t quedan = 0;                      /* V7/H7 #593: los bloques que siguen vivos, la cuenta nueva */
    uint32_t pend_start = 0, pend_size = 0;   /* 0 = sin run pendiente (heap_start>0) */
    uint32_t prev_blk = 0;                    /* #355: el vecino de la izquierda del culpable */
    while (cur < vm->heap_next) {
        uint32_t total = block_total_size(vm, cur);
        /* V7/H7 #595 — un barrido cortado deja los bloques de detrás con la marca del GC puesta, y sin recorrer:
         * es un heap descarrilado (#494) aunque el mapa no lo viera. Se dice, y nadie lo compacta después (la
         * compactación toma el bit 31 por un bloque enhebrado; la revisión de #595). */
        if (total == 0) { vm->gc_descarrilo = 1; break; }
        /* #355 — el mismo cazador que en el mapa. Si salta AQUÍ y no allí, la
         * corrupción nace ENTRE los dos recorridos, y en medio sólo pasa una
         * cosa: el marcado escribiendo MARK_BIT en las cabeceras. Si salta en
         * los dos, ya venía de antes. Esa diferencia es la respuesta. */
        if (total > vm->heap_top - vm->heap_start) {
            bpvm_diag_bloque_mentiroso(vm, "barrido", cur, prev_blk, total);
            vm->gc_descarrilo = 1;   /* V7/H7 #595: ídem */
            break;   /* no seguir: el recorrido ya no significa nada */
        }
        uint32_t tag = bpvm_read_u32_be(mem + cur);
        int is_free     = (tag & BPVM_TAG_FREE_BIT) != 0;
        int is_unmarked = !is_free && !(tag & BPVM_TAG_MARK_BIT);
        if (is_free || is_unmarked) {
            if (pend_start == 0) { pend_start = cur; pend_size = 0; }
            pend_size += total;
            freed += total;
        } else {
            /* Vivo: cierra el run pendiente y limpia el mark. */
            if (pend_start != 0) { add_to_free_list(vm, pend_start, pend_size); pend_start = 0; }
            tag &= ~BPVM_TAG_MARK_BIT;
            bpvm_write_u32_be(mem + cur, tag);
            kept += total;
            quedan++;
        }
        prev_blk = cur;
        cur += total;
    }
    if (!vm->gc_descarrilo) vm->heap_bloques = quedan;   /* V7/H7 #593: lo que el barrido deja */
    if (pend_start != 0) {
        /* El run libre FINAL toca heap_next → retroceder (devolver al bump). */
        vm->heap_next = pend_start;
    }
    vm->last_gc_heap_next = vm->heap_next;
    vm->alloc_since_gc    = 0u;   /* #357: empieza a contar el siguiente ciclo */
    /* #355 — EL NUMERO QUE CONTESTA LA PREGUNTA, y llevaba todo el dia escondido
     * tras --trace y un fprintf que en placa no llega a ninguna parte.
     *
     * `vivo` = bytes que el MARCADO dio por alcanzables. Si sale ~0 mientras el
     * programa esta a media vuelta con sus cadenas en uso, el marcado NO VE LAS
     * RAICES y el barrido se lleva datos vivos por delante. Eso es exactamente
     * lo que delato en la Pico el "I32_TO_U8: valor fuera de rango 792" en el
     * MISMO milisegundo que una colecta que libero el heap entero: un byte de
     * cadena no puede valer 792; se leyo de memoria ya reciclada.
     *
     * Sale SIEMPRE, no solo con --trace, y por bpvm_diag para que llegue al log
     * de la placa. Una linea por colecta, y las colectas son pocas.
     * De paso cierra otro de los 29 fprintf de #353. */
    bpvm_diag("[gc] vivo=%u liberado=%u heap=[%u..%u) freelist=%s",
              (unsigned) kept, (unsigned) freed,
              (unsigned) vm->heap_start, (unsigned) vm->heap_next,
              vm->free_list_head ? "si" : "vacia");
    /* #355 — LA PREGUNTA DE EDUARDO, CONTESTADA CON NUMEROS: "se está agotando
     * la memoria sin que esta se recicle". Con 48 bytes vivos, un recolector que
     * hiciera su trabajo daria vueltas indefinidamente; da ~200 mas. Si
     * `de_lista` se queda pegado a cero mientras `de_bump` sube, el GC libera
     * pero el alocador NO recoge — y entonces el problema no esta en el
     * recolector sino en la lista de libres. `astilla` mide cuantas veces se
     * descarto un bloque que SI cabia porque el sobrante no era representable
     * (la regla que cerro el UAF de #352): si esa cifra es alta, la lista se
     * llena de huecos que nadie puede usar y el efecto es el mismo que no
     * reciclar. Acumulados desde el arranque; se leen por diferencia. */
    bpvm_diag("[gc] reservas: de_lista=%u de_bump=%u astilla=%u",
              (unsigned) g_alloc_de_lista, (unsigned) g_alloc_de_bump,
              (unsigned) g_alloc_astilla);
    {   /* #355 — la lista, contada: cuantos huecos quedan, el mayor, y si las
         * fusiones estan pasando de verdad. Si la lista crece sin parar mientras
         * fus_izq/fus_der se quedan a cero, la fusion NO funciona. */
        uint32_t n = 0, mayor = 0, suma = 0, c = vm->free_list_head;
        while (c != 0u && n < 100000u) {
            uint32_t sz = bpvm_read_u32_be(vm->memory + c + 4);
            n++; suma += sz; if (sz > mayor) mayor = sz;
            c = bpvm_read_u32_be(vm->memory + c + 8);
        }
        bpvm_diag("[gc] libres: %u huecos, %u B en total, el mayor %u | altas=%u "
                  "fusion_izq=%u fusion_der=%u",
                  (unsigned) n, (unsigned) suma, (unsigned) mayor,
                  (unsigned) g_altas, (unsigned) g_fus_izq, (unsigned) g_fus_der);
    }
}


/* #355 — el interruptor del recolector. Una línea, pero puesta sobre el MISMO
 * guarda que ya usaba la migración a handles (gc_suspended), que es el único
 * sitio por el que pasan las tres puertas del GC. Así "apagado" es apagado de
 * verdad y no queda un camino olvidado que siga recolectando. */
void bpvm_set_gc_enabled(bpvm_t* vm, int enabled) {
    if (vm) vm->gc_suspended = enabled ? 0 : 1;
}


static void bpvm_gc(bpvm_t* vm) {
    /* V4: GC suspendido durante la migración a handles — guarda en el NÚCLEO
     * (espejo del gcLocked de miVM): cubre gc_stw Y bpvm_heap_gc (builtin gc()).
     * Sin esto, el gc() manual corría y su escaneo conservativo no reconoce los
     * handles tageados → no marca nada → libera TODO lo vivo (ownerreassign). */
    if (vm->gc_suspended) return;
    /* V7/H5 P2b.3 — pedido DENTRO del ejecutor de una native: el marcado es recursivo (una lista larga es una
     * recursión igual de honda) y la pila del ejecutor es pequeña a propósito. Se le pasa el testigo al worker
     * con el encargo y corre en SU pila; la del ejecutor se escanea como la de una native parada (2e). */
    if (bpvm_aot_callctx()->ejecutor != NULL) {
        bpvm_nativa_gc_en_worker(vm);
        return;
    }
    bpvm_gc_cuerpo(vm);
}

/* V7/H5 P2b.3 — el GC en sí. Lo llama bpvm_gc y, por encargo de un ejecutor, el worker (sin el cerrojo del SMP:
 * el ejecutor que lo pide ya lo tiene tomado). */
void bpvm_gc_cuerpo(bpvm_t* vm) {
    gc_mark_phase(vm);
    if (vm->gc_descarrilo) return;   /* V7 #494: ni cerrar Files ni reciclar handles ni barrer */
    bpvm_files_gc(vm);         /* V7/H3 (#491): un File inalcanzable se cierra AQUÍ, con su bloque intacto y su handle vivo */
    bpvm_handles_gc_sweep(vm); /* paso 6: recicla slots de tabla de lo inalcanzable (bpvm_handles.c) */
    gc_sweep_phase(vm);
    vm->gc_colectas++;         /* V7 (28-sep): Heap.getCollections */
}

/* V7 (28-sep) — lo que un programa puede preguntar del heap (Heap.bp; V7_IDEAS §H7). Lee contadores: no reserva nada
 * ni recorre el heap, sólo la lista de libres (como la línea «[gc] libres» de gc_sweep_phase, con el mismo tope). El
 * techo que ve un programa es heap_top menos la reserva de emergencia (#355), que no se le presta. */
void bpvm_heap_cuentas(bpvm_t* vm, bpvm_heap_cuentas_t* out) {
    memset(out, 0, sizeof *out);
    bpvm_smp_lock(vm);
    uint32_t techo = (vm->heap_top > vm->heap_reserve) ? vm->heap_top - vm->heap_reserve : vm->heap_top;
    uint32_t final = (techo > vm->heap_next) ? techo - vm->heap_next : 0u;
    out->tamano = (techo > vm->heap_start) ? techo - vm->heap_start : 0u;
    out->libre = final;
    out->mayor_libre = final;
    uint32_t c = vm->free_list_head;
    while (c != 0u && out->huecos < 100000u) {
        uint32_t sz = bpvm_read_u32_be(vm->memory + c + 4);
        out->huecos++;
        out->libre += sz;
        if (sz > out->mayor_libre) out->mayor_libre = sz;
        c = bpvm_read_u32_be(vm->memory + c + 8);
    }
    uint32_t repartidos = (vm->handles.next > 0u) ? vm->handles.next - 1u : 0u;   /* el slot 0 es el null */
    out->handles = (repartidos > vm->handles.free_top) ? repartidos - vm->handles.free_top : 0u;
    out->colectas = vm->gc_colectas;
    bpvm_smp_unlock(vm);
}

/* V6/U5 — la TABLA DE HANDLES vive en src/bpvm_handles.c: registro, baja,
 * crecimiento (dentro del bloque, hacia abajo, #451) y barrido del GC. Aquí
 * sólo quedan sus dos llamadas: bpvm_handles_gc_sweep (en bpvm_gc) y
 * bpvm_handles_grow (en la puerta de heap_alloc). */

/* El mundo parado (SMP): se pide y se espera a que el único worker en marcha sea el que llama. Asume vm_lock
 * tomado. Lo comparten gc_stw y la compactación (V7/H7 #595). */
static void stw_parar(bpvm_t* vm) {
    if (!vm->smp) return;
    vm->smp->stop_the_world = true;
    bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
    while (vm->smp->running_workers > 1) {
        bpvm_platform_cond_wait(&vm->smp->sched_cond, &vm->smp->vm_lock);
    }
}
static void stw_seguir(bpvm_t* vm) {
    if (!vm->smp) return;
    vm->smp->stop_the_world = false;
    bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
}
/* Si OTRO worker ya ha parado el mundo (está colectando), el que llega no puede esperarlo como un worker en marcha:
 * se descuenta de `running_workers` mientras espera a que acabe (ver gc_stw). Asume vm_lock tomado. */
static void stw_esperar_al_otro(bpvm_t* vm) {
    vm->smp->running_workers--;
    bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
    while (vm->smp->stop_the_world) {
        bpvm_platform_cond_wait(&vm->smp->sched_cond, &vm->smp->vm_lock);
    }
    vm->smp->running_workers++;
}

/* V7/H7 #610 (29-sep) — LA TABLA CRECE CON EL MUNDO PARADO. bpvm_handles_grow la MUEVE —un memmove dentro del heap:
 * el addr[] nuevo cae encima del gen[] viejo, y el final del addr[] viejo se pone a cero— y los demás workers la leen
 * sin cerrojo (bpref_deref, bpvm_ref_dead). Quien leyera a mitad encontraría una dirección donde espera una generación
 * (un use-after-free falso) o un cero donde espera una dirección: medido en la prueba de estrés de #593 (TablaCrece
 * con --nogc: 58 y 93 de 150 con 2 y 4 workers). Así que se crece como se colecta, con los demás parados, también con
 * el GC apagado. Si otro worker ya ha parado el mundo, se le deja acabar y se vuelve a mirar si hay sitio. Asume
 * vm_lock tomado. Sólo la llama la puerta de la reserva (tabla_con_sitio), donde quien reserva aún no tiene nada a
 * medias: esperar dentro de la ventana entre la reserva y el alta del handle corrompe (medido: V7_IDEAS §H7 #593). */
static int tabla_crecer(bpvm_t* vm) {
    if (vm->smp) {
        if (vm->smp->stop_the_world) {
            stw_esperar_al_otro(vm);
            if (vm->handles.free_top > 0u || vm->handles.next < vm->handles.cap) return 1;
        }
        stw_parar(vm);
    }
    int r = bpvm_handles_grow(vm);
    stw_seguir(vm);
    return r;
}

/* H3: GC stop-the-world. Asume vm_lock tomado. Lo usan el disparo proactivo
 * por umbral y la ruta de OOM. En legacy/single-worker no hay baile. */
static void gc_stw(bpvm_t* vm) {
    if (vm->gc_suspended) return;   /* V4: GC suspendido durante la migración a handles */
    if (vm->smp) {
        /* ── UN SOLO COLECTOR ──────────────────────────────────────────────────
         *
         * Sin esto, DOS workers podian entrar aqui a la vez y quedarse esperando
         * el uno al otro: cada uno ponia stop_the_world y se dormia hasta que
         * `running_workers` bajara de 1 — pero los dos SON workers en ejecucion
         * (el contador se sube antes de soltar el vm_lock para correr el quantum),
         * y ninguno puede bajarlo sin salir del quantum, que es justo lo que no van
         * a hacer. Cuelgue mudo, y con su huella: los DOS imprimen «GC por TABLA de
         * handles» y ahi se acaba la salida. Medido con --smp=2 y --smp=4.
         *
         * Asi que el que llega y encuentra una colecta en marcha NO colecta: cede.
         * Y mientras espera se DESCUENTA de `running_workers`, porque no esta
         * ejecutando bytecode — si no, el colector lo estaria esperando a el.
         * Al volver, el heap ya esta colectado por el otro: el llamante reintenta
         * su asignacion, que es lo que hace igualmente tras un GC propio. */
        if (vm->smp->stop_the_world) {
            stw_esperar_al_otro(vm);
            return;
        }
    }
    stw_parar(vm);
    bpvm_gc(vm);
    stw_seguir(vm);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V7/H7 #595 (28-sep) — Heap.compact(): COMPACTAR el heap a petición del programa. Eduardo: «me gustaría la
 * posibilidad de que el usuario pudiese pedir compactar el heap. Siempre en condiciones controladas, como por
 * ejemplo al terminar la aplicación». Mover un objeto es posible porque una referencia es un HANDLE (V4): basta
 * con cambiar su entrada de la tabla. Espejo de compactarLocked (miVM): el MISMO algoritmo y las mismas reglas
 * (qué se mueve y qué no); los heaps de las dos VMs no son iguales ni antes ni después.
 *
 * Precondición: el mundo parado y una colecta RECIÉN hecha (ningún bloque con la marca del GC; los libres, con
 * FREE_BIT). Sin memoria aparte —en una placa no la hay—: lo que hace falta saber vive, mientras dura, en la
 * propia tabla de handles y en las cabeceras.
 *  1. CLAVAR lo que no se puede mover aunque tenga handle: el bloque de estado de un fichero abierto
 *     (bpvm_files_clavar: el motor guarda ahí su estado, y littlefs lo enlaza en su lista de abiertos).
 *  2. ENHEBRAR la tabla en las cabeceras: cada bloque apuntado queda con la cadena de los índices que lo apuntan
 *     (casi siempre uno). Un bloque vivo SIN handle no se enhebra —nadie sabe quién lo apunta—: se queda en su
 *     sitio, como uno clavado.
 *  3. DESLIZAR en orden de dirección: un bloque enhebrado baja al primer sitio libre (el destino nunca pasa del
 *     origen: memmove) y cada índice de su cadena pasa a apuntar ahí; uno clavado se queda, y el hueco que le
 *     quede delante vuelve a la lista de libres. Lo que queda detrás del último bloque es el bump.
 * ═══════════════════════════════════════════════════════════════════════════ */
static void compactar(bpvm_t* vm) {
    uint8_t* mem = vm->memory;
    uint32_t antes = vm->heap_next;
    /* 1. Clavar el estado de los ficheros abiertos (los de un File inalcanzable ya los cerró la colecta). */
    bpvm_files_clavar(vm);
    /* 2. Enhebrar. */
    for (uint32_t idx = 1u; idx < vm->handles.next; idx++) {
        uint32_t a = vm->handles.addr[idx];
        if (a == 0u) continue;                                        /* slot libre */
        uint32_t h = a - 4u;
        if (h < vm->heap_start || h >= vm->heap_next) continue;       /* defensivo */
        uint32_t w = bpvm_read_u32_be(mem + h);
        if (!(w & BPVM_TAG_ENHEBRADO) && (w & (BPVM_TAG_FREE_BIT | BPVM_TAG_CLAVADO))) continue;   /* clavado (o libre) */
        vm->handles.addr[idx] = w;
        bpvm_write_u32_be(mem + h, BPVM_TAG_ENHEBRADO | idx);
    }
    /* 3. Deslizar. */
    vm->free_list_head = 0u;   /* la rehacen los huecos que queden delante de un clavado */
    uint32_t dst = vm->heap_start, cur = vm->heap_start;
    uint32_t movidos = 0, clavados = 0, huecos = 0;
    while (cur < vm->heap_next) {
        uint32_t w = bpvm_read_u32_be(mem + cur);
        uint32_t tag = w;
        while (tag & BPVM_TAG_ENHEBRADO) tag = vm->handles.addr[tag & ~BPVM_TAG_ENHEBRADO];   /* al final, la etiqueta */
        bpvm_write_u32_be(mem + cur, tag & ~BPVM_TAG_CLAVADO);   /* la cabecera vuelve a ser la de siempre */
        uint32_t total = block_total_size(vm, cur);
        if (tag & BPVM_TAG_FREE_BIT) { cur += total; continue; }
        if (w & BPVM_TAG_ENHEBRADO) {
            for (uint32_t c = w; c & BPVM_TAG_ENHEBRADO; ) {         /* cada índice de la cadena, al sitio nuevo */
                uint32_t idx = c & ~BPVM_TAG_ENHEBRADO;
                c = vm->handles.addr[idx];
                vm->handles.addr[idx] = dst + 4u;
            }
            if (dst != cur) { memmove(mem + dst, mem + cur, total); movidos++; }
            dst += total;
        } else {
            if (dst < cur) { add_to_free_list(vm, dst, cur - dst); huecos++; }
            clavados++;
            dst = cur + total;
        }
        cur += total;
    }
    vm->heap_next = dst;
    vm->last_gc_heap_next = dst;
    vm->alloc_since_gc = 0u;
    for (bpvm_thread_t* th = bpvm_hilos_primero(vm); th; th = bpvm_hilos_siguiente(vm, th))
        th->alloc_anchor = 0;   /* un ancla es transitoria */
    bpvm_diag("[gc] compactado: bump %u -> %u (%u bloques movidos, %u clavados, %u huecos)",
              (unsigned) antes, (unsigned) vm->heap_next,
              (unsigned) movidos, (unsigned) clavados, (unsigned) huecos);
}

int bpvm_heap_compactar(bpvm_t* vm, const bpvm_thread_t* tc) {
    int r = 0;
    bpvm_smp_lock(vm);
    /* Si otro worker está colectando, se le deja acabar; luego, el mundo parado es de la compactación. */
    if (vm->smp && vm->smp->stop_the_world) stw_esperar_al_otro(vm);
    stw_parar(vm);
    /* SIN OTROS HILOS VIVOS, mirado AQUÍ, con el mundo ya parado: fuera, entre mirar y parar, un worker podía
     * resucitar a un hilo con eventos pendientes (#342) y dejarlo a medias de un builtin o de una expresión con
     * una dirección cruda (la revisión de #595). Parado, los workers esperan antes de despertar a nadie. Un hilo
     * TERMINATED que aún debe eventos cuenta como vivo, como en miVM. */
    for (const bpvm_thread_t* o = bpvm_hilos_primero(vm); o && r == 0; o = bpvm_hilos_siguiente(vm, o)) {
        if (o != tc && (o->status != BPVM_THREAD_TERMINATED || bpvm_events_con_deuda(vm, o))) r = 2;
    }
    if (r == 0 && !vm->gc_suspended) {   /* con el GC apagado, nada: apagado es apagado (#355) */
        bpvm_gc_cuerpo(vm);
        if (vm->gc_descarrilo) { vm->reserva_descarrilada = 1; r = 1; }   /* #494: sin recorrido fiable, no */
        else compactar(vm);
    }
    stw_seguir(vm);
    bpvm_smp_unlock(vm);
    return r;
}

/* H3: intenta asignar `total` bytes. Devuelve la dirección de la cabecera, o
 * 0 si no cabe (heap_start>0 → 0 nunca es una cabecera válida). 1) free-list
 * first-fit con split; 2) bump desde heap_next. */
static uint32_t try_allocate_inner(bpvm_t* vm, uint32_t total) {
    uint8_t* mem = vm->memory;
    uint32_t prev = 0, cur = vm->free_list_head;
    while (cur != 0) {
        /* #355 — DETRÁS DEL HEAP ESTÁ LA PILA (pregunta de Eduardo, y la
         * respuesta era mala). El mapa es
         *
         *   [ módulos ][ HEAP →→→ ][ PILA del thread main ][ pilas del resto ]
         *               heap_start  stack_base
         *
         * o sea que el heap acaba EXACTAMENTE donde empiezan los locales y los
         * frames del programa. El camino de bump ya no puede cruzar (comprueba
         * el techo). Este SÍ podía: `cur` sale de una palabra LEÍDA DE LA
         * MEMORIA (el `next` del bloque anterior) y no se validaba contra nada.
         * Con esa palabra corrompida —la misma que en la Pico valía
         * 0xC30C0000— el recorrido se va a la zona de pilas, `return cur`
         * entrega esa dirección como bloque libre, y el que la pidió escribe su
         * objeto ENCIMA DE LA PILA. Corrupción a un sitio donde el GC ni mira.
         *
         * Aquí se corta: un nodo de la lista tiene que estar dentro del heap
         * USADO y medir algo representable que quepa. Si no, la lista está rota:
         * se abandona el recorrido (mejor reservar por bump que servir basura) y
         * se dice por el canal urgente, que es el que sobrevive a un cuelgue. */
        if (cur < vm->heap_start || cur + BPVM_MIN_FREE_BLOCK > vm->heap_top) {
            bpvm_diag_urgente("[gc] !! LISTA DE LIBRES ROTA: el nodo %u cae fuera del "
                              "heap [%u..%u). Detras del heap estan las PILAS: servir "
                              "ese bloque habria escrito encima de los locales del "
                              "programa. Se abandona la lista y se reserva por bump.",
                              (unsigned) cur, (unsigned) vm->heap_start,
                              (unsigned) vm->heap_top);
            vm->free_list_head = 0u;   /* la lista ya no significa nada */
            break;
        }
        uint32_t block_size = bpvm_read_u32_be(mem + cur + 4);
        uint32_t next = bpvm_read_u32_be(mem + cur + 8);
        if (block_size < BPVM_MIN_FREE_BLOCK || cur + block_size > vm->heap_top) {
            bpvm_diag_urgente("[gc] !! BLOQUE LIBRE IMPOSIBLE en %u: dice medir %u B y "
                              "acabaria en %u, pasado el final del heap (%u). Se abandona "
                              "la lista: entregarlo pisaria la pila del programa.",
                              (unsigned) cur, (unsigned) block_size,
                              (unsigned)(cur + block_size), (unsigned) vm->heap_top);
            vm->free_list_head = 0u;
            break;
        }
        if (block_size >= total) {
            uint32_t remaining = block_size - total;
            /* INVARIANTE DEL HEAP: un bloque ASIGNADO no guarda su tamaño en
             * ningún sitio — block_total_size lo RECALCULA desde type/length.
             * Luego el tamaño físico del bloque debe coincidir SIEMPRE con ese
             * recálculo, o el recorrido (build_gc_valid_map / gc_sweep_phase)
             * aterriza a mitad del siguiente y DESCARRILA en silencio.
             *
             * BUG (cazado 15-jul, repro `--mem=131072 samples/MemT4d_Count.bp`,
             * petaba en el concat nº 193 igual en host que en la Pico): si el
             * sobrante era < MIN_FREE_BLOCK (no representable como bloque libre)
             * se REGALABA al bloque asignado → ocupaba block_size pero
             * block_total_size decía `total` → recorrido corto → mapa de
             * cabeceras truncado → is_heap_ref rechazaba objetos VIVOS → el
             * barrido se los llevaba → USE-AFTER-FREE.
             *
             * FIX: solo aceptar el bloque si encaja EXACTO (remaining == 0) o si
             * el resto es un bloque libre representable. Un sobrante-astilla se
             * salta (se queda en la lista para una petición que le encaje). */
            if (remaining == 0u || remaining >= BPVM_MIN_FREE_BLOCK) {
                if (remaining >= BPVM_MIN_FREE_BLOCK) {
                    /* Split: usar [cur, cur+total); dejar el resto libre. */
                    uint32_t nf = cur + total;
                    bpvm_write_u32_be(mem + nf,     BPVM_TAG_FREE_BIT);
                    bpvm_write_u32_be(mem + nf + 4, remaining);
                    bpvm_write_u32_be(mem + nf + 8, next);
                    if (prev == 0) vm->free_list_head = nf;
                    else bpvm_write_u32_be(mem + prev + 8, nf);
                } else {
                    /* Encaje exacto: usar el bloque entero; quitarlo de la lista. */
                    if (prev == 0) vm->free_list_head = next;
                    else bpvm_write_u32_be(mem + prev + 8, next);
                }
                g_alloc_de_lista++;   /* #355: servida reciclando */
                vm->alloc_since_gc += total;   /* #357: el umbral cuenta VOLUMEN, no bump */
                return cur;
            }
            /* sobrante no representable → este bloque no sirve, seguir buscando */
            g_alloc_astilla++;   /* #355: cabia, pero se descarta por la astilla */
        }
        prev = cur;
        cur = next;
    }
    /* 2) Bump.
     *
     * #355 — RESERVA DE EMERGENCIA (idea de Eduardo). El techo NO es stack_base:
     * es stack_base menos `heap_reserve`. Ese hueco queda intocable para el
     * programa y sólo se suelta cuando la reserva ya ha fallado de verdad, para
     * que quede sitio con el que CONSTRUIR el error.
     *
     * Por qué hacía falta: lanzar el OOM necesita memoria (la cadena del mensaje
     * y el objeto RuntimeError). Con el heap lleno esa reserva falla también,
     * `bpvm_throw_runtime_error` devuelve nulo y no hay excepción que lanzar —
     * el programa se queda sin aviso justo cuando más falta hace. La pescadilla
     * clásica: te quedas sin memoria para avisar de que no hay memoria.
     *
     * Se hace bajando el TECHO y no reservando un bloque a propósito: así no
     * fragmenta, no hay que llevarle la cuenta a nadie y soltarla es poner un
     * campo a cero. */
    if (vm->heap_next + total > vm->heap_top - vm->heap_reserve) return 0;
    uint32_t addr = vm->heap_next;
    vm->heap_next += total;
    g_alloc_de_bump++;   /* #355: servida ampliando el heap, sin reciclar nada */
    vm->alloc_since_gc += total;   /* #357 */
    return addr;
}

/* --- API pública del heap ---
 *
 * V7/H7 #593 (29-sep) — LA reserva, en UN método (Eduardo: «al final la reserva la ha de hacer un método único, no
 * n métodos»). Hasta hoy la repartían el asignador —el tipo y un 0 donde va la longitud o la clase— y 23 sitios que
 * escribían esa palabra a mano y daban de alta el handle aparte, a veces después de llenar el contenido: correcto
 * porque todos lo hacían bien, no porque no pudiera salir mal. Ahora, bajo el cerrojo y de una vez: el hueco (abajo,
 * heap_reservar), la cabecera ENTERA, el contenido a cero, la cuenta del bloque y el handle. El bloque nace completo
 * y con su handle: ningún GC lo ve a medias, y la tabla de handles conoce todo bloque vivo (con eso, las dos
 * comprobaciones de build_gc_valid_map son exactas). Devuelve el handle —bpref_deref da la dirección, para llenar
 * el contenido— o nulo si no cabe, en el heap o en la tabla; quien llama lanza «No space in heap» y
 * bpvm_throw_runtime_error escoge la prefabricada que toque (#430, #494). Espejo de heapNuevo (miVM). */
static uint32_t heap_reservar(bpvm_t* vm, uint32_t payload_bytes, int type, uint32_t cabecera);

bpref_t bpvm_heap_nuevo(bpvm_t* vm, int type, uint32_t cabecera, uint32_t payload_bytes) {
    bpref_t h = bpref_null();
    bpvm_smp_lock(vm);
    uint32_t o = heap_reservar(vm, payload_bytes, type, cabecera);
    if (o != 0u) h = bpvm_handle_register_bajo_cerrojo(vm, o);
    bpvm_smp_unlock(vm);
    return h;
}

/* V7/H7 #610 — la puerta de la TABLA de handles, en su función porque se mira DOS veces en heap_reservar: antes de
 * reservar y tras la colecta del reintento (gc_stw suelta el cerrojo mientras espera, y otros workers pueden gastarse
 * los slots que había). El alta del handle va detrás de la reserva sin soltarlo, así que lo último que se mira es
 * esto: 1 = hay sitio para ella; 0 = no cabe, el OOM de siempre. Asume vm_lock tomado. */
static int tabla_con_sitio(bpvm_t* vm) {
    /* #430 — resolver la presion de tabla, si la habia. Dos salidas buenas:
     * la colecta rellena la free-list (recicla) o, si todo sigue VIVO, se
     * crece AQUI — donde fallar tiene camino digno: el mismo contrato
     * "return 0 = OOM atrapable" del heap lleno. El flag se desarma en las
     * dos; volvera a armarse solo al cruzar la marca siguiente. (El segundo
     * termino: mismo lleno-duro que arriba — la colecta de la puerta pudo no
     * reciclar nada y el flag estar en 0.) */
    if (vm->handles.pressure != 0 ||
        (vm->handles.cap != 0u && vm->handles.free_top == 0u &&
         vm->handles.next >= vm->handles.cap)) {
        if (vm->handles.free_top > 0u) {
            vm->handles.pressure = 0;          /* reciclado: presion resuelta */
        } else if (tabla_crecer(vm)) {   /* #610: con el mundo parado */
            vm->handles.pressure = 0;          /* todo vivo: tabla mas grande */
        } else {
            /* ⚠️ EL AVISO ESTABA MUDO (29-ago). Era un `static` de fichero, o sea
             * UNA VEZ POR ARRANQUE — no por ejecucion, que es lo que decia su
             * propio texto. En la Pico, con la placa encendida y varias pruebas
             * corridas, ya estaba gastado: `SyncListDiag` murio por la tabla y
             * el unico mensaje que lo decia no salio. Se buscó el fallo en el
             * heap durante media hora. Ahora vive en el `vm`, que nace a cero en
             * cada RUN (calloc).
             *
             * Y dice el heap LIBRE a proposito: el OOM que el programa recibe se
             * llama "No space in heap" —no se puede cambiar sin romper la
             * paridad con la VM-Java— asi que sin este numero al lado, el
             * mensaje manda a mirar precisamente donde no esta el problema. */
            if (!vm->handles.oom_avisado) {
                vm->handles.oom_avisado = 1;
                /* #451 — desde que la tabla vive DENTRO del bloque, quedarse sin
                 * handles ya significa quedarse sin memoria de verdad: la tabla
                 * bajo hasta tocar el heap. Ya no hay que aclarar "no es el
                 * heap", porque ahora si lo es — es la misma bolsa. */
                bpvm_diag_urgente("[bpvm] SIN SITIO para mas handles: %u slots ocupan "
                          "%u KB y la tabla ya toca el heap (vivo hasta %u, techo %u). "
                          "OOM atrapable.",
                          (unsigned) vm->handles.cap,
                          (unsigned)((vm->handles.cap * 8u) / 1024u),
                          (unsigned) vm->heap_next, (unsigned) vm->heap_top);
            }
            return 0;   /* mismo cauce que el heap lleno: el caller lanza OOM */
        }
    }
    return 1;
}

/* El hueco: el GC proactivo, la presión de la tabla, la lista de libres, el bump y el OOM. Con el vm_lock tomado
 * (bpvm_heap_nuevo); sólo la llama él. Devuelve user_ref (= header_addr + 4), con la cabecera entera y el
 * contenido a cero, o 0 si no cabe. */
static uint32_t heap_reservar(bpvm_t* vm, uint32_t payload_bytes, int type, uint32_t cabecera) {
    uint32_t total = align4(BPVM_OBJ_HEADER_SIZE + payload_bytes);
    if (total < BPVM_MIN_FREE_BLOCK) total = BPVM_MIN_FREE_BLOCK;

    /* H2 — Heap allocation y GC son críticos en SMP: bump pointer y
     * mark/sweep escriben estado compartido. Tomar el vm_lock global
     * serializa los workers contra heap. En legacy mode (vm->smp NULL)
     * el lock es no-op. V7/H7 #593: lo toma bpvm_heap_nuevo, para la reserva y el alta del handle juntas. */

    /* H3 (V2): GC PROACTIVO por umbral de crecimiento de bump. Evita el
     * over-commit (que el heap suba a su pico antes de colectar): si el bump
     * avanzó >= umbral desde el último GC, colecta ahora. gc_stw hace el baile
     * STW (mark scanea las pilas de todos los threads → deben estar en
     * safepoint con tc->sp sincronizado; en legacy/single-worker no hay baile).
     *
     * #430 — SEGUNDO EJE de presión: la TABLA DE HANDLES. El umbral #357 cuenta
     * VOLUMEN, y un programa de objetos chicos se le escapaba: 30.000 strings
     * son ~600 KB (bajo el umbral de 704 KB de la Metro) pero 30.000 slots — y
     * el salto de tabla a 65536 pide ~512 KB de SRAM que la Pico no tiene. Se
     * moría SIN haber colectado ni una vez (AotGcRt/AotGcRt2, 17-ago). El aviso
     * llega por la MARCA del final de la tabla (register arma handle_pressure
     * al repartir slot de la zona final) y dispara la MISMA colecta en la MISMA
     * puerta segura. Resolución abajo: reciclado ⇒ listo; todo vivo ⇒ crecer;
     * no se puede crecer ⇒ OOM atrapable. */
    {
        /* #430 — la presion de tabla llega como FLAG, no como aritmetica: la
         * MARCA al final de la tabla (idea de Eduardo) se cruza en register al
         * repartir un slot fresco de la zona final, y aqui solo se consulta.
         * Una colecta por cruce — el ritmo correcto sin heuristicas.
         * El segundo termino es el LLENO-DURO: los reusos de free-list no
         * rearman la marca, asi que la tabla puede llegar a llenarse del todo
         * con el flag en 0 (se vio con --handlecap=2048: el register de un
         * OP_NEW_OBJECT devolvia null y moria en INVOKE_VIRTUAL). Barato: solo
         * es verdad con la free-list seca. */
        int por_tabla = (vm->handles.pressure != 0) ||
                        (vm->handles.cap != 0u && vm->handles.free_top == 0u &&
                         vm->handles.next >= vm->handles.cap);
        int por_volumen = (vm->gc_bump_threshold != 0 &&
                           vm->alloc_since_gc >= vm->gc_bump_threshold);
        if (!vm->gc_suspended && (por_volumen || por_tabla)) {
            if (por_tabla)
                bpvm_diag("[bpvm] GC por TABLA de handles: %u/%u slots (marca cruzada)",
                          (unsigned) vm->handles.next, (unsigned) vm->handles.cap);
            /* #355 — QUE SE VEA SI EL GC RECICLA. Sin esto, una colecta que no
             * recupera nada es indistinguible de una que va bien: el heap se acaba
             * y el fallo aparece diez pasos mas alla, disfrazado de cadena vacia.
             *  es el bump antes de colectar; , tras el barrido (el
             * sweep RETROCEDE heap_next si la cola queda libre). Si los dos numeros
             * son iguales colecta a colecta, es que NO se esta reciclando. */
            uint32_t antes = vm->heap_next;
            gc_stw(vm);
            bpvm_diag("[bpvm] GC: bump %u -> %u (recupera %d B, tope %u, heap %u..%u)",
                      (unsigned) antes, (unsigned) vm->heap_next,
                      (int)((long) antes - (long) vm->heap_next),
                      (unsigned) vm->gc_bump_threshold,
                      (unsigned) vm->heap_start, (unsigned) vm->heap_top);
            if (vm->gc_descarrilo) {   /* V7 #494: esta reserva lanza la del heap descarrilado */
                vm->reserva_descarrilada = 1;
                return 0;
            }
        }
    }

    if (!tabla_con_sitio(vm)) return 0;   /* #430: la presión de la tabla, resuelta ANTES de reservar (#610) */

    uint32_t addr = try_allocate_inner(vm, total);
    if (addr == 0) {
        /* Sin sitio en free-list ni bump: STW GC y reintentar. */
        gc_stw(vm);
        if (vm->gc_descarrilo) {   /* V7 #494: no hay reintento; la del heap descarrilado */
            vm->reserva_descarrilada = 1;
            return 0;
        }
        /* V7/H7 #610 — la tabla, otra vez: gc_stw soltó el cerrojo mientras esperaba, y otros workers pueden haberse
         * gastado los slots que había. Aún no hay nada reservado: si hay que crecer, se puede esperar. */
        if (!tabla_con_sitio(vm)) return 0;
        addr = try_allocate_inner(vm, total);
        if (addr == 0) {
            /* #355 — EL OOM ERA MUDO: se devolvia 0 y el que llama decidia si
             * mirarlo. Los que no lo miran empujan una ref nula, y de ahi salen
             * cadenas VACIAS que se imprimen como lineas en blanco y revientan
             * al leerlas. Con handles el OOM es ATRAPABLE desde H1: esto tiene
             * que llegar ahi, no evaporarse. De momento GRITA. */
            /* #355 — LA RESERVA SE GASTA AQUI Y SOLO AQUI: una reserva que
             * falla MIENTRAS se construye una excepcion. Antes la soltaba el
             * throw nada mas entrar, y el log de la Pico lo delato — se la
             * llevaba el primer RuntimeError de cualquier clase y encima decia
             * "heap lleno" con el heap recien vaciado. La red es para cuando
             * de verdad no cabe el error, no para cada excepcion. */
            if (vm->building_error && vm->heap_reserve != 0u) {
                bpvm_heap_release_reserve(vm);
                addr = try_allocate_inner(vm, total);
                if (addr != 0) goto con_reserva;
            }
            /* #355 — CON FRENO. La 1a version de esto no lo tenia y fue un
             * ERROR CARO: con el heap lleno CADA intento de reserva ya hace DOS
             * barridos completos (el proactivo y el del reintento), y encima
             * escribia una linea de log. En un bucle, a 150 MHz y con 268 KB,
             * eso no parece lento: parece un CUELGUE. Lo vio Eduardo en la Pico.
             * El 1er aviso es el que informa; los demas solo estorban. */
            /* Mismo arreglo que el aviso de la tabla: era un `static`, o sea una
             * vez por ARRANQUE aunque el texto dijera "por ejecucion". Ahora en
             * el `vm`, que nace a cero en cada RUN. Este SI es el heap lleno de
             * verdad — y por eso importa que se distinga del otro. */
            if (!vm->heap_oom_avisado) {
                vm->heap_oom_avisado = 1;
                bpvm_diag_urgente("[bpvm] SIN MEMORIA en el heap DE VERDAD: pedidos %u B y no "
                          "caben (bump %u, tope de pila %u, libres %u KB). Este aviso sale "
                          "UNA vez por ejecucion; el que llamo deberia lanzar OOM atrapable.",
                          (unsigned) total, (unsigned) vm->heap_next,
                          (unsigned) vm->heap_top,
                          (unsigned)((vm->heap_top - vm->heap_next) / 1024u));
            }
            return 0;   /* OOM real: ni con la reserva de emergencia cabe */
        }
    }
con_reserva: ;

    uint32_t tag = ((uint32_t)(type & 0x3F)) << BPVM_TAG_TYPE_SHIFT;
    bpvm_write_u32_be(vm->memory + addr, tag);
    /* V7/H7 #593 — la cabecera ENTERA, aquí y no en quien llama: la longitud de un array o la clase de un
     * objeto. Antes se dejaba un 0 y la escribían a mano 23 sitios. */
    bpvm_write_u32_be(vm->memory + addr + 4, cabecera);

    uint32_t user_ref = addr + 4;
    /* Zero-inicializar el resto del payload (importante al reusar free-list). */
    if (payload_bytes > 0) {
        memset(vm->memory + user_ref + 4, 0, payload_bytes);
    }
    vm->heap_bloques++;   /* V7/H7 #593: un bloque más */
    return user_ref;
}

/* Helper: aloca un string a partir de una const-string C. H2 (V2): los
 * strings son TYPE_ARRAY_I8 con los bytes UTF-8 tal cual (1 byte/elem).
 * `len` = nº de bytes de `s`. */
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len) {
    bpref_t h = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, (uint32_t) len, (uint32_t) len);   /* V7/H7 #593 */
    if (h.v == 0u) return 0;
    uint32_t ref = bpref_deref(vm, h);
    for (size_t i = 0; i < len; i++) {
        vm->memory[ref + 4 + i] = (uint8_t) s[i];
    }
    return (uint32_t) h.v;   /* string: idx|TAG (gen=0); reuso de strings = paso 6 */
}

/* Trigger manual de GC. Devuelve bytes liberados (aproximado: el delta
 * entre heap_next antes y después es 0 porque F2 v1 no compacta, así
 * que devolvemos 0). */
void bpvm_heap_gc(bpvm_t* vm) {
    bpvm_smp_lock(vm);
    bpvm_gc(vm);
    bpvm_smp_unlock(vm);
}
