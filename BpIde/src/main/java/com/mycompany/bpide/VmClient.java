// ============================================================
// VmClient.java
// Cliente del IDE para hablar con la VM cuando ésta corre como
// PROCESO SEPARADO (A1.5).
//
// Responsabilidades:
//   1) Lanzar bpgenvm como subproceso con `--listen <port>
//      --wait-client <fichero.mod>` (o `<proyecto.bpproject>`).
//   2) Conectar por TCP a localhost:<port>.
//   3) Leer líneas JSON entrantes y traducirlas a:
//        - "print"     → outputSink (consola del IDE)
//        - "paused"    → DebugListener (PausedEvent)
//        - "resumed"   → DebugListener (ResumedEvent)
//        - "exited"    → DebugListener (ExitedEvent) + close()
//        - "exception" → DebugListener (ExceptionEvent)
//        - "hello"     → log diagnóstico
//   4) Enviar comandos (continue/step/setBreakpoint/stop) como
//      líneas JSON al socket.
//
// Strategy de localización del binario:
//   - Lanzamos `java -cp <classpath actual> edu.bpgenvm.Main`. Eso
//     funciona desde Maven, IntelliJ y desde un jar empaquetado, sin
//     necesidad de buscar el jar de bpgenvm.
//   - El java binario es el mismo que ejecuta el IDE
//     (`java.home`/bin/java).
//
// Strategy del puerto:
//   - Reservamos un puerto libre con ServerSocket(0), lo cerramos
//     inmediatamente, lo pasamos a la VM. Pequeña race con otros
//     procesos que pudieran agarrar el puerto; aceptable para
//     localhost.
//
// Lifecycle:
//   - start(modPath) bloquea hasta tener handshake (hello del server)
//     o timeout. Tras start() el lector y los comandos están listos.
//   - close() corta la conexión, manda SIGTERM al subproceso y espera.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.ExceptionEvent;
import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
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

public final class VmClient implements AutoCloseable {

    /** N20 — Listener para promptRequest del servidor. El IDE registra un
     *  callback que muestra el formulario y devuelve los valores como JSON.
     *  La VM espera bloqueada hasta que `respondToPrompt(requestId, json)`
     *  sea llamado. */
    @FunctionalInterface
    public interface PromptHandler {
        void onPrompt(long requestId, String spec);
    }
    private volatile PromptHandler promptHandler;
    public void setPromptHandler(PromptHandler h) { this.promptHandler = h; }

    /** Envía la respuesta del usuario para un prompt en vuelo (N20). */
    public void respondToPrompt(long requestId, String valuesJson) {
        sendRaw("{\"cmd\":\"promptResponse\",\"requestId\":" + requestId
                + ",\"values\":" + jsonStr(valuesJson == null ? "" : valuesJson) + "}");
    }

    /** Sink para los chunks "print" del programa BP. Lo invoca el thread
     *  lector — los handlers deben reenviar al EDT con invokeLater. */
    private volatile Consumer<String> outputSink;

    /** Listener para eventos paused/resumed/exited/exception. Idem hilo. */
    private volatile DebugListener eventListener;

    /** Sink para "logs" del subproceso (stderr) y mensajes diagnósticos
     *  del propio cliente. Si está a null se descarta. */
    private volatile Consumer<String> diagSink;

    private Process process;
    private Socket socket;
    private PrintWriter out;
    private BufferedReader in;

    private Thread readerThread;
    private Thread stderrPumpThread;
    private final CountDownLatch helloLatch = new CountDownLatch(1);

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

    // ---- Request/response (A1.6) ----
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
     * por socket. Devuelve cuando el hello banner del server ha llegado, o
     * lanza IOException si timeout/error.
     *
     * @param modOrProjectPath  ruta absoluta a un .mod o .bpproject.
     * @param waitClient        si true se pasa --wait-client (la VM bloquea
     *                          hasta que conectemos; recomendado).
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

    /** A2.6 — Conecta a una VM REMOTA ya corriendo en {@code host:port}.
     *  No lanza subproceso; asume que el daemon vive en el otro extremo y
     *  está aceptando conexiones. El IDE sube ficheros y manda runModule
     *  via wire — mismo flujo que startDaemon local, salto el spawn.
     *  Útil para apuntar al dispositivo objetivo. */
    public void connectRemote(String host, int port) throws IOException {
        try {
            this.socket = new java.net.Socket(host, port);
            this.out = new PrintWriter(
                    new java.io.OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8),
                    false);
            this.in = new BufferedReader(
                    new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
        } catch (IOException e) {
            throw new IOException("no se pudo conectar a la VM remota " + host + ":" + port
                    + " — ¿está corriendo `bpgenvm --listen " + port + " --workdir ...` allí?", e);
        }
        startReaderThread();
        try {
            if (!helloLatch.await(5, TimeUnit.SECONDS)) {
                diag("[VmClient] timeout esperando hello del server remoto");
            }
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
        }
    }

    /** Variante con stdlibDir explícito: se pasa como --stdlibDir al
     *  subproceso VM y gana sobre cualquier BpVM.cfg autodiscovery. Útil
     *  cuando el IDE conoce el cfg pero el subproceso se arranca desde un
     *  cwd que no contiene BpVM.cfg. */
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
        // Esperamos al hello banner para considerar la sesión "lista".
        try {
            if (!helloLatch.await(5, TimeUnit.SECONDS)) {
                diag("[VmClient] timeout esperando hello del server");
            }
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
        }
    }

    /** Cierra el socket, mata el subproceso, junta los threads. */
    @Override public void close() {
        closed = true;
        try { if (out != null) out.flush(); } catch (Throwable ignored) {}
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
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

    public synchronized void sendRaw(String jsonLine) {
        PrintWriter w = this.out;
        if (w == null) return;
        w.print(jsonLine);
        w.print('\n');
        w.flush();
    }

    public void setBreakpoint(String file, int line, boolean enabled) {
        sendRaw("{\"cmd\":\"setBreakpoint\",\"file\":\"" + Json.escape(file)
                + "\",\"line\":" + line
                + ",\"enabled\":" + (enabled ? "true" : "false") + "}");
    }

    public void clearAllBreakpoints() {
        sendRaw("{\"cmd\":\"clearAllBreakpoints\"}");
    }

    /** A2.3 — Ordena al daemon arrancar el módulo `module` (path relativo
     *  al workdir). Sólo válido si la VM se lanzó en modo daemon (sin
     *  fichero en CLI). Fire-and-forget: el ExitedEvent es la confirmación
     *  del fin de ejecución. */
    public void runModule(String module) {
        sendRaw("{\"cmd\":\"runModule\",\"module\":\"" + Json.escape(module) + "\"}");
    }

    public void sendCommand(StepCommand cmd) {
        switch (cmd) {
            case CONTINUE:  sendRaw("{\"cmd\":\"continue\"}"); break;
            case STEP_INTO: sendRaw("{\"cmd\":\"stepInto\"}"); break;
            case STEP_OVER: sendRaw("{\"cmd\":\"stepOver\"}"); break;
            case STEP_OUT:  sendRaw("{\"cmd\":\"stepOut\"}"); break;
            case STOP:      sendRaw("{\"cmd\":\"stop\"}"); break;
        }
    }

    // ============================================================
    // Request/response — queries síncronas al VM (A1.6)
    // ============================================================

    /** Envía un request y devuelve el mapa de respuesta (incluyendo el campo
     *  "resp" para discriminar). Lanza si timeout o si la respuesta es "error". */
    private Map<String, Object> sendRequest(String req, String extraJson, long timeoutMs)
            throws IOException {
        long id = nextRequestId.getAndIncrement();
        CompletableFuture<Map<String, Object>> future = new CompletableFuture<>();
        synchronized (pendingLock) { pendingRequests.put(id, future); }
        StringBuilder sb = new StringBuilder();
        sb.append("{\"req\":\"").append(req).append("\",\"requestId\":").append(id);
        if (extraJson != null && !extraJson.isEmpty()) {
            sb.append(',').append(extraJson);   // sin envoltorio { } — caller lo formatea
        }
        sb.append('}');
        sendRaw(sb.toString());
        try {
            Map<String, Object> resp = future.get(timeoutMs, TimeUnit.MILLISECONDS);
            String type = Json.getString(resp, "resp", "");
            if ("error".equals(type)) {
                throw new IOException("VM error: " + Json.getString(resp, "message", "?"));
            }
            return resp;
        } catch (TimeoutException te) {
            synchronized (pendingLock) { pendingRequests.remove(id); }
            throw new IOException("timeout esperando respuesta a '" + req + "' (id=" + id + ")");
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrupted", ie);
        } catch (java.util.concurrent.ExecutionException ee) {
            throw new IOException("fallo en future", ee.getCause());
        }
    }

    /** Locales del thread pausado: i32 entre bp y sp. Falla si no hay pausa. */
    public int[] getLocals(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("getLocals", null, timeoutMs);
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
        Map<String, Object> resp = sendRequest("stackFrames", null, timeoutMs);
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

    /** Snapshot de properties públicas de todos los módulos. Reconstruye
     *  PropertyView para que el código del IDE no sepa que viene por wire. */
    public List<ModuleManager.PropertyView> getModuleProperties(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("moduleProperties", null, timeoutMs);
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
        Map<String, Object> resp = sendRequest("readInt", "\"addr\":" + addr, timeoutMs);
        return (int) Json.getLong(resp, "value", 0);
    }

    /** Lee un string BP por ref; "" si no es un string válido. */
    public String readStringIfPossible(int ref, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("readString", "\"ref\":" + ref, timeoutMs);
        return Json.getString(resp, "value", "");
    }

    // ============================================================
    // Transferencia de ficheros (A2.2) — paths relativos al workdir
    // ============================================================

    /** Sube `bytes` al workdir de la VM en `remotePath`. Devuelve el
     *  tamaño confirmado por el server. */
    public int uploadFile(String remotePath, byte[] bytes, long timeoutMs) throws IOException {
        String b64 = java.util.Base64.getEncoder().encodeToString(bytes);
        String extra = "\"path\":" + jsonStr(remotePath) + ",\"data\":\"" + b64 + "\"";
        Map<String, Object> resp = sendRequest("uploadFile", extra, timeoutMs);
        return (int) Json.getLong(resp, "size", 0);
    }

    /** Sube un fichero local al workdir de la VM. */
    public int uploadFile(java.nio.file.Path localFile, String remotePath, long timeoutMs)
            throws IOException {
        byte[] data = java.nio.file.Files.readAllBytes(localFile);
        return uploadFile(remotePath, data, timeoutMs);
    }

    /** Descarga `remotePath` del workdir de la VM. */
    public byte[] downloadFile(String remotePath, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("downloadFile",
                "\"path\":" + jsonStr(remotePath), timeoutMs);
        String b64 = Json.getString(resp, "data", "");
        return java.util.Base64.getDecoder().decode(b64);
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
        Map<String, Object> resp = sendRequest("listFiles", extra, timeoutMs);
        List<Object> arr = Json.getList(resp, "files");
        List<RemoteFile> out = new ArrayList<>();
        if (arr == null) return out;
        for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new RemoteFile(
                    Json.getString(m, "name", ""),
                    Json.getLong(m, "size", 0),
                    Json.getBool(m, "isDirectory", false)));
        }
        return out;
    }

    /** Borra un fichero (no recursivo para dirs — el dir debe estar vacío). */
    public void deleteFile(String remotePath, long timeoutMs) throws IOException {
        sendRequest("deleteFile", "\"path\":" + jsonStr(remotePath), timeoutMs);
    }

    /** Crea un directorio (incluyendo intermedios) en el workdir. */
    public void mkdir(String remotePath, long timeoutMs) throws IOException {
        sendRequest("mkdir", "\"path\":" + jsonStr(remotePath), timeoutMs);
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

        // Localizamos el classpath para el subproceso. Estrategia:
        //   1) Resolver el origen físico de edu.bpgenvm.Main (su jar o
        //      directorio de clases). Es lo único que el subproceso
        //      necesita para ejecutarse como VM.
        //   2) Concatenarlo con `java.class.path` actual. En producción
        //      (jar empaquetado) ambos suelen coincidir; en mvn exec:java
        //      `java.class.path` puede no incluir el jar de bpgenvm, así
        //      que el paso 1 lo añade y todo va bien.
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
        // stderr en pipe separado para bombearlo al diagSink.
        pb.redirectErrorStream(false);
        this.process = pb.start();
        startStderrPump();
        // (Process.pid() requiere Java 9+; mantenemos el log sin PID por compat 1.8.)
        diag("[VmClient] subproceso lanzado en puerto " + port);
    }

    /** Devuelve la ruta del jar (o dir de clases) que aloja edu.bpgenvm.Main,
     *  o null si no se puede determinar. */
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
                this.out = new PrintWriter(
                        new java.io.OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8),
                        false);
                this.in = new BufferedReader(
                        new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
                return;
            } catch (IOException e) {
                last = e;
                // El subproceso quizás aún no ha bindeado; pausa breve y reintenta.
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
            String line;
            while (!closed && (line = in.readLine()) != null) {
                handleLine(line);
            }
        } catch (IOException e) {
            if (!closed) diag("[VmClient] readLoop terminó: " + e.getMessage());
        }
    }

    private void handleLine(String line) {
        Map<String, Object> m;
        try {
            m = Json.parseFlatObject(line);
        } catch (Throwable t) {
            diag("[VmClient] línea no parseable: " + line);
            return;
        }
        // Respuesta a un request previo: completar el future correspondiente.
        if (m.get("resp") instanceof String) {
            long reqId = Json.getLong(m, "requestId", -1);
            CompletableFuture<Map<String, Object>> fut;
            synchronized (pendingLock) { fut = pendingRequests.remove(reqId); }
            if (fut != null) fut.complete(m);
            else diag("[VmClient] resp sin request pendiente, requestId=" + reqId);
            return;
        }
        String type = Json.getString(m, "type", "");
        switch (type) {
            case "hello": {
                diag("[VmClient] handshake con VM: " + Json.getString(m, "vm", "?"));
                helloLatch.countDown();
                break;
            }
            case "print": {
                String data = Json.getString(m, "data", "");
                Consumer<String> sink = outputSink;
                if (sink != null) sink.accept(data);
                break;
            }
            case "promptRequest": {
                // N20 — el programa BP llamó a IO.prompt(spec).
                long reqId = Json.getLong(m, "requestId", -1);
                String spec = Json.getString(m, "spec", "");
                PromptHandler h = promptHandler;
                if (h != null) {
                    // Mismo dispatcher pattern que los DebugEvents — listenerExec
                    // serializa para no bloquear el reader thread mientras la UI
                    // construye el form.
                    listenerExec.execute(() -> {
                        try { h.onPrompt(reqId, spec); }
                        catch (Throwable t) {
                            diag("[VmClient] promptHandler: " + t.getMessage());
                            // Fallback: responder con JSON vacío para no dejar la VM colgada.
                            respondToPrompt(reqId, "{}");
                        }
                    });
                } else {
                    // No hay handler: responder vacío para que la VM no se quede colgada.
                    diag("[VmClient] promptRequest sin handler; respondiendo vacío");
                    respondToPrompt(reqId, "{}");
                }
                break;
            }
            case "paused": {
                PausedEvent e = new PausedEvent(
                        (int) Json.getLong(m, "tid", 0),
                        (int) Json.getLong(m, "absPc", 0),
                        (int) Json.getLong(m, "line", -1),
                        Json.getString(m, "file", null),
                        (int) Json.getLong(m, "bp", 0),
                        (int) Json.getLong(m, "sp", 0),
                        (int) Json.getLong(m, "cs", 0),
                        (int) Json.getLong(m, "stackBase", 0));
                fire(e);
                break;
            }
            case "resumed": {
                fire(new ResumedEvent((int) Json.getLong(m, "tid", 0)));
                break;
            }
            case "exited": {
                fire(new ExitedEvent(
                        (int) Json.getLong(m, "exitCode", 0),
                        Json.getString(m, "reason", "")));
                break;
            }
            case "exception": {
                fire(new ExceptionEvent(
                        (int) Json.getLong(m, "tid", 0),
                        Json.getString(m, "message", ""),
                        Json.getString(m, "stackTrace", "")));
                break;
            }
            default:
                diag("[VmClient] tipo desconocido: " + type);
        }
    }

    private void fire(edu.bpgenvm.vm.debug.DebugEvent ev) {
        DebugListener l = eventListener;
        if (l == null) return;
        // Dispatchear FUERA del reader thread para evitar deadlock cuando
        // el listener hace queries síncronas vía sendRequest.
        listenerExec.execute(() -> {
            try { l.onEvent(ev); }
            catch (Throwable t) { diag("[VmClient] listener: " + t.getMessage()); }
        });
    }

    private void diag(String line) {
        Consumer<String> s = diagSink;
        if (s != null) s.accept(line);
    }

    /** Bloquea hasta que el subproceso termina O timeout. Devuelve true
     *  si terminó, false si expiró. Útil para que el IDE espere "fin de
     *  ejecución" antes de cerrar el VmClient. */
    public boolean awaitTermination(long timeoutMs) throws InterruptedException {
        if (process == null) return true;
        return process.waitFor(timeoutMs, TimeUnit.MILLISECONDS);
    }

    /** Bloquea sin timeout hasta que el subproceso muere. Pensado para
     *  el "Run" del IDE — el SwingWorker bloquea en este hilo de fondo. */
    public int waitForExit() throws InterruptedException {
        if (process == null) return 0;
        return process.waitFor();
    }

    // Convenience getter para tests.
    public boolean isHandshakeDone() { return helloLatch.getCount() == 0; }
}
