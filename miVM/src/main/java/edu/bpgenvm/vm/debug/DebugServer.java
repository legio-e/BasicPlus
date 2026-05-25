// ============================================================
// DebugServer.java
// Servidor TCP del lado VM. Escucha en un puerto, acepta UN cliente,
// y mantiene dos canales sobre el mismo socket:
//
//   - Saliente (VM → cliente): los eventos del DebugController y los
//     chunks OUTPUT del SocketSink. Cada mensaje es una línea JSON.
//
//   - Entrante (cliente → VM): líneas JSON con `{"type":"X","id":N,...}`.
//     Se parsean y se traducen a invocaciones del DebugController. Cada
//     request recibe una reply síncrona con el mismo id.
//
// Vida del servidor:
//   1) start(port) abre el ServerSocket y arranca un thread de accept.
//   2) Al primer cliente, conecta sus streams al controller + sink y
//      arranca el thread de lectura de comandos.
//   3) Si el cliente se desconecta, el VM sigue corriendo (modo
//      headless). NO reaceptamos otro cliente todavía — esto se
//      añadirá si se necesita reattach en caliente.
//
// La VM puede empezar a ejecutar ANTES de que llegue el cliente
// (modo "fire and forget") o BLOQUEAR esperándolo (modo "debugger
// attach"). Lo controla `waitForClient`.
//
// Protocolo wire — v1 (docs/BPVM_WIRE_PROTOCOL.md).
// PR-1: framing base + nombres v1. Deviations conscientes (cerradas
// en PRs siguientes):
//   - SET_BP_REPLY devuelve bpId=0 placeholder (PR-5: real bpId).
//   - BP_HIT/RESUMED conservan campos planos (PR-5: nested frame).
//   - PUT/GET aún transportan los bytes como base64 en `data`
//     (PR-2: bulk binario raw inline).
//   - No hay sessionId todavía (PR-3).
//   - No hay INFO/PING/RESET/STAT/RMDIR/RENAME/FORMAT/DF (PR-4),
//     ni PAUSE/LIST_BP/EVAL/INSPECT (PR-5).
//   - Códigos de error v1 sin uso aún — todo va con
//     code="INTERNAL_ERROR" (PR-6).
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public final class DebugServer implements AutoCloseable {

    public static final int    PROTO_VERSION = 1;
    public static final String SERVER_NAME   = "bpvm-java";
    public static final String SERVER_BUILD  = "1.0";

    /** Capacidades v1 que declaramos en HELLO_REPLY. Por ahora sólo las
     *  familias que el código realmente cubre tras los PRs en vuelo. */
    private static final String[] CAPABILITIES = {
            "META", "FILES", "TERMINAL", "DEBUG"
    };

    private final VirtualMachine vm;
    private final DebugController controller;

    private ServerSocket serverSocket;
    private Socket client;
    private PrintWriter clientOut;
    private BufferedReader clientIn;
    private Thread readerThread;

    /** Latch para esperar el primer cliente cuando se arranca en modo
     *  "debugger attach". countDown al hacer accept. */
    private final CountDownLatch clientConnected = new CountDownLatch(1);

    /** A2.3 — Latch + slot para que el modo daemon de Main espere un
     *  RUN request y reciba el nombre del módulo a ejecutar. */
    private final java.util.concurrent.CompletableFuture<String> runModuleRequest =
            new java.util.concurrent.CompletableFuture<>();

    private volatile boolean closed = false;

    public DebugServer(VirtualMachine vm, DebugController controller) {
        this.vm = vm;
        this.controller = controller;
    }

    /**
     * Abre el puerto y arranca el thread de accept. NO bloquea —
     * para esperar al cliente, llamar {@link #awaitClient(long, TimeUnit)}.
     */
    public void start(int port) throws IOException {
        serverSocket = new ServerSocket(port);
        System.err.println("[DebugServer] escuchando en localhost:" + port);
        Thread acceptT = new Thread(this::acceptLoop, "bp-debug-accept");
        acceptT.setDaemon(true);
        acceptT.start();
    }

    /** Espera a que se conecte un cliente. Devuelve true si llegó dentro
     *  del timeout, false si expiró. */
    public boolean awaitClient(long timeout, TimeUnit unit) throws InterruptedException {
        return clientConnected.await(timeout, unit);
    }

    /** ¿Hay cliente conectado ahora mismo? */
    public boolean hasClient() {
        return client != null && !client.isClosed();
    }

    // ---- Accept ----

    private void acceptLoop() {
        try {
            Socket s = serverSocket.accept();   // bloquea hasta el primer cliente
            if (closed) { try { s.close(); } catch (IOException ignored) {} return; }
            this.client = s;
            this.clientOut = new PrintWriter(
                    new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                    false);
            this.clientIn = new BufferedReader(
                    new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
            System.err.println("[DebugServer] cliente conectado desde " + s.getRemoteSocketAddress());

            // Conectar sink + listener + promptSender.
            SocketSink sink = new SocketSink(clientOut);
            vm.setProgramOut(sink);
            controller.addListener(this::onEvent);
            vm.setPromptSender(this.promptSender);

            // v1: NO mandamos banner. El cliente debe enviar HELLO y
            // recibir HELLO_REPLY como primer mensaje (§5.1 del wire spec).

            // Arrancar el thread de lectura de comandos.
            readerThread = new Thread(this::readLoop, "bp-debug-reader");
            readerThread.setDaemon(true);
            readerThread.start();

            clientConnected.countDown();
        } catch (IOException e) {
            if (!closed)
                System.err.println("[DebugServer] accept falló: " + e.getMessage());
        }
    }

    // ---- Eventos VM → cliente ----

    private void onEvent(DebugEvent ev) {
        if (!hasClient()) return;
        try {
            if (ev instanceof PausedEvent) {
                PausedEvent e = (PausedEvent) ev;
                // PR-1: BP_HIT con campos planos + bpId=0 placeholder. PR-5 introducirá
                // el objeto `frame` anidado y el bpId real.
                send("{\"type\":\"BP_HIT\",\"bpId\":0,\"tid\":" + e.tid
                        + ",\"absPc\":" + e.absPc
                        + ",\"line\":" + e.line
                        + ",\"file\":\"" + Json.escape(e.sourceFile == null ? "" : e.sourceFile) + "\""
                        + ",\"bp\":" + e.bp + ",\"sp\":" + e.sp + ",\"cs\":" + e.cs
                        + ",\"stackBase\":" + e.stackBase + "}");
            } else if (ev instanceof ResumedEvent) {
                // RESUMED es extensión Java (no en v1). Útil para la UI:
                // saber cuándo limpiar el highlight del breakpoint.
                ResumedEvent e = (ResumedEvent) ev;
                send("{\"type\":\"RESUMED\",\"tid\":" + e.tid + "}");
            } else if (ev instanceof ExitedEvent) {
                ExitedEvent e = (ExitedEvent) ev;
                // PR-1: `reason` se conserva como campo informativo. v1 puro
                // tiene status/exitCode/elapsedMs/errorMessage — se completa
                // en PR-3 cuando entren sessions.
                send("{\"type\":\"EXITED\",\"exitCode\":" + e.exitCode
                        + ",\"reason\":\"" + Json.escape(e.reason) + "\"}");
            } else if (ev instanceof ExceptionEvent) {
                // EXCEPTION es extensión Java (en v1 esto va dentro de EXITED
                // con status=RUNTIME_ERROR). Se consolidará en PR-3.
                ExceptionEvent e = (ExceptionEvent) ev;
                send("{\"type\":\"EXCEPTION\",\"tid\":" + e.tid
                        + ",\"message\":\"" + Json.escape(e.message) + "\""
                        + ",\"stackTrace\":\"" + Json.escape(e.stackTrace) + "\"}");
            } else {
                // Tipo desconocido: lo envolvemos para no perderlo si añadimos
                // eventos nuevos sin tocar este servidor.
                send("{\"type\":\"" + Json.escape(ev.type()) + "\"}");
            }
        } catch (Throwable t) {
            System.err.println("[DebugServer] error emitiendo evento: " + t.getMessage());
        }
    }

    /** Visible para tests: invoca el mismo camino que toma un evento que
     *  llega por el listener. Usar sólo desde test code. */
    public void onEventForTest(DebugEvent ev) { onEvent(ev); }

    /** N20 — implementación de PromptSender que serializa el PROMPT_REQUEST
     *  al cliente conectado. Si no hay cliente, NO se debe llegar aquí
     *  porque vm.promptSender es null en ese caso. */
    public final PromptSender promptSender = new PromptSender() {
        @Override public void send(long requestId, String spec) {
            // Llamada CUALIFICADA al send(jsonLine) de la outer class.
            DebugServer.this.send("{\"type\":\"PROMPT_REQUEST\",\"promptId\":" + requestId
                    + ",\"spec\":" + Json.quote(spec) + "}");
        }
    };

    /** Envía una línea JSON. Si el cliente está cerrado, no-op. */
    public synchronized void send(String jsonLine) {
        PrintWriter w = this.clientOut;
        if (w == null) return;
        try {
            w.print(jsonLine);
            w.print('\n');
            w.flush();
        } catch (Throwable t) {
            System.err.println("[DebugServer] send falló: " + t.getMessage());
        }
    }

    // ---- Comandos cliente → VM ----

    private void readLoop() {
        try {
            String line;
            while (!closed && (line = clientIn.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty()) continue;
                try {
                    handleMessage(line);
                } catch (Throwable t) {
                    System.err.println("[DebugServer] mensaje ignorado: " + t.getMessage()
                            + "  (línea: " + line + ")");
                }
            }
        } catch (IOException e) {
            if (!closed)
                System.err.println("[DebugServer] readLoop terminó: " + e.getMessage());
        } finally {
            System.err.println("[DebugServer] cliente desconectado");
            // No mata la VM: sigue ejecutando headless.
            vm.setPromptSender(null);   // futuros prompt() lanzarán RuntimeError BP
            this.client = null;
            this.clientOut = null;
            this.clientIn = null;
        }
    }

    private void handleMessage(String jsonLine) {
        Map<String, Object> m = Json.parseFlatObject(jsonLine);
        String type = Json.getString(m, "type", "");
        long id = Json.getLong(m, "id", -1);
        try {
            switch (type) {
                // ---- META ----
                case "HELLO":
                    sendHelloReply(id);
                    break;

                // ---- TERMINAL ----
                case "RUN": {
                    String module = Json.getString(m, "path", "");
                    if (module.isEmpty()) {
                        // Compat: en PR-1 también aceptamos `module` por si
                        // queda algún caller que use la forma antigua antes de
                        // recompilar.
                        module = Json.getString(m, "module", "");
                    }
                    if (module.isEmpty()) {
                        sendError(id, "INVALID_PARAM", "RUN: falta 'path'");
                        break;
                    }
                    if (!runModuleRequest.isDone()) {
                        runModuleRequest.complete(module);
                        sendReply(id, "RUN_REPLY", "");
                    } else {
                        sendError(id, "BUSY", "RUN ignorado: ya hay módulo cargado");
                    }
                    break;
                }
                case "KILL":
                    controller.sendCommand(StepCommand.STOP);
                    sendReply(id, "KILL_REPLY", "");
                    break;
                case "PROMPT_RESPONSE": {
                    long promptId = Json.getLong(m, "promptId", -1);
                    String values = Json.getString(m, "values", "");
                    vm.deliverPromptResponse(promptId, values);
                    sendReply(id, "PROMPT_RESPONSE_REPLY", "");
                    break;
                }

                // ---- DEBUG ----
                case "SET_BP": {
                    String file = Json.getString(m, "file", "");
                    int line = (int) Json.getLong(m, "line", 0);
                    boolean enabled = Json.getBool(m, "enabled", true);
                    controller.setBreakpoint(file, line, enabled);
                    // PR-1: bpId placeholder. PR-5 introducirá tracking real.
                    sendReply(id, "SET_BP_REPLY", "\"bpId\":0");
                    break;
                }
                case "CLR_BP": {
                    // PR-1: si no llega bpId, comportamiento legacy =
                    // clearAllBreakpoints. PR-5 implementará clear por bpId.
                    long bpId = Json.getLong(m, "bpId", -1);
                    if (bpId < 0) {
                        controller.clearAllBreakpoints();
                    }
                    // Con bpId concreto no hacemos nada aún (TODO PR-5).
                    sendReply(id, "CLR_BP_REPLY", "");
                    break;
                }
                case "CONTINUE":
                    controller.sendCommand(StepCommand.CONTINUE);
                    sendReply(id, "CONTINUE_REPLY", "");
                    break;
                case "STEP": {
                    String mode = Json.getString(m, "mode", "into");
                    switch (mode) {
                        case "into": controller.sendCommand(StepCommand.STEP_INTO); break;
                        case "over": controller.sendCommand(StepCommand.STEP_OVER); break;
                        case "out":  controller.sendCommand(StepCommand.STEP_OUT);  break;
                        default:
                            sendError(id, "INVALID_PARAM", "STEP.mode inválido: " + mode);
                            return;
                    }
                    sendReply(id, "STEP_REPLY", "");
                    break;
                }
                case "STACK":  sendStackReply(id); break;
                case "LOCALS": sendLocalsReply(id); break;

                // ---- FILES ----
                case "LIST":  sendListReply(id, m); break;
                case "GET":   sendGetReply(id, m); break;
                case "PUT":   sendPutReply(id, m); break;
                case "DEL":   sendDelReply(id, m); break;
                case "MKDIR": sendMkdirReply(id, m); break;

                // ---- Extensiones Java-only (no en v1; futuros INSPECT en PR-5) ----
                case "MODULE_PROPERTIES": sendModulePropertiesReply(id); break;
                case "READ_INT":          sendReadIntReply(id, m); break;
                case "READ_STRING":       sendReadStringReply(id, m); break;

                default:
                    sendError(id, "UNSUPPORTED", "unknown type: " + type);
            }
        } catch (Throwable t) {
            sendError(id, "INTERNAL_ERROR", "error procesando '" + type + "': " + t.getMessage());
        }
    }

    /** Bloquea hasta que llegue un RUN, o el timeout. Devuelve el path
     *  del módulo (relativo al workdir) o null si timeout. */
    public String awaitRunModule(long timeoutMs) throws InterruptedException {
        try {
            return runModuleRequest.get(timeoutMs, java.util.concurrent.TimeUnit.MILLISECONDS);
        } catch (java.util.concurrent.TimeoutException te) {
            return null;
        } catch (java.util.concurrent.ExecutionException ee) {
            return null;
        }
    }

    // ============================================================
    // Replies — todas devuelven JSON `{"type":"X_REPLY","id":N,...}`
    // ============================================================

    /** Helper: envía un reply v1 con un payload `extraJson` ya formateado
     *  (sin llaves, sin coma inicial). `extraJson` puede ser "" si no hay
     *  campos adicionales. */
    private void sendReply(long id, String type, String extraJson) {
        StringBuilder sb = new StringBuilder(64);
        sb.append("{\"type\":\"").append(type).append("\",\"id\":").append(id);
        if (extraJson != null && !extraJson.isEmpty()) {
            sb.append(',').append(extraJson);
        }
        sb.append('}');
        send(sb.toString());
    }

    /** ERROR reply v1: `{"type":"ERROR","id":N,"code":"...","message":"..."}`. */
    private void sendError(long id, String code, String message) {
        send("{\"type\":\"ERROR\",\"id\":" + id
                + ",\"code\":\"" + Json.escape(code) + "\""
                + ",\"message\":" + Json.quote(message) + "}");
    }

    private void sendHelloReply(long id) {
        StringBuilder caps = new StringBuilder("[");
        for (int i = 0; i < CAPABILITIES.length; i++) {
            if (i > 0) caps.append(',');
            caps.append('"').append(CAPABILITIES[i]).append('"');
        }
        caps.append(']');
        String payload = "\"protoVersion\":" + PROTO_VERSION
                + ",\"serverName\":\"" + SERVER_NAME + "\""
                + ",\"serverBuild\":\"" + SERVER_BUILD + "\""
                + ",\"capabilities\":" + caps.toString();
        sendReply(id, "HELLO_REPLY", payload);
    }

    private void sendLocalsReply(long id) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(id, "INTERNAL_ERROR", "LOCALS: VM no está pausada");
            return;
        }
        int nLocals = Math.max(0, (ctx.sp - ctx.bp) / 4);
        StringBuilder sb = new StringBuilder();
        sb.append("\"tid\":").append(ctx.tid);
        sb.append(",\"locals\":[");
        for (int i = 0; i < nLocals; i++) {
            if (i > 0) sb.append(',');
            sb.append(ctx.readLocal(i * 4));
        }
        sb.append("]");
        sendReply(id, "LOCALS_REPLY", sb.toString());
    }

    private void sendStackReply(long id) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(id, "INTERNAL_ERROR", "STACK: VM no está pausada");
            return;
        }
        List<int[]> frames = ctx.stackFrames();
        StringBuilder sb = new StringBuilder();
        sb.append("\"tid\":").append(ctx.tid);
        sb.append(",\"frames\":[");
        for (int i = 0; i < frames.size(); i++) {
            if (i > 0) sb.append(',');
            int[] f = frames.get(i);
            sb.append("[").append(f[0]).append(',').append(f[1]).append(']');
        }
        sb.append("]");
        sendReply(id, "STACK_REPLY", sb.toString());
    }

    private void sendModulePropertiesReply(long id) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(id, "INTERNAL_ERROR", "MODULE_PROPERTIES: VM no está pausada");
            return;
        }
        List<ModuleManager.PropertyView> props = ctx.moduleProperties();
        StringBuilder sb = new StringBuilder();
        sb.append("\"props\":[");
        for (int i = 0; i < props.size(); i++) {
            if (i > 0) sb.append(',');
            ModuleManager.PropertyView p = props.get(i);
            sb.append("{\"module\":").append(Json.quote(p.module));
            sb.append(",\"name\":").append(Json.quote(p.name));
            sb.append(",\"type\":").append(Json.quote(p.type));
            sb.append(",\"rawValue\":").append(p.rawValue);
            sb.append(",\"display\":").append(Json.quote(p.display));
            sb.append('}');
        }
        sb.append("]");
        sendReply(id, "MODULE_PROPERTIES_REPLY", sb.toString());
    }

    private void sendReadIntReply(long id, Map<String, Object> m) {
        long addr = Json.getLong(m, "addr", -1);
        int value;
        try {
            value = vm.readMemoryInt((int) addr);
        } catch (Throwable t) {
            sendError(id, "INTERNAL_ERROR", "READ_INT(" + addr + "): " + t.getMessage());
            return;
        }
        sendReply(id, "READ_INT_REPLY", "\"value\":" + value);
    }

    private void sendReadStringReply(long id, Map<String, Object> m) {
        long ref = Json.getLong(m, "ref", 0);
        String s;
        try {
            s = vm.readStringIfPossible((int) ref);
        } catch (Throwable t) {
            sendError(id, "INTERNAL_ERROR", "READ_STRING(" + ref + "): " + t.getMessage());
            return;
        }
        sendReply(id, "READ_STRING_REPLY", "\"value\":" + Json.quote(s == null ? "" : s));
    }

    // ---- File transfer (A2.2) — PR-1 mantiene base64; PR-2 cambia a bulk raw ----

    /** Resuelve un path del wire dentro del workdir. Si no hay workdir
     *  configurado, error: la VM debe haberse arrancado con --workdir
     *  para aceptar transferencias. */
    private java.nio.file.Path workdirPath(String userPath) {
        ModuleManager mm = vm.getModuleManager();
        if (mm == null || mm.getWorkdir() == null) {
            throw new IllegalStateException("VM sin workdir; arrancar con --workdir");
        }
        return mm.resolveInWorkdir(userPath);
    }

    private void sendPutReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        String b64  = Json.getString(m, "data", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "PUT: falta 'path'"); return;
        }
        byte[] bytes;
        try { bytes = java.util.Base64.getDecoder().decode(b64); }
        catch (IllegalArgumentException iae) {
            sendError(id, "INVALID_PARAM", "PUT: base64 inválido"); return;
        }
        try {
            java.nio.file.Path dst = workdirPath(path);
            java.nio.file.Path parent = dst.getParent();
            if (parent != null) java.nio.file.Files.createDirectories(parent);
            java.nio.file.Files.write(dst, bytes);
            sendReply(id, "PUT_REPLY", "\"size\":" + bytes.length);
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "PUT: " + e.getMessage());
        }
    }

    private void sendGetReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "GET: falta 'path'"); return;
        }
        try {
            java.nio.file.Path src = workdirPath(path);
            byte[] data = java.nio.file.Files.readAllBytes(src);
            String b64 = java.util.Base64.getEncoder().encodeToString(data);
            sendReply(id, "GET_REPLY",
                    "\"size\":" + data.length + ",\"data\":\"" + b64 + "\"");
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "GET: " + e.getMessage());
        }
    }

    private void sendListReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        try {
            java.nio.file.Path dir = workdirPath(path.isEmpty() ? "." : path);
            if (!java.nio.file.Files.isDirectory(dir)) {
                sendError(id, "NOT_FOUND", "LIST: no es un directorio: " + path); return;
            }
            StringBuilder sb = new StringBuilder();
            sb.append("\"entries\":[");
            boolean first = true;
            try (java.util.stream.Stream<java.nio.file.Path> s = java.nio.file.Files.list(dir)) {
                java.util.List<java.nio.file.Path> sorted = s
                        .sorted(java.util.Comparator.comparing(java.nio.file.Path::getFileName))
                        .collect(java.util.stream.Collectors.toList());
                for (java.nio.file.Path p : sorted) {
                    if (!first) sb.append(',');
                    first = false;
                    boolean isDir = java.nio.file.Files.isDirectory(p);
                    long size = isDir ? 0 : java.nio.file.Files.size(p);
                    sb.append("{\"name\":").append(Json.quote(p.getFileName().toString()));
                    sb.append(",\"size\":").append(size);
                    sb.append(",\"isDir\":").append(isDir ? "true" : "false");
                    sb.append('}');
                }
            }
            sb.append("]");
            sendReply(id, "LIST_REPLY", sb.toString());
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "LIST: " + e.getMessage());
        }
    }

    private void sendDelReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "DEL: falta 'path'"); return;
        }
        try {
            java.nio.file.Path p = workdirPath(path);
            java.nio.file.Files.delete(p);
            sendReply(id, "DEL_REPLY", "");
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "DEL: " + e.getMessage());
        }
    }

    private void sendMkdirReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "MKDIR: falta 'path'"); return;
        }
        try {
            java.nio.file.Path p = workdirPath(path);
            java.nio.file.Files.createDirectories(p);
            sendReply(id, "MKDIR_REPLY", "");
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "MKDIR: " + e.getMessage());
        }
    }

    @Override public void close() {
        closed = true;
        clientConnected.countDown();
        try { if (client != null)       client.close();       } catch (IOException ignored) {}
        try { if (serverSocket != null) serverSocket.close(); } catch (IOException ignored) {}
    }
}
