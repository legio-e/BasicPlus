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
//     breakpoints con setBreakpoint / clearBreakpointById, ajusta el
//     modo arrancando con STEP_INTO o RUN, y reacciona a los eventos
//     que llegan a sus DebugListeners.
//   - Cuando la VM pausa, el cliente debe invocar sendCommand(...)
//     para soltar al worker.
//
// PR-5: breakpoints con bpId estable. Cada (file,line) recibe un long
// único al primer setBreakpoint(enabled=true). Los hits se contabilizan
// para que LIST_BP pueda reportar uso por breakpoint.
//
// Thread-safety:
//   - byKey/byId: protegidos por bpLock (acceso multi-thread: hook
//     desde worker BP, comandos desde reader thread).
//   - mode / stepFromBp / pausedAt: volatile (lectura del hook,
//     escritura del cliente o del propio hook).
//   - commandQueue: SynchronousQueue (rendezvous; safe by design).
//   - listeners: CopyOnWriteArrayList (lectura frecuente, escritura
//     rara).
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.DebugHook;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.SynchronousQueue;

public final class DebugController {

    private enum Mode { RUN, STEP_INTO, STEP_OVER, STEP_OUT }

    /** Información de un breakpoint individual. bpId estable durante la
     *  vida del controller. */
    public static final class BreakpointInfo {
        public final long bpId;
        public final String file;
        public final int line;
        public volatile boolean enabled;
        public volatile long hits;

        BreakpointInfo(long bpId, String file, int line, boolean enabled) {
            this.bpId = bpId;
            this.file = file;
            this.line = line;
            this.enabled = enabled;
            this.hits = 0;
        }
    }

    // ---- Estado del breakpoint set ----
    // Key estable que viaja en el wire: "<basename>:<line>". Usamos
    // basename porque ctx.sourceFile puede llegar como path absoluto
    // distinto de la representación que ve el usuario en el editor;
    // el hook normaliza a basename antes de consultar.
    private final Map<String, BreakpointInfo> byKey = new HashMap<>();
    private final Map<Long, BreakpointInfo> byId   = new HashMap<>();
    private long nextBpId = 1;
    private final Object bpLock = new Object();

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

    /** Crea o actualiza un breakpoint en (file, line). Si ya existe uno
     *  con esa key, reutiliza el bpId previo (sólo cambia `enabled`).
     *  Si no existe y enabled=true, asigna un nuevo bpId.
     *  Devuelve el bpId del breakpoint (0 si se acaba de des-activar
     *  uno que no existía — caso degenerado). */
    public long setBreakpoint(String file, int line, boolean enabled) {
        String key = basenameOf(file) + ":" + line;
        synchronized (bpLock) {
            BreakpointInfo info = byKey.get(key);
            if (info == null) {
                if (!enabled) return 0;   // nada que crear
                long id = nextBpId++;
                info = new BreakpointInfo(id, basenameOf(file), line, true);
                byKey.put(key, info);
                byId.put(id, info);
                return id;
            }
            info.enabled = enabled;
            return info.bpId;
        }
    }

    /** Comprueba si hay un breakpoint habilitado en (file, line). Si lo
     *  encuentra, incrementa su contador de hits y devuelve true. */
    public boolean checkAndCountBreakpointAt(String file, int line) {
        String key = basenameOf(file) + ":" + line;
        synchronized (bpLock) {
            BreakpointInfo info = byKey.get(key);
            if (info != null && info.enabled) {
                info.hits++;
                return true;
            }
            return false;
        }
    }

    /** Variante sin contar — útil para tests y para preguntar sin afectar
     *  estadísticas. */
    public boolean isBreakpointAt(String file, int line) {
        String key = basenameOf(file) + ":" + line;
        synchronized (bpLock) {
            BreakpointInfo info = byKey.get(key);
            return info != null && info.enabled;
        }
    }

    /** Elimina un breakpoint por bpId. Devuelve true si existía. */
    public boolean clearBreakpointById(long bpId) {
        synchronized (bpLock) {
            BreakpointInfo info = byId.remove(bpId);
            if (info == null) return false;
            byKey.remove(basenameOf(info.file) + ":" + info.line);
            return true;
        }
    }

    public void clearAllBreakpoints() {
        synchronized (bpLock) {
            byKey.clear();
            byId.clear();
        }
    }

    /** Snapshot inmutable de los breakpoints, ordenado por bpId. */
    public List<BreakpointInfo> listBreakpoints() {
        synchronized (bpLock) {
            List<BreakpointInfo> out = new ArrayList<>(byId.values());
            out.sort((a, b) -> Long.compare(a.bpId, b.bpId));
            return out;
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
     *  No-op si la VM no está pausada. */
    public void sendCommand(StepCommand cmd) {
        if (pausedAt == null) return;
        try {
            commandQueue.put(cmd);
        } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
        }
    }

    /** PR-5 — solicita una pausa "manual" (PAUSE wire message). Cambia el
     *  modo a STEP_INTO para que el hook pause en la próxima llamada,
     *  sin necesidad de un breakpoint registrado. No tiene efecto si la
     *  VM ya está pausada (el hook está bloqueado en commandQueue.take). */
    public void requestPause() {
        mode = Mode.STEP_INTO;
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
                    shouldPause = checkAndCountBreakpointAt(ctx.sourceFile, ctx.line);
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

    /** Emite un evento ARBITRARIO a los listeners. */
    public void emitEvent(DebugEvent ev) {
        if (ev != null) emit(ev);
    }

    // ---- Helpers ----

    private void emit(DebugEvent ev) {
        for (DebugListener l : listeners) {
            try { l.onEvent(ev); }
            catch (Throwable t) {
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
