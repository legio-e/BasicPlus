package com.mycompany.bpide;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.StepCommand;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Capa fina del IDE que ahora envuelve un {@link BpvmClient} (A1.9).
 *
 * <p>Lo que vive aquí:</p>
 * <ul>
 *   <li>{@link ObservableList}<Breakpoint> — modelo UI que el editor usa
 *       para pintar el gutter. Es la única fuente de verdad para la UI;
 *       cuando hay un BpvmClient conectado, los cambios se replican al
 *       wire vía setBreakpoint.</li>
 *   <li>Bridge de listeners — la UI se suscribe a esta sesión con
 *       {@link #addListener(DebugListener)}; los eventos llegan del
 *       BpvmClient cuando hay uno attached y se reenvían a los suscriptores.</li>
 * </ul>
 *
 * <p>Lo que vive en el BpvmClient (lado remoto, en el subproceso bpgenvm):</p>
 * <ul>
 *   <li>Set autoritativo de breakpoints (DebugController).</li>
 *   <li>Modo step/run y rendezvous con el worker BP.</li>
 *   <li>Queries de memoria/locales/stack/properties.</li>
 * </ul>
 *
 * <p>Lifecycle: una DebugSession se reutiliza entre runs. Para cada
 * sesión de debug, el IDE crea un {@link BpvmClient}, lo conecta con
 * {@link #attach(BpvmClient)}, deja correr al usuario, y al terminar
 * llama a {@link #detach()}. Los breakpoints persisten en la
 * ObservableList; al hacer attach se re-sincronizan al nuevo wire.</p>
 */
public final class DebugSession {

    private final ObservableList<Breakpoint> breakpoints = new ObservableList<>();
    private final List<DebugListener> listeners = new ArrayList<>();

    private volatile BpvmClient client;
    /** Lambda registrada en el BpvmClient actual para reenviar eventos.
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

    // ---- Attach / detach del BpvmClient ----

    /** Conecta al BpvmClient activo: replica los breakpoints actuales al
     *  wire y registra el puente de eventos. Llamar UNA vez por sesión,
     *  después de que el BpvmClient haya completado el handshake. */
    public void attach(BpvmClient c) {
        if (c == null) throw new IllegalArgumentException("BpvmClient null");
        if (this.client != null) detach();
        this.client = c;
        // Re-publicar breakpoints existentes al wire (la VM nueva no los
        // conoce todavía).
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            c.setBreakpoint(bp.file, bp.line, true);
        }
        // Puente: cualquier evento del BpvmClient se reenvía a los listeners
        // de la sesión.
        this.bridge = this::fanout;
        c.setEventListener(this.bridge);
    }

    public void detach() {
        BpvmClient c = this.client;
        if (c != null && bridge != null) {
            c.setEventListener(null);
        }
        this.bridge  = null;
        this.client  = null;
    }

    public boolean isAttached() { return client != null; }

    public BpvmClient client() { return client; }

    // ---- Breakpoints API ----

    public boolean isBreakpointAt(String file, int line) {
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) return true;
        }
        return false;
    }

    /** Añade o quita un breakpoint en (file, line). Si hay BpvmClient
     *  attached, replica el cambio al wire. Devuelve true si quedó
     *  AÑADIDO. */
    public boolean toggleBreakpoint(String file, int line) {
        BpvmClient c = this.client;
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

    // ---- Step / continue / stop (delegado al BpvmClient si attached) ----

    public void sendCommand(StepCommand cmd) {
        BpvmClient c = this.client;
        if (c != null) c.sendCommand(cmd);
    }

    // ---- Queries (RPC al subproceso VM) ----

    /** Locales del thread actualmente pausado. Bloquea brevemente; NO
     *  llamar desde EDT. Si no hay BpvmClient o no hay pausa, devuelve []. */
    public int[] getLocals(long timeoutMs) {
        BpvmClient c = this.client;
        if (c == null) return new int[0];
        try { return c.getLocals(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getLocals falló: " + ex.getMessage());
            return new int[0];
        }
    }

    /** H6.a.1 — locales por nombre del thread pausado (.dbg v3). Lista vacía si
     *  no hay cliente, no hay pausa, o el módulo no trae sección `vars`. */
    public List<BpvmClient.NamedLocal> getNamedLocals(long timeoutMs) {
        BpvmClient c = this.client;
        if (c == null) return java.util.Collections.emptyList();
        try { return c.getNamedLocals(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getNamedLocals falló: " + ex.getMessage());
            return java.util.Collections.emptyList();
        }
    }

    public List<int[]> getStackFrames(long timeoutMs) {
        BpvmClient c = this.client;
        if (c == null) return java.util.Collections.emptyList();
        try { return c.getStackFrames(timeoutMs); }
        catch (IOException ex) {
            System.err.println("[DebugSession] getStackFrames falló: " + ex.getMessage());
            return java.util.Collections.emptyList();
        }
    }

    public List<ModuleManager.PropertyView> getModuleProperties(long timeoutMs) {
        BpvmClient c = this.client;
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
        // Nada interno que limpiar: el BpvmClient se reemplaza en attach()
        // y el modo/step lo gestiona el DebugController del subproceso.
    }
}
