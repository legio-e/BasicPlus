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
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
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
static void build_gc_valid_map(bpvm_t* vm) {
    /* Dimensiona el bitmap por el heap USADO [heap_start, heap_next), NO por la
     * capacidad total (stack_base - heap_start): is_valid_header solo consulta
     * direcciones < heap_next, y build recorre hasta heap_next. Sizearlo por la
     * capacidad hacía un calloc de ~256 KB en el heap de 8 MB (PSRAM) del Metro
     * que COLGABA el firmware en cada gc() (regresión de Camino 1). El bitmap
     * crece con el uso real (el chequeo gc_valid_map_size<bytes solo re-aloca al
     * crecer). Verificado: Pico 128 KB no colgaba; Metro 8 MB sí. */
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
    if (vm->gc_valid_map == NULL) return;   /* sin memoria → is_heap_ref rechaza todo (conservador) */
    uint32_t cur = vm->heap_start;
    while (cur < vm->heap_next) {
        uint32_t total = block_total_size(vm, cur);
        if (total == 0) break;
        uint32_t word = (cur - vm->heap_start) / 4u;
        vm->gc_valid_map[word / 8u] |= (uint8_t)(1u << (word % 8u));
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
     * sería peor que dejar que el error normal de la VM siga su curso. */
    if (cur != vm->heap_next) {
        fprintf(stderr, "[gc] !! HEAP INCONSISTENTE: el recorrido de cabeceras acabó en %" PRIu32
                        ", no en heap_next=%" PRIu32 ". Hay un bloque cuyo tamaño real no coincide "
                        "con block_total_size() → el mapa del GC sale truncado y se barrerán "
                        "objetos vivos.\n", cur, vm->heap_next);
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

/* `ref_word` es una PALABRA de referencia tal cual vive en pila/heap: un HANDLE
 * tageado (idx|TAG en la palabra baja) o una dirección cruda/constante sin tag.
 * bpref_deref la resuelve a user_ref (tabla si tageada, identidad si no) — espejo del
 * refDeref del scanRegion de miVM. Sin esto, un handle (idx|TAG > heap_next) se
 * rechazaría como no-heap y NADA vivo se marcaría. */
static void mark_recursive(bpvm_t* vm, uint32_t ref_word) {
    bpref_t rr; rr.v = ref_word;
    uint32_t user_ref = bpref_deref(vm, rr);
    if (!is_heap_ref(vm, user_ref)) return;
    uint32_t header_addr = user_ref - 4;
    uint32_t tag = bpvm_read_u32_be(vm->memory + header_addr);
    if (tag & BPVM_TAG_MARK_BIT) return;   /* ya marcado */
    if (tag & BPVM_TAG_FREE_BIT) return;   /* libre, no marcar */
    bpvm_write_u32_be(vm->memory + header_addr, tag | BPVM_TAG_MARK_BIT);

    int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
    uint32_t length = bpvm_read_u32_be(vm->memory + user_ref);

    if (type == BPVM_TYPE_ARRAY_REF) {
        for (uint32_t i = 0; i < length; i++) {
            /* H1.2a (V4): elemento ref = 8 bytes (stride 8); la dirección va en la
             * palabra baja (big-endian) → read_i64 y (uint32_t) toma los 32 bajos. */
            uint32_t slot = (uint32_t) bpvm_read_i64_be(vm->memory + user_ref + 4 + i * 8);
            mark_recursive(vm, slot);
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
                mark_recursive(vm, slot);
            }
        }
        (void) bw;
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
    /* 1. Stacks de threads. */
    for (int t = 0; t < vm->thread_count; t++) {
        const bpvm_thread_t* tc = &vm->threads[t];
        uint32_t lo = tc->stack_base;
        uint32_t hi = tc->sp;
        for (uint32_t addr = lo; addr + 4 <= hi; addr += 4) {
            uint32_t v = bpvm_read_u32_be(vm->memory + addr);
            mark_recursive(vm, v);
        }
    }
    /* 2. allocAnchor por thread: objeto anclado durante el unwind de un
     *    RuntimeError (F5), raíz mientras no esté en la pila (H-001/GC-2). */
    for (int t = 0; t < vm->thread_count; t++) {
        uint32_t anchor = (uint32_t) vm->threads[t].alloc_anchor;
        if (anchor != 0) mark_recursive(vm, anchor);
    }
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
}

/* H3: añade un bloque libre [tag FREE][size@+4][next@+8] al head de la lista. */
static void add_to_free_list(bpvm_t* vm, uint32_t addr, uint32_t size) {
    uint8_t* mem = vm->memory;
    bpvm_write_u32_be(mem + addr,     BPVM_TAG_FREE_BIT);
    bpvm_write_u32_be(mem + addr + 4, size);
    bpvm_write_u32_be(mem + addr + 8, vm->free_list_head);
    vm->free_list_head = addr;
}

/* H-010 (v3.0.1): libera un bloque de objeto dejándolo CONSISTENTE (espejo del
 * freeOwnedObject de la VM-Java). Antes, OP_FREE_REF/OP_SET_FIELD_OWNER solo
 * ponían FREE_BIT sin el tamaño en +4 → block_total_size leía el class_ptr como
 * tamaño y DESINCRONIZABA el recorrido del heap (el sweep y el build_gc_valid_map
 * de Camino 1). Aquí calculamos el tamaño ANTES de tocar la cabecera y lo
 * escribimos vía add_to_free_list ([FREE_BIT][size@+4][next]) → bloque caminable
 * y reutilizable de inmediato. */
void bpvm_heap_free_block(bpvm_t* vm, uint32_t header_addr) {
    uint32_t size = block_total_size(vm, header_addr);
    if (size == 0) return;   /* defensivo: no tocar si no sabemos el tamaño */
    add_to_free_list(vm, header_addr, size);
}

/* H3 (V2): sweep que RECONSTRUYE la free-list coalesciendo runs de bloques
 * libres/muertos adyacentes, y RETROCEDE heap_next si el run final lo toca
 * (devuelve memoria al bump sin compactar). Espejo del gcLocked de la VM-Java. */
static void gc_sweep_phase(bpvm_t* vm) {
    uint8_t* mem = vm->memory;
    vm->free_list_head = 0;
    uint32_t cur = vm->heap_start;
    uint32_t freed = 0, kept = 0;
    uint32_t pend_start = 0, pend_size = 0;   /* 0 = sin run pendiente (heap_start>0) */
    while (cur < vm->heap_next) {
        uint32_t total = block_total_size(vm, cur);
        if (total == 0) break;
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
        }
        cur += total;
    }
    if (pend_start != 0) {
        /* El run libre FINAL toca heap_next → retroceder (devolver al bump). */
        vm->heap_next = pend_start;
    }
    vm->last_gc_heap_next = vm->heap_next;
    if (vm->tracing) {
        fprintf(stderr, "[gc] kept=%" PRIu32 " freed=%" PRIu32 " heap=[%" PRIu32 "..%" PRIu32 ") freelist=%s\n",
                kept, freed, vm->heap_start, vm->heap_next,
                vm->free_list_head ? "si" : "vacia");
    }
}

static void handle_kill_idx(bpvm_t* vm, uint32_t idx);   /* def. más abajo (junto a bpvm_handle_kill) */

/* Paso 6 — BARRIDO DE TABLA (handle-aware): un slot VIVO (addr!=0) cuyo bloque quedó
 * SIN marcar es inalcanzable → handle_kill_idx (bump gen + addr=0 + free-list) para que
 * un handle rancio a él GRITE (contrato B también para lo que recolecta el GC). Debe ir
 * ANTES del sweep de heap (que limpia el MARK_BIT). El bloque físico lo libera el sweep
 * de heap. Bajo el STW del GC → seguro reciclar ya. Espejo del barrido de tabla de miVM. */
static void gc_table_sweep_phase(bpvm_t* vm) {
    for (uint32_t idx = 1u; idx < vm->handle_next; idx++) {
        uint32_t a = vm->handle_addr[idx];
        if (a == 0u) continue;                                  /* slot libre */
        uint32_t hh = a - 4u;
        if (hh < vm->heap_start || hh >= vm->heap_next) continue;   /* defensivo */
        uint32_t tag = bpvm_read_u32_be(vm->memory + hh);
        if ((tag & BPVM_TAG_MARK_BIT) == 0u) {                  /* no alcanzable */
            handle_kill_idx(vm, idx);
        }
    }
}

static void bpvm_gc(bpvm_t* vm) {
    /* V4: GC suspendido durante la migración a handles — guarda en el NÚCLEO
     * (espejo del gcLocked de miVM): cubre gc_stw Y bpvm_heap_gc (builtin gc()).
     * Sin esto, el gc() manual corría y su escaneo conservativo no reconoce los
     * handles tageados → no marca nada → libera TODO lo vivo (ownerreassign). */
    if (vm->gc_suspended) return;
    gc_mark_phase(vm);
    gc_table_sweep_phase(vm);   /* paso 6: recicla slots de tabla de lo inalcanzable */
    gc_sweep_phase(vm);
}

/* V4/paso4c — registra un objeto de HEAP y devuelve su HANDLE 64b (gen(slot)<<32 |
 * idx|TAG). Reusa un slot de la free-list si lo hay (con su gen ya bumpeada); si no,
 * crece la tabla. Devuelve bpref_t: al asignarlo a un uint32_t da error de compilación
 * → el compilador caza cada sitio que perdería la generación. Si no puede crecer,
 * devuelve la dirección cruda (sin tag → bpref_deref por identidad). */
bpref_t bpvm_handle_register(bpvm_t* vm, uint32_t addr) {
    /* Paso 7 — la tabla (free-list/handle_next) es estado COMPARTIDO: bajo SMP dos
     * workers registran a la vez (handle_register corre FUERA del vm_lock, heap_alloc
     * ya lo soltó) → carrera en free_top → roban el mismo idx. Serializamos con el
     * vm_lock (no-op en single-worker → coste cero en el default de envío). El GC usa
     * handle_kill_idx (sin lock) porque corre bajo STW (todos parados). */
    bpref_t r;
    bpvm_smp_lock(vm);
    uint32_t idx;
    if (vm->handle_free_top > 0u) {
        idx = vm->handle_free_list[--vm->handle_free_top];   /* REUSO: slot reciclado, gen ya bumpeada */
    } else {
        if (vm->handle_next >= vm->handle_cap) {
            uint32_t new_cap = vm->handle_cap ? vm->handle_cap * 2u : 4096u;
            uint32_t* na = (uint32_t*) bpvm_realloc(vm->handle_addr, (size_t) new_cap * sizeof(uint32_t));
            if (!na) { r.v = addr; bpvm_smp_unlock(vm); return r; }
            uint32_t* ng = (uint32_t*) bpvm_realloc(vm->handle_gen,  (size_t) new_cap * sizeof(uint32_t));
            if (!ng) { vm->handle_addr = na; r.v = addr; bpvm_smp_unlock(vm); return r; }
            vm->handle_addr = na;
            vm->handle_gen  = ng;
            vm->handle_cap  = new_cap;
        }
        idx = vm->handle_next++;
        vm->handle_gen[idx] = 0u;   /* slot fresco */
    }
    /* Paso 7c — A1: publica el slot con RELEASE → todo lo escrito ANTES (init del objeto)
     * es visible para quien lo lea con ACQUIRE (bpref_deref). Mitad ESCRITOR del apretón.
     * (El unlock de abajo ya da release; esto lo hace EXPLÍCITO y sobrevive a quitar el
     * lock en 7b.2.) Inerte en x86; barrera en ARM/RISC-V. */
    __atomic_store_n(&vm->handle_addr[idx], addr, __ATOMIC_RELEASE);
    r.v = ((uint64_t) vm->handle_gen[idx] << 32) | (uint64_t)(idx | BPVM_HANDLE_TAG);
    bpvm_smp_unlock(vm);
    return r;
}

/* Paso 4c/6 — recicla un slot de la tabla por índice: BUMP de la generación (handles
 * rancios dejan de matchear → gritan), addr=0 (slot libre: vivo ⟺ addr!=0, lo usa el
 * barrido de tabla del GC) y RECICLA el índice a la free-list para reuso. Lo comparten
 * el owner-bpvm_free(bpvm_handle_kill) y el barrido de tabla del GC (paso 6). */
static void handle_kill_idx(bpvm_t* vm, uint32_t idx) {
    if (vm->handle_gen == NULL || idx == 0u || idx >= vm->handle_next) return;
    vm->handle_gen[idx]++;                        /* gen bumpeada → handles rancios mueren */
    vm->handle_addr[idx] = 0u;                    /* slot libre en la tabla (paso 6) */
    if (vm->handle_free_top >= vm->handle_free_cap) {
        uint32_t nc = vm->handle_free_cap ? vm->handle_free_cap * 2u : 256u;
        uint32_t* nl = (uint32_t*) bpvm_realloc(vm->handle_free_list, (size_t) nc * sizeof(uint32_t));
        if (!nl) return;   /* sin free-list no reciclamos este slot (se pierde, no se corrompe) */
        vm->handle_free_list = nl;
        vm->handle_free_cap  = nc;
    }
    vm->handle_free_list[vm->handle_free_top++] = idx;   /* reciclar el slot */
}

/* Paso 4c — libera el slot de un handle (owner-free): delega en handle_kill_idx.
 * No-op para null/constantes. El TAG_FREE_BIT del bloque físico evita el doble-free real. */
void bpvm_handle_kill(bpvm_t* vm, bpref_t r) {
    if ((r.v & BPVM_HANDLE_TAG) == 0u) return;
    bpvm_smp_lock(vm);   /* paso 7: serializa la free-list contra registers/kills de otros workers */
    /* Paso 7b.1 — FREE CON GENERACIÓN VALIDADA (refuerzo de la maqueta): solo el PRIMER
     * kill de un handle vivo actúa; un kill RANCIO (slot ya reciclado, gen no matchea) es
     * NO-OP seguro — si no, bumpearía la gen del ocupante NUEVO y lo corromperría (doble
     * free-list, etc). check+kill atómicos bajo el lock = el compareAndSet(slot,g,g+1). */
    if (!bpvm_ref_dead(vm, r)) {
        handle_kill_idx(vm, (uint32_t) r.v & ~BPVM_HANDLE_TAG);
    }
    bpvm_smp_unlock(vm);
}

/* H3: GC stop-the-world. Asume vm_lock tomado. Lo usan el disparo proactivo
 * por umbral y la ruta de OOM. En legacy/single-worker no hay baile. */
static void gc_stw(bpvm_t* vm) {
    if (vm->gc_suspended) return;   /* V4: GC suspendido durante la migración a handles */
    if (vm->smp) {
        vm->smp->stop_the_world = true;
        bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
        while (vm->smp->running_workers > 1) {
            bpvm_platform_cond_wait(&vm->smp->sched_cond, &vm->smp->vm_lock);
        }
    }
    bpvm_gc(vm);
    if (vm->smp) {
        vm->smp->stop_the_world = false;
        bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
    }
}

/* H3: intenta asignar `total` bytes. Devuelve la dirección de la cabecera, o
 * 0 si no cabe (heap_start>0 → 0 nunca es una cabecera válida). 1) free-list
 * first-fit con split; 2) bump desde heap_next. */
static uint32_t try_allocate_inner(bpvm_t* vm, uint32_t total) {
    uint8_t* mem = vm->memory;
    uint32_t prev = 0, cur = vm->free_list_head;
    while (cur != 0) {
        uint32_t block_size = bpvm_read_u32_be(mem + cur + 4);
        uint32_t next = bpvm_read_u32_be(mem + cur + 8);
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
                return cur;
            }
            /* sobrante no representable → este bloque no sirve, seguir buscando */
        }
        prev = cur;
        cur = next;
    }
    /* 2) Bump. */
    if (vm->heap_next + total > vm->stack_base) return 0;
    uint32_t addr = vm->heap_next;
    vm->heap_next += total;
    return addr;
}

/* --- API pública del heap ---
 *
 * Devuelve user_ref (= header_addr + 4) o 0 si OOM tras GC.
 * Tras heapAlloc el caller normalmente escribe `length` (o `class_ptr`)
 * en mem[user_ref] y luego zero-inicializa el payload.
 */
uint32_t bpvm_heap_alloc(bpvm_t* vm, uint32_t payload_bytes, int type) {
    uint32_t total = align4(BPVM_OBJ_HEADER_SIZE + payload_bytes);
    if (total < BPVM_MIN_FREE_BLOCK) total = BPVM_MIN_FREE_BLOCK;

    /* H2 — Heap allocation y GC son críticos en SMP: bump pointer y
     * mark/sweep escriben estado compartido. Tomar el vm_lock global
     * serializa los workers contra heap. En legacy mode (vm->smp NULL)
     * el lock es no-op. */
    bpvm_smp_lock(vm);

    /* H3 (V2): GC PROACTIVO por umbral de crecimiento de bump. Evita el
     * over-commit (que el heap suba a su pico antes de colectar): si el bump
     * avanzó >= umbral desde el último GC, colecta ahora. gc_stw hace el baile
     * STW (mark scanea las pilas de todos los threads → deben estar en
     * safepoint con tc->sp sincronizado; en legacy/single-worker no hay baile). */
    if (!vm->gc_suspended && vm->gc_bump_threshold != 0 &&
        vm->heap_next - vm->last_gc_heap_next >= vm->gc_bump_threshold) {
        gc_stw(vm);
    }

    uint32_t addr = try_allocate_inner(vm, total);
    if (addr == 0) {
        /* Sin sitio en free-list ni bump: STW GC y reintentar. */
        gc_stw(vm);
        addr = try_allocate_inner(vm, total);
        if (addr == 0) {
            bpvm_smp_unlock(vm);
            return 0;   /* OOM real */
        }
    }

    uint32_t tag = ((uint32_t)(type & 0x3F)) << BPVM_TAG_TYPE_SHIFT;
    bpvm_write_u32_be(vm->memory + addr, tag);
    /* length lo escribe el caller; zero-inicializamos el slot por si lo olvida. */
    bpvm_write_u32_be(vm->memory + addr + 4, 0);

    uint32_t user_ref = addr + 4;
    /* Zero-inicializar el resto del payload (importante al reusar free-list). */
    if (payload_bytes > 0) {
        memset(vm->memory + user_ref + 4, 0, payload_bytes);
    }
    bpvm_smp_unlock(vm);
    return user_ref;
}

/* Helper: aloca un string a partir de una const-string C. H2 (V2): los
 * strings son TYPE_ARRAY_I8 con los bytes UTF-8 tal cual (1 byte/elem).
 * `len` = nº de bytes de `s`. */
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len) {
    uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) len, BPVM_TYPE_ARRAY_I8);
    if (ref == 0) return 0;
    bpvm_write_u32_be(vm->memory + ref, (uint32_t) len);   /* length = bytes */
    for (size_t i = 0; i < len; i++) {
        vm->memory[ref + 4 + i] = (uint8_t) s[i];
    }
    return (uint32_t) bpvm_handle_register(vm, ref).v;   /* string: idx|TAG (gen=0); reuso de strings = paso 6 */
}

/* Trigger manual de GC. Devuelve bytes liberados (aproximado: el delta
 * entre heap_next antes y después es 0 porque F2 v1 no compacta, así
 * que devolvemos 0). */
void bpvm_heap_gc(bpvm_t* vm) {
    bpvm_smp_lock(vm);
    bpvm_gc(vm);
    bpvm_smp_unlock(vm);
}
