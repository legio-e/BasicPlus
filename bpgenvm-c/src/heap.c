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
#include <stdio.h>

static uint32_t align4(uint32_t v) {
    return (v + 3) & ~3u;
}

/* Devuelve el tamaño total del bloque (header + payload alineado) según
 * el tag del header. Sirve tanto para objetos vivos como para libres. */
static uint32_t block_total_size(const bpvm_t* vm, uint32_t header_addr) {
    uint32_t tag    = bpvm_read_u32_be(vm->memory + header_addr);
    uint32_t length = bpvm_read_u32_be(vm->memory + header_addr + 4);
    int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
    uint32_t payload;
    switch (type) {
    case BPVM_TYPE_ARRAY_I8:  payload = length;          break;
    case BPVM_TYPE_ARRAY_I16: payload = length * 2;      break;
    case BPVM_TYPE_ARRAY_I32: payload = length * 4;      break;
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

static int is_heap_ref(const bpvm_t* vm, uint32_t v) {
    if (v < vm->heap_start || v >= vm->heap_next) return 0;
    /* Heurística: refs apuntan al user_ref que está 4 bytes tras un
     * header válido. Verificamos que (v - 4) tenga un tag con type
     * válido (0..4). Esto reduce falsos positivos del scan conservativo. */
    if (v < vm->heap_start + 4) return 0;
    uint32_t tag = bpvm_read_u32_be(vm->memory + v - 4);
    int type = (int)((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
    return (type >= 0 && type <= BPVM_TYPE_OBJECT);
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
    /* 2. allocAnchor — TODO en F2.b cuando se añada el campo al thread. */
}

static void gc_sweep_phase(bpvm_t* vm) {
    uint32_t cur = vm->heap_start;
    uint32_t freed = 0;
    uint32_t kept  = 0;
    while (cur < vm->heap_next) {
        uint32_t total = block_total_size(vm, cur);
        if (total == 0) break;
        uint32_t tag = bpvm_read_u32_be(vm->memory + cur);
        if (tag & BPVM_TAG_MARK_BIT) {
            /* Vivo: quita el bit mark para la próxima ronda. */
            tag &= ~BPVM_TAG_MARK_BIT;
            bpvm_write_u32_be(vm->memory + cur, tag);
            kept += total;
        } else if (!(tag & BPVM_TAG_FREE_BIT)) {
            /* No marcado y no ya libre: liberar. */
            tag |= BPVM_TAG_FREE_BIT;
            bpvm_write_u32_be(vm->memory + cur, tag);
            freed += total;
        }
        cur += total;
    }
    if (vm->tracing) {
        fprintf(stderr, "[gc] kept=%u freed=%u heap=[%u..%u)\n",
                kept, freed, vm->heap_start, vm->heap_next);
    }
}

static void bpvm_gc(bpvm_t* vm) {
    gc_mark_phase(vm);
    gc_sweep_phase(vm);
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

    if (vm->heap_next + total > vm->stack_base) {
        /* Sin espacio: correr STW GC.
         *
         * H2 — DANCE STW: el GC mark-phase scanea tc->sp/stack_base de
         * TODOS los threads para encontrar roots. Si otro worker está
         * mid-interp con tc->sp obsoleto (pendiente de sync local), el
         * scan se pierde refs vivas → free de objetos referenciados →
         * corrupción heap.
         *
         * Solución: izar `stop_the_world`, esperar a que cada worker
         * llegue al safepoint del interp y se parquee en sched_cond.
         * Cuando running_workers == 1 (sólo nosotros), corremos GC.
         * Tras GC: bajamos flag y broadcast para que los workers
         * resuman.
         *
         * En legacy mode no hay smp, no hay otros workers — el GC corre
         * directo sin dance. */
        if (vm->smp) {
            vm->smp->stop_the_world = true;
            bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
            /* Esperar a que el resto de workers ceda y se parquee. */
            while (vm->smp->running_workers > 1) {
                bpvm_platform_cond_wait(&vm->smp->sched_cond,
                                         &vm->smp->vm_lock);
            }
        }
        bpvm_gc(vm);
        if (vm->smp) {
            vm->smp->stop_the_world = false;
            bpvm_platform_cond_broadcast(&vm->smp->sched_cond);
        }
        /* F2 v1: el sweep marca FREE pero no compacta — el bump pointer
         * sigue avanzando. Si tras GC el bump todavía no cabe, OOM. */
        if (vm->heap_next + total > vm->stack_base) {
            bpvm_smp_unlock(vm);
            return 0;
        }
    }

    uint32_t header_addr = vm->heap_next;
    vm->heap_next += total;

    uint32_t tag = ((uint32_t)(type & 0x3F)) << BPVM_TAG_TYPE_SHIFT;
    bpvm_write_u32_be(vm->memory + header_addr, tag);
    /* length lo escribe el caller. Pero zero-inicializamos el slot por
     * si el caller olvida. */
    bpvm_write_u32_be(vm->memory + header_addr + 4, 0);

    uint32_t user_ref = header_addr + 4;
    /* Zero-inicializar el resto del payload. */
    if (payload_bytes > 0) {
        memset(vm->memory + user_ref + 4, 0, payload_bytes);
    }
    bpvm_smp_unlock(vm);
    return user_ref;
}

/* Helper: aloca un string a partir de una const-string C, codificándola
 * como TYPE_ARRAY_I32 con un codepoint por slot (4 bytes cada uno). */
uint32_t bpvm_heap_alloc_string(bpvm_t* vm, const char* s, size_t len) {
    uint32_t ref = bpvm_heap_alloc(vm, (uint32_t)len * 4, BPVM_TYPE_ARRAY_I32);
    if (ref == 0) return 0;
    bpvm_write_u32_be(vm->memory + ref, (uint32_t) len);   /* length */
    for (size_t i = 0; i < len; i++) {
        bpvm_write_u32_be(vm->memory + ref + 4 + i * 4, (uint8_t) s[i]);
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
