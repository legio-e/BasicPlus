// ============================================================
// PausedEvent.java
// La VM se ha detenido en un cambio de línea (breakpoint o step).
// Lleva el snapshot estático del estado del thread; las queries de
// memoria/locals/stack se hacen aparte vía DebugQueries.
// ============================================================
package edu.bpgenvm.vm.debug;

public final class PausedEvent extends DebugEvent {
    /** Id del ThreadContext BP que está pausado (0 = main). */
    public final int tid;
    /** PC absoluto del próximo opcode a ejecutar. */
    public final int absPc;
    /** Línea 1-based en el .bp; -1 si no hay info de debug. */
    public final int line;
    /** Path al .bp original (informativo; el cliente puede mostrarlo). */
    public final String sourceFile;
    /** Registros BP en el momento de la pausa. */
    public final int bp, sp, cs;
    /** Dirección más baja del stack del thread (para acotar el stack walk). */
    public final int stackBase;

    public PausedEvent(int tid, int absPc, int line, String sourceFile,
                       int bp, int sp, int cs, int stackBase) {
        this.tid = tid;
        this.absPc = absPc;
        this.line = line;
        this.sourceFile = sourceFile;
        this.bp = bp;
        this.sp = sp;
        this.cs = cs;
        this.stackBase = stackBase;
    }

    @Override public String type() { return "paused"; }

    @Override public String toString() {
        return "PausedEvent{tid=" + tid + ", line=" + line
                + ", file=" + sourceFile + ", absPc=" + absPc
                + ", bp=" + bp + ", sp=" + sp + ", cs=" + cs + "}";
    }
}
