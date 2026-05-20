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

    public ExitedEvent(int exitCode, String reason) {
        this.exitCode = exitCode;
        this.reason = (reason != null) ? reason : "";
    }

    @Override public String type() { return "exited"; }

    @Override public String toString() {
        return "ExitedEvent{exitCode=" + exitCode + ", reason=" + reason + "}";
    }
}
