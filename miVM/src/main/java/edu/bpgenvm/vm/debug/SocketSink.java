// ============================================================
// SocketSink.java
// OutputSink que serializa cada chunk PRINT_* del programa BP como un
// mensaje OUTPUT v1 (docs/BPVM_WIRE_PROTOCOL.md §6.3) sobre un Writer
// (socket en la práctica).
//
// Formato del mensaje: una línea JSON por chunk, terminada en \n.
//   {"type":"OUTPUT","data":"hola\n","stream":"stdout"}
//
// El IDE concatena los `data` recibidos en orden. PR-3 añadirá el
// campo `session` cuando entren multi-sesión.
//
// Thread-safety: writeText/writeChar/newline/flush pueden invocarse
// desde cualquier worker BP a la vez. Sincronizamos en el writer.
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.OutputSink;

import java.io.PrintWriter;

public final class SocketSink implements OutputSink {

    /** Tamaño del buffer antes de forzar un flush aunque no haya newline.
     *  Evita que un programa que escribe mucho sin saltos de línea acumule
     *  indefinidamente en memoria. */
    private static final int FLUSH_THRESHOLD = 4096;

    private final PrintWriter out;
    private final Object lock = new Object();
    /** Buffer de chunks pendientes — se consolida en un solo mensaje JSON
     *  cuando llega un newline() o se supera FLUSH_THRESHOLD. */
    private final StringBuilder buf = new StringBuilder();

    public SocketSink(PrintWriter out) {
        if (out == null) throw new IllegalArgumentException("out null");
        this.out = out;
    }

    @Override public void writeText(String s) {
        if (s == null || s.isEmpty()) return;
        synchronized (lock) {
            buf.append(s);
            if (buf.length() > FLUSH_THRESHOLD) flushBuffer();
        }
    }

    @Override public void writeChar(char c) {
        synchronized (lock) {
            buf.append(c);
            if (buf.length() > FLUSH_THRESHOLD) flushBuffer();
        }
    }

    @Override public void newline() {
        synchronized (lock) {
            buf.append('\n');
            flushBuffer();
        }
    }

    @Override public void flush() {
        synchronized (lock) {
            flushBuffer();
            out.flush();
        }
    }

    /** Emite el buffer (si tiene algo) como un único evento OUTPUT v1. */
    private void flushBuffer() {
        if (buf.length() == 0) return;
        out.print("{\"type\":\"OUTPUT\",\"stream\":\"stdout\",\"data\":\"");
        out.print(Json.escape(buf.toString()));
        out.print("\"}\n");
        out.flush();
        buf.setLength(0);
    }
}
