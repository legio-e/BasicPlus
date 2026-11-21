// ============================================================
// ExitedEvent.java
// La VM ha terminado de ejecutar el programa (vía RET del main o por
// HALT). exitCode = 0 si terminó limpiamente, !=0 si por error.
// ============================================================
package edu.bpgenvm.vm.debug;

public final class ExitedEvent extends DebugEvent {
    public final int exitCode;
    /** Texto humano-legible explicando por qué terminó. */
    public final String reason;

    /** V6/E1 — cuánto duró la EJECUCIÓN, en ms; -1 si el peer no lo mandó.
     *
     *  El campo `elapsedMs` del EXITED lo mandan los SEIS emisores (las cuatro
     *  familias de firmware, el simulador y esta misma VM) desde hace tiempo, y
     *  nadie lo leía: `git log -S elapsedMs -- BpIde/` salía vacío. No se
     *  rompió nunca — es que no llegó a enchufarse.
     *
     *  ⚠️ -1 NO es cero: un peer viejo que no manda el campo no tarda 0 ms, es
     *  que no lo dice. Quien lo pinte tiene que callarse, no escribir "0 ms". */
    public final long elapsedMs;

    /** V8 #582 — dónde: la cadena de llamadas de un error sin atrapar, «Mod+rel Mod+rel…», o "". La placa da las
     *  posiciones, sin símbolos; quien tiene el .dbg las traduce. */
    public final String stackTrace;

    public ExitedEvent(int exitCode, String reason) {
        this(exitCode, reason, -1L);
    }

    public ExitedEvent(int exitCode, String reason, long elapsedMs) {
        this(exitCode, reason, elapsedMs, "");
    }

    public ExitedEvent(int exitCode, String reason, long elapsedMs, String stackTrace) {
        this.exitCode = exitCode;
        this.reason = (reason != null) ? reason : "";
        this.elapsedMs = elapsedMs;
        this.stackTrace = (stackTrace != null) ? stackTrace : "";
    }

    @Override public String type() { return "exited"; }

    @Override public String toString() {
        return "ExitedEvent{exitCode=" + exitCode + ", reason=" + reason
                + ", elapsedMs=" + elapsedMs + "}";
    }
}
