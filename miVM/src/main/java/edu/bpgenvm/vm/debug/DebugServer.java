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
//   - Bulk binario inline (v1 §2.2): cuando un mensaje declara
//     `"bulk":N`, los N bytes raw vienen inmediatamente tras el `\n`
//     del JSON, sin separador. Aplicable hoy a PUT (request) y
//     GET_REPLY (reply).
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
// PR-6 estandariza los códigos de error según v1 §8.1 y añade FATAL
// para errores de protocolo (JSON inválido, bulk size mismatch) que
// cierran la conexión inmediatamente.
// Deviations pendientes:
//   - KILL no termina realmente el programa — sólo manda STOP por
//     la cola de comandos del controller. Sólo tiene efecto si la VM
//     está pausada en el hook. Kill real requiere shutdown del
//     WorkerLoop — diferido a un PR de A1 que toque la VM.
//   - EVAL no implementado (UNSUPPORTED).
//   - ID duplicado (v1 §8.4): no enforced — el server procesa cada
//     mensaje secuencialmente, así que dos requests con el mismo id
//     se procesan en orden de llegada, ambos producirán reply.
// ============================================================
package edu.bpgenvm.vm.debug;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.util.WireFraming;
import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
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
    /** Stream crudo para escribir frames al cliente. Protegido por `writeLock`. */
    private OutputStream clientOutRaw;
    /** Stream crudo para leer frames del cliente. Usado SÓLO desde el readLoop thread. */
    private InputStream clientInRaw;
    /** Lock para escrituras al socket (eventos VM y replies pueden venir
     *  de distintos threads — controller listeners, prompt sender, reader). */
    private final Object writeLock = new Object();
    /** PR-4 — instante de arranque del proceso VM. Para reportar uptimeMs
     *  en INFO. */
    private final long startMs = System.currentTimeMillis();
    /** PR-4 — uniqueId estable durante la vida del proceso. Lo generamos
     *  una vez al construir el servidor. */
    private final String uniqueId = generateUniqueId();
    private Thread readerThread;

    /** Latch para esperar el primer cliente cuando se arranca en modo
     *  "debugger attach". countDown al hacer accept. */
    private final CountDownLatch clientConnected = new CountDownLatch(1);

    /** A2.3 — Latch + slot para que el modo daemon de Main espere un
     *  RUN request y reciba el nombre del módulo a ejecutar. */
    private final java.util.concurrent.CompletableFuture<String> runModuleRequest =
            new java.util.concurrent.CompletableFuture<>();

    /** PR-3 — sessionId allocator. v1 §9: una sesión activa por server.
     *  La primera asignación es 1; activeSession=0 = sin sesión. */
    private int nextSessionId = 1;
    private volatile int activeSession = 0;
    /** Wallclock cuando se asignó activeSession — para `elapsedMs` en EXITED.
     *  0 cuando no hay sesión activa. */
    private volatile long activeSessionStartMs = 0;

    /** PR-3 — true cuando el último comando del cliente fue STEP_INTO/OVER/OUT
     *  y todavía no hemos visto la PausedEvent resultante. Determina si la
     *  siguiente pausa se serializa como STEP_DONE (step) o BP_HIT (BP real
     *  o pausa inicial). */
    private volatile boolean stepInProgress = false;

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
            this.clientOutRaw = new BufferedOutputStream(s.getOutputStream());
            this.clientInRaw  = new BufferedInputStream(s.getInputStream());
            System.err.println("[DebugServer] cliente conectado desde " + s.getRemoteSocketAddress());

            // Conectar sink + listener + promptSender. El SocketSink emite
            // OUTPUT events a través de nuestro send(jsonLine), garantizando
            // sincronización con el resto de tráfico. La sessionId se lee
            // perezosamente en cada flush — refleja la sesión activa en ese
            // momento (0 = sin sesión).
            SocketSink sink = new SocketSink(this::send, () -> activeSession);
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
        int sess = this.activeSession;
        try {
            if (ev instanceof PausedEvent) {
                PausedEvent e = (PausedEvent) ev;
                // PR-3: distinguir step-done de breakpoint-hit. PR-5: incluir
                // bpId real (busca el bp por file:line si lo hay) y `frame`
                // anidado v1. Los campos planos se conservan para back-compat
                // hasta que PR-7 cierre BpvmClient.
                boolean wasStep = this.stepInProgress;
                this.stepInProgress = false;
                String type = wasStep ? "STEP_DONE" : "BP_HIT";
                // bpId: para BP_HIT, buscar bp por file:line; para STEP_DONE
                // no aplica.
                long bpId = 0;
                if (!wasStep && e.sourceFile != null) {
                    for (DebugController.BreakpointInfo bi : controller.listBreakpoints()) {
                        if (bi.line == e.line && bi.file.equals(basenameOf(e.sourceFile))) {
                            bpId = bi.bpId;
                            break;
                        }
                    }
                }
                String safeFile = Json.escape(e.sourceFile == null ? "" : e.sourceFile);
                String frame = "{\"file\":\"" + safeFile + "\""
                        + ",\"line\":" + e.line
                        + ",\"function\":\"?\"}";  // función no disponible aún
                StringBuilder sb = new StringBuilder(192);
                sb.append("{\"type\":\"").append(type).append("\"");
                sb.append(",\"session\":").append(sess);
                if (!wasStep) sb.append(",\"bpId\":").append(bpId);
                sb.append(",\"frame\":").append(frame);
                // PR-5: file/line ya están dentro de `frame` (v1 puro). El
                // resto son extensión Java-only (no están en v1 standard) —
                // los conservamos al nivel del mensaje porque el debugger
                // in-process Java los necesita para queries de estado.
                sb.append(",\"tid\":").append(e.tid);
                sb.append(",\"absPc\":").append(e.absPc);
                sb.append(",\"bp\":").append(e.bp);
                sb.append(",\"sp\":").append(e.sp);
                sb.append(",\"cs\":").append(e.cs);
                sb.append(",\"stackBase\":").append(e.stackBase);
                sb.append("}");
                send(sb.toString());
            } else if (ev instanceof ResumedEvent) {
                // RESUMED es extensión Java (no en v1). Útil para la UI:
                // saber cuándo limpiar el highlight del breakpoint.
                ResumedEvent e = (ResumedEvent) ev;
                send("{\"type\":\"RESUMED\",\"session\":" + sess + ",\"tid\":" + e.tid + "}");
            } else if (ev instanceof ExitedEvent) {
                ExitedEvent e = (ExitedEvent) ev;
                // Wire v1 canon (§6.3): {type, session, status, exitCode,
                // elapsedMs, errorMessage?}. Replica exactamente el formato
                // que envía el firmware Pico (repl_v1.c handle_run). El
                // mapeo de e.reason → status:
                //   exitCode==0  → status="OK"            (no errorMessage)
                //   exitCode!=0  → status="RUNTIME_ERROR" + errorMessage=e.reason
                long elapsed = activeSessionStartMs > 0
                        ? System.currentTimeMillis() - activeSessionStartMs
                        : 0;
                String status = (e.exitCode == 0) ? "OK" : "RUNTIME_ERROR";
                StringBuilder sb = new StringBuilder(128);
                sb.append("{\"type\":\"EXITED\",\"session\":").append(sess)
                  .append(",\"status\":\"").append(status).append('"')
                  .append(",\"exitCode\":").append(e.exitCode)
                  .append(",\"elapsedMs\":").append(elapsed);
                if (e.reason != null && !e.reason.isEmpty()) {
                    sb.append(",\"errorMessage\":\"").append(Json.escape(e.reason)).append('"');
                }
                sb.append('}');
                send(sb.toString());
                // Limpieza de estado de la sesión: tras EXITED ya no hay
                // pausa pendiente que esperar.
                this.stepInProgress = false;
                this.activeSession = 0;
                this.activeSessionStartMs = 0;
            } else if (ev instanceof ExceptionEvent) {
                // Wire v1 (§6.3) — las excepciones runtime se folddean en
                // EXITED.status="RUNTIME_ERROR" + errorMessage. Para no
                // perder el stackTrace lo concatenamos al message. El
                // VM también emite ExitedEvent justo después del fault,
                // así que el cliente verá los dos: EXITED es la fuente
                // de verdad del fin de sesión.
                ExceptionEvent e = (ExceptionEvent) ev;
                long elapsed = activeSessionStartMs > 0
                        ? System.currentTimeMillis() - activeSessionStartMs
                        : 0;
                StringBuilder sb = new StringBuilder(256);
                sb.append("{\"type\":\"EXITED\",\"session\":").append(sess)
                  .append(",\"status\":\"RUNTIME_ERROR\"")
                  .append(",\"exitCode\":1")
                  .append(",\"elapsedMs\":").append(elapsed)
                  .append(",\"errorMessage\":\"").append(Json.escape(e.message));
                if (e.stackTrace != null && !e.stackTrace.isEmpty()) {
                    sb.append("\\n").append(Json.escape(e.stackTrace));
                }
                sb.append("\"}");
                send(sb.toString());
                this.stepInProgress = false;
            } else {
                // Tipo desconocido: lo envolvemos para no perderlo si añadimos
                // eventos nuevos sin tocar este servidor.
                send("{\"type\":\"" + Json.escape(ev.type()) + "\",\"session\":" + sess + "}");
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
            int sess = DebugServer.this.activeSession;
            DebugServer.this.send("{\"type\":\"PROMPT_REQUEST\",\"session\":" + sess
                    + ",\"promptId\":" + requestId
                    + ",\"spec\":" + Json.quote(spec) + "}");
        }
    };

    /** Envía una línea JSON (sin bulk). Si el cliente está cerrado, no-op. */
    public void send(String jsonLine) {
        sendFrame(jsonLine, null);
    }

    /** Envía una línea JSON + opcionalmente un bulk binario raw inmediatamente
     *  después del `\n`. Atómico para el peer: ambos writes van bajo el mismo
     *  writeLock y el flush sale al final.
     *
     *  Si el cliente está cerrado, no-op silencioso (log a stderr). */
    public void sendFrame(String jsonLine, byte[] bulkOrNull) {
        OutputStream w = this.clientOutRaw;
        if (w == null) return;
        synchronized (writeLock) {
            try {
                WireFraming.sendLine(w, jsonLine);
                if (bulkOrNull != null && bulkOrNull.length > 0) {
                    WireFraming.sendBulk(w, bulkOrNull);
                }
                w.flush();
            } catch (Throwable t) {
                System.err.println("[DebugServer] sendFrame falló: " + t.getMessage());
            }
        }
    }

    // ---- Comandos cliente → VM ----

    private void readLoop() {
        try {
            String line;
            while (!closed && (line = WireFraming.recvLine(clientInRaw)) != null) {
                line = line.trim();
                if (line.isEmpty()) continue;
                byte[] bulk = null;
                Map<String, Object> m;
                try {
                    m = Json.parseFlatObject(line);
                } catch (Throwable t) {
                    // v1 §8.3: JSON inválido → FATAL + cierre.
                    sendFatal("PROTOCOL_ERROR", "JSON inválido: " + t.getMessage());
                    break;
                }
                // Si el mensaje declara bulk, leer los bytes del wire ANTES
                // de procesar nada más — la siguiente línea JSON viene
                // después de los bulk bytes y aún no la hemos visto.
                long bulkSize = Json.getLong(m, "bulk", 0);
                if (bulkSize > 0) {
                    if (bulkSize > Integer.MAX_VALUE) {
                        sendFatal("PROTOCOL_ERROR",
                                "bulk size fuera de rango: " + bulkSize);
                        break;
                    }
                    try {
                        bulk = WireFraming.recvBulk(clientInRaw, (int) bulkSize);
                    } catch (IOException ioe) {
                        // v1 §8.5: bulk mismatch / corte de stream → FATAL.
                        sendFatal("PROTOCOL_ERROR", "bulk lectura falló: " + ioe.getMessage());
                        break;
                    }
                }
                try {
                    handleMessage(m, bulk);
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
            this.clientOutRaw = null;
            this.clientInRaw = null;
        }
    }

    /** v1 §8.3: emite un FATAL y cierra el socket inmediatamente. Tras un
     *  FATAL el cliente debe reconectar. */
    private void sendFatal(String code, String message) {
        send("{\"type\":\"FATAL\",\"code\":\"" + Json.escape(code) + "\""
                + ",\"message\":" + Json.quote(message) + "}");
        try { if (client != null) client.close(); } catch (IOException ignored) {}
    }

    private void handleMessage(Map<String, Object> m, byte[] bulk) {
        String type = Json.getString(m, "type", "");
        long id = Json.getLong(m, "id", -1);
        try {
            switch (type) {
                // ---- META ----
                case "HELLO":
                    sendHelloReply(id);
                    break;
                case "INFO":
                    sendInfoReply(id);
                    break;
                case "PING":
                    sendReply(id, "PONG", "");
                    break;
                case "TIME": {
                    long epochSec = Json.getLong(m, "epochSec", -1);
                    if (epochSec < 0) {
                        sendError(id, "INVALID_PARAM", "TIME: falta 'epochSec' (>=0)");
                        break;
                    }
                    vm.setRtcEpochSec(epochSec);
                    sendReply(id, "TIME_REPLY", "");
                    break;
                }
                case "RESET":
                    sendResetReply(id);
                    break;
                case "BOOTSEL":
                case "SAVE":
                case "LOG_DUMP":
                    // Sólo aplicables al firmware Pico. La VM Java los rechaza.
                    sendError(id, "UNSUPPORTED", type + ": no soportado en VM Java (sólo Pico)");
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
                    if (this.activeSession != 0) {
                        sendError(id, "BUSY", "RUN ignorado: ya hay sesión activa (session=" + activeSession + ")");
                        break;
                    }
                    if (runModuleRequest.isDone()) {
                        // v1 §9: una ejecución por proceso (mismo lifecycle
                        // que en PR-1/PR-2 — no soportamos re-runs todavía).
                        sendError(id, "BUSY", "RUN ignorado: ya se ejecutó un módulo en este proceso");
                        break;
                    }
                    int sess = this.nextSessionId++;
                    this.activeSession = sess;
                    this.activeSessionStartMs = System.currentTimeMillis();
                    this.stepInProgress = false;
                    runModuleRequest.complete(module);
                    sendReply(id, "RUN_REPLY", "\"session\":" + sess);
                    break;
                }
                case "KILL": {
                    // v1 KILL es session-targeted. En PR-3 el server tiene
                    // una sola sesión activa; aceptamos `session` opcional y
                    // si llega debe coincidir, si no la inferimos.
                    long reqSess = Json.getLong(m, "session", -1);
                    if (reqSess >= 0 && reqSess != activeSession) {
                        sendError(id, "NO_SESSION", "KILL: session " + reqSess
                                + " no existe (activa=" + activeSession + ")");
                        break;
                    }
                    controller.sendCommand(StepCommand.STOP);
                    this.stepInProgress = false;
                    sendReply(id, "KILL_REPLY", "");
                    break;
                }
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
                    long bpId = controller.setBreakpoint(file, line, enabled);
                    sendReply(id, "SET_BP_REPLY", "\"bpId\":" + bpId);
                    break;
                }
                case "CLR_BP": {
                    long bpId = Json.getLong(m, "bpId", -1);
                    if (bpId < 0) {
                        // Sin bpId: convención legacy del IDE — borrar todos.
                        controller.clearAllBreakpoints();
                    } else {
                        controller.clearBreakpointById(bpId);
                    }
                    sendReply(id, "CLR_BP_REPLY", "");
                    break;
                }
                case "LIST_BP": {
                    java.util.List<DebugController.BreakpointInfo> bps = controller.listBreakpoints();
                    StringBuilder sb = new StringBuilder();
                    sb.append("\"breakpoints\":[");
                    for (int i = 0; i < bps.size(); i++) {
                        if (i > 0) sb.append(',');
                        DebugController.BreakpointInfo bi = bps.get(i);
                        sb.append("{\"bpId\":").append(bi.bpId);
                        sb.append(",\"file\":").append(Json.quote(bi.file));
                        sb.append(",\"line\":").append(bi.line);
                        sb.append(",\"enabled\":").append(bi.enabled ? "true" : "false");
                        sb.append(",\"hits\":").append(bi.hits);
                        sb.append('}');
                    }
                    sb.append(']');
                    sendReply(id, "LIST_BP_REPLY", sb.toString());
                    break;
                }
                case "PAUSE":
                    // PR-5: solicita pausa "manual". Cambia el modo a
                    // STEP_INTO para que el hook pause en la próxima
                    // llamada. Tras esa pausa, el server emitirá BP_HIT
                    // con bpId=0 (pausa manual).
                    controller.requestPause();
                    sendReply(id, "PAUSE_REPLY", "");
                    break;
                case "EVAL":
                    sendError(id, "UNSUPPORTED", "EVAL no implementado todavía");
                    break;
                case "INSPECT": {
                    long ref = Json.getLong(m, "ref", 0);
                    int depth = (int) Json.getLong(m, "depth", 1);
                    sendInspectReply(id, ref, depth);
                    break;
                }
                case "CONTINUE":
                    this.stepInProgress = false;
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
                    // PR-3: la próxima PausedEvent del controller se serializa
                    // como STEP_DONE (no BP_HIT).
                    this.stepInProgress = true;
                    sendReply(id, "STEP_REPLY", "");
                    break;
                }
                case "STACK":  sendStackReply(id); break;
                case "LOCALS": sendLocalsReply(id); break;

                // ---- FILES ----
                case "LIST":   sendListReply(id, m); break;
                case "STAT":   sendStatReply(id, m); break;
                case "GET":    sendGetReply(id, m); break;
                case "PUT":    sendPutReply(id, m, bulk); break;
                case "DEL":    sendDelReply(id, m); break;
                case "MKDIR":  sendMkdirReply(id, m); break;
                case "RMDIR":  sendRmdirReply(id, m); break;
                case "RENAME": sendRenameReply(id, m); break;
                case "FORMAT": sendFormatReply(id, m); break;
                case "DF":     sendDfReply(id); break;

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
        // H6.a.1 — locales POR NOMBRE: si el .dbg v3 del módulo trae la tabla
        // var→offset de la función que contiene el pc actual, resolvemos cada
        // variable (params + locales) a {name, offset, value, size, isArray}.
        // El IDE muestra "x = 5" en vez del array crudo. Si no hay .dbg v3,
        // `named` queda vacío y el cliente cae al array `locals` por índice.
        ModuleManager mm = vm.getModuleManager();
        ModuleManager.FunctionVars fv = (mm != null) ? mm.functionForPc(ctx.absPc) : null;
        sb.append(",\"named\":[");
        if (fv != null) {
            boolean first = true;
            for (ModuleManager.LocalVarDescriptor v : fv.vars) {
                if (!first) sb.append(',');
                first = false;
                // long/double i64 big-endian: high word en offset, low en offset+4
                // (idéntico a VirtualMachine.readI64).
                long value = (v.sizeBytes == 8)
                        ? ((long) ctx.readLocal(v.offset) << 32)
                          | ((long) ctx.readLocal(v.offset + 4) & 0xFFFFFFFFL)
                        : ctx.readLocal(v.offset);
                String display = renderLocalDisplay(v, value);   // H6.a.2: render por tipo
                sb.append("{\"name\":").append(Json.quote(v.name));
                sb.append(",\"offset\":").append(v.offset);
                sb.append(",\"size\":").append(v.sizeBytes);
                sb.append(",\"isArray\":").append(v.isArray);
                sb.append(",\"type\":").append(Json.quote(v.type));
                sb.append(",\"value\":").append(value);
                sb.append(",\"display\":").append(Json.quote(display));
                sb.append('}');
            }
        }
        sb.append("]");
        sendReply(id, "LOCALS_REPLY", sb.toString());
    }

    /**
     * H6.a.2 — renderiza un local a texto legible según su tipo BP (del .dbg v3).
     * El host tiene el .dbg + acceso al heap, así que produce el `display` listo
     * (regla de oro: la traducción simbólica la hace el host). `value` es el i32
     * del slot, o el i64 big-endian combinado si el slot es de 8 bytes.
     */
    private String renderLocalDisplay(ModuleManager.LocalVarDescriptor v, long value) {
        if (v.isArray) return "array[len=" + value + "]";   // array local inline: el slot guarda la longitud
        switch (v.type) {
            case "integer": return Integer.toString((int) value);
            case "long":    return Long.toString(value);
            case "float":   return Float.toString(Float.intBitsToFloat((int) value));
            case "double":  return Double.toString(Double.longBitsToDouble(value));
            case "boolean": return (value != 0) ? "true" : "false";
            case "string": {
                if (value == 0) return "null";
                String s = null;
                try { s = vm.readStringIfPossible((int) value); } catch (Throwable ignored) { }
                return (s != null) ? ("\"" + s + "\"") : ("@" + value);
            }
            case "ref":     return (value == 0) ? "null" : ("@" + value);
            default:        return Long.toString(value);   // "?" / desconocido → valor crudo
        }
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

    // ---- File transfer (A2.2) — PR-2: bulk binario raw inline ----

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

    /** Traduce una excepción al code de error v1 más apropiado. */
    private static String codeFor(Throwable t) {
        if (t instanceof java.nio.file.NoSuchFileException) return "NOT_FOUND";
        if (t instanceof java.nio.file.FileAlreadyExistsException) return "EXISTS";
        if (t instanceof java.nio.file.DirectoryNotEmptyException) return "INVALID_PARAM";
        if (t instanceof IllegalStateException) return "INVALID_PATH";   // sandbox escape
        return "INTERNAL_ERROR";
    }

    private void sendPutReply(long id, Map<String, Object> m, byte[] bulk) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "PUT: falta 'path'"); return;
        }
        if (bulk == null) {
            sendError(id, "INVALID_PARAM", "PUT: falta bulk binario (campo 'bulk':N + N bytes raw)"); return;
        }
        try {
            java.nio.file.Path dst = workdirPath(path);
            java.nio.file.Path parent = dst.getParent();
            if (parent != null) java.nio.file.Files.createDirectories(parent);
            java.nio.file.Files.write(dst, bulk);
            sendReply(id, "PUT_REPLY", "\"size\":" + bulk.length);
        } catch (Throwable e) {
            sendError(id, codeFor(e), "PUT: " + e.getMessage());
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
            // v1: reply lleva `bulk:N`, los N bytes raw vienen inmediatamente
            // tras el `\n`. Atómico bajo writeLock.
            String header = "{\"type\":\"GET_REPLY\",\"id\":" + id
                    + ",\"size\":" + data.length
                    + ",\"bulk\":" + data.length + "}";
            sendFrame(header, data);
        } catch (Throwable e) {
            sendError(id, codeFor(e), "GET: " + e.getMessage());
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
        } catch (Throwable e) {
            sendError(id, codeFor(e), "LIST: " + e.getMessage());
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
        } catch (Throwable e) {
            sendError(id, codeFor(e), "DEL: " + e.getMessage());
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
        } catch (Throwable e) {
            sendError(id, codeFor(e), "MKDIR: " + e.getMessage());
        }
    }

    private void sendStatReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "STAT: falta 'path'"); return;
        }
        try {
            java.nio.file.Path p = workdirPath(path);
            if (!java.nio.file.Files.exists(p)) {
                sendError(id, "NOT_FOUND", "STAT: no existe: " + path); return;
            }
            boolean isDir = java.nio.file.Files.isDirectory(p);
            long size = isDir ? 0 : java.nio.file.Files.size(p);
            long mtime = java.nio.file.Files.getLastModifiedTime(p).toMillis() / 1000L;
            sendReply(id, "STAT_REPLY",
                    "\"size\":" + size
                    + ",\"isDir\":" + (isDir ? "true" : "false")
                    + ",\"mtime\":" + mtime);
        } catch (Throwable e) {
            sendError(id, codeFor(e), "STAT: " + e.getMessage());
        }
    }

    private void sendRmdirReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "RMDIR: falta 'path'"); return;
        }
        try {
            java.nio.file.Path p = workdirPath(path);
            if (!java.nio.file.Files.isDirectory(p)) {
                sendError(id, "INVALID_PARAM", "RMDIR: no es directorio: " + path); return;
            }
            java.nio.file.Files.delete(p);   // falla si no está vacío
            sendReply(id, "RMDIR_REPLY", "");
        } catch (Throwable e) {
            sendError(id, codeFor(e), "RMDIR: " + e.getMessage());
        }
    }

    private void sendRenameReply(long id, Map<String, Object> m) {
        String from = Json.getString(m, "from", "");
        String to   = Json.getString(m, "to", "");
        if (from.isEmpty() || to.isEmpty()) {
            sendError(id, "INVALID_PARAM", "RENAME: faltan 'from'/'to'"); return;
        }
        try {
            java.nio.file.Path src = workdirPath(from);
            java.nio.file.Path dst = workdirPath(to);
            java.nio.file.Files.move(src, dst);
            sendReply(id, "RENAME_REPLY", "");
        } catch (Throwable e) {
            sendError(id, codeFor(e), "RENAME: " + e.getMessage());
        }
    }

    private void sendFormatReply(long id, Map<String, Object> m) {
        String confirm = Json.getString(m, "confirm", "");
        if (!"YES".equals(confirm)) {
            sendError(id, "MISSING_CONFIRM", "FORMAT requiere confirm:\"YES\"");
            return;
        }
        try {
            ModuleManager mm = vm.getModuleManager();
            if (mm == null || mm.getWorkdir() == null) {
                sendError(id, "INTERNAL_ERROR", "FORMAT: VM sin workdir");
                return;
            }
            java.nio.file.Path root = mm.getWorkdir();
            // Walk en post-orden para poder borrar dirs después de su contenido.
            try (java.util.stream.Stream<java.nio.file.Path> walk = java.nio.file.Files.walk(root)) {
                walk.sorted(java.util.Comparator.reverseOrder())
                    .filter(p -> !p.equals(root))   // mantener el workdir
                    .forEach(p -> {
                        try { java.nio.file.Files.delete(p); }
                        catch (java.io.IOException ignored) { /* best effort */ }
                    });
            }
            sendReply(id, "FORMAT_REPLY", "");
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "FORMAT: " + e.getMessage());
        }
    }

    private void sendDfReply(long id) {
        try {
            ModuleManager mm = vm.getModuleManager();
            if (mm == null || mm.getWorkdir() == null) {
                // Sin workdir devolvemos un DF "neutro" para que el cliente
                // no tenga que tratar el caso como error.
                sendReply(id, "DF_REPLY",
                        "\"totalBytes\":0,\"usedBytes\":0,\"freeBytes\":0,\"fileCount\":0");
                return;
            }
            java.nio.file.Path root = mm.getWorkdir();
            long[] used = {0};
            long[] count = {0};
            try (java.util.stream.Stream<java.nio.file.Path> walk = java.nio.file.Files.walk(root)) {
                walk.filter(java.nio.file.Files::isRegularFile)
                    .forEach(p -> {
                        try { used[0] += java.nio.file.Files.size(p); count[0]++; }
                        catch (java.io.IOException ignored) {}
                    });
            }
            java.nio.file.FileStore fs = java.nio.file.Files.getFileStore(root);
            long total = fs.getTotalSpace();
            long free  = fs.getUsableSpace();
            sendReply(id, "DF_REPLY",
                    "\"totalBytes\":" + total
                    + ",\"usedBytes\":" + used[0]
                    + ",\"freeBytes\":" + free
                    + ",\"fileCount\":" + count[0]);
        } catch (java.io.IOException e) {
            sendError(id, "INTERNAL_ERROR", "DF: " + e.getMessage());
        }
    }

    private void sendInfoReply(long id) {
        long uptimeMs = System.currentTimeMillis() - startMs;
        long fsTotal = 0, fsUsed = 0;
        try {
            ModuleManager mm = vm.getModuleManager();
            if (mm != null && mm.getWorkdir() != null) {
                java.nio.file.Path root = mm.getWorkdir();
                java.nio.file.FileStore fsx = java.nio.file.Files.getFileStore(root);
                fsTotal = fsx.getTotalSpace();
                long[] used = {0};
                try (java.util.stream.Stream<java.nio.file.Path> walk = java.nio.file.Files.walk(root)) {
                    walk.filter(java.nio.file.Files::isRegularFile)
                        .forEach(p -> {
                            try { used[0] += java.nio.file.Files.size(p); }
                            catch (java.io.IOException ignored) {}
                        });
                }
                fsUsed = used[0];
            }
        } catch (java.io.IOException ignored) { /* mantenemos 0 */ }
        String payload = "\"uniqueId\":\"" + uniqueId + "\""
                + ",\"boardName\":\"java-host\""
                + ",\"cpuFreqHz\":0"
                + ",\"uptimeMs\":" + uptimeMs
                + ",\"tempC\":0"
                + ",\"fsTotalBytes\":" + fsTotal
                + ",\"fsUsedBytes\":" + fsUsed;
        sendReply(id, "INFO_REPLY", payload);
    }

    private void sendResetReply(long id) {
        // v1: "El servidor responde antes de rebootear; la conexión se cae
        // poco después." En la VM Java, "reboot" = salir del proceso. El
        // IDE puede reconectarse si rearranca el daemon.
        sendReply(id, "RESET_REPLY", "");
        // Pequeño delay para que el reply salga del socket antes de
        // matar el JVM. Daemon thread para no bloquear el reader.
        Thread shutdown = new Thread(() -> {
            try { Thread.sleep(100); } catch (InterruptedException ignored) {}
            System.exit(0);
        }, "bp-debug-reset");
        shutdown.setDaemon(true);
        shutdown.start();
    }

    /** PR-5 — INSPECT mínimo: si ref apunta a un string BP, devuelve su
     *  valor. Si no, devuelve `class:"?"` y el i32 leído en esa dirección.
     *  La versión completa con walk de campos llegará cuando la VM exponga
     *  la información del object header / class descriptor de forma
     *  reutilizable (hoy esa lógica vive dispersa en VirtualMachine). */
    private void sendInspectReply(long id, long ref, int depth) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(id, "INTERNAL_ERROR", "INSPECT: VM no está pausada");
            return;
        }
        try {
            String s = vm.readStringIfPossible((int) ref);
            if (s != null) {
                sendReply(id, "INSPECT_REPLY",
                        "\"class\":\"string\""
                        + ",\"value\":" + Json.quote(s));
                return;
            }
            // Fallback: leer un int crudo de la dirección.
            int rawInt = vm.readMemoryInt((int) ref);
            sendReply(id, "INSPECT_REPLY",
                    "\"class\":\"?\""
                    + ",\"ref\":" + ref
                    + ",\"rawInt\":" + rawInt
                    + ",\"depth\":" + depth);
        } catch (Throwable t) {
            sendError(id, "INTERNAL_ERROR", "INSPECT(" + ref + "): " + t.getMessage());
        }
    }

    /** Igual que DebugController.basenameOf — duplicado aquí para evitar
     *  hacer público el helper en el controller. */
    private static String basenameOf(String path) {
        if (path == null) return "?";
        int sep = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
        return (sep >= 0) ? path.substring(sep + 1) : path;
    }

    /** Genera un identificador único para esta instancia del proceso.
     *  No persistente entre runs; suficiente para que el IDE pueda
     *  detectar "es el mismo proceso?" durante la sesión. */
    private static String generateUniqueId() {
        // 16 hex chars random, prefijo "j" para distinguir de Pico.
        long r = java.util.concurrent.ThreadLocalRandom.current().nextLong();
        return "j" + String.format("%016x", r);
    }

    @Override public void close() {
        closed = true;
        clientConnected.countDown();
        try { if (client != null)       client.close();       } catch (IOException ignored) {}
        try { if (serverSocket != null) serverSocket.close(); } catch (IOException ignored) {}
    }
}
