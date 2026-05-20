// ============================================================
// SocketSink.java
// OutputSink que serializa cada chunk PRINT_* del programa BP como un
// mensaje JSON sobre un Writer (socket en la práctica).
//
// Formato del mensaje: una línea JSON por chunk, terminada en \n.
//   {"type":"print","data":"hola"}
//   {"type":"print","data":"\n"}
//
// El IDE concatena los `data` recibidos en orden. NO bufferizamos para
// que la latencia entre `print` BP y la ventana del IDE sea baja —
// si en algún momento eso causa overhead notable, añadimos un buffer
// con flush por timer.
//
// Thread-safety: writeText/writeChar/newline/flush pueden invocarse
// desde cualquier worker BP a la vez. Sincronizamos en el writer.
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.OutputSink;

import java.io.PrintWriter;

public final class SocketSink implements OutputSink {

    private final PrintWriter out;
    private final Object lock = new Object();

    public SocketSink(PrintWriter out) {
        if (out == null) throw new IllegalArgumentException("out null");
        this.out = out;
    }

    @Override public void writeText(String s) {
        if (s == null || s.isEmpty()) return;
        sendChunk(s);
    }

    @Override public void writeChar(char c) {
        sendChunk(String.valueOf(c));
    }

    @Override public void newline() {
        sendChunk("\n");
    }

    @Override public void flush() {
        synchronized (lock) {
            out.flush();
        }
    }

    private void sendChunk(String data) {
        synchronized (lock) {
            out.print("{\"type\":\"print\",\"data\":\"");
            out.print(Json.escape(data));
            out.print("\"}\n");
            out.flush();
        }
    }
}
