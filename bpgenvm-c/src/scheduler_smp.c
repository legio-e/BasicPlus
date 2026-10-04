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
#include "bpvm_io.h"   /* V6/A1: si hay `io`, la comm task sobra */
#include "bpvm_internal.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

/* ============================================================ */
/*  Helpers (bajo vm_lock)                                       */
/* ============================================================ */

/* V7 #616 (1-oct) — EN RUEDA, como scheduler.c y como miVM (su cola): se busca desde el siguiente al último que se
 * eligió, no desde el primero (buscando siempre desde el primero, con más hilos listos que workers, los primeros
 * acaparaban). V7/H12, etapa 2: sobre LA COLA de hilos (threading.c, bpvm_hilos_elegir), una vuelta como mucho. H2 —
 * sólo uno RUNNABLE Y que nadie tenga (sched_owner == -1): el status lo escribe el intérprete sin lock, así que no
 * basta; el worker que tiene el tc lo marca con su wid bajo vm_lock y lo suelta bajo vm_lock al volver. */

static int any_alive_locked(const bpvm_t* vm) {
    for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->status != BPVM_THREAD_TERMINATED) return 1;
    return 0;
}

static int64_t earliest_wake_locked(const bpvm_t* vm) {
    int64_t min = INT64_MAX;
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
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
    /* #342 — un thread que terminó dejando eventos SUYOS encolados es otra
     * razón para volver a ser elegible, igual que un sleep vencido o un join
     * completado. La regla vive en events.c y la llaman los DOS schedulers:
     * una sola verdad, no una copia por scheduler.
     *
     * V7/H7 #595 (28-sep) — y ANTES de los joins, como en scheduler.c: si no, el
     * que espera a ese thread lo ve TERMINATED, se despierta y sigue antes de que
     * el handler post-mortem corra (la revisión de #595: «join vuelto» salía antes
     * que el handler, sólo con --smp; en miVM y sin SMP, después). Resucitado,
     * vuelve a estar RUNNABLE y el join espera a que salde su deuda. */
    woken += bpvm_events_revive_terminated(vm);
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= now) {
            tc->status = BPVM_THREAD_RUNNABLE;
            woken++;
        } else if (tc->status == BPVM_THREAD_BLOCKED_JOIN && tc->blocked_on_join >= 0) {
            /* Su hilo terminó —o ya salió de la cola: su handle no resuelve (H12, etapa 2)—, y sin deuda: el
             * TERMINATED se escribe sin cerrojo (interp.c), así que pudo llegar entre la resurrección de arriba y esta
             * línea; con deuda, la siguiente pasada lo resucita. */
            const bpvm_thread_t* t = bpvm_hilo_por_tid(vm, tc->blocked_on_join);
            if (t == NULL || (t->status == BPVM_THREAD_TERMINATED && !bpvm_events_con_deuda(vm, t))) {
                tc->blocked_on_join = -1;
                tc->status = BPVM_THREAD_RUNNABLE;
                woken++;
            }
        }
    }
    /* H12, etapa 2 — los que acabaron del todo salen de la cola: su struct y su pila, devueltas. */
    bpvm_hilos_recoger(vm);
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
    bpvm_free(arg);

    for (;;) {
        /* P-run-stop (#257) — poll cooperativo del transporte ENTRE quanta
         * (fuera del lock; con 1 worker es exactamente entre quanta).
         * V7/H7 (27-sep) — con el hilo `io` en marcha el wire es SUYO (un KILL
         * llega como `kill_requested`), así que aquí no se mira: la guarda
         * `vm->io == NULL` de scheduler.c, que no había viajado a este fichero
         * (la auditoría de capas, A3). Sin ella, dos hilos atenderían el mismo
         * transporte. `test-kill`, fases 4-6. */
        if (vm->io == NULL && vm->poll_cb != NULL
                && vm->poll_cb(vm, vm->poll_user) != 0)
            vm->kill_requested = 1;

        /* ------- Scheduler decision bajo vm_lock ------- */
        bpvm_thread_t* tc = NULL;
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
            tc = bpvm_hilos_elegir(vm, 1);
            if (tc != NULL) break;
            if (!any_alive_locked(vm)) {
                smp->shutdown = true;
                bpvm_platform_cond_broadcast(&smp->sched_cond);
                bpvm_platform_mutex_unlock(&smp->vm_lock);
                return;
            }
            /* P-run-stop — sin RUNNABLE: poll del transporte fuera del
             * lock antes de esperar (un programa dormido también tiene
             * que poder morir). Con `io`, no: es suyo (arriba). */
            if (vm->io == NULL && vm->poll_cb != NULL) {
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
        /* H5.c — ENTRE QUANTA: inyecta el handler de un evento pendiente para
         * este thread. Va DENTRO del vm_lock y después de tomar sched_owner:
         * el tc es nuestro y nadie más lo está ejecutando, así que su
         * pc/sp/bp/cs son los buenos y la cola no se toca en paralelo. */
        bpvm_event_drain_one(vm, tc);
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
        /* V7/H7 #608 (29-sep, Eduardo: la A) — como en scheduler.c: la excepción sin atrapar de un hilo que no es el
         * main acaba ese hilo, no el RUN; al final, el RUN dice que hubo un error (bpvm_scheduler_run_smp). */
        if (s == BPVM_ERR_RUNTIME && tc->status == BPVM_THREAD_TERMINATED && tc->id != 0) {
            vm->hilo_sin_atrapar = 1;
            s = BPVM_OK;
        }
        if (s != BPVM_OK) {
            /* Fallo terminal del intérprete → shutdown VM. */
            bpvm_diag_urgente("[bpvm-c smp w%d] interp error: status=%d",
                    wid, (int) s);   /* era `arg ? 0 : 0`, con `arg` ya liberado: siempre 0 (el mapa del 4-oct) */
            if (smp->fallo == 0) smp->fallo = (int) s;   /* #596: el RUN acaba con ÉSTE */
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

    bpvm_smp_t* smp = (bpvm_smp_t*) bpvm_calloc(1, sizeof(bpvm_smp_t));
    if (!smp) return -1;
    smp->n_workers = n_workers;
    smp->shutdown = false;
    smp->fallo = 0;   /* #596 (el calloc ya lo deja a 0; se dice) */
    smp->stop_the_world = false;
    smp->gc_in_progress = false;
    smp->running_workers = 0;

    if (bpvm_platform_mutex_init(&smp->vm_lock) != 0) goto fail_free;
    if (bpvm_platform_cond_init(&smp->sched_cond) != 0) goto fail_lock;

    smp->worker_threads = (bpvm_platform_thread_handle_t*)
            bpvm_calloc(n_workers, sizeof(bpvm_platform_thread_handle_t));
    if (!smp->worker_threads) goto fail_cond;

    vm->smp = smp;

    /* Arranca comm task (host: drena queue a stdout). */
    /* V6/A1 — con el hilo `io` en marcha, la salida ya la drena el: la comm task
     * de este camino sobra y arrancarla seria tener DOS consumidores para una
     * cola que solo usa uno. Se conserva para el camino SMP puro (sin io), que
     * es el que se rehara cuando lleguen las dos VM sobre una cola de threads. */
    smp->comm_task_on = !bpvm_io_running(vm);
    if (smp->comm_task_on && bpvm_comm_start(vm) != 0) {
        vm->smp = NULL;
        bpvm_free(smp->worker_threads);
        goto fail_cond;
    }
    return 0;

fail_cond:
    bpvm_platform_cond_destroy(&smp->sched_cond);
fail_lock:
    bpvm_platform_mutex_destroy(&smp->vm_lock);
fail_free:
    bpvm_free(smp);
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
    if (vm->smp->comm_task_on) bpvm_comm_stop(vm);
    bpvm_free(smp->worker_threads);
    bpvm_platform_cond_destroy(&smp->sched_cond);
    bpvm_platform_mutex_destroy(&smp->vm_lock);
    bpvm_free(smp);
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
    int creados = 0;
    for (int i = 0; i < smp->n_workers; i++) {
        worker_arg_t* arg = (worker_arg_t*) bpvm_malloc(sizeof(*arg));
        if (!arg) break;
        arg->vm = vm;
        arg->worker_id = i;
        int core_id = 1;         /* core 1 = "core de la VM"; core 0 = I/O */
        if (bpvm_platform_thread_create_pinned(&smp->worker_threads[i],
                                                worker_loop, arg,
                                                core_id) != 0) {
            bpvm_free(arg);
            break;
        }
        creados++;
    }
    if (creados < smp->n_workers) {
        /* #473 (punto 7; 3-oct) — UN WORKER QUE NO NACE: los que ya corren se paran y se UNEN antes de volver. Se
         * volvía con -1 dejándolos en marcha sobre una VM que el llamador da por acabada (y destruye). Miran
         * `shutdown` en cada decisión del planificador; el aviso despierta a los que esperan en la condición. */
        bpvm_platform_mutex_lock(&smp->vm_lock);
        smp->shutdown = true;
        bpvm_platform_cond_broadcast(&smp->sched_cond);
        bpvm_platform_mutex_unlock(&smp->vm_lock);
        for (int i = 0; i < creados; i++) bpvm_platform_thread_join(&smp->worker_threads[i]);
        return -1;
    }

    /* Joins. Si cualquier worker termina con shutdown=true, todos
     * salen y aquí los recogemos en orden. */
    for (int i = 0; i < smp->n_workers; i++) {
        bpvm_platform_thread_join(&smp->worker_threads[i]);
    }
    /* #342 — mismo guardián de salida que el scheduler simple: si queda algo
     * en la cola, que se sepa. Tirar eventos en silencio es la familia de bug
     * de #326. */
    if (vm->ev_count > 0) {
        bpvm_diag("[bpvm] fin de ejecución con %d evento(s) sin atender "
                        "(destinatario muerto o encolados por un handler tardío)",
                vm->ev_count);
    }
    /* #596 — el estado del primer fallo (0 si ninguno), como scheduler.c devuelve el suyo: una excepción sin
     * atrapar acaba el RUN en error también con --smp. V7/H7 #608: y la de un hilo que no es el main, que no acaba el
     * RUN pero sí lo deja en error al final. */
    if (smp->fallo == 0 && vm->hilo_sin_atrapar) return BPVM_ERR_RUNTIME;
    return smp->fallo;
}
