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
#include "bpvm_bus.h"     /* V7/H12, 2b: el árbitro de los buses, creado con un solo hilo vivo */
#include "bpvm_spi.h"     /* y la memoria de la fachada SPI */
#include <limits.h>
#include <stdio.h>

bpvm_cola_t bpvm_la_cola;

/* V7/H12, 2b — el cerrojo de la zona de rascar (bpvm_util.c): el de la cola, que es permanente. */
static void zona_cerrar(void) { bpvm_platform_mutex_lock(&bpvm_la_cola.vm_lock); }
static void zona_abrir(void)  { bpvm_platform_mutex_unlock(&bpvm_la_cola.vm_lock); }

int bpvm_cola_asegurar(void) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (q->iniciada) return 0;
    if (bpvm_platform_mutex_init(&q->vm_lock) != 0) return -1;
    if (bpvm_platform_cond_init(&q->sched_cond) != 0) {
        bpvm_platform_mutex_destroy(&q->vm_lock);
        return -1;
    }
    if (bpvm_platform_cond_init(&q->mundo_cond) != 0) {  /* V7/H12, la 3.2: la del mundo parado (bpvm_smp.h) */
        bpvm_platform_cond_destroy(&q->sched_cond);
        bpvm_platform_mutex_destroy(&q->vm_lock);
        return -1;
    }
    if (bpvm_platform_mutex_init(&q->hal_lock) != 0) {   /* V7/H12, 2b: el de la HAL (bpvm_smp.h) */
        bpvm_platform_cond_destroy(&q->mundo_cond);
        bpvm_platform_cond_destroy(&q->sched_cond);
        bpvm_platform_mutex_destroy(&q->vm_lock);
        return -1;
    }
    q->iniciada = 1;
    /* Esto corre con un solo hilo vivo (bpvm_init, o el arranque que registra la tarea `run`): lo que tiene que estar
     * antes de que haya dos —la zona de rascar con su cerrojo; y el árbitro de los buses, que una placa crea en el
     * arranque pero el PC creaba al primer turno, con dos runs a la vez (la revisión de la 2b)—. */
    bpvm_scratch_set_cerrojo(zona_cerrar, zona_abrir);
    bpvm_bus_arbitro_init();
    bpvm_spi_preparar();   /* la memoria de la fachada SPI, que se iniciaba al primer uso */
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

void bpvm_cola_aviso_mundo(bpvm_cola_t* q) {
    bpvm_platform_cond_broadcast(&q->mundo_cond);
    if (q->sabotaje_mundo) bpvm_platform_cond_broadcast(&q->sched_cond);   /* SÓLO PRUEBAS: como antes (test-cola 17) */
}

/* ─── COLGAR un programa: el main a la cola, prepararlo, publicarlo ──────────────────────────────────────────────── */

/* Un programa que no se puede colgar: el porqué, al EXITED (runtime_error) y al log, no un «RuntimeError no atrapado»
 * mudo (la revisión de la 2a.2). */
static int no_se_cuelga(bpvm_t* vm, const char* porque) {
    snprintf(vm->runtime_error, sizeof vm->runtime_error, "%s", porque);
    bpvm_diag_urgente("[bpvm] %s", porque);
    return BPVM_ERR_RUNTIME;
}

int bpvm_cola_colgar(bpvm_t* vm, int n_runs, int para_permanentes, bpvm_cola_al_acabar_fn al_acabar, void* user) {
    bpvm_cola_t* q = &bpvm_la_cola;
    if (!vm) return BPVM_ERR_BAD_PC;
    if (bpvm_cola_asegurar() != 0) return BPVM_ERR_OOM;
    if (n_runs < 1) n_runs = 1;
    /* 1. El main, a la cola: desde aquí la cola es de este programa, hasta que se destruya. Antes de prepararlo: la
     *    preparación reserva memoria (las excepciones prefabricadas, con el main de ancla) y el GC busca sus raíces en
     *    la cola. Para los runs permanentes, que los haya: si no, quien lo cuelga esperaría para siempre. */
    bpvm_platform_mutex_lock(&q->vm_lock);
    int sin_runs = para_permanentes && q->permanentes == 0;
    int r = sin_runs ? 0 : bpvm_hilos_meter_main(vm);
    bpvm_platform_mutex_unlock(&q->vm_lock);
    if (sin_runs) return no_se_cuelga(vm, "no hay ningún run permanente en la cola que lo ejecute (H12)");
    if (r != 0) return no_se_cuelga(vm, "la cola de hilos es de otro programa (o éste ya corrió): un programa a la vez (H12)");
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
    q->cuantos[0] = q->cuantos[1] = 0;
    q->gc_con_otro_dentro = 0;
    q->vueltas_sin_hilo = 0;
    q->gui_viajes = 0;
    q->gui_fuera_del_run0 = 0;
    q->nativas_aparcadas = 0;
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
    /* V7/H12, 2b (la revisión) — LO QUE OTRO RUN TIENE ENTRE MANOS NO SE TOCA, ni se da por acabado (la regla de la
     * resurrección, events.c): su estado lo escribe ese run sin cerrojo, a mitad de su cuanto —un `sleep` pone la hora y
     * el estado en dos escrituras; un hilo que termina queda TERMINATED antes de soltar su última línea—, y lo deja bueno
     * al devolverlo (bajo el cerrojo, con aviso). Con un run, el que despierta es el único y no tiene ninguno: nada
     * cambia. (SÓLO PRUEBAS: el sabotaje, sin mirarlo.) */
    const int mirar = !bpvm_la_cola.sabotaje_suyo;
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (mirar && tc->sched_owner != -1) continue;
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= ahora) {
            tc->status = BPVM_THREAD_RUNNABLE;
        } else if (tc->status == BPVM_THREAD_BLOCKED_JOIN && tc->blocked_on_join >= 0) {
            const bpvm_thread_t* t = bpvm_hilo_por_tid(vm, tc->blocked_on_join);
            if (t == NULL || (t->status == BPVM_THREAD_TERMINATED && !bpvm_events_con_deuda(vm, t)
                              && (!mirar || t->sched_owner == -1))) {
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
    if (!bpvm_kill_pedido(vm) && q->programa == vm && !q->acabando) {
        if (ms < 0) bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);
        else        (void) bpvm_platform_cond_timed_wait(&q->sched_cond, &q->vm_lock, ms);
    }
    if (!varios) bpvm_platform_mutex_unlock(&q->vm_lock);
}

/* EL FIN, por el último run que sale: nadie más tiene un hilo entre manos. Con varios se entra con el cerrojo:
 * `acabando` les dice a los demás que ya no lo toquen y que salgan, y éste espera a ser el ÚNICO dentro —uno sin hilo
 * entre manos, dormido en la cola o entre dos vueltas, todavía puede mirar la VM: la revisión de la 2a.2—. Las líneas a
 * medias pueden tener que esperar a `io`, que vacía la salida, y `io` coge el cerrojo para avisar de un KILL: nunca se
 * escribe con él cogido. Lo último, descolgarlo —y salir él— y avisar a quien lo colgó: desde ahí ningún run toca el
 * programa (quien lo colgó lo destruye). */
static void acabar(bpvm_cola_t* q, bpvm_t* vm, int varios) {
    q->acabando = 1;
    if (q->ensayo_tras_cuanto_ms > 0) q->t_fin = bpvm_platform_now_us();   /* SÓLO PRUEBAS */
    if (varios) {
        bpvm_platform_cond_broadcast(&q->sched_cond);                     /* los demás: que salgan */
        while (q->dentro > 1 && !q->sabotaje_salida)                       /* (el sabotaje: SÓLO PRUEBAS) */
            bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);
        bpvm_platform_mutex_unlock(&q->vm_lock);
    }
    /* #342 — GUARDIÁN DE SALIDA. Si aún queda algo en la cola de eventos es que su destinatario no existe, o que un
     * handler post-mortem levantó eventos después de agotarse el presupuesto. Tirarlos es legítimo; tirarlos EN
     * SILENCIO es la familia de bug que costó #326. Que lo diga. */
    if (vm->ev_count > 0) {
        bpvm_diag("[bpvm] fin de ejecución con %d evento(s) sin atender "
                  "(destinatario muerto o encolados por un handler tardío)", vm->ev_count);
    }
    bpvm_programa_cerrar(vm);   /* las líneas a medias (#600) y las líneas `pila:` e `hilos:` */
    /* 2b — con varios runs, cuánto trabajó cada uno: la prueba de que el segundo run de verdad ejecuta */
    if (q->n_runs > 1)
        bpvm_diag("[bpvm] runs: %d; cuantos del run 0, %lu; del run 1, %lu; vueltas sin hilo, %lu; viajes al run 0 "
                  "(GUI), %lu; natives aparcadas vistas por el colector, %lu", q->n_runs, (unsigned long) q->cuantos[0],
                  (unsigned long) q->cuantos[1], (unsigned long) q->vueltas_sin_hilo, (unsigned long) q->gui_viajes,
                  (unsigned long) q->nativas_aparcadas);
    if (q->gc_con_otro_dentro > 0)   /* no debe pasar: una colecta con otro run ejecutando (heap.c) */
        bpvm_diag_urgente("[bpvm] GC: %lu colecta(s) con OTRO run dentro de su cuanto (no debe pasar)",
                          (unsigned long) q->gc_con_otro_dentro);
    if (q->gui_fuera_del_run0 > 0)   /* no debe pasar: LVGL y gui.c, tocados desde otro run (builtins.c) */
        bpvm_diag_urgente("[bpvm] GUI: %lu builtin(s) de la GUI FUERA del run 0 (no debe pasar)",
                          (unsigned long) q->gui_fuera_del_run0);
    /* #462 — UN PROGRAMA AL QUE SE LE PIDIÓ PARAR DICE QUE FUE MATADO, SIEMPRE, aunque acabase solo: si no, el estado de
     * un `stop` dependía de DÓNDE cayera el KILL (medido el 5-sep con GuiColorDemo: `Gui.run()` acaba limpio al ver el
     * KILL, y el IDE recibía `exit 0`). La bandera es la verdad; también cuando el main muere de una excepción por el
     * kill de una native sin ejecutor (#640). Si no, el primer fallo (#596); si no, RUNTIME_ERROR si un hilo que no es
     * el main murió sin atrapar (#608). */
    int rs;
    if (bpvm_kill_pedido(vm))      rs = BPVM_KILLED;
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
    q->dentro--;                  /* y sale éste, el último */
    bpvm_platform_cond_broadcast(&q->sched_cond);
    bpvm_platform_mutex_unlock(&q->vm_lock);
    if (cb) cb(vm, rs, user);
}

/* Un run que no lo cierra sale del programa: desde aquí no lo toca. El que lo cierra puede estar esperándolo. Con
 * varios, se entra con el cerrojo cogido. */
static void salir(bpvm_cola_t* q, int varios) {
    if (q->ensayo_salida_ms > 0) {   /* SÓLO PRUEBAS (test_cola): la ventana ensanchada —tarda en contarse fuera— */
        if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
        bpvm_platform_thread_sleep_ms(q->ensayo_salida_ms);
        varios = 0;                  /* (el cerrojo, suelto: se coge abajo) */
    }
    if (!varios) bpvm_platform_mutex_lock(&q->vm_lock);
    q->dentro--;
    bpvm_platform_cond_broadcast(&q->sched_cond);
    bpvm_platform_mutex_unlock(&q->vm_lock);
}

/* Un run sobre UN programa, hasta que acaba (o lo acaba otro run). Entra contado en `dentro` (bpvm_cola_correr) y sale
 * por acabar —si lo cierra él— o por salir: mientras tanto la VM vive, porque el que cierra lo espera. */
static void correr_programa(bpvm_cola_t* q, bpvm_t* vm, int id, int varios) {
    for (;;) {
        /* P-run-stop (#257) — SIN `io`, el cable lo mira el planificador entre cuantos. Con `io` es suyo y un KILL llega
         * como kill_requested (A3: dos hilos no atienden el mismo transporte; `test-kill`, fases 4-6). */
        /* V7 #668 — en la fase del final, no: ese KILL es «pegajoso» (lo pide en cada vuelta) y cortaría la
         * destrucción nada más empezar. La corta su tope (destruir.c); un KILL por `io`, sí, si no es el que la abrió
         * (bpvm_request_kill, #695). */
        if (vm->io == NULL && vm->poll_cb != NULL && !bpvm_destruir_en_fase_final(vm)
                && vm->poll_cb(vm, vm->poll_user) != 0)
            bpvm_request_kill(vm);

        /* #462 — CEDER EL NÚCLEO ENTRE CUANTOS. Observación de Eduardo (31-ago): *«en las STM32, cuando se ejecuta un
         * programa, el resto del sistema sigue vivo, incluidas las comunicaciones; en las ESP32 se utilizan todos los
         * recursos y se queda todo bloqueado»*: el planificador sólo cedía cuando no había ningún hilo listo, y bajo
         * FreeRTOS se comía todo lo que estuviera a su prioridad o por debajo, incluido lo que bombea LVGL. Coste: una
         * llamada por cuanto (1024 opcodes); en la STM32 es nula. */
        bpvm_platform_thread_yield();

        if (varios) bpvm_platform_mutex_lock(&q->vm_lock);
        if (q->programa != vm || q->acabando) {       /* lo está cerrando otro run: a salir (lo espera) */
            salir(q, varios);
            return;
        }
        if (varios && q->stop_the_world) {             /* otro run colecta (heap.c): a que acabe */
            bpvm_platform_cond_wait(&q->mundo_cond, &q->vm_lock);   /* 3.2: la del mundo (bpvm_smp.h) */
            bpvm_platform_mutex_unlock(&q->vm_lock);
            continue;
        }
        bpvm_destruir_vigila(vm);   /* V7 #668: el tope de la destrucción del final */
        /* «Nadie tiene un hilo entre manos» (bpvm_smp.h: `con_hilo`, no `running_workers`). */
        const int con_hilo = q->sabotaje_contador ? q->running_workers : q->con_hilo;

        /* EL KILL, un fallo o el STOP del depurador: no se elige más; y cuando nadie tenga un hilo entre manos, el
         * fin. */
        if (bpvm_kill_pedido(vm) || q->parar) {
            q->parar = 1;
            if (con_hilo == 0 || q->sabotaje_fin) {
                /* V7 #668 — antes de acabar, la destrucción de lo que queda vivo (también tras un KILL o un fallo):
                 * si hay una ronda montada, a ella. */
                if (con_hilo == 0 && bpvm_destruir_fase_final(vm)) { if (varios) bpvm_platform_mutex_unlock(&q->vm_lock); continue; }
                acabar(q, vm, varios);
                return;
            }
            dormir(q, vm, varios, -1);
            if (varios) bpvm_platform_mutex_unlock(&q->vm_lock);
            continue;
        }

        /* Despertar a quien toca, y fuera de la cola los que acabaron del todo (H12: su struct y su pila, devueltas). */
        despertar(vm);
        bpvm_hilos_recoger(vm);

        /* Elegir en rueda: desde el siguiente al último elegido (#616). Con varios runs, sólo uno que no tenga otro; y
         * uno que espera al run 0 (la GUI), sólo el 0. */
        bpvm_thread_t* tc = bpvm_hilos_elegir(vm, varios, id);
        if (tc == NULL) {
            q->vueltas_sin_hilo++;   /* #662 (con varios, bajo el cerrojo) */
            int ms = -1;
            if (!hay_vivos(vm)) {
                /* EL FIN: no queda nadie, y nadie tiene uno entre manos (con varios, el que lo tiene puede no haber
                 * vuelto todavía: se le espera). */
                if (con_hilo == 0 || q->sabotaje_fin) {
                    /* V7 #668 — antes de acabar, la destrucción de lo que queda vivo: si hay una ronda montada, a ella. */
                    if (con_hilo == 0 && bpvm_destruir_fase_final(vm)) { if (varios) bpvm_platform_mutex_unlock(&q->vm_lock); continue; }
                    acabar(q, vm, varios);
                    return;
                }
            } else {
                int64_t pronto = sueno_mas_cercano(vm);
                if (pronto == INT64_MAX) {
                    /* EL INTERBLOQUEO: todos bloqueados, ninguno durmiendo y nadie con uno entre manos —con varios, el
                     * que tiene uno puede despertar a otro (soltar un Mutex, acabar): se le espera—. V7/H12: y uno
                     * listo que sólo puede elegir el run 0 (la GUI) no es un interbloqueo para otro run: es trabajo
                     * del 0, al que su viaje ya avisó. */
                    if (con_hilo == 0 && !(id != 0 && bpvm_hilos_esperan_al_run0(vm))) {
                        bpvm_diag_urgente("[bpvm-c sched] deadlock — todos los threads "
                                          "bloqueados sin posibilidad de progreso");
                        if (q->fallo == 0) q->fallo = BPVM_ERR_RUNTIME;
                        q->parar = 1;
                        /* V7 #668 — también tras un interbloqueo, la destrucción de lo que queda vivo. */
                        if (bpvm_destruir_fase_final(vm)) { if (varios) bpvm_platform_mutex_unlock(&q->vm_lock); continue; }
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
        /* V7/H12 — LA GUI, SÓLO EN EL RUN 0: el que llega de viaje (otro run lo soltó en un builtin de la GUI) repite
         * aquí la instrucción, y desde ahora lo puede elegir cualquiera. Que el otro run lo sepa al acabar este cuanto
         * (abajo): con dos hilos listos, si no, el otro seguiría dormido. */
        const int de_viaje = tc->al_run0;
        tc->al_run0 = 0;
        if (id >= 0 && id < 2) q->cuantos[id]++;
        bpvm_event_drain_one(vm, tc);
        bpvm_destruir_inyecta(vm, tc);   /* V7 #668: y lo pendiente de destruir, como un evento más */
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
            if (q->stop_the_world) bpvm_cola_aviso_mundo(q);   /* el colector espera quedarse solo */
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
            /* #662 (5-oct) — avisar a la cola SÓLO si el cuanto cambió algo que pueda dar trabajo a otro run: el hilo
             * terminó, se bloqueó o cedió (ya no está RUNNABLE), o hubo un fallo. Un cuanto agotado no cambia nada para
             * los demás, y avisar tras CADA uno (cada 1 024 pasos) despertaba en vano al run ocioso: un programa de un
             * hilo iba un 35 % más lento en el PC con dos runs y un 7,3 % en la Pico 2 (V7_IDEAS §H12 «La medida»). Lo
             * que deja listo a OTRO hilo avisa por su cuenta: arrancarlo (bpvm_thread_spawn), soltar un Mutex con
             * esperas (threading.c), el KILL (bpvm_request_kill) y parar el mundo (heap.c); los eventos, los atiende el
             * hilo que los levanta. (El sabotaje: SÓLO PRUEBAS, test-cola 16.) V7/H12: y el VIAJE al run 0 —el hilo
             * sigue listo, pero sólo para el 0, que puede estar dormido— y su vuelta —tras su cuanto en el 0, lo puede
             * coger cualquiera—. */
            if (tc->status != BPVM_THREAD_RUNNABLE || s != BPVM_OK || q->sabotaje_aviso || tc->al_run0 || de_viaje)
                bpvm_platform_cond_broadcast(&q->sched_cond);
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
        /* Un programa publicado para los suyos, que no se esté cerrando y con sitio: no entran más runs que los que se
         * publicaron (n_runs; con uno, el planificador va sin cerrojo y no puede haber otro dentro). V7/H12: y uno para
         * UN run lo corre el run 0 —antes, el primero que despertaba—: la GUI es del run 0 (la ventana del simulado,
         * que sigue abierta para el siguiente programa, sólo la atiende el hilo que la creó). */
        while (q->programa == NULL || q->para_permanentes != para || q->acabando || q->dentro >= q->n_runs
               || (q->n_runs == 1 && id != 0)) {
            if (!permanente) {                 /* el suyo ya acabó, lo está cerrando otro o no cabe: vuelve */
                bpvm_platform_mutex_unlock(&q->vm_lock);
                return;
            }
            bpvm_platform_cond_wait(&q->sched_cond, &q->vm_lock);   /* sin programa: dormido en la cola */
        }
        bpvm_t* vm = q->programa;
        const int varios = (q->n_runs > 1);    /* leído aquí, con el cerrojo, con el programa que coge */
        q->dentro++;
        bpvm_platform_mutex_unlock(&q->vm_lock);
        correr_programa(q, vm, id, varios);    /* sale de él contado: por acabar o por salir */
        if (!permanente) return;
    }
}
