// ============================================================
// DebugController.java
// Autoridad del debugger DEL LADO VM. Encapsula la lógica de pausa que
// antes vivía en DebugSession (lado IDE): set de breakpoints, modo
// (run/step-into/step-over/step-out), rendezvous con el worker BP y
// notificación de eventos al listener.
//
// Por qué aquí y no en el IDE: cuando la VM corra en otro proceso o
// en un dispositivo, el hook DEBE bloquear al worker BP en el lado VM.
// Si la lógica de "¿debo parar en esta línea?" viviera en el IDE,
// habría un round-trip por cada cambio de línea — inviable.
//
// Contrato:
//   - El VM instala el hook devuelto por hook() antes de arrancar
//     con executeRootModule.
//   - El cliente (IDE in-process, o cliente RPC mañana) configura
//     breakpoints con setBreakpoint / clearAllBreakpoints, ajusta el
//     modo arrancando con STEP_INTO o RUN, y reacciona a los eventos
//     que llegan a sus DebugListeners.
//   - Cuando la VM pausa, el cliente debe invocar sendCommand(...)
//     para soltar al worker.
//
// Thread-safety:
//   - breakpoints: HashSet protegido por sí mismo (synchronized).
//   - mode / stepFromBp / pausedAt: volatile (lectura del hook,
//     escritura del cliente o del propio hook).
//   - commandQueue: SynchronousQueue (rendezvous; safe by design).
//   - listeners: CopyOnWriteArrayList (lectura frecuente, escritura
//     rara).
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.DebugHook;

import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.SynchronousQueue;

public final class DebugController {

    private enum Mode { RUN, STEP_INTO, STEP_OVER, STEP_OUT }

    // ---- Estado del breakpoint set ----
    // Key estable que viaja en el wire en A1.4.b: "<basename>:<line>".
    // Usamos basename porque ctx.sourceFile puede llegar como path
    // absoluto distinto de la representación que ve el usuario en el
    // editor; el hook normaliza a basename antes de consultar.
    private final Set<String> breakpoints =
            Collections.synchronizedSet(new HashSet<>());

    // ---- Estado del run actual ----
    private volatile Mode mode = Mode.STEP_INTO;     // primer break = primera línea
    private volatile int stepFromBp = 0;
    private volatile DebugContext pausedAt = null;

    private final SynchronousQueue<StepCommand> commandQueue = new SynchronousQueue<>();
    private final List<DebugListener> listeners = new CopyOnWriteArrayList<>();

    // ---- Breakpoints API ----

    public void addListener(DebugListener l) {
        if (l != null) listeners.add(l);
    }

    public void removeListener(DebugListener l) {
        listeners.remove(l);
    }

    public void setBreakpoint(String file, int line, boolean enabled) {
        String key = basenameOf(file) + ":" + line;
        if (enabled) breakpoints.add(key);
        else         breakpoints.remove(key);
    }

    public boolean isBreakpointAt(String file, int line) {
        return breakpoints.contains(basenameOf(file) + ":" + line);
    }

    public void clearAllBreakpoints() {
        breakpoints.clear();
    }

    /** Snapshot del set, ordenado, en el formato del wire. Útil para
     *  que el cliente que reconecta refresque su mirror. */
    public List<String> listBreakpoints() {
        synchronized (breakpoints) {
            return new java.util.ArrayList<>(new java.util.TreeSet<>(breakpoints));
        }
    }

    // ---- Pause / step (lo invoca el cliente cuando hay PausedEvent) ----

    /** Devuelve el contexto del último pause; null si la VM no está
     *  pausada. El cliente in-process lo usa para leer memoria/locales;
     *  el cliente remoto NO debe usarlo (en remoto será null y las
     *  queries van por RPC). */
    public DebugContext currentContext() { return pausedAt; }

    public boolean isPaused() { return pausedAt != null; }

    /** Envía un comando al worker BP que está bloqueado en el hook.
     *  No-op si la VM no está pausada. Rendezvous: bloquea brevemente
     *  hasta que el worker retire — en práctica el worker ya espera. */
    public void sendCommand(StepCommand cmd) {
        if (pausedAt == null) return;
        try {
            commandQueue.put(cmd);
        } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
        }
    }

    /** Resetea modo/pausa para arrancar una nueva sesión. NO toca los
     *  breakpoints (es lo que el usuario espera entre runs). */
    public void reset() {
        pausedAt = null;
        mode = Mode.STEP_INTO;
        stepFromBp = 0;
    }

    // ---- Hook que el VM instala ----

    public DebugHook hook() {
        return ctx -> {
            boolean shouldPause;
            switch (mode) {
                case STEP_INTO:
                    shouldPause = true;
                    break;
                case STEP_OVER:
                    // Pausar cuando volvemos al MISMO frame o uno más
                    // superficial. Mientras estamos en un frame más
                    // profundo (creado por un CALL dentro del step-over),
                    // seguimos sin parar.
                    shouldPause = (ctx.bp <= stepFromBp);
                    break;
                case STEP_OUT:
                    shouldPause = (ctx.bp < stepFromBp);
                    break;
                case RUN:
                default:
                    shouldPause = isBreakpointAt(ctx.sourceFile, ctx.line);
                    break;
            }
            if (!shouldPause) return;

            // Pausa: publicamos contexto, emitimos evento, bloqueamos.
            pausedAt = ctx;
            emit(new PausedEvent(
                    ctx.tid, ctx.absPc, ctx.line, ctx.sourceFile,
                    ctx.bp, ctx.sp, ctx.cs, ctx.stackBase));

            StepCommand cmd;
            try {
                cmd = commandQueue.take();
            } catch (InterruptedException ex) {
                Thread.currentThread().interrupt();
                return;
            }
            // Aplicar el comando antes de soltar pausedAt para que el
            // modo entre en vigor en la próxima invocación del hook.
            switch (cmd) {
                case CONTINUE:  mode = Mode.RUN; break;
                case STEP_INTO: mode = Mode.STEP_INTO; break;
                case STEP_OVER: mode = Mode.STEP_OVER; stepFromBp = ctx.bp; break;
                case STEP_OUT:  mode = Mode.STEP_OUT;  stepFromBp = ctx.bp; break;
                case STOP:      mode = Mode.RUN;  /* TODO: signal shutdown */ break;
            }
            int resumedTid = ctx.tid;
            pausedAt = null;
            emit(new ResumedEvent(resumedTid));
        };
    }

    /** Emite un evento ARBITRARIO a los listeners. Pensado para que código
     *  fuera del hook (e.g. el WorkerLoop al detectar BpThreadFault, o Main
     *  al terminar vm.run()) pueda inyectar ExitedEvent / ExceptionEvent.
     *  No participa del rendezvous del hook — sólo notifica. */
    public void emitEvent(DebugEvent ev) {
        if (ev != null) emit(ev);
    }

    // ---- Helpers ----

    private void emit(DebugEvent ev) {
        for (DebugListener l : listeners) {
            try { l.onEvent(ev); }
            catch (Throwable t) {
                // Un listener mal escrito no debe tumbar el worker BP.
                System.err.println("DebugController listener falló: " + t.getMessage());
            }
        }
    }

    private static String basenameOf(String path) {
        if (path == null) return "?";
        int sep = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
        return (sep >= 0) ? path.substring(sep + 1) : path;
    }
}
