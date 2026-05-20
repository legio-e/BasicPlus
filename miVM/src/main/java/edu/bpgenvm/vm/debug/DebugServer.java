// ============================================================
// DebugServer.java
// Servidor TCP del lado VM. Escucha en un puerto, acepta UN cliente,
// y mantiene dos canales sobre el mismo socket:
//
//   - Saliente (VM → cliente): los eventos del DebugController y los
//     chunks de print del SocketSink. Cada mensaje es una línea JSON.
//
//   - Entrante (cliente → VM): líneas JSON con `{"cmd": "..."}`. Se
//     parsean y se traducen a invocaciones del DebugController.
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
// Protocolo wire — comandos aceptados HOY:
//   {"cmd":"setBreakpoint","file":"x.bp","line":10,"enabled":true}
//   {"cmd":"clearAllBreakpoints"}
//   {"cmd":"continue"}
//   {"cmd":"stepInto"} / "stepOver" / "stepOut" / "stop"
//   {"cmd":"hello"}    → opcional, devuelve {"type":"hello","vm":"bpgenvm/1.0"}
//
// Eventos emitidos:
//   {"type":"hello","vm":"bpgenvm/1.0"}    al conectar
//   {"type":"print","data":"..."}          de SocketSink
//   {"type":"paused", ...}
//   {"type":"resumed","tid":N}
//   {"type":"exited","exitCode":N,"reason":"..."}
//   {"type":"exception","tid":N,"message":"...","stackTrace":"..."}
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

    public static final String SERVER_BANNER = "bpgenvm/1.0";

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

            // Conectar sink + listener.
            SocketSink sink = new SocketSink(clientOut);
            vm.setProgramOut(sink);
            controller.addListener(this::onEvent);

            // Handshake: anunciamos versión / capacidades.
            send("{\"type\":\"hello\",\"vm\":\"" + SERVER_BANNER + "\"}");

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
                send("{\"type\":\"paused\",\"tid\":" + e.tid
                        + ",\"absPc\":" + e.absPc
                        + ",\"line\":" + e.line
                        + ",\"file\":\"" + Json.escape(e.sourceFile == null ? "" : e.sourceFile) + "\""
                        + ",\"bp\":" + e.bp + ",\"sp\":" + e.sp + ",\"cs\":" + e.cs
                        + ",\"stackBase\":" + e.stackBase + "}");
            } else if (ev instanceof ResumedEvent) {
                ResumedEvent e = (ResumedEvent) ev;
                send("{\"type\":\"resumed\",\"tid\":" + e.tid + "}");
            } else if (ev instanceof ExitedEvent) {
                ExitedEvent e = (ExitedEvent) ev;
                send("{\"type\":\"exited\",\"exitCode\":" + e.exitCode
                        + ",\"reason\":\"" + Json.escape(e.reason) + "\"}");
            } else if (ev instanceof ExceptionEvent) {
                ExceptionEvent e = (ExceptionEvent) ev;
                send("{\"type\":\"exception\",\"tid\":" + e.tid
                        + ",\"message\":\"" + Json.escape(e.message) + "\""
                        + ",\"stackTrace\":\"" + Json.escape(e.stackTrace) + "\"}");
            } else {
                // Tipo desconocido: lo envolvemos como "unknown" para no
                // perderlo si añadimos eventos nuevos sin tocar este servidor.
                send("{\"type\":\"" + Json.escape(ev.type()) + "\"}");
            }
        } catch (Throwable t) {
            System.err.println("[DebugServer] error emitiendo evento: " + t.getMessage());
        }
    }

    /** Visible para tests: invoca el mismo camino que toma un evento que
     *  llega por el listener. Usar sólo desde test code. */
    public void onEventForTest(DebugEvent ev) { onEvent(ev); }

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
                    handleCommand(line);
                } catch (Throwable t) {
                    System.err.println("[DebugServer] comando ignorado: " + t.getMessage()
                            + "  (línea: " + line + ")");
                }
            }
        } catch (IOException e) {
            if (!closed)
                System.err.println("[DebugServer] readLoop terminó: " + e.getMessage());
        } finally {
            System.err.println("[DebugServer] cliente desconectado");
            // No mata la VM: sigue ejecutando headless.
            this.client = null;
            this.clientOut = null;
            this.clientIn = null;
        }
    }

    private void handleCommand(String jsonLine) {
        Map<String, Object> m = Json.parseFlatObject(jsonLine);
        // Requests (request/response) llegan con el campo "req" en lugar de "cmd".
        if (m.get("req") instanceof String) {
            handleRequest(m);
            return;
        }
        String cmd = Json.getString(m, "cmd", "");
        switch (cmd) {
            case "setBreakpoint": {
                String file = Json.getString(m, "file", "");
                int line = (int) Json.getLong(m, "line", 0);
                boolean enabled = Json.getBool(m, "enabled", true);
                controller.setBreakpoint(file, line, enabled);
                break;
            }
            case "clearAllBreakpoints":
                controller.clearAllBreakpoints();
                break;
            case "continue":   controller.sendCommand(StepCommand.CONTINUE); break;
            case "stepInto":   controller.sendCommand(StepCommand.STEP_INTO); break;
            case "stepOver":   controller.sendCommand(StepCommand.STEP_OVER); break;
            case "stepOut":    controller.sendCommand(StepCommand.STEP_OUT); break;
            case "stop":       controller.sendCommand(StepCommand.STOP); break;
            case "hello":
                send("{\"type\":\"hello\",\"vm\":\"" + SERVER_BANNER + "\"}");
                break;
            default:
                System.err.println("[DebugServer] comando desconocido: '" + cmd + "'");
        }
    }

    // ============================================================
    // Requests (request/response sobre el mismo socket)
    // ============================================================

    private void handleRequest(Map<String, Object> m) {
        String req = Json.getString(m, "req", "");
        long reqId = Json.getLong(m, "requestId", -1);
        try {
            switch (req) {
                case "getLocals":         sendLocalsResponse(reqId); break;
                case "stackFrames":       sendStackFramesResponse(reqId); break;
                case "moduleProperties":  sendModulePropertiesResponse(reqId); break;
                case "readInt":           sendReadIntResponse(reqId, m); break;
                case "readString":        sendReadStringResponse(reqId, m); break;
                default:
                    sendError(reqId, "req desconocido: " + req);
            }
        } catch (Throwable t) {
            sendError(reqId, "error procesando '" + req + "': " + t.getMessage());
        }
    }

    private void sendError(long requestId, String message) {
        send("{\"resp\":\"error\",\"requestId\":" + requestId
                + ",\"message\":" + Json.quote(message) + "}");
    }

    private void sendLocalsResponse(long requestId) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(requestId, "getLocals: VM no está pausada");
            return;
        }
        // Locals = slots i32 entre bp y sp (no incluye saved PC/BP/CS bajo bp).
        int nLocals = Math.max(0, (ctx.sp - ctx.bp) / 4);
        StringBuilder sb = new StringBuilder();
        sb.append("{\"resp\":\"getLocals\",\"requestId\":").append(requestId);
        sb.append(",\"tid\":").append(ctx.tid);
        sb.append(",\"locals\":[");
        for (int i = 0; i < nLocals; i++) {
            if (i > 0) sb.append(',');
            sb.append(ctx.readLocal(i * 4));
        }
        sb.append("]}");
        send(sb.toString());
    }

    private void sendStackFramesResponse(long requestId) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(requestId, "stackFrames: VM no está pausada");
            return;
        }
        List<int[]> frames = ctx.stackFrames();
        StringBuilder sb = new StringBuilder();
        sb.append("{\"resp\":\"stackFrames\",\"requestId\":").append(requestId);
        sb.append(",\"tid\":").append(ctx.tid);
        sb.append(",\"frames\":[");
        for (int i = 0; i < frames.size(); i++) {
            if (i > 0) sb.append(',');
            int[] f = frames.get(i);
            sb.append("[").append(f[0]).append(',').append(f[1]).append(']');
        }
        sb.append("]}");
        send(sb.toString());
    }

    private void sendModulePropertiesResponse(long requestId) {
        DebugContext ctx = controller.currentContext();
        if (ctx == null) {
            sendError(requestId, "moduleProperties: VM no está pausada");
            return;
        }
        List<ModuleManager.PropertyView> props = ctx.moduleProperties();
        StringBuilder sb = new StringBuilder();
        sb.append("{\"resp\":\"moduleProperties\",\"requestId\":").append(requestId);
        sb.append(",\"props\":[");
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
        sb.append("]}");
        send(sb.toString());
    }

    private void sendReadIntResponse(long requestId, Map<String, Object> m) {
        // Para leer una dirección absoluta de memoria de la VM. Útil para
        // inspectores avanzados; no se usa hoy desde el IDE.
        long addr = Json.getLong(m, "addr", -1);
        int value;
        try {
            value = vm.readMemoryInt((int) addr);
        } catch (Throwable t) {
            sendError(requestId, "readInt(" + addr + "): " + t.getMessage());
            return;
        }
        send("{\"resp\":\"readInt\",\"requestId\":" + requestId
                + ",\"value\":" + value + "}");
    }

    private void sendReadStringResponse(long requestId, Map<String, Object> m) {
        // Si ref apunta a un VM string, devuelve su contenido; si no, "".
        long ref = Json.getLong(m, "ref", 0);
        String s;
        try {
            s = vm.readStringIfPossible((int) ref);
        } catch (Throwable t) {
            sendError(requestId, "readString(" + ref + "): " + t.getMessage());
            return;
        }
        send("{\"resp\":\"readString\",\"requestId\":" + requestId
                + ",\"value\":" + Json.quote(s == null ? "" : s) + "}");
    }

    @Override public void close() {
        closed = true;
        clientConnected.countDown();
        try { if (client != null)       client.close();       } catch (IOException ignored) {}
        try { if (serverSocket != null) serverSocket.close(); } catch (IOException ignored) {}
    }
}
