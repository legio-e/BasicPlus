// ============================================================
// SocketSink.java
// OutputSink que serializa cada chunk PRINT_* del programa BP como un
// mensaje OUTPUT v1 (docs/BPVM_WIRE_PROTOCOL.md §6.3) y lo emite por
// un `Consumer<String>` (en práctica DebugServer::send, que cuida del
// framing y la sincronización con el resto del tráfico).
//
// Formato del mensaje: una línea JSON por chunk, terminada en `\n`
// por el lado del sender:
//   {"type":"OUTPUT","session":N,"data":"hola\n","stream":"stdout"}
//
// El IDE concatena los `data` recibidos en orden. PR-3 introdujo
// `session`; el supplier la lee en cada flush para reflejar la sesión
// activa en el momento exacto del print.
//
// Thread-safety: writeText/writeChar/newline/flush pueden invocarse
// desde cualquier worker BP a la vez. Sincronizamos en el lock interno;
// la atomicidad de la emisión al wire la garantiza el sender.
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.OutputSink;

import java.util.function.Consumer;
import java.util.function.IntSupplier;

public final class SocketSink implements OutputSink {

    /** Tamaño del buffer antes de forzar un flush aunque no haya newline.
     *  Evita que un programa que escribe mucho sin saltos de línea acumule
     *  indefinidamente en memoria. */
    private static final int FLUSH_THRESHOLD = 4096;

    private final Consumer<String> lineSink;
    private final IntSupplier sessionSupplier;
    private final Object lock = new Object();
    /** Buffer de chunks pendientes — se consolida en un solo mensaje JSON
     *  cuando llega un newline() o se supera FLUSH_THRESHOLD. */
    private final StringBuilder buf = new StringBuilder();

    public SocketSink(Consumer<String> lineSink, IntSupplier sessionSupplier) {
        if (lineSink == null) throw new IllegalArgumentException("lineSink null");
        if (sessionSupplier == null) throw new IllegalArgumentException("sessionSupplier null");
        this.lineSink = lineSink;
        this.sessionSupplier = sessionSupplier;
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
        }
    }

    /** Emite el buffer (si tiene algo) como un único evento OUTPUT v1. */
    private void flushBuffer() {
        if (buf.length() == 0) return;
        int sess = sessionSupplier.getAsInt();
        String line = "{\"type\":\"OUTPUT\",\"session\":" + sess
                + ",\"stream\":\"stdout\",\"data\":\""
                + Json.escape(buf.toString()) + "\"}";
        buf.setLength(0);
        lineSink.accept(line);
    }
}
