/*
 * scheduler.c — scheduler cooperativo single-worker (F4 v1).
 *
 * Equivalente al modo `--workers=1` de la VM Java. Sin paralelismo
 * real, pero con multithreading correcto y determinístico: round-
 * robin sobre RUNNABLE, wake-up automático de BLOCKED_SLEEP cuando
 * pasa wakeAt, transfer de mutex al primer waiter en unlock.
 *
 * F4 v2 (futuro) puede portar a multi-pthread con vm_lock global.
 * Por ahora F4 v1 corre todo en el thread main del proceso —
 * platform_pthread.c queda compilado pero no usado.
 */

#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include <stdio.h>

/* Pickea el siguiente thread RUNNABLE empezando desde
 * current_thread_idx + 1 (round-robin). Devuelve -1 si no hay. */
static int pick_next_runnable(const bpvm_t* vm, int start) {
    int n = vm->thread_count;
    if (n == 0) return -1;
    for (int i = 0; i < n; i++) {
        int idx = (start + 1 + i) % n;
        if (vm->threads[idx].status == BPVM_THREAD_RUNNABLE) return idx;
    }
    return -1;
}

/* ¿Algún thread BP no terminado? */
static int any_alive(const bpvm_t* vm) {
    for (int i = 0; i < vm->thread_count; i++) {
        if (vm->threads[i].status != BPVM_THREAD_TERMINATED) return 1;
    }
    return 0;
}

/* Despierta a los threads BLOCKED_SLEEP cuyo wake_at_ms ha pasado. */
static int wake_expired_sleeps(bpvm_t* vm, int64_t now) {
    int woken = 0;
    for (int i = 0; i < vm->thread_count; i++) {
        bpvm_thread_t* tc = &vm->threads[i];
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= now) {
            tc->status = BPVM_THREAD_RUNNABLE;
            woken++;
        }
    }
    return woken;
}

/* Despierta a los threads BLOCKED_JOIN cuyo target ya terminó. */
static int wake_completed_joins(bpvm_t* vm) {
    int woken = 0;
    for (int i = 0; i < vm->thread_count; i++) {
        bpvm_thread_t* tc = &vm->threads[i];
        if (tc->status == BPVM_THREAD_BLOCKED_JOIN
                && tc->blocked_on_join >= 0
                && tc->blocked_on_join < vm->thread_count
                && vm->threads[tc->blocked_on_join].status == BPVM_THREAD_TERMINATED) {
            tc->blocked_on_join = -1;
            tc->status = BPVM_THREAD_RUNNABLE;
            woken++;
        }
    }
    return woken;
}

/* Próximo wake_at_ms (el más cercano en el futuro). INT64_MAX si no
 * hay sleepers. */
static int64_t earliest_wake(const bpvm_t* vm) {
    int64_t min = INT64_MAX;
    for (int i = 0; i < vm->thread_count; i++) {
        const bpvm_thread_t* tc = &vm->threads[i];
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms < min) {
            min = tc->wake_at_ms;
        }
    }
    return min;
}

bpvm_status_t bpvm_scheduler_run(bpvm_t* vm) {
    int last_idx = -1;
    while (any_alive(vm)) {
        /* P-run-stop (#257) — KILL cooperativo: el poll_cb (si hay) mira
         * el transporte ENTRE quanta; kill_requested termina la ejecución
         * limpiamente (paramos entre opcodes → heap/FS consistentes). */
        if (vm->poll_cb != NULL && vm->poll_cb(vm, vm->poll_user) != 0)
            vm->kill_requested = 1;
        if (vm->kill_requested) return BPVM_KILLED;

        /* 1) Despierta sleeps expirados + joins completados. */
        wake_expired_sleeps(vm, bpvm_platform_now_ms());
        wake_completed_joins(vm);

        /* 2) Pick RUNNABLE. */
        int idx = pick_next_runnable(vm, last_idx);
        if (idx < 0) {
            /* No hay RUNNABLE. ¿Sleep pendiente?  */
            int64_t earliest = earliest_wake(vm);
            if (earliest == INT64_MAX) {
                /* Todos bloqueados sin sleep: deadlock. */
                fprintf(stderr, "[bpvm-c sched] deadlock — todos los threads "
                        "bloqueados sin posibilidad de progreso\n");
                return BPVM_ERR_RUNTIME;
            }
            int64_t now = bpvm_platform_now_ms();
            int dt = (int)(earliest - now);
            /* P-run-stop: con poll_cb instalado, tope de 50 ms para poder
             * atender un KILL aunque todos los threads BP duerman. */
            if (vm->poll_cb != NULL && dt > 50) dt = 50;
            if (dt > 0) bpvm_platform_thread_sleep_ms(dt);
            continue;
        }

        /* 3) Ejecuta un quantum del tc elegido. */
        bpvm_thread_t* tc = &vm->threads[idx];
        vm->current_thread_idx = idx;
        last_idx = idx;
        int yielded = 0;
        int quantum = vm->quantum_ops > 0 ? vm->quantum_ops : 1024;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, quantum, &yielded);
        if (s != BPVM_OK) return s;
        /* Si el tc terminó o cedió, el while continúa. */
    }
    return BPVM_OK;
}
