// ============================================================
// AbstractBpvmBackend.java
// Base común para los dos backends BPVM v1 del IDE:
//   - BpvmBackend  (TCP a la VM Java daemon)
//   - SerialBackend (USB CDC al firmware Pico)
//
// Toda la lógica que NO depende del transporte vive aquí — list/get/
// put/del/run/reset, plus la conversión chunk→línea del OUTPUT y el
// auto-CONTINUE en PausedEvent. La subclase solo aporta:
//   - openTransport(endpoint, client): connectRemote o connectSerial.
//   - displayName() / helloLabel(endpoint).
//   - mem() / save() / log(): los 3 puntos donde el wire v1 admite
//     legítima divergencia plataforma-específica.
//
// El motivo del refactor: con la migración del SerialBackend a v1
// (PR-ide-serial-v1, #151), los dos backends se parecían tanto que
// la duplicación de run() era simplemente ruido. Compartir base
// también garantiza que ambos lados se comportan idéntico en el
// chunk-to-line buffering (FrmMain añade \n por sink-call) y en la
// gestión de PausedEvent (auto-CONTINUE).
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Consumer;

public abstract class AbstractBpvmBackend implements Backend {

    /** Timeout estándar para request/reply síncronas. */
    protected static final long TIMEOUT_MS = 10_000L;
    /** Timeout para RUN. Generoso porque un programa BP puede tardar. */
    protected static final long RUN_TIMEOUT_MS = 60 * 60 * 1000L;   // 1h

    protected BpvmClient client;

    // ---- Hooks subclase ----

    /** Abre el transporte concreto en `client`. La subclase invoca
     *  `client.connectRemote(host, port)` o `client.connectSerial(port, baud)`
     *  según corresponda. */
    protected abstract void openTransport(String endpoint, BpvmClient client) throws IOException;

    /** Etiqueta legible para devolver desde connect(). Se muestra en la
     *  barra de status del PicoExplorer. */
    protected abstract String helloLabel(String endpoint);

    /** Hook opcional para sincronización post-handshake (e.g., TIME en
     *  Pico). La implementación por defecto no hace nada. */
    protected void postConnect(BpvmClient client) throws IOException { /* no-op */ }

    // ---- Conexión / lifecycle ----

    @Override public String connect(String endpoint) throws IOException {
        if (endpoint == null || endpoint.isEmpty()) {
            throw new IOException("falta endpoint para " + displayName());
        }
        BpvmClient c = new BpvmClient();
        openTransport(endpoint, c);
        if (!c.isHandshakeDone()) {
            c.close();
            throw new IOException("handshake con " + endpoint + " no completado");
        }
        try { postConnect(c); } catch (IOException ignored) { /* best effort */ }
        this.client = c;
        return helloLabel(endpoint);
    }

    @Override public boolean isConnected() {
        return client != null && client.isHandshakeDone();
    }

    /** H6.b.3.b — expone el {@link BpvmClient} subyacente para que el IDE
     *  pueda engancharle una {@code DebugSession} ("Debug on Pico") sobre el
     *  MISMO transporte ya conectado (serie o TCP), en lugar de abrir una
     *  segunda conexión al puerto (que es de acceso único). null si no hay
     *  conexión activa. */
    public BpvmClient debugClient() { return client; }

    @Override public void close() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    // ---- FS — idénticos en ambos backends ----

    @Override public List<Entry> list() throws IOException {
        require();
        List<BpvmClient.RemoteFile> raw = client.listFiles("", TIMEOUT_MS);
        List<Entry> out = new ArrayList<>(raw.size());
        for (BpvmClient.RemoteFile f : raw) {
            out.add(new Entry(f.name, f.size, f.isDirectory));
        }
        return out;
    }

    @Override public byte[] get(String path) throws IOException {
        require();
        return client.downloadFile(path, TIMEOUT_MS);
    }

    @Override public void put(String path, byte[] data) throws IOException {
        require();
        client.uploadFile(path, data, TIMEOUT_MS);
    }

    @Override public void del(String path) throws IOException {
        require();
        client.deleteFile(path, TIMEOUT_MS);
    }

    // ---- RUN — idéntico en ambos backends ----

    /** P-run-stop (#257) — manda KILL por el wire. El run() en curso (otro
     *  thread) desbloquea cuando el peer emite el EXITED con status KILLED.
     *  Si no hay programa corriendo, el peer responde NO_SESSION (one-shot:
     *  no esperamos la respuesta — aparecerá como diagnóstico). */
    @Override public void kill() throws IOException {
        require();
        client.sendCommand(edu.bpgenvm.vm.debug.StepCommand.STOP);
    }

    /**
     * Lanza `path` en el peer. Acumula los chunks OUTPUT hasta encontrar
     * `\n` y emite cada línea completa (sin el \n) al `lineSink` — el
     * FrmMain añade su propio salto al pintar. El residual sin `\n`
     * final se flushea al recibir EXITED. Si llega un PausedEvent (modo
     * --wait-client del daemon Java; el firmware Pico no emite paused
     * porque no tiene capability DEBUG), respondemos auto-CONTINUE
     * para que el RUN no se quede colgado.
     */
    @Override public String run(String path, Consumer<String> lineSink) throws IOException {
        require();
        final StringBuilder lineBuf = new StringBuilder();
        final CompletableFuture<String> done = new CompletableFuture<>();

        client.setOutputSink(chunk -> {
            if (chunk == null || chunk.isEmpty()) return;
            int last = 0;
            for (int i = 0; i < chunk.length(); i++) {
                if (chunk.charAt(i) == '\n') {
                    lineBuf.append(chunk, last, i);
                    String full = lineBuf.toString();
                    lineBuf.setLength(0);
                    if (lineSink != null) lineSink.accept(full);
                    last = i + 1;
                }
            }
            if (last < chunk.length()) {
                lineBuf.append(chunk, last, chunk.length());
            }
        });

        client.setEventListener(ev -> {
            if (ev instanceof edu.bpgenvm.vm.debug.ExitedEvent) {
                if (lineBuf.length() > 0) {
                    String trailing = lineBuf.toString();
                    lineBuf.setLength(0);
                    if (lineSink != null) lineSink.accept(trailing);
                }
                edu.bpgenvm.vm.debug.ExitedEvent e =
                        (edu.bpgenvm.vm.debug.ExitedEvent) ev;
                // BpvmClient.handleMessage ya consolidó reason desde
                // {errorMessage → reason → status}. "(OK)" en éxito,
                // "(detalle)" en fallo, "" si peer no envió ninguno.
                done.complete("exit " + e.exitCode
                        + (e.reason == null || e.reason.isEmpty() ? "" : " (" + e.reason + ")"));
            } else if (ev instanceof edu.bpgenvm.vm.debug.PausedEvent) {
                // Daemon Java arranca en STEP_INTO si --wait-client → primer
                // hook pausa. En el flow "Run" del Explorer queremos
                // no-debug: auto-CONTINUE al primer paused. En el firmware
                // Pico la capability DEBUG no está expuesta todavía, así
                // que esto nunca se dispara — listener inocuo allí.
                client.sendCommand(edu.bpgenvm.vm.debug.StepCommand.CONTINUE);
            }
            // ExceptionEvent ya no se emite — el wire v1 fold runtime
            // errors dentro de EXITED.status="RUNTIME_ERROR". Si algún
            // peer histórico lo enviara, BpvmClient lo dispatchea pero
            // aquí lo ignoramos: el EXITED que sigue cerrará el future.
        });

        client.runModule(path);
        try {
            return done.get(RUN_TIMEOUT_MS, TimeUnit.MILLISECONDS);   // ← kill() desbloquea vía EXITED
        } catch (TimeoutException te) {
            throw new IOException("timeout esperando EXITED del programa");
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrupted", ie);
        } catch (java.util.concurrent.ExecutionException ee) {
            throw new IOException("fallo en RUN", ee.getCause());
        } finally {
            client.setOutputSink(null);
            client.setEventListener(null);
        }
    }

    // ---- RESET — idéntico ----

    @Override public void reset() throws IOException {
        require();
        BpvmClient c = this.client;
        this.client = null;
        c.reset();
    }

    // ---- Helpers ----

    protected final void require() throws IOException {
        if (!isConnected()) {
            throw new IOException(displayName() + " no conectado");
        }
    }
}
