// ============================================================
// ResumedEvent.java
// La VM ha reanudado tras un comando del cliente (continue, step).
// ============================================================
package edu.bpgenvm.vm.debug;

public final class ResumedEvent extends DebugEvent {
    /** Id del ThreadContext BP que reanuda. */
    public final int tid;

    public ResumedEvent(int tid) {
        this.tid = tid;
    }

    @Override public String type() { return "resumed"; }

    @Override public String toString() {
        return "ResumedEvent{tid=" + tid + "}";
    }
}
