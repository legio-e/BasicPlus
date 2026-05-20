package com.mycompany.bpide;

import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.DebugHook;
import edu.bpgenvm.vm.debug.DebugController;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.StepCommand;

/**
 * Capa fina del IDE que envuelve un {@link DebugController} del lado VM.
 *
 * <p>Responsabilidades que SE QUEDAN aquí (en el IDE):</p>
 * <ul>
 *   <li>{@link ObservableList} de breakpoints — el editor los pinta en
 *       rojo en cuanto cambia el modelo; cualquier listener UI se
 *       suscribe a este observable, no al controller.</li>
 *   <li>Conversión "click del usuario en el gutter" → llamada al
 *       controller (que es la autoridad).</li>
 * </ul>
 *
 * <p>Responsabilidades que se han MOVIDO al controller (en miVM):</p>
 * <ul>
 *   <li>Hook que se instala en la VM (decide pausa, bloquea worker BP).</li>
 *   <li>Set de breakpoints autoritativo.</li>
 *   <li>Modo step/run y rendezvous con el worker.</li>
 *   <li>Lista de DebugListeners y emisión de eventos.</li>
 * </ul>
 *
 * <p>Cuando A1.4.b cablee el TCP, este wrapper sigue intacto: el
 * controller se sustituye por un cliente RPC con la misma API y la
 * sincronización ObservableList → controller pasa a generar mensajes
 * de red en lugar de llamadas directas.</p>
 */
public final class DebugSession {

    private final DebugController controller = new DebugController();

    /** Mirror de breakpoints para la UI. Mantenemos sincronía con
     *  controller en toggleBreakpoint(); cualquier observer del editor
     *  se engancha aquí. */
    private final ObservableList<Breakpoint> breakpoints = new ObservableList<>();

    public ObservableList<Breakpoint> breakpoints() { return breakpoints; }

    public DebugController controller() { return controller; }

    // ---- Listeners (delegan al controller) ----

    public void addListener(DebugListener l) { controller.addListener(l); }
    public void removeListener(DebugListener l) { controller.removeListener(l); }

    // ---- Breakpoints API (vista UI sincronizada con la VM) ----

    public boolean isBreakpointAt(String file, int line) {
        return controller.isBreakpointAt(file, line);
    }

    /** Alterna breakpoint. Mantiene en sincronía el observable (UI) y
     *  el set autoritativo (VM). Devuelve true si quedó AÑADIDO. */
    public boolean toggleBreakpoint(String file, int line) {
        for (int i = 0; i < breakpoints.size(); i++) {
            Breakpoint bp = breakpoints.get(i);
            if (bp.line == line && bp.file.equals(file)) {
                breakpoints.remove(i);
                controller.setBreakpoint(file, line, false);
                return false;
            }
        }
        breakpoints.add(new Breakpoint(file, line));
        controller.setBreakpoint(file, line, true);
        return true;
    }

    // ---- Estado de pausa (delegado) ----

    public DebugContext pausedAt()      { return controller.currentContext(); }
    public DebugContext currentContext(){ return controller.currentContext(); }
    public boolean      isPaused()      { return controller.isPaused(); }

    public void sendCommand(StepCommand cmd) { controller.sendCommand(cmd); }

    public void reset() { controller.reset(); }

    // ---- Hook que se instala en la VM ----

    public DebugHook hook() { return controller.hook(); }
}
