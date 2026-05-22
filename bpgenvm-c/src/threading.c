/*
 * threading.c — helpers para Thread y Mutex BP (F4 v1).
 *
 * F4 v1 — single-worker cooperative scheduler en main thread del
 * proceso. Sin pthreads adicionales. Cada Thread BP es un
 * ThreadContext del array vm->threads[]; el scheduler los va
 * pickeando round-robin. F4 v2 (futuro) puede migrar a multi-pthread.
 *
 * Mutex BP: pool dinámico indexado por mid (id de mutex). __mutexCreate
 * devuelve el id. El layout del objeto Mutex BP es opaco para nosotros
 * — el frontend pone el mid en un field del objeto que es lo que llega
 * a __mutexLock/Unlock.
 */

#include "bpvm_internal.h"
#include <stdio.h>

/* ---- Mutex pool ---- */

int bpvm_mutex_alloc(bpvm_t* vm) {
    if (vm->mutex_count >= vm->mutex_capacity) {
        int new_cap = vm->mutex_capacity == 0 ? 8 : vm->mutex_capacity * 2;
        bpvm_bp_mutex_t* arr = (bpvm_bp_mutex_t*) realloc(vm->mutexes,
                                (size_t) new_cap * sizeof(bpvm_bp_mutex_t));
        if (!arr) return -1;
        vm->mutexes = arr;
        vm->mutex_capacity = new_cap;
    }
    int mid = vm->mutex_count++;
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    m->owner_tid = -1;
    m->waiters = NULL;
    m->waiter_count = 0;
    m->waiter_capacity = 0;
    return mid;
}

void bpvm_mutex_add_waiter(bpvm_t* vm, int mid, int tid) {
    if (mid < 0 || mid >= vm->mutex_count) return;
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    if (m->waiter_count >= m->waiter_capacity) {
        int new_cap = m->waiter_capacity == 0 ? 4 : m->waiter_capacity * 2;
        int32_t* arr = (int32_t*) realloc(m->waiters,
                        (size_t) new_cap * sizeof(int32_t));
        if (!arr) return;
        m->waiters = arr;
        m->waiter_capacity = new_cap;
    }
    m->waiters[m->waiter_count++] = tid;
}

/* Saca y devuelve el primer waiter en FIFO. -1 si no hay. */
int bpvm_mutex_pop_waiter(bpvm_t* vm, int mid) {
    if (mid < 0 || mid >= vm->mutex_count) return -1;
    bpvm_bp_mutex_t* m = &vm->mutexes[mid];
    if (m->waiter_count == 0) return -1;
    int tid = m->waiters[0];
    /* shift down (FIFO). Pocos waiters habitualmente. */
    for (int i = 1; i < m->waiter_count; i++) {
        m->waiters[i - 1] = m->waiters[i];
    }
    m->waiter_count--;
    return tid;
}

/* ---- Thread spawn ---- *
 *
 * Llamado por el builtin __threadStart. `thread_ref` es el user_ref del
 * objeto Thread BP (que tiene un campo `run` virtual). Creamos un
 * nuevo ThreadContext con su propia región de stack, inicializamos su
 * frame para llamar a t.run(), y lo marcamos RUNNABLE para que el
 * scheduler lo pickee.
 *
 * Convención del frontend (Thread sintetizado): el método `run` está
 * en vtSlot 0 de la vtable de Thread. Lo emitimos como CALL al método
 * (no INVOKE_VIRTUAL porque el frame es nuevo y el `this` ya está
 * pusheado). Pero como nosotros NO sabemos el offset del método run
 * — depende de la subclass del usuario — tenemos que hacerlo via
 * INVOKE_VIRTUAL: empujamos `this` y llamamos `run` (slot 0).
 *
 * Devuelve el tid del thread nuevo, o -1 si error.
 */
int bpvm_thread_spawn(bpvm_t* vm, uint32_t thread_ref) {
    if (vm->thread_count >= BPVM_MAX_THREADS) return -1;
    if (thread_ref == 0) return -1;

    /* Reservar región de stack. */
    uint32_t stack_base = vm->next_thread_stack;
    uint32_t stack_top  = stack_base + BPVM_THREAD_STACK_BYTES;
    if (stack_top > vm->memory_size) return -1;
    vm->next_thread_stack = stack_top;

    /* Layout inicial del frame: simulamos un CALL desde un caller
     * imaginario que ya pusheó `this` (thread_ref) en su stack.
     * - sp inicia en stack_base.
     * - Push `this` = thread_ref (arg para run).
     * - Push saved pc/bp/cs FALSOS apuntando a memory[0] (HALT/
     *   THREAD_EXIT sentinela). El intérprete tras RET de run hará
     *   sp -= 4 (ret), restaura cs/bp/pc desde los falsos, y luego
     *   ejecuta mem[0] = THREAD_EXIT que termina este thread sin
     *   tumbar la VM.
     * - bp = sp (justo tras los saves).
     *
     * El primer opcode que ejecutaremos es el INVOKE_VIRTUAL slot 0
     * (Thread.run). Eso ya hace push de pc/bp/cs reales y entra al
     * cuerpo de run. Pero antes hay que tener `this` en el stack.
     */
    int tid = vm->thread_count++;
    bpvm_thread_t* tc = &vm->threads[tid];
    tc->id = tid;
    tc->stack_base = stack_base;
    tc->stack_top  = stack_top;
    tc->blocked_on_mutex = -1;
    tc->blocked_on_join = -1;
    tc->wake_at_ms = 0;
    tc->thread_ref_heap = (int32_t) thread_ref;

    /* Guarda el tid en field[0] del Thread BP (convención del frontend
     * Java: el constructor de Thread sintetizado deja field[0]=0 hasta
     * que se llama start()). Esto es lo que __threadJoin lee para
     * encontrar el tc target. */
    bpvm_write_i32_be(vm->memory + thread_ref + 4 + 0 * 4, (int32_t) tid);

    /* Resolver dirección de Thread.run() (slot 0 de la vtable). */
    uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + thread_ref);
    uint16_t bw       = bpvm_read_u16_be(vm->memory + class_ptr + BPVM_CLS_OFF_BITMAP_WORDS);
    uint32_t vt_base  = class_ptr + BPVM_CLS_OFF_FIELD_BITMAP + 2u * (uint32_t) bw * 4u;
    int32_t  off      = bpvm_read_i32_be(vm->memory + vt_base);
    if (off == -1) {
        /* run() heredada — buscar en parent. F4 v1 no lo soporta. */
        fprintf(stderr, "[bpvm-c] Thread.run() en slot 0 = -1 (heredada; no soportado F4 v1)\n");
        vm->thread_count--;
        vm->next_thread_stack = stack_base;
        return -1;
    }
    uint32_t target_cs = bpvm_get_cs_for_data_addr(vm, class_ptr);

    /* Frame inicial exacto del Java THREAD_START (líneas 2899-2912):
     *   [sb+0]  thisRef
     *   [sb+4]  saved PC = 0 (sentinela mem[0]=THREAD_EXIT cuando run() RET)
     *   [sb+8]  saved BP = sb
     *   [sb+12] saved CS = 0
     *   bp = sb + 16, sp = sb + 16. */
    uint32_t sb = stack_base;
    bpvm_write_i32_be(vm->memory + sb,      (int32_t) thread_ref);
    bpvm_write_i32_be(vm->memory + sb + 4,  0);
    bpvm_write_i32_be(vm->memory + sb + 8,  (int32_t) sb);
    bpvm_write_i32_be(vm->memory + sb + 12, 0);
    tc->sp = sb + 16;
    tc->bp = sb + 16;
    tc->cs = target_cs;
    tc->pc = target_cs + (uint32_t) off;
    tc->status = BPVM_THREAD_RUNNABLE;
    return tid;
}
