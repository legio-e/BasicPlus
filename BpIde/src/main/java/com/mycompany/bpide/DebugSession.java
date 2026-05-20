package com.mycompany.bpide;

import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.DebugHook;
import edu.bpgenvm.vm.debug.DebugEvent;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.SynchronousQueue;

/**
 * Estado y orquestación de una sesión de depuración. Pegada en dos
 * puntos:
 *
 *   1. {@link #hook()} → se instala en {@link edu.bpgenvm.vm.VirtualMachine}
 *      antes de arrancar la VM. El hook corre en el thread worker BP y
 *      decide si pausar; cuando pausa, bloquea hasta recibir un comando.
 *
 *   2. La UI del IDE escucha por {@link DebugListener} para refrescar
 *      paneles cuando llegan {@link PausedEvent}/{@link ResumedEvent}.
 *      Envía comandos vía {@link #sendCommand}.
 *
 * <p>A1.3 — los listeners reciben ahora objetos {@link DebugEvent}
 * (POJOs serializables) en lugar de callbacks específicos. La query
 * de memoria/locals/stack sigue saliendo del {@link DebugContext}
 * accesible vía {@link #currentContext()}, que cuando la VM viva en
 * otro proceso será sustituido por un cliente RPC con la misma API.</p>
 *
 * <p>Concurrencia: los campos compartidos entre worker BP y EDT son
 * {@code volatile}. El paso de comandos va por una {@link SynchronousQueue}
 * que rendezvous-bloquea hasta que el otro lado retire. Los listeners se
 * notifican en el thread del worker; los handlers DEBEN reenviar a EDT
 * con {@code SwingUtilities.invokeLater}.</p>
 */
public final class DebugSession {

    // ---- Estado observable ----

    private final ObservableList<Breakpoint> breakpoints = new ObservableList<>();
    private final List<DebugListener> listeners = new ArrayList<>();

    public ObservableList<Breakpoint> breakpoints() { return breakpoints; }

    public void addListener(DebugListener l) { listeners.add(l); }

    // ---- Estado del run actual ----

    /** Modo del próximo "step" pendiente. RUN = libre hasta próximo breakpoint. */
    private enum Mode { RUN, STEP_INTO, STEP_OVER, STEP_OUT }
    private volatile Mode mode = Mode.STEP_INTO;     // primer break: parar en la 1ª línea
    /** bp del frame donde se emitió el último STEP_OVER / STEP_OUT. */
    private volatile int stepFromBp = 0;

    /** Contexto del último pause; null si la VM no está pausada. Los
     *  listeners pueden leerlo para hacer queries de memoria que aún no
     *  encajan en un evento (hasta que A1.4 + RPC los reemplace). */
    private volatile DebugContext pausedAt = null;
    public DebugContext pausedAt() { return pausedAt; }
    public DebugContext currentContext() { return pausedAt; }
    public boolean isPaused() { return pausedAt != null; }

    // Rendezvous para coordinar EDT → worker BP (envío de comando).
    private final SynchronousQueue<StepCommand> commandQueue = new SynchronousQueue<>();

    // ---- Breakpoints API ----

    public boolean isBreakpointAt(String file, int line) {
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) return true;
        }
        return false;
    }

    /** Añade o quita un breakpoint en (file, line). Devuelve true si quedó AÑADIDO. */
    public boolean toggleBreakpoint(String file, int line) {
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) {
                breakpoints.remove(i);
                return false;
            }
        }
        breakpoints.add(new Breakpoint(file, line));
        return true;
    }

    // ---- Resumir / step (lo llama el IDE desde EDT) ----

    /**
     * Envía un comando al worker BP que está bloqueado en el hook. Si la
     * VM no está pausada, es no-op. Bloquea brevemente hasta que el otro
     * lado retire (rendezvous); en la práctica el worker está ya esperando
     * cuando llega el comando.
     */
    public void sendCommand(StepCommand cmd) {
        if (pausedAt == null) return;
        try {
            commandQueue.put(cmd);
        } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
        }
    }

    /** Resetea estado para arrancar una nueva sesión. */
    public void reset() {
        pausedAt = null;
        mode = Mode.STEP_INTO;
        stepFromBp = 0;
        // breakpoints persisten entre runs (es lo que el usuario espera).
    }

    // ---- Helpers para emitir eventos a los listeners ----

    private void emit(DebugEvent ev) {
        for (DebugListener l : listeners) {
            try { l.onEvent(ev); }
            catch (Throwable t) {
                // Un listener mal escrito no debe tumbar el worker BP.
                System.err.println("DebugSession listener falló: " + t.getMessage());
            }
        }
    }

    // ---- Hook que se instala en la VM ----

    public DebugHook hook() {
        return ctx -> {
            String basename = basenameOf(ctx.sourceFile);
            boolean shouldPause;
            switch (mode) {
                case STEP_INTO:
                    shouldPause = true;
                    break;
                case STEP_OVER:
                    // Pausar cuando volvemos al MISMO frame o uno más superficial.
                    // Mientras estamos en un frame "más profundo" (bp > stepFromBp,
                    // creado por un CALL dentro del step-over), seguimos sin parar.
                    shouldPause = (ctx.bp <= stepFromBp);
                    break;
                case STEP_OUT:
                    shouldPause = (ctx.bp < stepFromBp);
                    break;
                case RUN:
                default:
                    shouldPause = isBreakpointAt(basename, ctx.line);
                    break;
            }
            if (!shouldPause) return;

            // Pausa: publicamos contexto + emitimos PausedEvent + bloqueamos.
            pausedAt = ctx;
            PausedEvent pe = new PausedEvent(
                    ctx.tid, ctx.absPc, ctx.line, ctx.sourceFile,
                    ctx.bp, ctx.sp, ctx.cs, ctx.stackBase);
            emit(pe);

            StepCommand cmd;
            try {
                cmd = commandQueue.take();
            } catch (InterruptedException ex) {
                Thread.currentThread().interrupt();
                return;
            }
            // Aplicar comando antes de soltar pausedAt para que el modo
            // entre en vigor para la próxima iteración del hook.
            switch (cmd) {
                case CONTINUE:  mode = Mode.RUN; break;
                case STEP_INTO: mode = Mode.STEP_INTO; break;
                case STEP_OVER: mode = Mode.STEP_OVER; stepFromBp = ctx.bp; break;
                case STEP_OUT:  mode = Mode.STEP_OUT;  stepFromBp = ctx.bp; break;
                case STOP:      mode = Mode.RUN;  /* TODO: signal vm shutdown */ break;
            }
            int resumedTid = ctx.tid;
            pausedAt = null;
            emit(new ResumedEvent(resumedTid));
        };
    }

    private static String basenameOf(String path) {
        if (path == null) return "?";
        int sep = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
        return (sep >= 0) ? path.substring(sep + 1) : path;
    }
}
