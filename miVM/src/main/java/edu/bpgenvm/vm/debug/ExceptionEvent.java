// ============================================================
// ExceptionEvent.java
// Un thread BP ha disparado una RuntimeError no atrapada (BpThreadFault
// que llegó hasta arriba del worker, sin try/catch que la maneje).
// El thread termina; otros siguen vivos. La VM no muere.
// ============================================================
package edu.bpgenvm.vm.debug;

public final class ExceptionEvent extends DebugEvent {
    /** Id del ThreadContext BP que disparó la excepción. */
    public final int tid;
    /** Mensaje extraído de la excepción nativa. */
    public final String message;
    /** Stack trace en formato texto (multi-línea), o "" si no se reconstruyó. */
    public final String stackTrace;

    public ExceptionEvent(int tid, String message, String stackTrace) {
        this.tid = tid;
        this.message = (message != null) ? message : "";
        this.stackTrace = (stackTrace != null) ? stackTrace : "";
    }

    @Override public String type() { return "exception"; }

    @Override public String toString() {
        return "ExceptionEvent{tid=" + tid + ", message=" + message + "}";
    }
}
