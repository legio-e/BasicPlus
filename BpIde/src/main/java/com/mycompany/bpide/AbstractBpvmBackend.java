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

    /** #425 — lo que el device dijo que dejó fuera en el último list(). */
    private volatile long lastOmitted = 0;
    @Override public long lastListOmitted() { return lastOmitted; }

    @Override public List<Entry> list() throws IOException {
        require();
        BpvmClient.DirListing lst = client.listFiles("", TIMEOUT_MS);
        lastOmitted = lst.omitidas;   // #425
        List<Entry> out = new ArrayList<>(lst.files.size());
        for (BpvmClient.RemoteFile f : lst.files) {
            out.add(new Entry(f.name, f.size, f.crc, f.isDirectory, f.mal));   // paso 4 cierre: el CRC; E1: el motivo
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
     *  V8 #694: y ESPERA su respuesta —KILL_REPLY, o el ERROR NO_SESSION si no
     *  había programa—; si no llega o es un ERROR, lo lanza, y quien llama lo
     *  dice. Antes iba sin respuesta, y el IDE decía «KILL enviado» igual. */
    @Override public void kill() throws IOException {
        require();
        client.killConRespuesta(TIMEOUT_MS);
    }

    /** #256 — true mientras un run() de ESTE backend está en vuelo (su
     *  event listener es el dueño del EXITED y ya lo muestra). false en el
     *  escenario "attach": el programa lo arrancó la placa (autorun) y el
     *  EXITED de un kill no lo consume nadie — el caller puede pedir
     *  mostrarlo con {@link #onNextExited}. */
    public boolean isRunActive() { return runActive; }
    private volatile boolean runActive;

    /** #256 — listener efímero para el escenario attach: muestra el primer
     *  EXITED que llegue (el del kill) por el sink y se autodesinstala.
     *  Un run() posterior lo pisa sin conflicto (setEventListener). */
    public void onNextExited(Consumer<String> sink) {
        BpvmClient c = this.client;
        if (c == null) return;
        c.setEventListener(ev -> {
            if (ev instanceof edu.bpgenvm.vm.debug.ExitedEvent) {
                edu.bpgenvm.vm.debug.ExitedEvent e =
                        (edu.bpgenvm.vm.debug.ExitedEvent) ev;
                if (sink != null) {
                    sink.accept("[Placa] programa abortado: exit " + e.exitCode
                            + (e.reason == null || e.reason.isEmpty()
                               ? "" : " (" + e.reason + ")"));
                }
                c.setEventListener(null);
            }
        });
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
    @Override public String run(String path, String arg, Consumer<String> lineSink) throws IOException {
        require();
        final BpvmClient c = this.client;                    // #803: close() lo pone a null desde otro hilo
        runActive = true;                                    // #256 — ver isRunActive()
        final StringBuilder lineBuf = new StringBuilder();
        final CompletableFuture<String> done = new CompletableFuture<>();

        c.setOutputSink(chunk -> {
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

        c.setEventListener(ev -> {
            if (ev instanceof edu.bpgenvm.vm.debug.ExitedEvent) {
                if (lineBuf.length() > 0) {
                    String trailing = lineBuf.toString();
                    lineBuf.setLength(0);
                    if (lineSink != null) lineSink.accept(trailing);
                }
                edu.bpgenvm.vm.debug.ExitedEvent e =
                        (edu.bpgenvm.vm.debug.ExitedEvent) ev;
                // V8 #582 — dónde: la placa da posiciones, y el .dbg las hace líneas.
                java.util.List<String> donde = TrazaError.traduce(e.stackTrace);
                if (lineSink != null)
                    for (int i = 0; i < donde.size(); i++)
                        lineSink.accept((i == 0 ? "  en " : "  llamado desde ") + donde.get(i));
                // BpvmClient.handleMessage ya consolidó reason desde
                // {errorMessage → reason → status}. "(OK)" en éxito,
                // "(detalle)" en fallo, "" si peer no envió ninguno.
                done.complete("exit " + e.exitCode
                        + (e.reason == null || e.reason.isEmpty() ? "" : " (" + e.reason
                                + (donde.isEmpty() ? "" : ", " + donde.get(0)) + ")")
                        + formatoDuracion(e.elapsedMs));
            } else if (ev instanceof edu.bpgenvm.vm.debug.PausedEvent) {
                // Daemon Java arranca en STEP_INTO si --wait-client → primer
                // hook pausa. En el flow "Run" del Explorer queremos
                // no-debug: auto-CONTINUE al primer paused. En el firmware
                // Pico la capability DEBUG no está expuesta todavía, así
                // que esto nunca se dispara — listener inocuo allí.
                c.sendCommand(edu.bpgenvm.vm.debug.StepCommand.CONTINUE);
            }
            // ExceptionEvent ya no se emite — el wire v1 fold runtime
            // errors dentro de EXITED.status="RUNTIME_ERROR". Si algún
            // peer histórico lo enviara, BpvmClient lo dispatchea pero
            // aquí lo ignoramos: el EXITED que sigue cerrará el future.
        });

        try {
            /* V6/#412; V8 #694 — el RUN espera su RUN_REPLY (la placa lo manda al poner el programa en marcha): si la
             * placa lo rechaza (NOT_FOUND, BUSY…) o no contesta, se dice aquí, en vez de esperar el EXITED una hora. */
            try {
                c.runModuleConRespuesta(path, arg, TIMEOUT_MS);
            } catch (BpvmClient.WireError we) {
                throw new IOException("la placa rechaza el RUN: " + we.getMessage(), we);
            } catch (IOException ioe) {
                throw new IOException("la placa no contesta al RUN: " + ioe.getMessage(), ioe);
            }
            return esperaElFin(c, done);                     // ← kill() desbloquea vía EXITED
        } catch (TimeoutException te) {
            throw new IOException("timeout esperando EXITED del programa");
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrupted", ie);
        } catch (java.util.concurrent.ExecutionException ee) {
            throw new IOException("fallo en RUN", ee.getCause());
        } finally {
            runActive = false;                               // #256
            c.setOutputSink(null);
            c.setEventListener(null);
        }
    }

    /** V8/H3 (#803) — el EXITED, por rodajas: con la conexión perdida no va a llegar, y esperarlo una hora dejaba los
     *  botones en gris sin decir nada. El programa no se toca: puede seguir en marcha en la placa. */
    private static String esperaElFin(BpvmClient c, CompletableFuture<String> done)
            throws IOException, InterruptedException, java.util.concurrent.ExecutionException, TimeoutException {
        final long tope = System.currentTimeMillis() + RUN_TIMEOUT_MS;
        for (;;) {
            try {
                return done.get(250, TimeUnit.MILLISECONDS);
            } catch (TimeoutException te) {
                if (c.conexionPerdida())
                    throw new IOException("se ha perdido la conexión: el programa puede seguir en marcha");
                if (c.cerrado())
                    throw new IOException("desconectado: el programa puede seguir en marcha");
                if (System.currentTimeMillis() > tope) throw te;
            }
        }
    }

    /** V6/E1 — el tiempo de ejecución, para pegarlo al «exit N (motivo)».
     *
     *  Devuelve "" si el peer no mandó el campo: un firmware que no lo dice no
     *  tardó 0 ms, y escribir «0 ms» seria inventarse una medida.
     *
     *  📌 Y significa lo mismo en los seis emisores: LA EJECUCIÓN, con el módulo
     *  ya cargado. miVM lo media desde que le llegaba el RUN —7 ms de mas,
     *  medidos— y se corrigió el mismo dia que se enchufó esto. */
    private static String formatoDuracion(long ms) {
        if (ms < 0) return "";
        if (ms < 10000) return " en " + ms + " ms";
        return " en " + String.format(java.util.Locale.ROOT, "%.1f", ms / 1000.0) + " s";
    }

    // ---- RESET — idéntico ----

    @Override public void reset() throws IOException {
        require();
        BpvmClient c = this.client;
        this.client = null;
        try {
            c.reset();
        } catch (BpvmClient.WireError we) {
            // V6/E1 (#452) — el device ha dicho que NO y sigue vivo: devolvemos
            // el cliente a su sitio. Soltarlo dejaba al usuario sin reset y sin
            // conexión, que es peor que el fallo que venía a contar.
            this.client = c;
            throw we;
        }
    }

    // ---- Helpers ----

    protected final void require() throws IOException {
        if (!isConnected()) {
            throw new IOException(displayName() + " no conectado");
        }
    }
}
