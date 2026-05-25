// ============================================================
// BpvmClient.java
// Cliente del IDE para hablar con la VM cuando ésta corre como
// PROCESO SEPARADO (A1.5).
//
// Responsabilidades:
//   1) Lanzar bpgenvm como subproceso con `--listen <port>
//      --wait-client <fichero.mod>` (o `<proyecto.bpproject>`).
//   2) Conectar por TCP a localhost:<port>.
//   3) Hablar el protocolo BPVM v1 (docs/BPVM_WIRE_PROTOCOL.md):
//        - Saliente: HELLO, RUN, KILL, STEP, CONTINUE, SET_BP, CLR_BP,
//          LOCALS, STACK, MODULE_PROPERTIES, READ_INT, READ_STRING,
//          PUT, GET, LIST, DEL, MKDIR, PROMPT_RESPONSE.
//        - Entrante eventos: HELLO_REPLY (handshake), OUTPUT,
//          BP_HIT (→ PausedEvent), RESUMED, EXITED, EXCEPTION,
//          PROMPT_REQUEST.
//        - Entrante replies: X_REPLY con `id` correlado, ERROR.
//        - Bulk binario inline en PUT (request) y GET_REPLY (reply):
//          tras el `\n` del JSON vienen N bytes raw declarados en
//          `"bulk":N`.
//
// PR-2: bulk binario raw inline (sustituye el base64 de PR-1).
// PR-1 framing limitaciones documentadas en DebugServer.java
// (sin session, sin códigos de error específicos).
//
// Lifecycle:
//   - start(modPath) bloquea hasta tener HELLO_REPLY del server o
//     timeout. Tras start() el lector y los comandos están listos.
//   - close() corta la conexión, manda SIGTERM al subproceso y espera.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.util.WireFraming;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.ExceptionEvent;
import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import purejavacomm.CommPortIdentifier;
import purejavacomm.NoSuchPortException;
import purejavacomm.PortInUseException;
import purejavacomm.SerialPort;
import purejavacomm.UnsupportedCommOperationException;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;

public final class BpvmClient implements AutoCloseable {

    /** Clave sintética que la readLoop pone en el Map de respuesta cuando
     *  el reply venía con bulk binario; el valor es el byte[] crudo. */
    private static final String BULK_KEY = "__bulk";

    /** N20 — Listener para PROMPT_REQUEST del servidor. El IDE registra un
     *  callback que muestra el formulario y devuelve los valores como JSON.
     *  La VM espera bloqueada hasta que `respondToPrompt(promptId, json)`
     *  sea llamado. */
    @FunctionalInterface
    public interface PromptHandler {
        void onPrompt(long promptId, String spec);
    }
    private volatile PromptHandler promptHandler;
    public void setPromptHandler(PromptHandler h) { this.promptHandler = h; }

    /** Envía la respuesta del usuario para un prompt en vuelo (N20). */
    public void respondToPrompt(long promptId, String valuesJson) {
        sendOneShot("PROMPT_RESPONSE",
                "\"promptId\":" + promptId
                + ",\"values\":" + jsonStr(valuesJson == null ? "" : valuesJson),
                null);
    }

    /** Sink para los chunks OUTPUT del programa BP. Lo invoca el thread
     *  lector — los handlers deben reenviar al EDT con invokeLater. */
    private volatile Consumer<String> outputSink;

    /** Listener para eventos paused/resumed/exited/exception. Idem hilo. */
    private volatile DebugListener eventListener;

    /** Sink para "logs" del subproceso (stderr) y mensajes diagnósticos
     *  del propio cliente. Si está a null se descarta. */
    private volatile Consumer<String> diagSink;

    private Process process;
    private Socket socket;
    /** USB CDC port cuando connectSerial() en lugar de TCP. */
    private SerialPort serialPort;
    /** Stream crudo de escritura. Protegido por `writeLock`. */
    private OutputStream outRaw;
    /** Stream crudo de lectura. Usado SÓLO desde readLoop thread. */
    private InputStream inRaw;
    private final Object writeLock = new Object();

    private Thread readerThread;
    private Thread stderrPumpThread;
    /** Latch que cierra cuando llega HELLO_REPLY (handshake completo). */
    private final CountDownLatch helloLatch = new CountDownLatch(1);
    /** Build/version reportado por el server en HELLO_REPLY. */
    private volatile String serverBuild;
    /** PR-3 — sessionId activa (0 = ninguna). La aprendemos de RUN_REPLY o
     *  del campo `session` de cualquier evento entrante. La usamos para
     *  componer KILL { session: N }. */
    private volatile long currentSession = 0;

    /** Executor single-thread donde se invocan los DebugListener. Es CRUCIAL
     *  que no corran en el reader thread: si lo hicieran y un listener
     *  invocase una query síncrona (sendRequest → future.get), el reader
     *  thread quedaría bloqueado esperando una respuesta que él mismo es
     *  el único que puede leer del socket. Single-thread mantiene el orden
     *  de eventos. */
    private final java.util.concurrent.ExecutorService listenerExec =
            java.util.concurrent.Executors.newSingleThreadExecutor(r -> {
                Thread t = new Thread(r, "bp-vmclient-listener");
                t.setDaemon(true);
                return t;
            });

    private volatile boolean closed = false;

    // ---- Request/response (A1.6, v1 framing) ----
    private final AtomicLong nextRequestId = new AtomicLong(1);
    private final Map<Long, CompletableFuture<Map<String, Object>>> pendingRequests = new HashMap<>();
    private final Object pendingLock = new Object();

    // ---- Configuración ----

    public void setOutputSink(Consumer<String> sink)       { this.outputSink = sink; }
    public void setEventListener(DebugListener listener)   { this.eventListener = listener; }
    public void setDiagSink(Consumer<String> sink)         { this.diagSink = sink; }

    // ---- Lifecycle ----

    /**
     * Arranca el subproceso bpgenvm con --listen y --wait-client, y conecta
     * por socket. Devuelve cuando el HELLO_REPLY del server ha llegado, o
     * lanza IOException si timeout/error.
     */
    public void start(String modOrProjectPath, boolean waitClient) throws IOException {
        startInternal(modOrProjectPath, null, waitClient, null);
    }

    /** A2.3 — Arranca la VM en MODO DAEMON: sin .mod en CLI, con un
     *  workdir activo. Tras conectar, sube ficheros con uploadFile() y
     *  arranca con runModule(). */
    public void startDaemon(String workdir, boolean waitClient) throws IOException {
        startDaemon(workdir, waitClient, null);
    }

    /** A2.6 — Conecta a una VM REMOTA ya corriendo en {@code host:port}. */
    public void connectRemote(String host, int port) throws IOException {
        try {
            this.socket = new java.net.Socket(host, port);
            this.outRaw = new BufferedOutputStream(socket.getOutputStream());
            this.inRaw  = new BufferedInputStream(socket.getInputStream());
        } catch (IOException e) {
            throw new IOException("no se pudo conectar a la VM remota " + host + ":" + port
                    + " — ¿está corriendo `bpgenvm --listen " + port + " --workdir ...` allí?", e);
        }
        startReaderThread();
        doHandshake();
    }

    /** Conecta por USB CDC al firmware Pico (wire v1 sobre serial). El
     *  flujo es idéntico al TCP: abrir transporte, drenar banner del
     *  boot, arrancar reader thread, hacer handshake HELLO/HELLO_REPLY.
     *
     *  Sobre purejavacomm:
     *   - DTR=true es CRÍTICO: el firmware mira DTR para detectar host
     *     conectado; sin él descarta la salida.
     *   - enableReceiveTimeout no se llama → reads bloquean indefinidos.
     *     close() del puerto interrumpe el read en curso. */
    public void connectSerial(String portName, int baud) throws IOException {
        SerialPort port;
        try {
            CommPortIdentifier id = CommPortIdentifier.getPortIdentifier(portName);
            port = (SerialPort) id.open("BpIde-BpvmClient-v1", 2000);
            port.setSerialPortParams(baud, SerialPort.DATABITS_8,
                    SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
            port.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
            port.setDTR(true);
            port.setRTS(false);
            // enableReceiveTimeout(500): purejavacomm devuelve -1 en read()
            // tras 500ms sin datos. Lo NECESITAMOS para que close() del puerto
            // desbloquee el reader thread limpiamente. Envolvemos la
            // InputStream para que ese -1-de-timeout se vea como "esperar más"
            // en lugar de EOF.
            port.enableReceiveTimeout(500);
        } catch (NoSuchPortException | PortInUseException
                | UnsupportedCommOperationException e) {
            throw new IOException("open " + portName + ": " + e.getMessage(), e);
        }
        this.serialPort = port;
        InputStream rawIn   = port.getInputStream();
        OutputStream rawOut = port.getOutputStream();

        // Drain inicial: descartar el banner de boot del firmware ("===
        // bpvm-pico REPL listo ===" y prompts "> " residuales). 300 ms es
        // suficiente — el firmware reparte el banner en 3 rondas con
        // delays de 200ms; en runtime estable apenas hay tráfico.
        // (Intento previo de drain adaptive con cap 500ms+50ms-de-silencio
        // causaba delays inexplicables de ~10s en el primer LIST tras
        // connect; pendiente de investigar — ver task #154. Hasta entender
        // la causa, volvemos al wait fijo conservador.)
        try {
            long endDrain = System.currentTimeMillis() + 300;
            byte[] tmp = new byte[256];
            while (System.currentTimeMillis() < endDrain) {
                int avail = rawIn.available();
                if (avail > 0) {
                    int n = rawIn.read(tmp, 0, Math.min(avail, tmp.length));
                    if (n <= 0) break;
                } else {
                    try { Thread.sleep(20); }
                    catch (InterruptedException ie) {
                        Thread.currentThread().interrupt();
                        throw new IOException("interrupted during drain", ie);
                    }
                }
            }
        } catch (IOException ioe) {
            try { port.close(); } catch (Throwable ignored) {}
            this.serialPort = null;
            throw new IOException("drain inicial falló: " + ioe.getMessage(), ioe);
        }

        this.inRaw  = new SerialBlockingInputStream(rawIn);
        this.outRaw = rawOut;   // raw — purejavacomm a veces se atraganta con buffered
        startReaderThread();
        doHandshake();
    }

    /** Wrapper que convierte el -1-de-timeout de purejavacomm en "espera
     *  más" para que WireFraming.recvLine() no lo interprete como EOF.
     *  Solo close() (que pone closed=true y cierra el puerto) sale del
     *  bucle con un -1 "real". */
    private final class SerialBlockingInputStream extends InputStream {
        private final InputStream delegate;
        SerialBlockingInputStream(InputStream d) { this.delegate = d; }
        @Override public int read() throws IOException {
            while (!closed) {
                int c;
                try { c = delegate.read(); }
                catch (IOException ioe) {
                    if (closed) return -1;
                    throw ioe;
                }
                if (c >= 0) return c;
                // -1: timeout en purejavacomm (NO EOF real). Reintentar.
            }
            return -1;
        }
        @Override public int read(byte[] b, int off, int len) throws IOException {
            if (len == 0) return 0;
            while (!closed) {
                int r;
                try { r = delegate.read(b, off, len); }
                catch (IOException ioe) {
                    if (closed) return -1;
                    throw ioe;
                }
                if (r > 0) return r;
                if (r == 0) continue;
                // r == -1: timeout, reintentar.
            }
            return -1;
        }
        @Override public int available() throws IOException { return delegate.available(); }
        @Override public void close() throws IOException { delegate.close(); }
    }

    /** Variante con stdlibDir explícito: se pasa como --stdlibDir al
     *  subproceso VM y gana sobre cualquier BpVM.cfg autodiscovery. */
    public void startDaemon(String workdir, boolean waitClient, String stdlibDir) throws IOException {
        if (workdir == null) throw new IllegalArgumentException("workdir requerido en modo daemon");
        startInternal(null, workdir, waitClient, stdlibDir);
    }

    private void startInternal(String modPath, String workdir, boolean waitClient,
                               String stdlibDir) throws IOException {
        int port = reserveFreePort();
        spawnVmProcess(port, modPath, workdir, waitClient, stdlibDir);
        try {
            connectWithRetry(port, 5000);
        } catch (IOException e) {
            terminateProcess();
            throw e;
        }
        startReaderThread();
        doHandshake();
    }

    /** v1 handshake: client → HELLO request; server → HELLO_REPLY.
     *  Bloquea hasta recibir el HELLO_REPLY o un timeout corto. */
    private void doHandshake() {
        try {
            Map<String, Object> resp = sendRequest("HELLO",
                    "\"protoVersion\":1,\"clientName\":\"BpIde\",\"clientBuild\":\"1.0\"",
                    null, 5000);
            serverBuild = Json.getString(resp, "serverBuild", "?");
            diag("[BpvmClient] handshake con VM: " + Json.getString(resp, "serverName", "?")
                    + " " + serverBuild
                    + " protoVersion=" + Json.getLong(resp, "protoVersion", 0));
        } catch (IOException e) {
            diag("[BpvmClient] handshake falló: " + e.getMessage());
        }
    }

    /** Cierra el socket o el puerto serie, mata el subproceso (si lo hay),
     *  junta los threads. Idempotente. */
    @Override public void close() {
        closed = true;
        try { if (outRaw != null) outRaw.flush(); } catch (Throwable ignored) {}
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
        // El SerialPort.close() interrumpe el reader thread que está dentro
        // de delegate.read() — purejavacomm lanza IOException que nuestro
        // SerialBlockingInputStream traduce a -1 (porque closed=true ya).
        try { if (serialPort != null) serialPort.close(); } catch (Throwable ignored) {}
        serialPort = null;
        terminateProcess();
        try { if (readerThread != null) readerThread.join(2000); } catch (InterruptedException ignored) {}
        try { if (stderrPumpThread != null) stderrPumpThread.join(1000); } catch (InterruptedException ignored) {}
        listenerExec.shutdown();
        try { listenerExec.awaitTermination(1, TimeUnit.SECONDS); }
        catch (InterruptedException ignored) {}
    }

    private void terminateProcess() {
        if (process != null && process.isAlive()) {
            process.destroy();
            try {
                if (!process.waitFor(3, TimeUnit.SECONDS)) {
                    process.destroyForcibly();
                }
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                process.destroyForcibly();
            }
        }
    }

    // ---- Comandos al VM (cliente → server) ----

    /** Escribe una línea JSON + opcionalmente bulk binario inmediatamente
     *  detrás. Atómico para el peer (todo bajo writeLock). */
    private void writeFrame(String jsonLine, byte[] bulkOrNull) throws IOException {
        OutputStream w = this.outRaw;
        if (w == null) throw new IOException("BpvmClient cerrado o no conectado");
        synchronized (writeLock) {
            WireFraming.sendLine(w, jsonLine);
            if (bulkOrNull != null && bulkOrNull.length > 0) {
                WireFraming.sendBulk(w, bulkOrNull);
            }
            w.flush();
        }
    }

    /** Envío fire-and-forget de un request v1 — asigna `id`, no espera
     *  reply. La reply correspondiente llega y se descarta (no hay future
     *  registrada). */
    private void sendOneShot(String type, String extraJson, byte[] bulkOrNull) {
        long id = nextRequestId.getAndIncrement();
        StringBuilder sb = new StringBuilder(64);
        sb.append("{\"type\":\"").append(type).append("\",\"id\":").append(id);
        if (extraJson != null && !extraJson.isEmpty()) {
            sb.append(',').append(extraJson);
        }
        if (bulkOrNull != null) {
            sb.append(",\"bulk\":").append(bulkOrNull.length);
        }
        sb.append('}');
        try {
            writeFrame(sb.toString(), bulkOrNull);
        } catch (IOException e) {
            diag("[BpvmClient] sendOneShot('" + type + "') falló: " + e.getMessage());
        }
    }

    public void setBreakpoint(String file, int line, boolean enabled) {
        sendOneShot("SET_BP",
                "\"file\":\"" + Json.escape(file) + "\""
                + ",\"line\":" + line
                + ",\"enabled\":" + (enabled ? "true" : "false"),
                null);
    }

    public void clearAllBreakpoints() {
        sendOneShot("CLR_BP", "", null);
    }

    /** A2.3 — Ordena al daemon arrancar el módulo `module`. Fire-and-forget:
     *  el EXITED event es la confirmación del fin de ejecución. */
    public void runModule(String module) {
        sendOneShot("RUN", "\"path\":\"" + Json.escape(module) + "\"", null);
    }

    /** PR-7c — Pide al server que se reinicie. El protocolo v1 manda que el
     *  server responda con RESET_REPLY ANTES de matar el proceso (delay
     *  ~100ms para que el reply salga del socket). En la VM Java
     *  &quot;reboot&quot; = System.exit(0); en el firmware Pico = reset HW.
     *
     *  Tras un RESET la conexión se rompe en breve, así que:
     *   - Usamos un timeout corto (2s) para la reply.
     *   - Al volver llamamos close() para limpiar el lado cliente y dejar
     *     todo en orden, sin esperar a que el peer cierre TCP.
     *
     *  Si el server no responde a tiempo (e.g. ya estaba muriendo) no se
     *  considera error fatal — devolvemos sin lanzar y aún así close().
     */
    public void reset() throws IOException {
        try {
            sendRequest("RESET", null, null, 2000);
        } catch (IOException ioe) {
            // Reply perdido es esperable cuando el proceso ya está muriendo.
            // Lo logueamos pero no propagamos — el cliente quiere "reset
            // best-effort", no una garantía de RESET_REPLY síncrona.
            diag("[BpvmClient] RESET sin reply síncrona: " + ioe.getMessage());
        } finally {
            close();
        }
    }

    public void sendCommand(StepCommand cmd) {
        switch (cmd) {
            case CONTINUE:  sendOneShot("CONTINUE", "", null); break;
            case STEP_INTO: sendOneShot("STEP", "\"mode\":\"into\"", null); break;
            case STEP_OVER: sendOneShot("STEP", "\"mode\":\"over\"", null); break;
            case STEP_OUT:  sendOneShot("STEP", "\"mode\":\"out\"", null); break;
            case STOP: {
                long sess = currentSession;
                String extra = sess > 0 ? "\"session\":" + sess : "";
                sendOneShot("KILL", extra, null);
                break;
            }
        }
    }

    // ============================================================
    // Request/response — queries síncronas al VM (A1.6, v1 framing)
    // ============================================================

    /** Envía un request v1 y devuelve el mapa de respuesta. Lanza
     *  IOException si timeout o si la respuesta es un ERROR. `extraJson`
     *  va como pares CSV ya formateados (sin llaves). `bulkOrNull` se
     *  serializa como `"bulk":N` + N bytes raw tras el `\n`. */
    private Map<String, Object> sendRequest(String type, String extraJson, byte[] bulkOrNull, long timeoutMs)
            throws IOException {
        long id = nextRequestId.getAndIncrement();
        CompletableFuture<Map<String, Object>> future = new CompletableFuture<>();
        synchronized (pendingLock) { pendingRequests.put(id, future); }
        StringBuilder sb = new StringBuilder();
        sb.append("{\"type\":\"").append(type).append("\",\"id\":").append(id);
        if (extraJson != null && !extraJson.isEmpty()) {
            sb.append(',').append(extraJson);
        }
        if (bulkOrNull != null) {
            sb.append(",\"bulk\":").append(bulkOrNull.length);
        }
        sb.append('}');
        writeFrame(sb.toString(), bulkOrNull);
        try {
            Map<String, Object> resp = future.get(timeoutMs, TimeUnit.MILLISECONDS);
            String respType = Json.getString(resp, "type", "");
            if ("ERROR".equals(respType)) {
                throw new IOException("VM error [" + Json.getString(resp, "code", "?") + "]: "
                        + Json.getString(resp, "message", "?"));
            }
            return resp;
        } catch (TimeoutException te) {
            synchronized (pendingLock) { pendingRequests.remove(id); }
            throw new IOException("timeout esperando respuesta a '" + type + "' (id=" + id + ")");
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrupted", ie);
        } catch (java.util.concurrent.ExecutionException ee) {
            throw new IOException("fallo en future", ee.getCause());
        }
    }

    /** Locales del thread pausado: i32 entre bp y sp. Falla si no hay pausa. */
    public int[] getLocals(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("LOCALS", null, null, timeoutMs);
        List<Object> locals = Json.getList(resp, "locals");
        if (locals == null) return new int[0];
        int[] out = new int[locals.size()];
        for (int i = 0; i < out.length; i++) {
            out[i] = (int) ((Long) locals.get(i)).longValue();
        }
        return out;
    }

    /** Stack frames como pares [pc, bp]. */
    public List<int[]> getStackFrames(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("STACK", null, null, timeoutMs);
        List<Object> frames = Json.getList(resp, "frames");
        List<int[]> out = new ArrayList<>();
        if (frames == null) return out;
        for (Object o : frames) {
            if (!(o instanceof List)) continue;
            List<?> pair = (List<?>) o;
            if (pair.size() < 2) continue;
            int pc = (int) ((Long) pair.get(0)).longValue();
            int bp = (int) ((Long) pair.get(1)).longValue();
            out.add(new int[]{pc, bp});
        }
        return out;
    }

    /** Snapshot de properties públicas de todos los módulos. MODULE_PROPERTIES
     *  es extensión Java-only (no está en v1). */
    public List<ModuleManager.PropertyView> getModuleProperties(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("MODULE_PROPERTIES", null, null, timeoutMs);
        List<Object> props = Json.getList(resp, "props");
        List<ModuleManager.PropertyView> out = new ArrayList<>();
        if (props == null) return out;
        for (Object o : props) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new ModuleManager.PropertyView(
                    Json.getString(m, "module", ""),
                    Json.getString(m, "name", ""),
                    Json.getString(m, "type", ""),
                    (int) Json.getLong(m, "rawValue", 0),
                    Json.getString(m, "display", "")));
        }
        return out;
    }

    /** Lee un i32 de la memoria de la VM en una dirección absoluta. */
    public int readMemoryInt(int addr, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("READ_INT", "\"addr\":" + addr, null, timeoutMs);
        return (int) Json.getLong(resp, "value", 0);
    }

    /** Lee un string BP por ref; "" si no es un string válido. */
    public String readStringIfPossible(int ref, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("READ_STRING", "\"ref\":" + ref, null, timeoutMs);
        return Json.getString(resp, "value", "");
    }

    // ============================================================
    // Transferencia de ficheros (A2.2) — PR-2: bulk binario raw inline.
    // ============================================================

    /** Sube `bytes` al workdir de la VM en `remotePath`. Bulk raw inline. */
    public int uploadFile(String remotePath, byte[] bytes, long timeoutMs) throws IOException {
        String extra = "\"path\":" + jsonStr(remotePath);
        Map<String, Object> resp = sendRequest("PUT", extra, bytes, timeoutMs);
        return (int) Json.getLong(resp, "size", 0);
    }

    /** Sube un fichero local al workdir de la VM. */
    public int uploadFile(java.nio.file.Path localFile, String remotePath, long timeoutMs)
            throws IOException {
        byte[] data = java.nio.file.Files.readAllBytes(localFile);
        return uploadFile(remotePath, data, timeoutMs);
    }

    /** Descarga `remotePath` del workdir de la VM. Bulk raw inline. */
    public byte[] downloadFile(String remotePath, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("GET",
                "\"path\":" + jsonStr(remotePath), null, timeoutMs);
        Object b = resp.get(BULK_KEY);
        if (b instanceof byte[]) return (byte[]) b;
        // Fallback defensivo: si por algún motivo el server no envió bulk,
        // devolvemos el tamaño cero como antes (el caller suele inspeccionar
        // la longitud por su cuenta).
        return new byte[0];
    }

    /** Una entrada de un directorio devuelta por listFiles. */
    public static final class RemoteFile {
        public final String name;
        public final long size;
        public final boolean isDirectory;
        public RemoteFile(String n, long s, boolean d) {
            this.name = n; this.size = s; this.isDirectory = d;
        }
        @Override public String toString() {
            return (isDirectory ? "[D] " : "[F] ") + name
                    + (isDirectory ? "" : " (" + size + ")");
        }
    }

    /** Lista los ficheros bajo `remotePath` ("" o "." = raíz del workdir). */
    public List<RemoteFile> listFiles(String remotePath, long timeoutMs) throws IOException {
        String extra = "\"path\":" + jsonStr(remotePath == null ? "" : remotePath);
        Map<String, Object> resp = sendRequest("LIST", extra, null, timeoutMs);
        List<Object> arr = Json.getList(resp, "entries");
        List<RemoteFile> out = new ArrayList<>();
        if (arr == null) return out;
        for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new RemoteFile(
                    Json.getString(m, "name", ""),
                    Json.getLong(m, "size", 0),
                    Json.getBool(m, "isDir", false)));
        }
        return out;
    }

    /** Borra un fichero (no recursivo para dirs — el dir debe estar vacío). */
    public void deleteFile(String remotePath, long timeoutMs) throws IOException {
        sendRequest("DEL", "\"path\":" + jsonStr(remotePath), null, timeoutMs);
    }

    /** Crea un directorio (incluyendo intermedios) en el workdir. */
    public void mkdir(String remotePath, long timeoutMs) throws IOException {
        sendRequest("MKDIR", "\"path\":" + jsonStr(remotePath), null, timeoutMs);
    }

    /** Persiste el FS RAM a flash (operación Pico-only; VM Java responde
     *  OK silente porque ya escribe directo al workdir host). Devuelve
     *  milisegundos que tardó la operación en el peer, 0 si no reporta. */
    public long save(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("SAVE", null, null, timeoutMs);
        return Json.getLong(resp, "durationMs", 0);
    }

    /** Lee el log persistente del firmware como texto. VM Java responde
     *  con ERROR UNSUPPORTED — el caller debe manejarlo. */
    public String logDump(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("LOG_DUMP", null, null, timeoutMs);
        return Json.getString(resp, "text", "");
    }

    /** Reboot al bootloader BOOTSEL (Pico-only). Mismo patrón que reset():
     *  request → reply corta → la VM se va — cerramos el cliente. */
    public void bootsel() throws IOException {
        try {
            sendRequest("BOOTSEL", null, null, 2000);
        } catch (IOException ioe) {
            diag("[BpvmClient] BOOTSEL sin reply síncrona: " + ioe.getMessage());
        } finally {
            close();
        }
    }

    /** Sincroniza el RTC del peer con un epoch en segundos (típicamente
     *  System.currentTimeMillis()/1000). El firmware Pico lo usa para
     *  Rtc.Clock; la VM Java ignora el comando (devuelve UNSUPPORTED). */
    public void syncTime(long epochSec, long timeoutMs) throws IOException {
        sendRequest("TIME", "\"epochSec\":" + epochSec, null, timeoutMs);
    }

    /** Pide INFO al peer. Devuelve el Map completo del reply para que el
     *  caller extraiga los campos relevantes (uniqueId, boardName, freq,
     *  uptimeMs, tempC, fsTotalBytes, fsUsedBytes, ...). */
    public Map<String, Object> getInfo(long timeoutMs) throws IOException {
        return sendRequest("INFO", null, null, timeoutMs);
    }

    /** Helper para serializar un string con comillas + escape. */
    private static String jsonStr(String s) { return "\"" + Json.escape(s) + "\""; }

    // ============================================================
    // Internos
    // ============================================================

    private static int reserveFreePort() throws IOException {
        try (ServerSocket s = new ServerSocket(0)) {
            return s.getLocalPort();
        }
    }

    private void spawnVmProcess(int port, String modOrProject, String workdir, boolean waitClient,
                                String stdlibDir) throws IOException {
        String javaBin = System.getProperty("java.home") + File.separator + "bin"
                + File.separator + "java";
        if (System.getProperty("os.name", "").toLowerCase().contains("win")) {
            javaBin += ".exe";
        }

        String bpgenvmCp = locateBpgenvmJar();
        String runtimeCp = System.getProperty("java.class.path", "");
        String classpath;
        if (bpgenvmCp == null) {
            classpath = runtimeCp;
        } else if (runtimeCp.isEmpty()) {
            classpath = bpgenvmCp;
        } else if (runtimeCp.contains(bpgenvmCp)) {
            classpath = runtimeCp;
        } else {
            classpath = runtimeCp + File.pathSeparator + bpgenvmCp;
        }

        List<String> argv = new ArrayList<>();
        argv.add(javaBin);
        argv.add("-cp");
        argv.add(classpath);
        argv.add("edu.bpgenvm.Main");
        argv.add("--listen");
        argv.add(Integer.toString(port));
        if (waitClient) argv.add("--wait-client");
        if (workdir != null) {
            argv.add("--workdir");
            argv.add(workdir);
        }
        if (stdlibDir != null && !stdlibDir.isEmpty()) {
            argv.add("--stdlibDir");
            argv.add(stdlibDir);
        }
        if (modOrProject != null) argv.add(modOrProject);

        ProcessBuilder pb = new ProcessBuilder(argv);
        pb.redirectErrorStream(false);
        this.process = pb.start();
        startStderrPump();
        diag("[BpvmClient] subproceso lanzado en puerto " + port);
    }

    private static String locateBpgenvmJar() {
        try {
            java.security.CodeSource cs = edu.bpgenvm.Main.class
                    .getProtectionDomain().getCodeSource();
            if (cs == null) return null;
            java.net.URL loc = cs.getLocation();
            if (loc == null) return null;
            return new java.io.File(loc.toURI()).getAbsolutePath();
        } catch (Throwable t) {
            return null;
        }
    }

    private void startStderrPump() {
        stderrPumpThread = new Thread(() -> {
            try (BufferedReader br = new BufferedReader(
                    new InputStreamReader(process.getErrorStream(), StandardCharsets.UTF_8))) {
                String ln;
                while (!closed && (ln = br.readLine()) != null) {
                    diag(ln);
                }
            } catch (IOException ignored) { /* proceso ya murió */ }
        }, "bp-vmclient-stderr");
        stderrPumpThread.setDaemon(true);
        stderrPumpThread.start();
    }

    private void connectWithRetry(int port, int totalTimeoutMs) throws IOException {
        long deadline = System.currentTimeMillis() + totalTimeoutMs;
        IOException last = null;
        while (System.currentTimeMillis() < deadline) {
            try {
                this.socket = new Socket("localhost", port);
                this.outRaw = new BufferedOutputStream(socket.getOutputStream());
                this.inRaw  = new BufferedInputStream(socket.getInputStream());
                return;
            } catch (IOException e) {
                last = e;
                try { Thread.sleep(50); } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    throw new IOException("interrupted while connecting", e);
                }
                if (process != null && !process.isAlive()) {
                    throw new IOException("la VM murió antes de aceptar la conexión", e);
                }
            }
        }
        throw new IOException("no se pudo conectar al server en " + totalTimeoutMs + "ms", last);
    }

    private void startReaderThread() {
        readerThread = new Thread(this::readLoop, "bp-vmclient-reader");
        readerThread.setDaemon(true);
        readerThread.start();
    }

    private void readLoop() {
        try {
            while (!closed) {
                String line = WireFraming.recvLine(inRaw);
                if (line == null) break;
                if (line.isEmpty()) continue;
                Map<String, Object> m;
                try {
                    m = Json.parseFlatObject(line);
                } catch (Throwable t) {
                    diag("[BpvmClient] línea no parseable: " + line);
                    continue;
                }
                // Si el frame trae bulk, leerlo del wire antes de procesar.
                long bulkSize = Json.getLong(m, "bulk", 0);
                if (bulkSize > 0) {
                    if (bulkSize > Integer.MAX_VALUE) {
                        diag("[BpvmClient] bulk size fuera de rango: " + bulkSize);
                        break;
                    }
                    try {
                        byte[] data = WireFraming.recvBulk(inRaw, (int) bulkSize);
                        m.put(BULK_KEY, data);
                    } catch (IOException ioe) {
                        diag("[BpvmClient] error leyendo bulk: " + ioe.getMessage());
                        break;
                    }
                }
                handleMessage(m);
            }
        } catch (IOException e) {
            if (!closed) diag("[BpvmClient] readLoop terminó: " + e.getMessage());
        }
    }

    private void handleMessage(Map<String, Object> m) {
        String type = Json.getString(m, "type", "");
        // PR-3: refrescar currentSession si el mensaje trae uno. Aplica a
        // replies (RUN_REPLY) y a eventos (BP_HIT/STEP_DONE/RESUMED/...).
        long s = Json.getLong(m, "session", 0);
        if (s > 0) this.currentSession = s;

        // v1: si trae `id` es una reply (o ERROR a un request específico).
        Object rawId = m.get("id");
        if (rawId instanceof Long) {
            long reqId = (Long) rawId;
            CompletableFuture<Map<String, Object>> fut;
            synchronized (pendingLock) { fut = pendingRequests.remove(reqId); }
            if (fut != null) {
                fut.complete(m);
            } else {
                // Reply sin request pendiente: probablemente respuesta a un
                // sendOneShot (fire-and-forget). Silencioso por diseño.
            }
            if ("HELLO_REPLY".equals(type)) {
                helloLatch.countDown();
            }
            return;
        }
        // PR-6: FATAL — el server reporta un error de protocolo y cerrará
        // la conexión. Completamos todos los pending requests con error
        // claro para que sus callers no se queden colgados.
        if ("FATAL".equals(type)) {
            String code = Json.getString(m, "code", "PROTOCOL_ERROR");
            String msg  = Json.getString(m, "message", "FATAL recibido");
            diag("[BpvmClient] FATAL [" + code + "]: " + msg);
            IOException ex = new IOException("VM FATAL [" + code + "]: " + msg);
            java.util.List<CompletableFuture<Map<String,Object>>> toFail;
            synchronized (pendingLock) {
                toFail = new ArrayList<>(pendingRequests.values());
                pendingRequests.clear();
            }
            for (CompletableFuture<Map<String,Object>> f : toFail) {
                f.completeExceptionally(ex);
            }
            return;
        }

        // Sin id → es un evento asíncrono.
        switch (type) {
            case "OUTPUT": {
                String data = Json.getString(m, "data", "");
                Consumer<String> sink = outputSink;
                if (sink != null) sink.accept(data);
                break;
            }
            case "PROMPT_REQUEST": {
                long promptId = Json.getLong(m, "promptId", -1);
                String spec = Json.getString(m, "spec", "");
                PromptHandler h = promptHandler;
                if (h != null) {
                    listenerExec.execute(() -> {
                        try { h.onPrompt(promptId, spec); }
                        catch (Throwable t) {
                            diag("[BpvmClient] promptHandler: " + t.getMessage());
                            respondToPrompt(promptId, "{}");
                        }
                    });
                } else {
                    diag("[BpvmClient] PROMPT_REQUEST sin handler; respondiendo vacío");
                    respondToPrompt(promptId, "{}");
                }
                break;
            }
            case "BP_HIT":
            case "STEP_DONE": {
                // PR-5: leer prioritariamente del campo `frame:{...}` v1.
                // Si está, file/line vienen de ahí. Los demás campos
                // (tid/absPc/bp/sp/cs/stackBase) son extensión Java-only
                // — no están en v1 puro y se leen del nivel superior por
                // back-compat con server PR-1..PR-4. Cuando el server
                // deje de emitirlos quedarán a 0; el debugger Java los
                // sigue usando para queries de estado del VM.
                Map<String, Object> frame = Json.getMap(m, "frame");
                String file;
                int    line;
                if (frame != null) {
                    file = Json.getString(frame, "file", null);
                    line = (int) Json.getLong(frame, "line", -1);
                } else {
                    // Fallback robusto a campos planos (server pre-PR-5).
                    file = Json.getString(m, "file", null);
                    line = (int) Json.getLong(m, "line", -1);
                }
                PausedEvent e = new PausedEvent(
                        (int) Json.getLong(m, "tid", 0),
                        (int) Json.getLong(m, "absPc", 0),
                        line,
                        file,
                        (int) Json.getLong(m, "bp", 0),
                        (int) Json.getLong(m, "sp", 0),
                        (int) Json.getLong(m, "cs", 0),
                        (int) Json.getLong(m, "stackBase", 0));
                fire(e);
                break;
            }
            case "RESUMED": {
                fire(new ResumedEvent((int) Json.getLong(m, "tid", 0)));
                break;
            }
            case "EXITED": {
                // El firmware Pico envía {status, errorMessage} (wire v1
                // canon); la VM Java envía {reason} (extensión histórica).
                // Aceptamos ambos — preferencia: errorMessage → reason →
                // status. Si solo hay status:"OK", lo usamos como reason
                // para que la UI muestre "exit 0 (OK)" en lugar de "exit 0 ()".
                String reason = Json.getString(m, "errorMessage", null);
                if (reason == null || reason.isEmpty()) {
                    reason = Json.getString(m, "reason", null);
                }
                if (reason == null || reason.isEmpty()) {
                    reason = Json.getString(m, "status", "");
                }
                fire(new ExitedEvent(
                        (int) Json.getLong(m, "exitCode", 0),
                        reason));
                // Tras EXITED no hay sesión activa.
                this.currentSession = 0;
                break;
            }
            case "EXCEPTION": {
                fire(new ExceptionEvent(
                        (int) Json.getLong(m, "tid", 0),
                        Json.getString(m, "message", ""),
                        Json.getString(m, "stackTrace", "")));
                break;
            }
            default:
                diag("[BpvmClient] tipo desconocido: " + type);
        }
    }

    private void fire(edu.bpgenvm.vm.debug.DebugEvent ev) {
        DebugListener l = eventListener;
        if (l == null) return;
        // Dispatchear FUERA del reader thread para evitar deadlock cuando
        // el listener hace queries síncronas vía sendRequest.
        listenerExec.execute(() -> {
            try { l.onEvent(ev); }
            catch (Throwable t) { diag("[BpvmClient] listener: " + t.getMessage()); }
        });
    }

    private void diag(String line) {
        Consumer<String> s = diagSink;
        if (s != null) s.accept(line);
    }

    /** Bloquea hasta que el subproceso termina O timeout. */
    public boolean awaitTermination(long timeoutMs) throws InterruptedException {
        if (process == null) return true;
        return process.waitFor(timeoutMs, TimeUnit.MILLISECONDS);
    }

    /** Bloquea sin timeout hasta que el subproceso muere. */
    public int waitForExit() throws InterruptedException {
        if (process == null) return 0;
        return process.waitFor();
    }

    // Convenience getter para tests.
    public boolean isHandshakeDone() { return helloLatch.getCount() == 0; }
}
