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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

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
    case BPVM_TYPE_ARRAY_REF: payload = length * 4;      break;
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
    uint32_t span  = vm->stack_base - vm->heap_start;      /* heap máximo */
    uint32_t words = span / 4u;                            /* nº de palabras de 4B */
    size_t   bytes = ((size_t) words + 7u) / 8u + 1u;      /* 1 bit por palabra + pad */
    if (vm->gc_valid_map == NULL || vm->gc_valid_map_size < bytes) {
        free(vm->gc_valid_map);
        vm->gc_valid_map = (uint8_t*) calloc(1, bytes);
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

static void mark_recursive(bpvm_t* vm, uint32_t user_ref) {
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
            uint32_t slot = bpvm_read_u32_be(vm->memory + user_ref + 4 + i * 4);
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
                uint32_t slot = bpvm_read_u32_be(vm->memory + user_ref + 4 + i * 4);
                mark_recursive(vm, slot);
            }
        }
        (void) bw;
    }
}

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

static void bpvm_gc(bpvm_t* vm) {
    gc_mark_phase(vm);
    gc_sweep_phase(vm);
}

/* H3: GC stop-the-world. Asume vm_lock tomado. Lo usan el disparo proactivo
 * por umbral y la ruta de OOM. En legacy/single-worker no hay baile. */
static void gc_stw(bpvm_t* vm) {
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
        if (block_size >= total) {
            uint32_t next = bpvm_read_u32_be(mem + cur + 8);
            uint32_t remaining = block_size - total;
            if (remaining >= BPVM_MIN_FREE_BLOCK) {
                /* Split: usar [cur, cur+total); dejar el resto libre. */
                uint32_t nf = cur + total;
                bpvm_write_u32_be(mem + nf,     BPVM_TAG_FREE_BIT);
                bpvm_write_u32_be(mem + nf + 4, remaining);
                bpvm_write_u32_be(mem + nf + 8, next);
                if (prev == 0) vm->free_list_head = nf;
                else bpvm_write_u32_be(mem + prev + 8, nf);
            } else {
                /* Usar el bloque entero; quitarlo de la lista. */
                if (prev == 0) vm->free_list_head = next;
                else bpvm_write_u32_be(mem + prev + 8, next);
            }
            return cur;
        }
        prev = cur;
        cur = bpvm_read_u32_be(mem + cur + 8);
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
    if (vm->gc_bump_threshold != 0 &&
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
    return ref;
}

/* Trigger manual de GC. Devuelve bytes liberados (aproximado: el delta
 * entre heap_next antes y después es 0 porque F2 v1 no compacta, así
 * que devolvemos 0). */
void bpvm_heap_gc(bpvm_t* vm) {
    bpvm_smp_lock(vm);
    bpvm_gc(vm);
    bpvm_smp_unlock(vm);
}
