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
#include "bpvm_io.h"   /* V6/A1 — con io, el wire no se mira desde aquí */
#include "bpvm_platform.h"
#include <stdio.h>

/* V7/H12, etapa 2 — todo sobre LA COLA de hilos (threading.c): cada recorrido da UNA vuelta. */

/* ¿Algún thread BP no terminado? */
static int any_alive(const bpvm_t* vm) {
    for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h))
        if (h->status != BPVM_THREAD_TERMINATED) return 1;
    return 0;
}

/* Despierta a los threads BLOCKED_SLEEP cuyo wake_at_ms ha pasado. */
static int wake_expired_sleeps(bpvm_t* vm, int64_t now) {
    int woken = 0;
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms <= now) {
            tc->status = BPVM_THREAD_RUNNABLE;
            woken++;
        }
    }
    return woken;
}

/* Despierta a los threads BLOCKED_JOIN cuyo target ya terminó: terminado, o ya fuera de la cola (su handle no
 * resuelve: H12, etapa 2). */
static int wake_completed_joins(bpvm_t* vm) {
    int woken = 0;
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (tc->status != BPVM_THREAD_BLOCKED_JOIN || tc->blocked_on_join < 0) continue;
        const bpvm_thread_t* t = bpvm_hilo_por_tid(vm, tc->blocked_on_join);
        if (t == NULL || t->status == BPVM_THREAD_TERMINATED) {
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
    for (bpvm_thread_t* tc = bpvm_hilos_primero(vm); tc; tc = bpvm_hilos_siguiente(vm, tc)) {
        if (tc->status == BPVM_THREAD_BLOCKED_SLEEP && tc->wake_at_ms < min) {
            min = tc->wake_at_ms;
        }
    }
    return min;
}

bpvm_status_t bpvm_scheduler_run(bpvm_t* vm) {
    for (;;) {
        /* #342 — ANTES de mirar si queda alguien vivo: un thread que terminó
         * dejando eventos SUYOS encolados vuelve a ser elegible para
         * atenderlos. Va aquí y no dentro del `while` porque el caso más
         * frecuente es justo el de main: si se comprueba después, main ya ha
         * terminado, any_alive da falso y salimos con la cola llena. */
        bpvm_events_revive_terminated(vm);
        if (!any_alive(vm)) break;
        /* P-run-stop (#257) — KILL cooperativo: el poll_cb (si hay) mira
         * el transporte ENTRE quanta; kill_requested termina la ejecución
         * limpiamente (paramos entre opcodes → heap/FS consistentes). */
        /* V6/A1 — con el hilo `io` en marcha, el wire NO se mira desde aquí: lo
         * atiende `io` en todo momento y un KILL llega como `kill_requested`,
         * que se comprueba abajo. Sin `io` (firmware aún sin migrar), el camino
         * de siempre. Ésta es la línea que hace que la VM deje de tener trato
         * con el transporte. */
        if (vm->io == NULL && vm->poll_cb != NULL
                && vm->poll_cb(vm, vm->poll_user) != 0)
            vm->kill_requested = 1;
        if (vm->kill_requested) return BPVM_KILLED;

        /* #462 — CEDER EL TURNO AL SO ENTRE QUANTA.
         *
         * Observación de Eduardo (31-ago): *«en las STM32, cuando se ejecuta un
         * programa, el resto del sistema sigue vivo, incluidas las comunicaciones;
         * en las ESP32 se utilizan todos los recursos y se queda todo bloqueado»*.
         *
         * Y la causa estaba aquí: este bucle sólo llamaba a `thread_sleep_ms`
         * cuando NO hay ningún thread ejecutable. Mientras el programa BP tenga
         * trabajo, la tarea de la VM no se bloquea nunca — en bare-metal (STM32)
         * da igual, no hay a quién matar de hambre; bajo FreeRTOS se come todo lo
         * que esté a su prioridad o por debajo, incluido lo que bombea LVGL.
         *
         * `bpvm_platform_thread_yield` YA EXISTÍA en el contrato y lo implementaban
         * las cuatro plataformas (`taskYIELD()` en ESP32 y Pico, no-op en el STM32
         * porque allí no hace falta). No lo llamaba NADIE. Un gancho que existe y
         * no se usa no es una abstracción: es una promesa sin cumplir.
         *
         * Coste: una llamada por quantum, y un quantum son 1024 opcodes. */
        bpvm_platform_thread_yield();

        /* 1) Despierta sleeps expirados + joins completados; y los que acabaron del todo salen de la cola
         *    (H12, etapa 2: su struct y su pila, devueltas). */
        wake_expired_sleeps(vm, bpvm_platform_now_ms());
        wake_completed_joins(vm);
        bpvm_hilos_recoger(vm);

        /* 2) Pick RUNNABLE: en rueda, desde el siguiente al último elegido. */
        bpvm_thread_t* tc = bpvm_hilos_elegir(vm, 0);
        if (tc == NULL) {
            /* No hay RUNNABLE. ¿Sleep pendiente?  */
            int64_t earliest = earliest_wake(vm);
            if (earliest == INT64_MAX) {
                /* Todos bloqueados sin sleep: deadlock. */
                bpvm_diag_urgente("[bpvm-c sched] deadlock — todos los threads "
                        "bloqueados sin posibilidad de progreso");
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
        /* H5.c — ENTRE QUANTA: si hay un evento pendiente para este thread, le
         * inyectamos el frame del handler antes de darle la CPU. Aquí el tc no
         * está corriendo, así que su pc/sp/bp/cs son los buenos. Uno por vuelta:
         * dos frames seguidos se ejecutarían en orden inverso y son FIFO. */
        bpvm_event_drain_one(vm, tc);
        int yielded = 0;
        int quantum = vm->quantum_ops > 0 ? vm->quantum_ops : 1024;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, quantum, &yielded);
        /* V7/H7 #608 (29-sep, Eduardo: la A) — una excepción sin atrapar en un hilo que NO es el main acaba ESE hilo;
         * los demás siguen, como en Java y como decía su propio aviso («el thread TERMINA aquí»). Antes cualquier
         * fallo de un cuanto acababa el RUN. Al final, el RUN dice que hubo un error (abajo), con el mensaje de la
         * excepción en runtime_error. La del main sí acaba el programa. */
        if (s == BPVM_ERR_RUNTIME && tc->status == BPVM_THREAD_TERMINATED && tc->id != 0) {
            vm->hilo_sin_atrapar = 1;
            continue;
        }
        if (s != BPVM_OK) return s;
        /* Si el tc terminó o cedió, el bucle continúa. */
    }
    /* #342 — GUARDIÁN DE SALIDA. Si aún queda algo en la cola es que su
     * destinatario no existe, o que un handler post-mortem levantó eventos
     * después de agotarse el presupuesto. Tirarlos es legítimo; tirarlos EN
     * SILENCIO es la familia de bug que costó #326. Que lo diga. */
    if (vm->ev_count > 0) {
        bpvm_diag("[bpvm] fin de ejecución con %d evento(s) sin atender "
                        "(destinatario muerto o encolados por un handler tardío)",
                vm->ev_count);
    }

    /* #462 — UN PROGRAMA AL QUE SE LE PIDIÓ PARAR DICE QUE FUE MATADO, SIEMPRE.
     *
     * Se salía por aquí con BPVM_OK aunque hubiera un KILL pendiente, y eso hacía
     * que el estado de un `stop` dependiera de DÓNDE cayera el KILL: si el
     * programa terminaba por su cuenta antes de que el planificador volviera a
     * mirar la bandera, el IDE recibía `exit 0 (OK)` por un programa que el
     * usuario había mandado parar.
     *
     * El camino que lo destapa es el del GUI, y no es raro: `Gui.run()` es
     * `while __guiRunOnce() do endwh`, y el builtin devuelve FALSO al ver el
     * KILL — o sea que el bucle de BasicPlus termina limpiamente, `Main` retorna
     * y el programa acaba «bien». Medido el 5-sep en el P4 y en la Discovery: un
     * KILL sobre `GuiColorDemo` daba `status OK, exitCode 0`. Con `quantum=32`,
     * el mismo programa daba `KILLED` — o sea que no era un valor equivocado,
     * era una CARRERA, que es peor de diagnosticar y peor de fiarse.
     *
     * La bandera es la verdad: si se pidió parar, se paró. */
    if (vm->kill_requested) return BPVM_KILLED;
    if (vm->hilo_sin_atrapar) return BPVM_ERR_RUNTIME;   /* V7/H7 #608: un hilo murió sin atrapar */
    return BPVM_OK;
}
