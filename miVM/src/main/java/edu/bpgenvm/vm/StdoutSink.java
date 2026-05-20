// ============================================================
// StdoutSink.java
// OutputSink por defecto: escribe a System.out con sincronización
// externa para que varios workers no entrelacen chunks a medio
// PRINT_STR_NONL.
// ============================================================
package edu.bpgenvm.vm;

public final class StdoutSink implements OutputSink {
    /** Lock dedicado del sink. NO uses System.out como monitor — algunos
     *  PrintStream lo hacen pero no es contrato. */
    private final Object lock = new Object();

    @Override public void writeText(String s) {
        synchronized (lock) { System.out.print(s); }
    }

    @Override public void writeChar(char c) {
        synchronized (lock) { System.out.print(c); }
    }

    @Override public void newline() {
        synchronized (lock) { System.out.println(); }
    }

    @Override public void flush() {
        synchronized (lock) { System.out.flush(); }
    }
}
