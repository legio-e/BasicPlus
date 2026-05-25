// ============================================================
// SerialBackend.java
// Implementación de Backend sobre PicoClient (USB CDC line-based).
// Es el path histórico del PicoExplorer; se mantiene para hablar con
// el firmware Pico hasta que éste se migre al wire v1.
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

public final class SerialBackend implements Backend {

    private final PicoClient client = new PicoClient();

    @Override public String displayName() { return "Pico (serial)"; }

    @Override public String connect(String endpoint) throws IOException {
        if (endpoint == null || endpoint.isEmpty()) {
            throw new IOException("falta puerto serie (ej. COM3)");
        }
        client.connect(endpoint, 115200);
        String hello = client.hello();
        // Sincronizar reloj — best effort, no falla si no soporta.
        try { client.syncTime(); } catch (IOException ignored) {}
        return hello;
    }

    @Override public boolean isConnected() { return client.isConnected(); }
    @Override public void close() { client.close(); }

    @Override public List<Entry> list() throws IOException {
        List<PicoClient.RemoteFile> raw = client.ls();
        List<Entry> out = new ArrayList<>(raw.size());
        for (PicoClient.RemoteFile f : raw) {
            // PicoClient.RemoteFile no distingue dirs (FS plano del Pico
            // con `/` como namespace). Marcamos isDir=false; el árbol
            // del UI los anida igualmente por segmentos.
            out.add(new Entry(f.name, f.size, false));
        }
        return out;
    }

    @Override public byte[] get(String path) throws IOException { return client.get(path); }
    @Override public void put(String path, byte[] data) throws IOException { client.put(path, data); }
    @Override public void del(String path) throws IOException { client.del(path); }

    @Override public String run(String path, Consumer<String> lineSink) throws IOException {
        return client.run(path, lineSink::accept);   // PicoClient.OutputSink wrapper
    }

    @Override public String mem() throws IOException { return client.mem(); }
    @Override public void save() throws IOException { client.save(); }
    @Override public String log() throws IOException { return client.log(); }
    @Override public void reset() throws IOException { client.reset(); }
}
