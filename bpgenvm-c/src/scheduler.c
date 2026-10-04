/*
 * scheduler.c — LA COLA y su PLANIFICADOR (V7/H12, etapa 2a.2; el diseño, docs/V7_IDEAS.md §H12 «Etapa 2a.2»).
 *
 * La cola es PERMANENTE (bpvm_smp.h): nace con el arranque y no muere. Un programa se CUELGA en ella —su main a la
 * cola, prepararlo, publicarlo— y lo ejecutan los RUNS: en una placa, la tarea `run`, que espera dormida en la cola
 * mientras no hay programa; en el PC, la STM32 y las pruebas de C, el hilo que llama a bpvm_run (y los de
 * bpvm_run_smp). El último run que sale lo cierra, lo descuelga y avisa a quien lo colgó.
 *
 * UN planificador para todos. Antes eran dos que no hacían lo mismo: el simple (F4) y el de varios workers (H2). Éste
 * tiene lo que sólo tenía el simple —ceder el núcleo entre cuantos (#462), el interbloqueo, el KILL que acaba ya (y que
 * ahora despierta: bpvm_request_kill), y el depurador, cuya pausa para al run (con uno, al programa entero)— y lo que
 * sólo tenía el de varios —el cerrojo, el mundo parado del GC (heap.c), el primer fallo (#596)—. Con UN run el cerrojo
 * no se toca en la vuelta de cada cuanto: nadie más toca la rueda (`io` sólo pone kill_requested y avisa); se toma para
 * dormir y para descolgar, y no se paga nada que el simple no pagara.
 *
 * Todo recorrido de los hilos da UNA vuelta (threading.c).
 */

#include "bpvm_internal.h"
#include "bpvm_comm.h"
#include "bpvm_io.h"
#include "bpvm_platform.h"
#include <limits.h>
#include <stdio.h>

bpvm_cola_t bpvm_la_cola;

int bpvm_cola_asegurar(void) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (q->iniciada) return 0;
    if (bpvm_platform_mutex_init(&q->vm_lock) != 0) return -1;
    if (bpvm_platform_cond_init(&q->sched_cond) != 0) {
        bpvm_platform_mutex_destroy(&q->vm_lock);
        return -1;
    }
    q->iniciada = 1;
    return 0;
}

int bpvm_cola_registrar_permanente(void) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (bpvm_cola_asegurar() != 0) return -1;
    bpvm_platform_mutex_lock(&q->vm_lock);
    q->permanentes++;
    bpvm_platform_mutex_unlock(&q->vm_lock);
    return 0;
}

void bpvm_cola_baja_permanente(void) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (!q->iniciada) return;
    bpvm_platform_mutex_lock(&q->vm_lock);
    if (q->permanentes > 0) q->permanentes--;
    bpvm_platform_mutex_unlock(&q->vm_lock);
}

void bpvm_cola_despertar(void) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (!q->iniciada) return;
    bpvm_platform_mutex_lock(&q->vm_lock);
    bpvm_platform_cond_broadcast(&q->sched_cond);
    bpvm_platform_mutex_unlock(&q->vm_lock);
}

/* ─── COLGAR un programa: el main a la cola, prepararlo, publicarlo ──────────────────────────────────────────────── */

int bpvm_cola_colgar(bpvm_t* vm, int n_runs, int para_permanentes, bpvm_cola_al_acabar_fn al_acabar, void* user) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (!vm) return BPVM_ERR_BAD_PC;
    if (bpvm_cola_asegurar() != 0) return BPVM_ERR_OOM;
    if (n_runs < 1) n_runs = 1;
    /* 1. El main, a la cola: desde aquí la cola es de este programa, hasta que se destruya. Antes de prepararlo: la
     *    preparación reserva memoria (las excepciones prefabricadas, con el main de ancla) y el GC busca sus raíces en
     *    la cola. */
    bpvm_platform_mutex_lock(&q->vm_lock);
    int r = bpvm_hilos_meter_main(vm);
    bpvm_platform_mutex_unlock(&q->vm_lock);
    if (r != 0) {
        bpvm_diag_urgente("[bpvm] la cola de hilos es de otro programa (o éste ya corrió): un programa a la vez (H12)");
        return BPVM_ERR_RUNTIME;
    }
    /* 2. Prepararlo (bpvm.c): enlazar, el main desde el punto de entrada, las excepciones prefabricadas. Si falla, no se
     *    publica; sus hilos se van con él al destruirlo. */
    bpvm_status_t ps = bpvm_programa_preparar(vm);
    if (ps != BPVM_OK) return ps;
    /* Con más de un run, el cerrojo de verdad (vm->smp) y, sin `io`, la comm task que saca la salida. Antes de
     * publicarlo: todavía no lo ejecuta nadie. */
    if (n_runs > 1) {
        vm->smp = q;
        q->comm_task_on = !bpvm_io_running(vm);
        if (q->comm_task_on && bpvm_comm_start(vm) != 0) {
            q->comm_task_on = 0;
            vm->smp = NULL;
            return BPVM_ERR_OOM;
        }
    }
    /* 3. Publicarlo: desde aquí lo ven los runs. */
    bpvm_platform_mutex_lock(&q->vm_lock);
    q->n_runs = n_runs;
    q->para_permanentes = para_permanentes ? 1 : 0;
    q->acabando = 0;
    q->con_hilo = 0;
    q->parar = 0;
    q->fallo = 0;
    q->rs = BPVM_OK;
    q->al_acabar = al_acabar;
    q->al_acabar_user = user;
    q->stop_the_world = false;
    q->gc_in_progress = false;
    q->running_workers = 0;
    q->t_ultimo_cuanto = q->t_fin = 0;
    q->programa = vm;
    bpvm_platform_cond_broadcast(&q->sched_cond);
    bpvm_platform_mutex_unlock(&q->vm_lock);
    return BPVM_OK;
}

/* ─── el planificador ────────────────────────────────────────────────────────────────────────────────────────────── */

/* ¿Algún hilo no terminado? */
static int hay_vivos(const bpvm_t* vm) {
    for (const bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->status != BPVM_THREAD_TERMINATED) return 1;
    return 0;
}

/* El sueño que vence antes; INT64_MAX si nadie duerme. */
static int64_t sueno_mas_cercano(const bpvm_t* vm) {
    int64_t min = INT64_MAX;
    for (const bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->status == BPVM_THREAD_BLOCKED_SLEEP && h->wake_at_ms < min) min = h->wake_at_ms;
    return min;
}

/* Despierta a quien toca: los muertos con eventos SUYOS pendientes (#342: vuelven a ser elegibles para atenderlos; y
 * ANTES que los join, #595: si no, el que espera a uno lo ve TERMINATED y sigue antes de que corra su handler
 * post-mortem), los sueños vencidos y los join cumplidos —su hilo terminó sin deuda, o ya salió de la cola: su handle
 * no resuelve (H12)—. Con varios runs el TERMINATED lo escribe el intérprete sin cerrojo y pudo llegar entre la
 * resurrección y el join: con deuda, la siguiente pasada lo resucita. */
static void despertar(bpvm_t* vm) {
    bpvm_events_revive_terminated(vm);
    int64_t ahora = bpvm_platform_now_ms();
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= ahora) {
            tc->status = BPVM_THREAD_RUNNABLE;
        } else if (tc->status == BPVM_THREAD_BLOCKED_JOIN && tc->blocked_on_join >= 0) {
            const bpvm_thread_t* t = bpvm_hilo_por_tid(vm, tc->blocked_on_join);
            if (t == NULL || (t->status == BPVM_THREAD_TERMINATED && !bpvm_events_con_deuda(vm, t))) {
                tc->blocked_on_join = -1;
                tc->status = BPVM_THREAD_RUNNABLE;
            }
        }
    }
}

/* DORMIR en la cola: `ms` < 0, sin plazo. Con un run el cerrojo se coge aquí; con varios ya se tiene (y se tiene al
 * volver). Antes de dormirse se mira lo que lo despertaría —un KILL, que el programa ya no sea el suyo—, con el cerrojo
 * cogido: bpvm_request_kill avisa con el mismo cerrojo, y el aviso no se pierde. */
static void dormir(bpvm_cola_t* q, const bpvm_t* vm, int varios, int ms) {
    if (!varios) bpvm_platform_mutex_lock(&q->vm_lock);
    if (!vm->kill_requested && q->programa == vm && !q->acabando) {
        if (ms < 0) bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);
        else        (void) bpvm_platform_cond_timed_wait(&q->sched_cond, &q->vm_lock, ms);
    }
    if (!varios) bpvm_platform_mutex_unlock(&q->vm_lock);
}

/* EL FIN, por el último run que sale: nadie más tiene un hilo entre manos. Con varios se entra con el cerrojo y se
 * suelta: `acabando` les dice a los demás que ya no lo toquen. Las líneas a medias pueden tener que esperar a `io`, que
 * vacía la salida, y `io` coge el cerrojo para avisar de un KILL: nunca se escribe con él cogido. Lo último,
 * descolgarlo y avisar a quien lo colgó: desde ahí este run no toca el programa (quien lo colgó lo destruye). */
static void acabar(bpvm_cola_t* q, bpvm_t* vm, int varios) {
    q->acabando = 1;
    if (q->ensayo_tras_cuanto_ms > 0) q->t_fin = bpvm_platform_now_us();   /* SÓLO PRUEBAS */
    if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
    /* #342 — GUARDIÁN DE SALIDA. Si aún queda algo en la cola de eventos es que su destinatario no existe, o que un
     * handler post-mortem levantó eventos después de agotarse el presupuesto. Tirarlos es legítimo; tirarlos EN
     * SILENCIO es la familia de bug que costó #326. Que lo diga. */
    if (vm->ev_count > 0) {
        bpvm_diag("[bpvm] fin de ejecución con %d evento(s) sin atender "
                  "(destinatario muerto o encolados por un handler tardío)", vm->ev_count);
    }
    bpvm_programa_cerrar(vm);   /* las líneas a medias (#600) y las líneas `pila:` e `hilos:` */
    /* #462 — UN PROGRAMA AL QUE SE LE PIDIÓ PARAR DICE QUE FUE MATADO, SIEMPRE, aunque acabase solo: si no, el estado de
     * un `stop` dependía de DÓNDE cayera el KILL (medido el 5-sep con GuiColorDemo: `Gui.run()` acaba limpio al ver el
     * KILL, y el IDE recibía `exit 0`). La bandera es la verdad; también cuando el main muere de una excepción por el
     * kill de una native sin ejecutor (#640). Si no, el primer fallo (#596); si no, RUNTIME_ERROR si un hilo que no es
     * el main murió sin atrapar (#608). */
    int rs;
    if (vm->kill_requested)        rs = BPVM_KILLED;
    else if (q->fallo != 0)        rs = q->fallo;
    else if (vm->hilo_sin_atrapar) rs = BPVM_ERR_RUNTIME;
    else                           rs = BPVM_OK;
    if (q->comm_task_on) {          /* después de las líneas a medias, que salen por ella */
        bpvm_comm_stop(vm);
        q->comm_task_on = 0;
    }
    bpvm_platform_mutex_lock(&q->vm_lock);
    bpvm_cola_al_acabar_fn cb = q->al_acabar;
    void* user = q->al_acabar_user;
    q->al_acabar = NULL;
    q->al_acabar_user = NULL;
    q->rs = rs;
    vm->smp = NULL;
    q->programa = NULL;           /* DESCOLGADO */
    q->acabando = 0;
    bpvm_platform_cond_broadcast(&q->sched_cond);
    bpvm_platform_mutex_unlock(&q->vm_lock);
    if (cb) cb(vm, rs, user);
}

/* Un run sobre UN programa, hasta que acaba (o lo acaba otro run). */
static void correr_programa(bpvm_cola_t* q, bpvm_t* vm, int id) {
    const int varios = (q->n_runs > 1);
    for (;;) {
        /* P-run-stop (#257) — SIN `io`, el cable lo mira el planificador entre cuantos. Con `io` es suyo y un KILL llega
         * como kill_requested (A3: dos hilos no atienden el mismo transporte; `test-kill`, fases 4-6). */
        if (vm->io == NULL && vm->poll_cb != NULL && vm->poll_cb(vm, vm->poll_user) != 0)
            bpvm_request_kill(vm);

        /* #462 — CEDER EL NÚCLEO ENTRE CUANTOS. Observación de Eduardo (31-ago): *«en las STM32, cuando se ejecuta un
         * programa, el resto del sistema sigue vivo, incluidas las comunicaciones; en las ESP32 se utilizan todos los
         * recursos y se queda todo bloqueado»*: el planificador sólo cedía cuando no había ningún hilo listo, y bajo
         * FreeRTOS se comía todo lo que estuviera a su prioridad o por debajo, incluido lo que bombea LVGL. Coste: una
         * llamada por cuanto (1024 opcodes); en la STM32 es nula. */
        bpvm_platform_thread_yield();

        if (varios) bpvm_platform_mutex_lock(&q->vm_lock);
        if (q->programa != vm || q->acabando) {       /* ya no es suyo: lo cierra otro run, o lo cerró */
            if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
            return;
        }
        if (varios && q->stop_the_world) {             /* otro run colecta (heap.c): a que acabe */
            bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);
            bpvm_platform_mutex_unlock(&q->vm_lock);
            continue;
        }
        /* «Nadie tiene un hilo entre manos» (bpvm_smp.h: `con_hilo`, no `running_workers`). */
        const int con_hilo = q->sabotaje_contador ? q->running_workers : q->con_hilo;

        /* EL KILL, un fallo o el STOP del depurador: no se elige más; y cuando nadie tenga un hilo entre manos, el
         * fin. */
        if (vm->kill_requested || q->parar) {
            q->parar = 1;
            if (con_hilo == 0 || q->sabotaje_fin) { acabar(q, vm, varios); return; }
            dormir(q, vm, varios, -1);
            if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
            continue;
        }

        /* Despertar a quien toca, y fuera de la cola los que acabaron del todo (H12: su struct y su pila, devueltas). */
        despertar(vm);
        bpvm_hilos_recoger(vm);

        /* Elegir en rueda: desde el siguiente al último elegido (#616). Con varios runs, sólo uno que no tenga otro. */
        bpvm_thread_t* tc = bpvm_hilos_elegir(vm, varios);
        if (tc == NULL) {
            int ms = -1;
            if (!hay_vivos(vm)) {
                /* EL FIN: no queda nadie, y nadie tiene uno entre manos (con varios, el que lo tiene puede no haber
                 * vuelto todavía: se le espera). */
                if (con_hilo == 0 || q->sabotaje_fin) { acabar(q, vm, varios); return; }
            } else {
                int64_t pronto = sueno_mas_cercano(vm);
                if (pronto == INT64_MAX) {
                    /* EL INTERBLOQUEO: todos bloqueados, ninguno durmiendo y nadie con uno entre manos —con varios, el
                     * que tiene uno puede despertar a otro (soltar un Mutex, acabar): se le espera—. */
                    if (con_hilo == 0) {
                        bpvm_diag_urgente("[bpvm-c sched] deadlock — todos los threads "
                                          "bloqueados sin posibilidad de progreso");
                        if (q->fallo == 0) q->fallo = BPVM_ERR_RUNTIME;
                        q->parar = 1;
                        acabar(q, vm, varios);
                        return;
                    }
                } else {
                    int64_t dt = pronto - bpvm_platform_now_ms();
                    ms = (dt <= 0) ? 0 : (dt > INT_MAX ? INT_MAX : (int) dt);
                }
            }
            /* P-run-stop — sin `io`, el cable cada 50 ms como mucho: un programa dormido también tiene que poder
             * morir. Con `io`, el KILL despierta (bpvm_request_kill). */
            if (vm->io == NULL && vm->poll_cb != NULL && (ms < 0 || ms > 50)) ms = 50;
            if (ms != 0) dormir(q, vm, varios, ms);
            if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
            continue;
        }

        /* Un cuanto del elegido. H5.c — antes, su evento pendiente: el tc no está corriendo, así que su pc/sp/bp/cs
         * son los buenos (con varios, bajo el cerrojo: la cola de eventos no se toca en paralelo). Uno por vuelta: dos
         * marcos seguidos se ejecutarían en orden inverso, y son FIFO. */
        tc->status = BPVM_THREAD_RUNNING;
        tc->sched_owner = id;
        bpvm_event_drain_one(vm, tc);
        q->con_hilo++;
        if (varios) {
            q->running_workers++;
            bpvm_platform_mutex_unlock(&q->vm_lock);
        }
        int yielded = 0;
        int quantum = vm->quantum_ops > 0 ? vm->quantum_ops : 1024;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, quantum, &yielded);
        /* SÓLO PRUEBAS — la ventana ensanchada, más larga en cada run (el 0, la de la prueba; el 1, cinco veces): uno
         * vuelve claramente antes que el otro. */
        if (q->ensayo_tras_cuanto_ms > 0) bpvm_platform_thread_sleep_ms(q->ensayo_tras_cuanto_ms * (1 + 4 * id));
        if (varios) {
            bpvm_platform_mutex_lock(&q->vm_lock);
            q->running_workers--;
            if (q->stop_the_world) bpvm_platform_cond_broadcast(&q->sched_cond);   /* el colector espera quedarse solo */
        }
        /* El cuanto agotado lo deja RUNNING: a la rueda otra vez. Si cedió o terminó, su estado ya es el bueno. */
        if (tc->status == BPVM_THREAD_RUNNING) tc->status = BPVM_THREAD_RUNNABLE;
        tc->sched_owner = -1;
        if (q->ensayo_tras_cuanto_ms > 0) q->t_ultimo_cuanto = bpvm_platform_now_us();   /* SÓLO PRUEBAS */
        q->con_hilo--;
        /* V7/H7 #608 (29-sep, Eduardo: la A) — una excepción sin atrapar en un hilo que NO es el main acaba ESE hilo;
         * los demás siguen, como en Java. Al final, el RUN dice que hubo un error (acabar). La del main sí acaba el
         * programa. */
        if (s == BPVM_ERR_RUNTIME && tc->status == BPVM_THREAD_TERMINATED && tc->id != 0) {
            vm->hilo_sin_atrapar = 1;
            s = BPVM_OK;
        }
        if (s != BPVM_OK) {
            /* Un fallo de la VM, una excepción sin atrapar en el main, el STOP del depurador: el primero manda (#596). */
            if (varios) bpvm_diag_urgente("[bpvm-c smp w%d] interp error: status=%d", id, (int) s);
            if (q->fallo == 0) q->fallo = (int) s;
            q->parar = 1;
        }
        if (varios) {
            bpvm_platform_cond_broadcast(&q->sched_cond);   /* un hilo terminado o soltado puede despertar a otros */
            bpvm_platform_mutex_unlock(&q->vm_lock);
        }
    }
}

void bpvm_cola_correr(int permanente, int id) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (bpvm_cola_asegurar() != 0) return;
    const int para = permanente ? 1 : 0;
    for (;;) {
        bpvm_platform_mutex_lock(&q->vm_lock);
        while (q->programa == NULL || q->para_permanentes != para || q->acabando) {
            if (!permanente) {                 /* el suyo ya acabó (o lo está cerrando otro): vuelve */
                bpvm_platform_mutex_unlock(&q->vm_lock);
                return;
            }
            bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);   /* sin programa: dormido en la cola */
        }
        bpvm_t* vm = q->programa;
        bpvm_platform_mutex_unlock(&q->vm_lock);
        correr_programa(q, vm, id);
        if (!permanente) return;
    }
}
