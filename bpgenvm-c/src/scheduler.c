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
        bpvm_thread_t* tc = &vm->threads[idx];
        /* H5.c — ENTRE QUANTA: si hay un evento pendiente para este thread, le
         * inyectamos el frame del handler antes de darle la CPU. Aquí el tc no
         * está corriendo, así que su pc/sp/bp/cs son los buenos. Uno por vuelta:
         * dos frames seguidos se ejecutarían en orden inverso y son FIFO. */
        bpvm_event_drain_one(vm, tc);
        vm->current_thread_idx = idx;
        last_idx = idx;
        int yielded = 0;
        int quantum = vm->quantum_ops > 0 ? vm->quantum_ops : 1024;
        bpvm_status_t s = bpvm_interp_run_quantum(vm, tc, quantum, &yielded);
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
    return BPVM_OK;
}
