package com.mycompany.bpide;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.StepCommand;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Capa fina del IDE que ahora envuelve un {@link VmClient} (A1.9).
 *
 * <p>Lo que vive aquí:</p>
 * <ul>
 *   <li>{@link ObservableList}<Breakpoint> — modelo UI que el editor usa
 *       para pintar el gutter. Es la única fuente de verdad para la UI;
 *       cuando hay un VmClient conectado, los cambios se replican al
 *       wire vía setBreakpoint.</li>
 *   <li>Bridge de listeners — la UI se suscribe a esta sesión con
 *       {@link #addListener(DebugListener)}; los eventos llegan del
 *       VmClient cuando hay uno attached y se reenvían a los suscriptores.</li>
 * </ul>
 *
 * <p>Lo que vive en el VmClient (lado remoto, en el subproceso bpgenvm):</p>
 * <ul>
 *   <li>Set autoritativo de breakpoints (DebugController).</li>
 *   <li>Modo step/run y rendezvous con el worker BP.</li>
 *   <li>Queries de memoria/locales/stack/properties.</li>
 * </ul>
 *
 * <p>Lifecycle: una DebugSession se reutiliza entre runs. Para cada
 * sesión de debug, el IDE crea un {@link VmClient}, lo conecta con
 * {@link #attach(VmClient)}, deja correr al usuario, y al terminar
 * llama a {@link #detach()}. Los breakpoints persisten en la
 * ObservableList; al hacer attach se re-sincronizan al nuevo wire.</p>
 */
public final class DebugSession {

    private final ObservableList<Breakpoint> breakpoints = new ObservableList<>();
    private final List<DebugListener> listeners = new ArrayList<>();

    private volatile VmClient client;
    /** Lambda registrada en el VmClient actual para reenviar eventos.
     *  La guardamos para poder removerla en detach(). */
    private DebugListener bridge;

    public ObservableList<Breakpoint> breakpoints() { return breakpoints; }

    // ---- Listener registry — la UI se suscribe aquí ----

    public void addListener(DebugListener l) {
        if (l != null) listeners.add(l);
    }

    public void removeListener(DebugListener l) {
        listeners.remove(l);
    }

    private void fanout(edu.bpgenvm.vm.debug.DebugEvent ev) {
        // Snapshot defensiva por si un listener se desuscribe en su handler.
        for (DebugListener l : new ArrayList<>(listeners)) {
            try { l.onEvent(ev); }
            catch (Throwable t) {
                System.err.println("DebugSession listener falló: " + t.getMessage());
            }
        }
    }

    // ---- Attach / detach del VmClient ----

    /** Conecta al VmClient activo: replica los breakpoints actuales al
     *  wire y registra el puente de eventos. Llamar UNA vez por sesión,
     *  después de que el VmClient haya completado el handshake. */
    public void attach(VmClient c) {
        if (c == null) throw new IllegalArgumentException("VmClient null");
        if (this.client != null) detach();
        this.client = c;
        // Re-publicar breakpoints existentes al wire (la VM nueva no los
        // conoce todavía).
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            c.setBreakpoint(bp.file, bp.line, true);
        }
        // Puente: cualquier evento del VmClient se reenvía a los listeners
        // de la sesión.
        this.bridge = this::fanout;
        c.setEventListener(this.bridge);
    }

    public void detach() {
        VmClient c = this.client;
        if (c != null && bridge != null) {
            c.setEventListener(null);
        }
        this.bridge  = null;
        this.client  = null;
    }

    public boolean isAttached() { return client != null; }

    public VmClient client() { return client; }

    // ---- Breakpoints API ----

    public boolean isBreakpointAt(String file, int line) {
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) return true;
        }
        return false;
    }

    /** Añade o quita un breakpoint en (file, line). Si hay VmClient
     *  attached, replica el cambio al wire. Devuelve true si quedó
     *  AÑADIDO. */
    public boolean toggleBreakpoint(String file, int line) {
        VmClient c = this.client;
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) {
                breakpoints.remove(i);
                if (c != null) c.setBreakpoint(file, line, false);
                return false;
            }
        }
        breakpoints.add(new Breakpoint(file, line));
        if (c != null) c.setBreakpoint(file, line, true);
        return true;
    }

    // ---- Step / continue / stop (delegado al VmClient si attached) ----

    public void sendCommand(StepCommand cmd) {
        VmClient c = this.client;
        if (c != null) c.sendCommand(cmd);
    }

    // ---- Queries (RPC al subproceso VM) ----

    /** Locales del thread actualmente pausado. Bloquea brevemente; NO
     *  llamar desde EDT. Si no hay VmClient o no hay pausa, devuelve []. */
    public int[] getLocals(long timeoutMs) {
        VmClient c = this.client;
        if (c == null) return new int[0];
        try { return c.getLocals(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getLocals falló: " + ex.getMessage());
            return new int[0];
        }
    }

    public List<int[]> getStackFrames(long timeoutMs) {
        VmClient c = this.client;
        if (c == null) return java.util.Collections.emptyList();
        try { return c.getStackFrames(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getStackFrames falló: " + ex.getMessage());
            return java.util.Collections.emptyList();
        }
    }

    public List<ModuleManager.PropertyView> getModuleProperties(long timeoutMs) {
        VmClient c = this.client;
        if (c == null) return java.util.Collections.emptyList();
        try { return c.getModuleProperties(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getModuleProperties falló: " + ex.getMessage());
            return java.util.Collections.emptyList();
        }
    }

    /** Resetea estado para arrancar una nueva sesión. Mantiene los
     *  breakpoints; el resto se limpia al attach() siguiente. */
    public void reset() {
        // Nada interno que limpiar: el VmClient se reemplaza en attach()
        // y el modo/step lo gestiona el DebugController del subproceso.
    }
}
