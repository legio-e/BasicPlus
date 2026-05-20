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
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

public final class VmClient implements AutoCloseable {

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

    private volatile boolean closed = false;

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
        int port = reserveFreePort();
        spawnVmProcess(port, modOrProjectPath, waitClient);
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
    // Internos
    // ============================================================

    private static int reserveFreePort() throws IOException {
        try (ServerSocket s = new ServerSocket(0)) {
            return s.getLocalPort();
        }
    }

    private void spawnVmProcess(int port, String modOrProject, boolean waitClient) throws IOException {
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
        argv.add(modOrProject);

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
        if (l != null) {
            try { l.onEvent(ev); }
            catch (Throwable t) { diag("[VmClient] listener: " + t.getMessage()); }
        }
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
