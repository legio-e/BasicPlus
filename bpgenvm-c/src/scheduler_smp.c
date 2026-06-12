/*
 * scheduler_smp.c — scheduler multi-worker (H2).
 *
 * Equivalente al WorkerLoop de VM-Java (VirtualMachine.java line 1363+).
 * Cada worker corre un loop:
 *   - bajo vm_lock: parquear si no hay tc RUNNABLE, despertarse cuando
 *     alguien hace cond_signal (sleep wake, mutex unlock, etc.).
 *   - pickear el primer tc RUNNABLE, marcarlo RUNNING.
 *   - liberar vm_lock y correr bpvm_interp_run_quantum (LOCK-FREE).
 *   - re-tomar vm_lock para gestionar el exit del quantum.
 *
 * NOTA — esta primera iteración no tiene STW GC dance todavía. Los
 * locks de heap_alloc se añaden en el siguiente paso. Sin esos, alocar
 * desde 2 workers en paralelo es race-prone — tests SMP con `gc`
 * agresivo van a fallar hasta que añadamos el dance.
 */

#include "bpvm_smp.h"
#include "bpvm_comm.h"
#include "bpvm_internal.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================ */
/*  Helpers (bajo vm_lock)                                       */
/* ============================================================ */

static int pick_next_runnable_locked(bpvm_t* vm) {
    int n = vm->thread_count;
    for (int i = 0; i < n; i++) {
        /* H2 — Sólo seleccionable si está RUNNABLE Y nadie lo tiene
         * asignado actualmente (sched_owner == -1). El status se puede
         * escribir desde el interp sin lock, por eso no basta — el
         * worker que "tiene" el tc lo marca con su wid bajo vm_lock y
         * lo libera bajo vm_lock al volver al scheduler. */
        if (vm->threads[i].status == BPVM_THREAD_RUNNABLE
                && vm->threads[i].sched_owner == -1) {
            return i;
        }
    }
    return -1;
}

static int any_alive_locked(const bpvm_t* vm) {
    for (int i = 0; i < vm->thread_count; i++) {
        if (vm->threads[i].status != BPVM_THREAD_TERMINATED) return 1;
    }
    return 0;
}

static int64_t earliest_wake_locked(const bpvm_t* vm) {
    int64_t min = INT64_MAX;
    for (int i = 0; i < vm->thread_count; i++) {
        const bpvm_thread_t* tc = &vm->threads[i];
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms < min) {
            min = tc->wake_at_ms;
        }
    }
    return min;
}

/* Despierta sleeps expirados + joins completados. Llamado bajo vm_lock.
 * Devuelve 1 si algo despertó (caller emitirá cond_broadcast). */
static int do_wakeups_locked(bpvm_t* vm) {
    int woken = 0;
    int64_t now = bpvm_platform_now_ms();
    for (int i = 0; i < vm->thread_count; i++) {
        bpvm_thread_t* tc = &vm->threads[i];
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= now) {
            tc->status = BPVM_THREAD_RUNNABLE;
            woken++;
        } else if (tc->status == BPVM_THREAD_BLOCKED_JOIN
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

/* ============================================================ */
/*  Worker loop                                                  */
/* ============================================================ */

typedef struct {
    bpvm_t* vm;
    int     worker_id;
} worker_arg_t;

static void worker_loop(void* raw) {
    worker_arg_t* arg = (worker_arg_t*) raw;
    bpvm_t* vm = arg->vm;
    bpvm_smp_t* smp = vm->smp;
    int wid = arg->worker_id;
    /* `arg` libre tras leerlo — fue malloc'd por el padre. */
    free(arg);

    for (;;) {
        /* P-run-stop (#257) — poll cooperativo del transporte ENTRE quanta
         * (fuera del lock; con 1 worker es exactamente entre quanta). */
        if (vm->poll_cb != NULL && vm->poll_cb(vm, vm->poll_user) != 0)
            vm->kill_requested = 1;

        /* ------- Scheduler decision bajo vm_lock ------- */
        bpvm_thread_t* tc = NULL;
        int tc_idx = -1;
        bpvm_platform_mutex_lock(&smp->vm_lock);
        for (;;) {
            if (smp->shutdown) {
                bpvm_platform_mutex_unlock(&smp->vm_lock);
                return;
            }
            /* P-run-stop — KILL: shutdown coordinado de todos los workers. */
            if (vm->kill_requested) {
                smp->shutdown = true;
                bpvm_platform_cond_broadcast(&smp->sched_cond);
                bpvm_platform_mutex_unlock(&smp->vm_lock);
                return;
            }
            /* H2 — STW GC: si otro worker pidió GC, parquéamos aquí
             * (cond_wait) hasta que limpie la flag. Antes el broadcast
             * del orquestador nos despertó saliendo de interp; ahora
             * esperamos a que el GC concluya. */
            if (smp->stop_the_world) {
                bpvm_platform_cond_wait(&smp->sched_cond, &smp->vm_lock);
                continue;
            }
            do_wakeups_locked(vm);
            tc_idx = pick_next_runnable_locked(vm);
            if (tc_idx >= 0) {
                tc = &vm->threads[tc_idx];
                break;
            }
            if (!any_alive_locked(vm)) {
                smp->shutdown = true;
                bpvm_platform_cond_broadcast(&smp->sched_cond);
                bpvm_platform_mutex_unlock(&smp->vm_lock);
                return;
            }
            /* P-run-stop — sin RUNNABLE: poll del transporte fuera del
             * lock antes de esperar (un programa dormido también tiene
             * que poder morir). */
            if (vm->poll_cb != NULL) {
                bpvm_platform_mutex_unlock(&smp->vm_lock);
                if (vm->poll_cb(vm, vm->poll_user) != 0)
                    vm->kill_requested = 1;
                bpvm_platform_mutex_lock(&smp->vm_lock);
                if (vm->kill_requested) continue;   /* el head hace shutdown */
            }
            /* Esperar próximo wake o signal. Con poll_cb, tope de 50 ms. */
            int64_t earliest = earliest_wake_locked(vm);
            int64_t now = bpvm_platform_now_ms();
            if (earliest == INT64_MAX) {
                if (vm->poll_cb != NULL) {
                    bpvm_platform_cond_timed_wait(&smp->sched_cond,
                                                   &smp->vm_lock, 50);
                } else {
                    bpvm_platform_cond_wait(&smp->sched_cond, &smp->vm_lock);
                }
            } else {
                int dt = (int)(earliest - now);
                if (vm->poll_cb != NULL && dt > 50) dt = 50;
                if (dt > 0) {
                    bpvm_platform_cond_timed_wait(&smp->sched_cond,
                                                   &smp->vm_lock, dt);
                }
            }
        }
        tc->status = BPVM_THREAD_RUNNING;
        tc->sched_owner = wid;               /* H2: lock semántico del tc */
        vm->current_thread_idx = tc_idx;
        smp->running_workers++;
        bpvm_platform_mutex_unlock(&smp->vm_lock);

        /* ------- Interp loop LOCK-FREE ------- */
        int yielded = 0;
        int quantum = vm->quantum_ops > 0 ? vm->quantum_ops : 1024;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, quantum, &yielded);

        /* ------- Volver a estado bajo vm_lock ------- */
        bpvm_platform_mutex_lock(&smp->vm_lock);
        smp->running_workers--;
        /* H2 — Si hay un GC orquestado esperando que running_workers
         * llegue a 1 (sólo él), avisarle. */
        if (smp->stop_the_world) {
            bpvm_platform_cond_broadcast(&smp->sched_cond);
        }
        /* Si el interp dejó al tc en RUNNING (cuanto agotado sin terminar),
         * volvemos a RUNNABLE para que se re-elija. Si yielded o terminó,
         * ya dejó tc->status correcto (BLOCKED_*, TERMINATED, RUNNABLE). */
        if (tc->status == BPVM_THREAD_RUNNING) {
            tc->status = BPVM_THREAD_RUNNABLE;
        }
        tc->sched_owner = -1;                /* H2: libera el tc */
        if (s != BPVM_OK) {
            /* Fallo terminal del intérprete → shutdown VM. */
            fprintf(stderr, "[bpvm-c smp w%d] interp error: status=%d\n",
                    arg ? 0 : 0, (int) s);
            smp->shutdown = true;
        }
        /* Notify: algún tc puede haber pasado de RUNNING → terminated,
         * lo que puede despertar joins. */
        bpvm_platform_cond_broadcast(&smp->sched_cond);
        bpvm_platform_mutex_unlock(&smp->vm_lock);
    }
}

/* ============================================================ */
/*  Init / destroy                                               */
/* ============================================================ */

int bpvm_smp_init(bpvm_t* vm, int n_workers) {
    if (!vm || n_workers < 1) return -1;
    if (vm->smp) return -1;            /* ya inicializado */

    bpvm_smp_t* smp = (bpvm_smp_t*) calloc(1, sizeof(bpvm_smp_t));
    if (!smp) return -1;
    smp->n_workers = n_workers;
    smp->shutdown = false;
    smp->stop_the_world = false;
    smp->gc_in_progress = false;
    smp->running_workers = 0;

    if (bpvm_platform_mutex_init(&smp->vm_lock) != 0) goto fail_free;
    if (bpvm_platform_cond_init(&smp->sched_cond) != 0) goto fail_lock;

    smp->worker_threads = (bpvm_platform_thread_handle_t*)
            calloc(n_workers, sizeof(bpvm_platform_thread_handle_t));
    if (!smp->worker_threads) goto fail_cond;

    vm->smp = smp;

    /* Arranca comm task (host: drena queue a stdout). */
    if (bpvm_comm_start(vm) != 0) {
        vm->smp = NULL;
        free(smp->worker_threads);
        goto fail_cond;
    }
    return 0;

fail_cond:
    bpvm_platform_cond_destroy(&smp->sched_cond);
fail_lock:
    bpvm_platform_mutex_destroy(&smp->vm_lock);
fail_free:
    free(smp);
    return -1;
}

void bpvm_smp_destroy(bpvm_t* vm) {
    if (!vm || !vm->smp) return;
    bpvm_smp_t* smp = vm->smp;
    /* Señal a workers de que terminen + drena comm task. */
    bpvm_platform_mutex_lock(&smp->vm_lock);
    smp->shutdown = true;
    bpvm_platform_cond_broadcast(&smp->sched_cond);
    bpvm_platform_mutex_unlock(&smp->vm_lock);
    /* Workers ya joineados en scheduler_run_smp; aquí solo cleanup. */
    bpvm_comm_stop(vm);
    free(smp->worker_threads);
    bpvm_platform_cond_destroy(&smp->sched_cond);
    bpvm_platform_mutex_destroy(&smp->vm_lock);
    free(smp);
    vm->smp = NULL;
}

/* Entry point: spawnea workers, joins, returns. */
int bpvm_scheduler_run_smp(bpvm_t* vm) {
    if (!vm || !vm->smp) return -1;
    bpvm_smp_t* smp = vm->smp;

    /* Spawnea N workers. Modelo ASIMÉTRICO (#153): core 0 reservado a
     * I/O (USB CDC + REPL + comm task), los workers de la VM van a
     * core 1+. Así el cómputo del intérprete NUNCA compite con el USB
     * ni con el draining de salida — "TX-exclusivo": un solo core habla
     * con el periférico. Con NUM_CORES=2 y 1 worker: VM en core 1, I/O
     * en core 0 — VM-cómputo concurrente con I/O sin paralelismo entre
     * threads BP (que se turnan cooperativamente en ese único worker).
     * Con 1 core (o en host pthread) el pin se ignora — no-op. */
    for (int i = 0; i < smp->n_workers; i++) {
        worker_arg_t* arg = (worker_arg_t*) malloc(sizeof(*arg));
        if (!arg) return -1;
        arg->vm = vm;
        arg->worker_id = i;
        int core_id = 1;         /* core 1 = "core de la VM"; core 0 = I/O */
        if (bpvm_platform_thread_create_pinned(&smp->worker_threads[i],
                                                worker_loop, arg,
                                                core_id) != 0) {
            free(arg);
            return -1;
        }
    }

    /* Joins. Si cualquier worker termina con shutdown=true, todos
     * salen y aquí los recogemos en orden. */
    for (int i = 0; i < smp->n_workers; i++) {
        bpvm_platform_thread_join(&smp->worker_threads[i]);
    }
    return 0;
}
