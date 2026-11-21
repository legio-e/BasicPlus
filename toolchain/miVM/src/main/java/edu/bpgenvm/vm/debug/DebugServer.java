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
//      headless) y el servidor VUELVE A ESCUCHAR: el siguiente cliente
//      se engancha con su HELLO, como en una placa (V7/H8 #617, 1-oct:
//      hasta entonces no reaceptaba, «se añadirá si se necesita
//      reattach en caliente», y hacía falta: el explorador del IDE se
//      conecta a una VM lanzada a mano, y tras un FATAL el protocolo
//      pide reconectar).
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
        boolean primero = true;
        while (!closed) {   // #617 — cuando el cliente se va, se vuelve a escuchar
            try {
                Socket s = serverSocket.accept();   // bloquea hasta el siguiente cliente
                if (closed) { try { s.close(); } catch (IOException ignored) {} return; }
                this.client = s;
                this.clientOutRaw = new BufferedOutputStream(s.getOutputStream());
                this.clientInRaw  = new BufferedInputStream(s.getInputStream());
                System.err.println("[DebugServer] cliente conectado desde " + s.getRemoteSocketAddress());

                // Conectar sink + listener (UNA vez: escriben en el cliente que haya en cada momento,
                // por send() y hasClient()). El SocketSink emite OUTPUT events a través de nuestro
                // send(jsonLine), garantizando sincronización con el resto de tráfico. La sessionId se
                // lee perezosamente en cada flush — refleja la sesión activa en ese momento (0 = sin
                // sesión).
                if (primero) {
                    SocketSink sink = new SocketSink(this::send, () -> activeSession);
                    vm.setProgramOut(sink);
                    controller.addListener(this::onEvent);
                    primero = false;
                }
                vm.setPromptSender(this.promptSender);   // el readLoop lo quita al irse el cliente

                // v1: NO mandamos banner. El cliente debe enviar HELLO y
                // recibir HELLO_REPLY como primer mensaje (§5.1 del wire spec).

                // Arrancar el thread de lectura de comandos, y esperar a que el cliente se vaya.
                readerThread = new Thread(this::readLoop, "bp-debug-reader");
                readerThread.setDaemon(true);
                readerThread.start();

                clientConnected.countDown();
                readerThread.join();
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                return;
            } catch (IOException e) {
                if (!closed)
                    System.err.println("[DebugServer] accept falló: " + e.getMessage());
                return;
            }
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
                // V7/H2 (#523, 17-sep) — un fallo de ENLACE sale como en el device (repl_v1.c):
                // status LINK_ERROR y errorMessage con el texto pelado del cargador, sin el
                // prefijo «link: » del exitReason del CLI. Antes iba como RUNTIME_ERROR con el
                // prefijo, y el IDE enseñaba dos cosas distintas para el mismo fallo.
                boolean enlace = e.reason != null && e.reason.startsWith("link: ");
                String status = (e.exitCode == 0)   ? "OK"
                              : (e.exitCode == 130) ? "KILLED"     // P-run-stop
                              : enlace              ? "LINK_ERROR"
                                                    : "RUNTIME_ERROR";
                String mensaje = enlace ? e.reason.substring("link: ".length()) : e.reason;
                StringBuilder sb = new StringBuilder(128);
                sb.append("{\"type\":\"EXITED\",\"session\":").append(sess)
                  .append(",\"status\":\"").append(status).append('"')
                  .append(",\"exitCode\":").append(e.exitCode)
                  .append(",\"elapsedMs\":").append(elapsed);
                // V8 #582 — antes del mensaje, como la placa (bpvm_repl.c: el mensaje es lo que se recorta).
                if (!e.stackTrace.isEmpty()) {
                    sb.append(",\"stackTrace\":\"").append(Json.escape(e.stackTrace)).append('"');
                }
                if (mensaje != null && !mensaje.isEmpty()) {
                    sb.append(",\"errorMessage\":\"").append(Json.escape(mensaje)).append('"');
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
                // EXITED.status="RUNTIME_ERROR" + errorMessage, y dónde, en
                // stackTrace, como en la placa (V8 #582). El
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
                  .append(",\"elapsedMs\":").append(elapsed);
                if (e.stackTrace != null && !e.stackTrace.isEmpty()) {
                    sb.append(",\"stackTrace\":\"").append(Json.escape(e.stackTrace)).append('"');
                }
                sb.append(",\"errorMessage\":\"").append(Json.escape(e.message));
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
            putAbortar("el cliente se fue");   // #617: lo que no termina, se borra (como en las placas)
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
        if (putSalida != null && ACABAN_LA_SUBIDA.contains(type))
            putAbortar("llego " + type);       // #617: la regla de las placas (bpvm_repl_antes_de_despachar)
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
                    // P-run-stop (#257) — kill REAL: la VM termina en el
                    // siguiente safepoint aunque esté corriendo a toda
                    // velocidad. El STOP del controller cubre además el
                    // caso "pausada en el hook del debugger".
                    vm.requestKill();
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
                case "PUT_BEGIN": sendPutBeginReply(id, m); break;   // #617: la subida por trozos (#294)
                case "PUT_DATA":  sendPutDataReply(id, m, bulk); break;
                case "PUT_END":   sendPutEndReply(id); break;
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
    /** V6/E1 — ARRANCA EL RELOJ DEL `elapsedMs`, con el módulo ya cargado y
     *  enlazado y justo antes de ejecutar. Lo llama {@code Main} en el punto
     *  exacto en que empieza el programa.
     *
     *  Por qué existe: el reloj se ponía en marcha al RECIBIR el RUN, así que el
     *  número incluía cargar y enlazar y no medía lo mismo que en las placas,
     *  que cronometran sólo la ejecución. Un campo, un significado.
     *
     *  📐 CUÁNTO ERA, MEDIDO — y es mucho menos de lo que yo dije: instrumentando
     *  este mismo método, el reloj se adelanta **7 ms** con `Bench.mod`. O sea
     *  que el sesgo existía pero era despreciable; este cambio es corrección de
     *  significado, no de cifra.
     *
     *  ⚠️ Y las dos trampas del experimento que lo midió, que casi cuelo como
     *  hallazgos:
     *   1. El primer intento daba ~230 ms de sesgo. Eran **la latencia de mi
     *      propio arnés** contestando al breakpoint de entrada (sondeaba cada
     *      200 ms). Bajando el sondeo a 5 ms, se evaporaron.
     *   2. La diferencia gorda entre las dos VMs con este módulo —3,8 s en Java
     *      contra 90 ms en C— **no** es este sesgo: es que la VM-Java interpreta
     *      más despacio. Lo que separa una cosa de otra es el cronómetro DEL
     *      PROGRAMA (`Pico.uptimeMs()` dentro del propio `Bench.bp`), que no
     *      depende ni del arnés ni del campo. */
    public void marcaInicioEjecucion() {
        this.activeSessionStartMs = System.currentTimeMillis();
    }

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
        // #589 (V7, 27-sep) — el MISMO render que el host aplica a un device (DeviceFrameResolver), con un
        // lector de la memoria de esta VM. Aquí había una copia que se separó: la cadena pasaba por
        // readStringIfPossible, que quiere una DIRECCIÓN y devuelve el texto YA entre comillas —una cadena del
        // heap salía «"@40000004"» y un literal, «""literal""»—.
        return DeviceFrameResolver.render(v, value, new DeviceFrameResolver.MemReader() {
            @Override public int readI32(int addr) { return vm.readMemoryInt(addr); }
            @Override public String readString(int ref) { return vm.leeCadenaDeRef(ref); }
        });
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
        // #589 (V7, 27-sep) — ESPEJO del READ_STRING de la VM-C (`dbg_read_string`): `ref` es una REFERENCIA
        // —un handle o la de un literal—, no una dirección; 0 (o menos) es la cadena vacía; una que no resuelve,
        // BAD_REF; y el valor, el texto TAL CUAL. Antes: sin la indirección («@40000004» para una cadena del heap)
        // y con las comillas dentro del valor.
        if (ref <= 0) {
            sendReply(id, "READ_STRING_REPLY", "\"value\":\"\"");
            return;
        }
        String s = vm.leeCadenaDeRef(ref);
        if (s == null) {
            sendError(id, "BAD_REF", "ref " + ref + " no resuelve a una cadena (muerta, fuera de la tabla o de la memoria)");
            return;
        }
        sendReply(id, "READ_STRING_REPLY", "\"value\":" + Json.quote(s));
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

    /* V7/H8 #617 — LA SUBIDA POR TROZOS (PUT_BEGIN / PUT_DATA / PUT_END, #294), con la regla de las placas
     * (bpvm_repl.c): el fichero, abierto toda la sesión; lo que no termina se BORRA (Eduardo, 1-oct: «si queda la
     * versión anterior el usuario se va a pensar que tiene el nuevo»), y la acaban los mismos verbos y el cliente que
     * se va. El IDE sube así todo lo que pasa de 8 KB, y esta VM sólo conocía el PUT de una pieza. La usa sólo el
     * hilo lector (handleMessage), y su final, el mismo hilo. */
    private static final java.util.Set<String> ACABAN_LA_SUBIDA = new java.util.HashSet<>(java.util.Arrays.asList(
            "HELLO", "RUN", "RESET", "PUT", "PUT_BEGIN", "DEL", "MKDIR", "RMDIR", "RENAME", "FORMAT", "SAVE"));
    private java.io.OutputStream putSalida;      // el fichero abierto; null = no hay subida
    private java.nio.file.Path    putRuta;
    private long                  putEsperado, putRecibido;

    private void putBorrar(String porQue) {
        boolean borrado;
        try { borrado = java.nio.file.Files.deleteIfExists(putRuta); } catch (Throwable t) { borrado = false; }
        System.err.println("[DebugServer] put: " + putRuta + " no termino (" + putRecibido + " de " + putEsperado
                + " B, " + porQue + "): " + (borrado ? "borrado" : "NO SE PUDO BORRAR"));
    }

    private void putAbortar(String porQue) {
        if (putSalida == null) return;
        try { putSalida.close(); } catch (Throwable ignored) { }
        putSalida = null;
        putBorrar(porQue);
    }

    private void sendPutBeginReply(long id, Map<String, Object> m) {
        String path = Json.getString(m, "path", "");
        if (path.isEmpty()) {
            sendError(id, "INVALID_PARAM", "PUT_BEGIN: falta 'path'"); return;
        }
        putAbortar("llego otra subida");
        try {
            java.nio.file.Path dst = workdirPath(path);
            java.nio.file.Path parent = dst.getParent();
            if (parent != null) java.nio.file.Files.createDirectories(parent);
            putSalida   = new BufferedOutputStream(java.nio.file.Files.newOutputStream(dst));   // crea o trunca
            putRuta     = dst;
            putEsperado = Json.getLong(m, "size", 0);
            putRecibido = 0;
            sendReply(id, "PUT_BEGIN_REPLY", "\"received\":0");
        } catch (Throwable e) {
            sendError(id, codeFor(e), "PUT_BEGIN: " + e.getMessage());
        }
    }

    private void sendPutDataReply(long id, Map<String, Object> m, byte[] bulk) {
        if (!m.containsKey("bulk")) {
            sendError(id, "INVALID_PARAM", "PUT_DATA: falta bulk"); return;
        }
        if (putSalida == null) {   // el bulk ya está leído (readLoop): el cable sigue en sincronía
            sendError(id, "NO_SESSION", "PUT_DATA sin PUT_BEGIN"); return;
        }
        byte[] datos = (bulk != null) ? bulk : new byte[0];
        try {
            putSalida.write(datos);
            putRecibido += datos.length;
            sendReply(id, "PUT_DATA_REPLY", "\"received\":" + putRecibido);
        } catch (Throwable e) {
            putAbortar("no se pudo escribir");
            sendError(id, codeFor(e), "PUT_DATA: " + e.getMessage());
        }
    }

    private void sendPutEndReply(long id) {
        if (putSalida == null) {
            sendError(id, "NO_SESSION", "PUT_END sin PUT_BEGIN"); return;
        }
        boolean falla = false;
        try { putSalida.close(); } catch (Throwable e) { falla = true; }
        putSalida = null;
        if (falla) {
            putBorrar("no se pudo escribir el final");
            sendError(id, "INTERNAL_ERROR", "PUT_END: no se pudo cerrar el fichero"); return;
        }
        if (putEsperado != 0 && putRecibido != putEsperado) {
            putBorrar("el tamano no cuadra");   // lo que no está entero, no se queda
            sendError(id, "SIZE_MISMATCH", "bytes recibidos != size anunciado"); return;
        }
        sendReply(id, "PUT_END_REPLY", "\"size\":" + putRecibido);
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
            if (Json.getBool(m, "chunked", false)) {
                /* V7/H8 #495 (paso 4) — el GET TROCEADO, con la forma de la VM-C: GET_PART (off y un bulk de 512 B como
                 * mucho) … GET_END (size y el CRC-32 de lo mandado). Cada trozo, un mensaje entero bajo writeLock: la
                 * salida del programa puede ir entre medias. */
                final int trozo = 512;
                for (int off = 0; off < data.length; off += trozo) {
                    int n = Math.min(trozo, data.length - off);
                    sendFrame("{\"type\":\"GET_PART\",\"id\":" + id + ",\"off\":" + off + ",\"bulk\":" + n + "}",
                              java.util.Arrays.copyOfRange(data, off, off + n));
                }
                java.util.zip.CRC32 crc = new java.util.zip.CRC32();
                crc.update(data, 0, data.length);
                sendReply(id, "GET_END", "\"size\":" + data.length + ",\"crc\":" + crc.getValue());
                return;
            }
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
                    /* V7/H8 E1 — un .mod que esta VM no cargaría lleva el motivo (el rojo del árbol del IDE), con la
                     * regla y los textos de la VM-C. Sólo el .mod: es lo único que carga la VM-Java (ni .mdn ni packs). */
                    if (!isDir && p.getFileName().toString().endsWith(".mod")) {
                        String mal = edu.bpgenvm.bytecode.ModFormat.examina(java.nio.file.Files.readAllBytes(p));
                        if (mal != null) sb.append(",\"mal\":").append(Json.quote(mal));
                    }
                    sb.append('}');
                }
            }
            sb.append("]");
            if (Json.getBool(m, "chunked", false)) {
                /* V7/H8 #495 (paso 4) — el LIST TROCEADO, con la forma de la VM-C: aquí, un directorio, una parte. */
                sendReply(id, "LIST_PART", sb.toString());
                sendReply(id, "LIST_END", "\"omitted\":0");
                return;
            }
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
            // V7/H9 #624 — como la placa: de un módulo, su formato (el MAGIC) y, desde MOD8, su versión (`gen`, +36)
            // y su interfaz (`ifaceCrc`, +40), de la cabecera; y el CRC si se pide. Con eso decide quien sube.
            StringBuilder modulo = new StringBuilder();
            if (!isDir) {
                byte[] h = new byte[44];
                int n = 0;
                try (java.io.InputStream in = java.nio.file.Files.newInputStream(p)) {
                    int r;
                    while (n < h.length && (r = in.read(h, n, h.length - n)) > 0) n += r;
                }
                if (n >= 4 && h[0] == 'M' && h[1] == 'O' && h[2] == 'D') {
                    modulo.append(",\"magic\":\"").append((char) h[0]).append((char) h[1]).append((char) h[2])
                          .append((char) h[3]).append('"');
                    if (n >= 44 && (h[3] == '8' || h[3] == '9')) {
                        java.nio.ByteBuffer bb = java.nio.ByteBuffer.wrap(h);   // big-endian, como el .mod
                        modulo.append(",\"gen\":").append(bb.getInt(36) & 0xFFFFFFFFL)
                              .append(",\"ifaceCrc\":").append(bb.getInt(40) & 0xFFFFFFFFL);
                    }
                }
                if (Json.getBool(m, "crc", false)) {
                    java.util.zip.CRC32 c = new java.util.zip.CRC32();
                    c.update(java.nio.file.Files.readAllBytes(p));
                    modulo.append(",\"crc\":").append(c.getValue());
                }
            }
            sendReply(id, "STAT_REPLY",
                    "\"size\":" + size
                    + ",\"isDir\":" + (isDir ? "true" : "false")
                    + ",\"mtime\":" + mtime
                    + modulo);
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
            // #589 — con la indirección (antes, un handle salía «@40000004» como si fuera el texto) y sólo si por
            // su forma puede ser una cadena: un objeto no se lee como texto.
            String s = vm.puedeSerCadena(ref) ? vm.leeCadenaDeRef(ref) : null;
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
