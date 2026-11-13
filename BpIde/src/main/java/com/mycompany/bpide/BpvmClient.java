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
import edu.bpgenvm.vm.debug.DbgFile;
import edu.bpgenvm.vm.debug.DebugListener;
import edu.bpgenvm.vm.debug.DeviceFrameResolver;
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
import java.io.FilterInputStream;
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

    /* #454 — LATIDO DEL CABLE: cuándo llegó el último byte de la placa.
     *
     * El timeout de `sendRequest` era un plazo ÚNICO para la respuesta entera, y
     * eso mide lo que no debe. Un timeout sirve para detectar que el otro lado
     * NO ESTÁ; acotar cuánto puede tardar una transferencia es otra cosa, y
     * ponerle un número fijo la rompe en cuanto el fichero crece.
     *
     * Se vio con dos ficheros del propio proyecto sobre un S3 (UART 115200):
     * uno de 9 KB abre y uno de 120 KB no — 120.763 B a 115200 son 10,5 s y el
     * plazo eran 10,0. Y lo que lo dejó claro fue que un PACK de ~580 KB SÍ se
     * graba: tarda ~50 s, pero va por PUT_BEGIN/DATA/END, o sea N peticiones
     * con SU plazo cada una. El camino de SUBIDA ya tenía resuelto esto; el de
     * bajada, que es un `GET` de una pieza, no lo recibió.
     *
     * Criterio de Eduardo: «el timeout se suele configurar por trama, no por
     * tiempo total del traspaso de un archivo; sirve para detectar conexiones
     * que se han caído». Con esto un fichero de 10 MB por un cable lento va
     * bien mientras sigan llegando bytes, y un cable muerto se detecta igual de
     * rápido aunque el fichero sea diminuto. */
    private volatile long lastRxMillis = System.currentTimeMillis();

    /** Envuelve el stream del wire para sellar la hora de CADA byte que entra.
     *  Va aquí y no en el bucle lector porque el cuerpo de un `bulk` se lee
     *  dentro de `recvBulk`, que no vuelve hasta tenerlo entero: sellar sólo por
     *  frame dejaría los 120 KB sin dar señales de vida. */
    private InputStream conLatido(InputStream d) {
        return new FilterInputStream(d) {
            @Override public int read() throws IOException {
                int c = super.read();
                if (c >= 0) lastRxMillis = System.currentTimeMillis();
                return c;
            }
            @Override public int read(byte[] b, int off, int len) throws IOException {
                int n = super.read(b, off, len);
                if (n > 0) lastRxMillis = System.currentTimeMillis();
                return n;
            }
        };
    }

    private Thread readerThread;
    private Thread stderrPumpThread;
    /** Latch que cierra cuando llega HELLO_REPLY (handshake completo). */
    private final CountDownLatch helloLatch = new CountDownLatch(1);
    /** Build/version reportado por el server en HELLO_REPLY. */
    private volatile String serverBuild;
    /** V7/H8 #495 — el trozo y la ventana que la placa ANUNCIA en el HELLO_REPLY (`putChunk`, `putWindow`): se
     *  sube en trozos de ese tamaño con hasta esa cantidad sin acuse. 0 = no los anuncia (un firmware anterior, la
     *  VM-Java): se sube como siempre, en trozos de PUT_STREAM_CHUNK y con un acuse por trozo. */
    private volatile int putTrozo = 0;
    private volatile int putVentana = 0;
    /** #495 — la hora del último byte que SALE (la pone writeFrame): antes del segundo intento se espera a que el
     *  cable lleve callado, en los dos sentidos, más que el plazo de bulk estancado de la placa. */
    private volatile long lastTxMillis = 0;
    /** #495 — cuántas subidas han necesitado el segundo intento (lo mira SubidaSmoke). */
    private volatile int segundosIntentos = 0;
    /** #495 — SÓLO para SubidaSmoke: en la PRÓXIMA subida por trozos, el trozo con este índice sale con 16 bytes
     *  menos de los que anuncia —un cable que pierde bytes—, para forzar el segundo intento. -1 = nada. */
    static volatile int pruebaRecortarTrozo = -1;
    /** PR-3 — sessionId activa (0 = ninguna). La aprendemos de RUN_REPLY o
     *  del campo `session` de cualquier evento entrante. La usamos para
     *  componer KILL { session: N }. */
    private volatile long currentSession = 0;

    /** V8 #693 (29-oct) — el `code` del ERROR SIN id con el que la placa dice que tiró una línea que le llegó a medias
     *  (bpvm_repl_linea_estancada): la orden no se ejecutó, y se repite una vez sin decir nada (Eduardo: «si un paquete
     *  falla se puede reintentar y el usuario ni se entera. Pero si la comunicación no es posible hay que avisar»). */
    public static final String LINEA_ESTANCADA = "LINE_TIMEOUT";
    /** V8 #693 — la espera de la PRIMERA vez del RUN y del KILL: la placa los contesta enseguida y una línea a medias la
     *  dice a los 3 s (WIRE_V1_ESTANCADA_MS); 5 s de cable callado es que la orden o su respuesta se perdieron. */
    private static final long PRIMERA_ESPERA_MS = 5000L;
    /** V8 #693 — el id de la última línea que salió (writeFrame): la de un ERROR LINE_TIMEOUT, que no lleva id. */
    private volatile long ultimoIdEscrito = 0;
    /** V8 #693 — los OUTPUT y EXITED que llegan (sólo los cuenta el hilo lector): un RUN sin respuesta cuyo programa
     *  ya los manda, llegó. */
    private volatile long senalesDePrograma = 0;

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

    /** V8 #808 (13-nov) — la flash del PC que se le da a la VM que arranca este cliente (--flash-dir): donde van las
     *  rutas absolutas del programa ("/sys/x"). null: la que la VM tenga por defecto. La pone el IDE, que es quien
     *  sabe dónde corre el programa (CarpetaDelPrograma), antes de start/startDaemon. */
    private volatile String flashDir;
    public void setFlashDir(String dir)                    { this.flashDir = dir; }

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
            /* V8/H3 (6-nov): con plazo: una placa apagada o fuera de la red no contesta, y Windows esperaría 20 s. Y sin
             * Nagle: el wire son mensajes cortos, y por la WiFi cada espera se nota. */
            this.socket = new java.net.Socket();
            socket.connect(new java.net.InetSocketAddress(host, port), 5000);
            socket.setTcpNoDelay(true);
            this.outRaw = new BufferedOutputStream(socket.getOutputStream());
            this.inRaw  = conLatido(new BufferedInputStream(socket.getInputStream()));
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
            // RTS=true (no false): en la Pico (USB-CDC) el firmware mira DTR
            // e ignora RTS, así que da igual. Pero en placas ESP32 (DevKitC),
            // DTR/RTS van al circuito de auto-reset (DTR→GPIO0, RTS→EN). Con
            // DTR=1/RTS=0, GPIO0 queda en bajo: si el open mete un pulso de
            // reset, el chip arranca en DOWNLOAD y el REPL no corre. Con
            // DTR=1/RTS=1 (iguales) NO se resetea al abrir, y si se reseteara
            // arrancaría normal (GPIO0 alto). Seguro para ambos targets.
            port.setRTS(true);
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
            cerrarPuerto(port);
            this.serialPort = null;
            throw new IOException("drain inicial falló: " + ioe.getMessage(), ioe);
        }

        this.inRaw  = conLatido(new SerialLoQueHay(new SerialBlockingInputStream(rawIn)));
        this.outRaw = rawOut;   // raw — purejavacomm a veces se atraganta con buffered
        startReaderThread();
        doHandshake();
    }

    /** #618 — LEER DEL PUERTO LO QUE HAYA, de una vez. `WireFraming.recvLine` lee byte a byte, y con purejavacomm
     *  leer de un byte en un puerto que recibe deprisa PIERDE bytes: en la C5 (USB-Serial-JTAG, ~0,6 MB/s) la bajada
     *  troceada de #495 —128 cabeceras leídas así, con los trozos llegando detrás— se descolocaba a la primera,
     *  siempre, y con Python (que lee lo que hay) llegaba entera. Pedir un bloque grande tampoco vale: purejavacomm
     *  espera a que llegue más antes de devolver, y por una UART cada respuesta tardaba ~70 ms más (la subida de 64 KB
     *  a la P4, de 6,4 a 14,7 s). Así que se pide justo lo que hay disponible (y 1 si no hay nada: entonces no hay
     *  prisa ni nada que perder), y se sirve desde aquí. */
    private static final class SerialLoQueHay extends InputStream {
        private final InputStream delegate;
        private final byte[] buf = new byte[8192];
        private int pos, lim;
        SerialLoQueHay(InputStream d) { this.delegate = d; }
        private boolean llenar() throws IOException {
            int hay;
            try { hay = delegate.available(); }
            catch (RuntimeException re) { hay = 0; }   // el espurio de jtermios (handleSerialFail): leer de uno, que lo aguanta
            int pide = Math.max(1, Math.min(buf.length, hay));
            int n = delegate.read(buf, 0, pide);
            if (n <= 0) return false;
            pos = 0;
            lim = n;
            return true;
        }
        @Override public int read() throws IOException {
            if (pos >= lim && !llenar()) return -1;
            return buf[pos++] & 0xFF;
        }
        @Override public int read(byte[] b, int off, int len) throws IOException {
            if (len == 0) return 0;
            if (pos >= lim && !llenar()) return -1;
            int k = Math.min(len, lim - pos);
            System.arraycopy(buf, pos, b, off, k);
            pos += k;
            return k;
        }
        @Override public int available() throws IOException { return (lim - pos) + delegate.available(); }
        @Override public void close() throws IOException { delegate.close(); }
    }

    /** Wrapper que convierte el -1-de-timeout de purejavacomm en "espera
     *  más" para que WireFraming.recvLine() no lo interprete como EOF.
     *  Solo close() (que pone closed=true y cierra el puerto) sale del
     *  bucle con un -1 "real". */
    private final class SerialBlockingInputStream extends InputStream {
        private final InputStream delegate;
        SerialBlockingInputStream(InputStream d) { this.delegate = d; }
        @Override public int read() throws IOException {
            int transient_ = 0;
            while (!closed) {
                int c;
                try { c = delegate.read(); }
                catch (IOException ioe) {
                    if (closed) return -1;
                    throw ioe;
                }
                catch (RuntimeException re) {
                    if (closed) return -1;
                    transient_ = handleSerialFail(re, transient_);
                    continue;
                }
                if (c >= 0) return c;
                // -1: timeout en purejavacomm (NO EOF real). Reintentar.
            }
            return -1;
        }
        @Override public int read(byte[] b, int off, int len) throws IOException {
            if (len == 0) return 0;
            int transient_ = 0;
            while (!closed) {
                int r;
                try { r = delegate.read(b, off, len); }
                catch (IOException ioe) {
                    if (closed) return -1;
                    throw ioe;
                }
                catch (RuntimeException re) {
                    if (closed) return -1;
                    transient_ = handleSerialFail(re, transient_);
                    continue;
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

    /**
     * purejavacomm/jtermios lanza una RuntimeException ({@code jtermios.windows.
     * JTermiosImpl$Fail}) cuando el {@code select()} del puerto falla. En Windows
     * eso ocurre a veces de forma ESPÚREA con el device sano (race en el wait del
     * handle), no sólo cuando el puerto muere. Antes el primer fallo mataba el
     * reader y abortaba el RUN. Aquí lo tratamos como transitorio: pequeño sleep
     * + reintento, hasta un tope; pasado el tope (puerto realmente caído)
     * propagamos como IOException para un cierre limpio.
     *
     * @return el contador de fallos consecutivos incrementado.
     * @throws IOException si supera el tope o la excepción no es de jtermios.
     */
    private int handleSerialFail(RuntimeException re, int consecutive) throws IOException {
        String cn = re.getClass().getName();
        boolean jtermios = cn.contains("jtermios") || cn.contains("JTermios");
        if (!jtermios || consecutive >= 200) {   // ~2s de fallos seguidos ⇒ puerto caído
            throw new IOException("lectura serie falló ("
                    + re.getClass().getSimpleName() + "): " + re.getMessage(), re);
        }
        if (consecutive == 0) {
            diag("[BpvmClient] select() serie falló (jtermios); reintentando — "
                    + re.getMessage());
        }
        try { Thread.sleep(10); }
        catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrumpido durante reintento serie", ie);
        }
        return consecutive + 1;
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
            Map<String, Object> resp = sendRequest("HELLO", HELLO_CAMPOS, null, 5000);
            aprenderHello(resp);
            diag("[BpvmClient] handshake con VM: " + Json.getString(resp, "serverName", "?")
                    + " " + serverBuild
                    + " protoVersion=" + Json.getLong(resp, "protoVersion", 0)
                    + (putTrozo > 0 ? " | subida: trozos de " + putTrozo + " B, ventana " + putVentana : ""));
        } catch (IOException e) {
            diag("[BpvmClient] handshake falló: " + e.getMessage());
        }
    }

    private static final String HELLO_CAMPOS =
            "\"protoVersion\":1,\"clientName\":\"BpIde\",\"clientBuild\":\"1.0\"";

    /** Lo que el HELLO_REPLY dice del server: su build y, desde #495, el trozo y la ventana de la subida. */
    private void aprenderHello(Map<String, Object> resp) {
        serverBuild = Json.getString(resp, "serverBuild", "?");
        putTrozo    = (int) Json.getLong(resp, "putChunk", 0);
        putVentana  = (int) Json.getLong(resp, "putWindow", 0);
    }

    /** Cierra el puerto serie bajando antes RTS y DESPUÉS DTR (23-sep).
     *
     *  Medido en la ESP32-C5 (USB-Serial-JTAG), con un arnés que abre como
     *  connectSerial: cerrar sin más —Windows suelta las dos líneas— o bajar
     *  DTR primero la deja en MODO DESCARGA (el cargador de la ROM): el
     *  firmware ni arranca hasta un reset a mano, y la pantalla sigue con la
     *  última imagen, así que parece viva. Con RTS primero sigue como estaba,
     *  y el programa lanzado, corriendo. Es la lógica de reinicio de esptool:
     *  DTR abajo con RTS arriba pide reinicio con la patilla de arranque
     *  marcada. En la Pico (USB-CDC) es lo mismo que cerrar: DTR abajo = no hay
     *  host. En los puentes USB-UART (S3, P4: RTS→EN, DTR→GPIO0), RTS primero
     *  tampoco toca el EN — razonado, sin medir. */
    private static void cerrarPuerto(SerialPort p) {
        try { p.setRTS(false); Thread.sleep(50); } catch (Throwable ignored) {}
        try { p.setDTR(false); Thread.sleep(50); } catch (Throwable ignored) {}
        try { p.close(); } catch (Throwable ignored) {}
    }

    /* V8/H3 (#803) — UNA CONEXIÓN QUE SE VA SIN DESPEDIRSE. Por el cable, quitar el USB da un error en el acto; por la
     * red, si la otra punta desaparece (se apaga su radio, se cae la red, se va la luz), TCP no avisa de nada, y el IDE se
     * quedaba esperando el fin del programa, con los botones en gris y sin decir nada (Eduardo, 7-nov). Es lo que pide el
     * protocolo (§8.6): tras un rato sin oír nada, y sin nada en vuelo, un PING, que las dos VMs contestan también con un
     * programa en marcha. Cualquier respuesta vale. Si no llega ni un byte, la conexión está muerta. */
    private static final long LATIDO_CALLADO_MS = 10_000L;
    private static final long LATIDO_PLAZO_MS   = 5_000L;

    private volatile boolean perdida = false;
    private volatile Runnable alPerderse;
    private final java.util.concurrent.atomic.AtomicBoolean avisada = new java.util.concurrent.atomic.AtomicBoolean();

    /** La conexión se ha ido sin que nadie la cerrara desde aquí. */
    public boolean conexionPerdida() { return perdida; }

    /** Se cerró desde aquí: Disconnect, un reset. */
    public boolean cerrado() { return closed; }

    /** Quién se entera de que se ha perdido: una vez, y desde otro hilo. Si ya se perdió, en el acto. */
    public void alPerderse(Runnable r) {
        alPerderse = r;
        if (perdida) avisa();
    }

    /** Empieza a vigilar la conexión: la de la red, porque por el cable no hace falta. */
    public void vigilarConexion() { vigilarConexion(LATIDO_CALLADO_MS, LATIDO_PLAZO_MS); }

    /** Con otros plazos, para LatidoSmoke. */
    void vigilarConexion(long calladoMs, long plazoMs) {
        Thread t = new Thread(() -> latir(calladoMs, plazoMs), "bp-vmclient-latido");
        t.setDaemon(true);
        t.start();
    }

    private void latir(long calladoMs, long plazoMs) {
        final long vuelta = Math.max(20L, Math.min(1000L, calladoMs / 4));
        while (!closed && !perdida) {
            try {
                Thread.sleep(vuelta);
            } catch (InterruptedException ie) {
                return;
            }
            if (System.currentTimeMillis() - lastRxMillis < calladoMs) continue;
            synchronized (pendingLock) {
                if (!pendingRequests.isEmpty()) continue;    // lo que está en vuelo tiene su plazo
            }
            try {
                pideUnaVez("PING", null, null, plazoMs);
            } catch (WireError we) {
                /* un ERROR también es una respuesta: ha contestado, que es lo que se preguntaba */
            } catch (IOException e) {
                seHaPerdido();
                return;
            }
        }
    }

    private void seHaPerdido() {
        if (closed) return;
        perdida = true;
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}   // despierta al lector
        avisa();
    }

    private void avisa() {
        Runnable r = alPerderse;
        if (r != null && avisada.compareAndSet(false, true)) r.run();
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
        if (serialPort != null) cerrarPuerto(serialPort);
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
     *  detrás. Atómico para el peer (todo bajo writeLock). `id`, el de la
     *  línea: el último que sale lo recuerda #693. */
    private void writeFrame(long id, String jsonLine, byte[] bulkOrNull) throws IOException {
        OutputStream w = this.outRaw;
        if (w == null) throw new IOException("BpvmClient cerrado o no conectado");
        synchronized (writeLock) {
            WireFraming.sendLine(w, jsonLine);
            if (bulkOrNull != null && bulkOrNull.length > 0) {
                WireFraming.sendBulk(w, bulkOrNull);
            }
            w.flush();
            lastTxMillis = System.currentTimeMillis();   /* #495 */
            ultimoIdEscrito = id;                        /* V8 #693 */
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
            writeFrame(id, sb.toString(), bulkOrNull);
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

    /**
     * H6.b.3.b — breakpoint por PC absoluto, para depurar una VM device-role
     * (server C / Pico) que NO conoce símbolos: el host convierte línea→pc con
     * el {@code .dbg} (pc = relPc + cs) y manda el pc crudo. Devuelve el bpId
     * que asigna el device (>0), o &lt;=0 si falló.
     */
    public int setBreakpointPc(int pc, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("SET_BP", "\"pc\":" + pc, null, timeoutMs);
        return (int) Json.getLong(resp, "bpId", -1);
    }

    /** H6.b.3.b — pide al device pausar en el próximo opcode (async). El BP_HIT
     *  llega por el event listener. Fire-and-forget. */
    public void requestPause() {
        sendOneShot("PAUSE", "", null);
    }

    /** A2.3 — Ordena al daemon arrancar el módulo `module`. Fire-and-forget:
     *  el EXITED event es la confirmación del fin de ejecución. */
    public void runModule(String module) { runModule(module, null); }

    /** V6/#412 — con ARGUMENTO DE EJECUCION. Viaja como campo ESCALAR `arg` del
     *  RUN (no `args:[]`: el mini-parser de las placas no sabe leer arrays
     *  anidados, y por eso esa forma que el doc prometia nunca existio). null o
     *  vacio = no se manda el campo, y en la placa manda el valor por defecto
     *  que declare `Main(arg: string := ...)`. */
    public void runModule(String module, String arg) {
        sendOneShot("RUN", camposRun(module, arg), null);
    }

    private static String camposRun(String module, String arg) {
        String campos = "\"path\":\"" + Json.escape(module) + "\"";
        if (arg != null && !arg.isEmpty()) campos += ",\"arg\":\"" + Json.escape(arg) + "\"";
        return campos;
    }

    /** V8 #694 (29-oct; Eduardo: «me parece bien que avise») — el RUN, ESPERANDO su respuesta. La placa manda el
     *  RUN_REPLY al poner el programa en marcha, o un ERROR de antes de arrancar: NOT_FOUND «fichero no existe», BUSY
     *  «ya hay una sesión RUN en curso», INVALID_PARAM. Por {@link #runModule} iba sin esperar: un RUN rechazado se tiraba
     *  («silencioso por diseño», abajo) y quien lo había lanzado esperaba el EXITED una hora. Devuelve la sesión;
     *  {@link WireError} si la placa lo rechaza, IOException si no contesta en el plazo.
     *
     *  V8 #693 — si la primera vez no contesta (PRIMERA_ESPERA_MS) o la placa tiró la línea a medias, otra vez, sin
     *  decir nada. En esa segunda vez, BUSY es que la primera llegó y sólo se perdió su respuesta. Y sin respuesta pero
     *  con el programa dando señales (su salida, su EXITED), llegó: repetirlo lo ejecutaría dos veces. */
    public long runModuleConRespuesta(String module, String arg, long timeoutMs) throws IOException {
        long senales = senalesDePrograma;
        boolean pudoLlegar;
        try {
            return Json.getLong(pideUnaVez("RUN", camposRun(module, arg), null,
                                           Math.min(timeoutMs, PRIMERA_ESPERA_MS)), "session", 0);
        } catch (WireError we) {
            if (!LINEA_ESTANCADA.equals(we.code)) throw we;   // un rechazo de verdad: NOT_FOUND, BUSY…
            pudoLlegar = false;
        } catch (IOException sinRespuesta) {
            if (senalesDePrograma != senales) return currentSession;
            pudoLlegar = true;
        }
        try {
            return Json.getLong(pideUnaVez("RUN", camposRun(module, arg), null, timeoutMs), "session", 0);
        } catch (WireError we) {
            if (pudoLlegar && "BUSY".equals(we.code)) return currentSession;
            throw we;
        }
    }

    /** V8 #694 — el KILL, ESPERANDO su respuesta: KILL_REPLY (la placa lo para; el EXITED, «terminado por KILL», llega
     *  después) o ERROR NO_SESSION (no había programa en marcha). Por sendCommand(STOP) iba sin esperar, y el IDE decía
     *  «KILL enviado» sin saber si había llegado. {@link WireError} o IOException, como el RUN.
     *
     *  V8 #693 — y como el RUN, otra vez si la primera no contesta o se tiró a medias; en esa segunda vez, NO_SESSION es
     *  que la primera lo paró y sólo se perdió su respuesta. */
    public void killConRespuesta(long timeoutMs) throws IOException {
        long sess = currentSession;
        String campos = sess > 0 ? "\"session\":" + sess : null;
        boolean pudoLlegar;
        try {
            pideUnaVez("KILL", campos, null, Math.min(timeoutMs, PRIMERA_ESPERA_MS));
            return;
        } catch (WireError we) {
            if (!LINEA_ESTANCADA.equals(we.code)) throw we;   // NO_SESSION de verdad: no había programa
            pudoLlegar = false;
        } catch (IOException sinRespuesta) {
            pudoLlegar = true;
        }
        try {
            pideUnaVez("KILL", campos, null, timeoutMs);
        } catch (WireError we) {
            if (pudoLlegar && "NO_SESSION".equals(we.code)) return;
            throw we;
        }
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
        } catch (WireError we) {
            // V6/E1 (#452) — UNA REPLY NO ES UNA REPLY PERDIDA. Si el device
            // CONTESTA rechazando (p.ej. BUSY con un programa en marcha), eso
            // no es "ya estaba muriendo": es un no, y el usuario tiene que
            // verlo. Antes caía en el catch de abajo y se iba a un diag() que,
            // sin sink, no va a ninguna parte — y la consola imprimía "reset
            // enviado" sobre una placa que seguía corriendo.
            //
            // Y no se cierra: el device está VIVO y al otro lado del cable.
            // Cerrar aquí dejaba al usuario sin reset Y sin conexión.
            throw we;
        } catch (IOException ioe) {
            // Reply perdido SÍ es esperable cuando el proceso ya está muriendo.
            // Lo logueamos pero no propagamos — el cliente quiere "reset
            // best-effort", no una garantía de RESET_REPLY síncrona.
            diag("[BpvmClient] RESET sin reply síncrona: " + ioe.getMessage());
        }
        close();
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
    /** Espera la respuesta rindiendose solo tras `inactividadMs` SIN recibir un
     *  byte. Trocear la espera es lo unico que hace falta: el latido lo sella el
     *  stream (ver conLatido).
     *
     *  V8 #693 — el silencio se cuenta desde que se ESPERA, no desde el último byte de antes: con el cable callado más
     *  de `inactividadMs` antes de pedir (el IDE quieto un rato), la espera se rendía en la primera rodaja, y una
     *  respuesta que tardara más de 250 ms se daba por perdida. Lo destapó la segunda vez de #693, que llega justo
     *  tras una primera sin respuesta: tenía 250 ms. */
    private Map<String, Object> esperarPorActividad(CompletableFuture<Map<String, Object>> f,
                                                    long inactividadMs)
            throws InterruptedException, java.util.concurrent.ExecutionException, TimeoutException {
        final long RODAJA_MS = 250L;
        final long desde = System.currentTimeMillis();
        for (;;) {
            try {
                return f.get(RODAJA_MS, TimeUnit.MILLISECONDS);
            } catch (TimeoutException te) {
                if (System.currentTimeMillis() - Math.max(lastRxMillis, desde) > inactividadMs) throw te;
                /* siguen llegando bytes: el cable esta vivo, se sigue esperando */
            }
        }
    }

    private Map<String, Object> sendRequest(String type, String extraJson, byte[] bulkOrNull, long timeoutMs)
            throws IOException {
        try {
            return pideUnaVez(type, extraJson, bulkOrNull, timeoutMs);
        } catch (WireError we) {
            if (!LINEA_ESTANCADA.equals(we.code)) throw we;
            /* V8 #693 — la placa tiró su línea, que le llegó a medias: no se ejecutó, y se repite una vez sin decir
             * nada. Si tampoco, el error llega a quien la pidió. */
            return pideUnaVez(type, extraJson, bulkOrNull, timeoutMs);
        }
    }

    private Map<String, Object> pideUnaVez(String type, String extraJson, byte[] bulkOrNull, long timeoutMs)
            throws IOException {
        return esperarPeticion(enviarPeticion(type, extraJson, bulkOrNull, 0), timeoutMs);
    }

    /* #495 (paso 4) — LAS RESPUESTAS TROCEADAS. El GET y el LIST se piden con "chunked":true y la placa los manda en
     * trozos con el id de la petición —GET_PART (off y un bulk) … GET_END (size y crc), LIST_PART … LIST_END—, mezclados
     * con lo demás (la salida del programa, otras respuestas). Aquí se juntan por id, y la petición se completa con el
     * último como si hubiera llegado de una pieza: GET_REPLY con los bytes, LIST_REPLY con las entradas. Un firmware
     * anterior no mira "chunked" y contesta entero, que sigue el camino de siempre. */
    private static final java.util.Set<String> TROCEADOS = new java.util.HashSet<>(
            java.util.Arrays.asList("GET_PART", "GET_END", "LIST_PART", "LIST_END"));

    private static final class Troceado {
        final java.io.ByteArrayOutputStream datos = new java.io.ByteArrayOutputStream();
        final java.util.zip.CRC32 crc = new java.util.zip.CRC32();
        final List<Object> entradas = new ArrayList<>();
        String roto;          // por qué no vale lo llegado (un trozo fuera de sitio), o null
    }
    private final Map<Long, Troceado> troceados = new HashMap<>();   // bajo pendingLock

    /** Junta un trozo. null = la petición sigue esperando; si no, la respuesta entera (o un ERROR con el porqué). */
    private Map<String, Object> juntaTrozo(long id, String type, Map<String, Object> m) {
        Troceado t;
        synchronized (pendingLock) {
            t = troceados.get(id);
            if (t == null) { t = new Troceado(); troceados.put(id, t); }
            if (type.endsWith("_END")) troceados.remove(id);
        }
        Map<String, Object> r = new HashMap<>();
        r.put("id", id);
        switch (type) {
            case "GET_PART": {
                Object b = m.get(BULK_KEY);
                byte[] d = (b instanceof byte[]) ? (byte[]) b : new byte[0];
                long off = Json.getLong(m, "off", -1);
                if (t.roto == null && off != t.datos.size())
                    t.roto = "un trozo fuera de sitio (en " + off + ", y llevaba " + t.datos.size() + " B)";
                t.datos.write(d, 0, d.length);
                t.crc.update(d, 0, d.length);
                return null;
            }
            case "LIST_PART": {
                List<Object> e = Json.getList(m, "entries");
                if (e != null) t.entradas.addAll(e);
                return null;
            }
            case "GET_END": {
                long size = Json.getLong(m, "size", -1);
                long crc = Json.getLong(m, "crc", -1);
                if (t.roto == null && size != t.datos.size())
                    t.roto = "llegaron " + t.datos.size() + " B y la placa dice " + size;
                if (t.roto == null && crc != t.crc.getValue())
                    t.roto = "el CRC no cuadra (la placa dice " + crc + "; lo llegado, " + t.crc.getValue() + ")";
                if (t.roto != null) {
                    r.put("type", "ERROR");
                    r.put("code", "BAD_TRANSFER");
                    r.put("message", "la descarga llegó dañada: " + t.roto);
                    return r;
                }
                boolean cambiado = Json.getBool(m, "changed", false);
                if (cambiado)
                    diag("[BpvmClient] GET (id=" + id + "): el fichero cambió en la placa mientras se bajaba"
                            + " (lo escribía el programa); lo bajado puede no cuadrar");
                r.put("type", "GET_REPLY");
                r.put(BULK_KEY, t.datos.toByteArray());
                r.put("changed", cambiado);
                return r;
            }
            default: {   // LIST_END
                r.put("type", "LIST_REPLY");
                r.put("entries", t.entradas);
                r.put("omitted", Json.getLong(m, "omitted", 0));
                return r;
            }
        }
    }

    /** #495 — una petición en vuelo: su id, su tipo (para el mensaje) y su future. */
    private static final class Pendiente {
        final long id;
        final String type;
        final CompletableFuture<Map<String, Object>> future;
        Pendiente(long id, String type, CompletableFuture<Map<String, Object>> future) {
            this.id = id; this.type = type; this.future = future;
        }
    }

    /** #495 — la mitad de sendRequest que MANDA: registra la future y escribe la trama sin esperar (la ventana de la
     *  subida manda varias antes de esperar la primera). `recorte` > 0, sólo para SubidaSmoke: el bulk sale con esos
     *  bytes de menos de los que anuncia. */
    private Pendiente enviarPeticion(String type, String extraJson, byte[] bulkOrNull, int recorte)
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
        try {
            writeFrame(id, sb.toString(), recorte > 0 && bulkOrNull != null
                    ? java.util.Arrays.copyOf(bulkOrNull, Math.max(0, bulkOrNull.length - recorte)) : bulkOrNull);
        } catch (IOException e) {
            synchronized (pendingLock) { pendingRequests.remove(id); }
            throw e;
        }
        return new Pendiente(id, type, future);
    }

    /** La mitad de sendRequest que ESPERA la respuesta de una petición ya mandada. */
    private Map<String, Object> esperarPeticion(Pendiente p, long timeoutMs) throws IOException {
        final long id = p.id;
        final String type = p.type;
        final CompletableFuture<Map<String, Object>> future = p.future;
        try {
            /* #454 — se espera POR INACTIVIDAD, no por tiempo total. Mientras
             * la placa siga mandando bytes, esto no se rinde; si deja de
             * mandarlos durante `timeoutMs`, sí. El plazo pasa de "cuánto puede
             * tardar la respuesta" a "cuánto puede callar el cable", que es lo
             * que un timeout debe medir. */
            Map<String, Object> resp = esperarPorActividad(future, timeoutMs);
            String respType = Json.getString(resp, "type", "");
            if ("ERROR".equals(respType)) {
                throw new WireError(Json.getString(resp, "code", "?"),
                                    Json.getString(resp, "message", "?"));
            }
            return resp;
        } catch (TimeoutException te) {
            synchronized (pendingLock) { pendingRequests.remove(id); troceados.remove(id); }
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

    /**
     * H6.a.1 — un local resuelto por NOMBRE (vía el .dbg v3 que tiene el host).
     * {@code value} es el i32 en bp+offset (o el i64 combinado, big-endian, si
     * {@code size==8}). Para arrays, {@code value} es la longitud (el slot
     * guarda la longitud; los elementos van inline a partir de bp+offset+4).
     */
    public static final class NamedLocal {
        public final String  name;
        public final long    value;
        public final int     size;
        public final boolean isArray;
        public final int     offset;
        public final String  type;     // H6.a.2: tag de tipo BP (integer/long/.../ref/"?")
        public final String  display;  // H6.a.2: texto ya renderizado por la VM
        public NamedLocal(String name, long value, int size, boolean isArray, int offset,
                          String type, String display) {
            this.name = name; this.value = value; this.size = size;
            this.isArray = isArray; this.offset = offset;
            this.type = (type != null ? type : "?");
            this.display = (display != null ? display : Long.toString(value));
        }
    }

    /**
     * Locales del thread pausado resueltos por NOMBRE. Vacío si el módulo no
     * trae sección `vars` en su .dbg (formato < v3): el caller cae a
     * {@link #getLocals}. NO modifica el .mod ni requiere nada del device.
     */
    public List<NamedLocal> getNamedLocals(long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("LOCALS", null, null, timeoutMs);
        List<Object> named = Json.getList(resp, "named");
        List<NamedLocal> out = new ArrayList<>();
        if (named == null) return out;
        for (Object o : named) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new NamedLocal(
                    Json.getString(m, "name", "?"),
                    Json.getLong(m, "value", 0),
                    (int) Json.getLong(m, "size", 4),
                    Json.getBool(m, "isArray", false),
                    (int) Json.getLong(m, "offset", 0),
                    Json.getString(m, "type", "?"),
                    Json.getString(m, "display", null)));
        }
        return out;
    }

    /**
     * H6.b.3.b — resuelve el frame de una VM <b>device-role</b> (server wire
     * de la VM-C, o el Pico) a locales POR NOMBRE usando el {@code .dbg} que
     * tiene el host. Es la regla de oro de H6: el device sólo reporta pc/bp/cs
     * CRUDOS (en {@code ev}); aquí leemos cada variable en bp+offset vía
     * {@code READ_INT}/{@code READ_STRING} del wire y la renderizamos por tipo,
     * del lado del host.
     *
     * <p>Complementa a {@link #getNamedLocals}: aquélla sirve cuando el server
     * mismo manda la sección {@code named[]} (VM-Java, que tiene el .dbg
     * cargado); ésta sirve cuando el server es un device que NO conoce símbolos
     * y sólo expone memoria cruda.</p>
     *
     * @param dbg el {@code .dbg} del módulo en ejecución (cargado en el host).
     * @param ev  el evento pausado (BP_HIT/STEP_DONE): lleva absPc, cs y bp.
     * @return el frame resuelto (función + línea + locales por nombre).
     */
    public DeviceFrameResolver.Frame resolveDeviceFrame(DbgFile dbg, PausedEvent ev,
                                                        long timeoutMs) {
        int relPc = ev.absPc - ev.cs;
        DeviceFrameResolver.MemReader reader = new DeviceFrameResolver.MemReader() {
            @Override public int readI32(int addr) {
                try { return readMemoryInt(addr, timeoutMs); }
                catch (IOException e) { return 0; }
            }
            @Override public String readString(int ref) {
                // #589 — una cadena VACÍA es "" (se enseñaba «@…» como si no se pudiera leer): lo que no se
                // puede leer llega como ERROR (BAD_REF), y sendRequest lo lanza como WireError.
                try { return readStringIfPossible(ref, timeoutMs); }
                catch (IOException e) { return null; }
            }
        };
        return DeviceFrameResolver.resolve(dbg, relPc, ev.bp, reader);
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

    /** Lee un string BP por su referencia (un handle o la de un literal): el texto tal cual, "" si la
     *  referencia es 0; una que no resuelve llega como ERROR (BAD_REF) y sale como {@link WireError}. */
    public String readStringIfPossible(int ref, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("READ_STRING", "\"ref\":" + ref, null, timeoutMs);
        return Json.getString(resp, "value", "");
    }

    // ============================================================
    // Transferencia de ficheros (A2.2) — PR-2: bulk binario raw inline.
    // ============================================================

    /* #294 — umbral del PUT clásico (whole-buffer). Por debajo del tope del wire
     * del firmware (s_put_buf=48K), con margen. Ficheros <= esto van por el PUT de
     * siempre (1 round-trip, probado en todas las placas); mayores van por
     * streaming (PUT_BEGIN/DATA/END). Cuando se escribió esto el streaming sólo
     * lo tenía el ESP32; desde la tanda 3 de #294 está en las tres familias, y
     * desde V6/U3 lo sirve el MISMO código común en todas. */
    /* #334 — 40K -> 8K. El umbral por debajo del cual se sube de una pieza fija el
     * buffer que el firmware tiene que reservar SIEMPRE. Con el streaming ya
     * verificado en placa en las 3 familias, bajarlo devuelve 28-36 KB de RAM por
     * placa. Se deja en 8K (y no en 0) porque casi todos los .mod son de 2-5 KB y
     * asi siguen subiendo en UN viaje en vez de tres. */
    private static final int PUT_SINGLE_SHOT_MAX = 8 * 1024;
    /* #338 — 16K -> 8K. Este numero y el buffer del firmware son EL MISMO numero
     * visto desde los dos lados: el trozo mas grande que el IDE manda de una pieza
     * es lo que la placa tiene que poder recibir. Bajarlo aqui es lo que permite
     * que alli s_put_buf baje a 8 KB (Pico/ESP32), que en un micro sin PSRAM son
     * 8 KB que vuelven al heap de la VM.
     *
     * Coste: un .mod de 44 KB viaja en 6 viajes en vez de 3. Sobre USB CDC / serie
     * el ida y vuelta por trozo es despreciable al lado de los datos, y el umbral
     * de arriba hace que la mayoria de .mod (2-5 KB) sigan yendo en UN viaje.
     *
     * Si un IDE viejo hablase con un firmware nuevo mandaria 16 KB a un buffer de
     * 8: el firmware responde NO_SPACE y drena — se queja, no se corrompe. Lo
     * correcto seria que la placa ANUNCIARA su capacidad en el HELLO y el IDE se
     * ajustara solo; queda apuntado, pero abre superficie de wire en 5
     * implementaciones y hoy los dos viajan juntos en el mismo ZIP. */
    private static final int PUT_STREAM_CHUNK     = 8 * 1024;

    /** Sube `bytes` al workdir de la VM en `remotePath`. Whole-buffer si cabe;
     *  streaming por trozos si es grande. Bulk raw inline.
     *
     *  V7/H8 #495 — EL SEGUNDO INTENTO lo hace el IDE (Eduardo, 1-oct: «que lo haga el IDE»): UNO, desde el
     *  principio, y SÓLO si falla el cable (sin respuesta, una respuesta rota, la placa que pierde el hilo); si la
     *  placa dice que no con nombre (sin espacio, la ruta, no preparada), repetir no arregla nada y se dice a la
     *  primera. Una subida que no termina, la placa la BORRA (Eduardo: «si queda la versión anterior el usuario se va
     *  a pensar que tiene el nuevo»): si el segundo también falla, el fichero no está en la placa. */
    public int uploadFile(String remotePath, byte[] bytes, long timeoutMs) throws IOException {
        try {
            return subirUnaVez(remotePath, bytes, timeoutMs);
        } catch (IOException e) {
            if (closed || !esDelCable(e)) throw e;
            segundosIntentos++;
            diag("[BpvmClient] subida de " + remotePath + ": " + motivo(e) + " — segundo intento");
            resincronizar(timeoutMs);
            try {
                return subirUnaVez(remotePath, bytes, timeoutMs);
            } catch (IOException e2) {
                if (closed || !esDelCable(e2)) throw e2;
                try { resincronizar(timeoutMs); } catch (IOException ignorada) { /* el HELLO es lo que la borra */ }
                throw new IOException("no se pudo subir " + remotePath + " (dos intentos): " + motivo(e2)
                        + "; el fichero no está en la placa", e2);
            }
        }
    }

    private int subirUnaVez(String remotePath, byte[] bytes, long timeoutMs) throws IOException {
        if (bytes.length <= PUT_SINGLE_SHOT_MAX) {
            String extra = "\"path\":" + jsonStr(remotePath);
            Map<String, Object> resp = sendRequest("PUT", extra, bytes, timeoutMs);
            return (int) Json.getLong(resp, "size", 0);
        }
        return uploadFileStreaming(remotePath, bytes, timeoutMs);
    }

    /** #294 — subida por trozos (PUT_BEGIN + N×PUT_DATA + PUT_END). Sin techo de
     *  tamaño: el firmware apende cada chunk al FS sin buferizar el fichero entero.
     *
     *  #495 — con el trozo y la VENTANA que anuncia la placa (512 B y 4): hasta 4 trozos sin acuse, y se espera
     *  el más antiguo antes de mandar el siguiente. Sin anuncio, como siempre: 8 KB y un acuse por trozo. */
    private int uploadFileStreaming(String remotePath, byte[] bytes, long timeoutMs)
            throws IOException {
        String p = "\"path\":" + jsonStr(remotePath);
        sendRequest("PUT_BEGIN", p + ",\"size\":" + bytes.length, null, timeoutMs);
        int trozo   = putTrozo   > 0 ? putTrozo   : PUT_STREAM_CHUNK;
        int ventana = putVentana > 0 ? putVentana : 1;
        int recortar = pruebaRecortarTrozo;
        pruebaRecortarTrozo = -1;
        java.util.ArrayDeque<Pendiente> enVuelo = new java.util.ArrayDeque<>();
        int off = 0, indice = 0;
        try {
            while (off < bytes.length || !enVuelo.isEmpty()) {
                while (off < bytes.length && enVuelo.size() < ventana) {
                    int n = Math.min(trozo, bytes.length - off);
                    enVuelo.add(enviarPeticion("PUT_DATA", "", java.util.Arrays.copyOfRange(bytes, off, off + n),
                                               indice == recortar ? 16 : 0));
                    off += n;
                    indice++;
                }
                esperarPeticion(enVuelo.poll(), timeoutMs);
            }
        } finally {
            /* tras un fallo, lo que quedaba en vuelo: si sus respuestas llegan, nadie las espera (se tiran) */
            synchronized (pendingLock) { for (Pendiente q : enVuelo) pendingRequests.remove(q.id); }
        }
        Map<String, Object> resp = sendRequest("PUT_END", "\"size\":" + bytes.length, null, timeoutMs);
        return (int) Json.getLong(resp, "size", 0);
    }

    /** #495 — ¿falló el CABLE (sin respuesta a tiempo, un FATAL, la placa que perdió el hilo de la subida) o la
     *  PLACA dijo que no con nombre? Sólo lo primero se arregla repitiendo. */
    static boolean esDelCable(IOException e) {
        if (e instanceof WireError) {
            String c = ((WireError) e).code;
            return "SIZE_MISMATCH".equals(c) || "NO_SESSION".equals(c) || "PROTOCOL_ERROR".equals(c);
        }
        return true;
    }

    private static String motivo(IOException e) {
        Throwable c = e.getCause();
        return (c != null && c.getMessage() != null && "fallo en future".equals(e.getMessage()))
                ? c.getMessage() : e.getMessage();
    }

    /** #495 — antes del segundo intento: que la placa suelte lo que tenía a medias y se entere de que el de antes ya
     *  no está. Un bulk incompleto lo da por perdido a los 3 s (WIRE_V1_BULK_ESTANCADO_MS), así que se espera a que
     *  el cable lleve callado más que eso EN LOS DOS SENTIDOS; luego un HELLO, que en la placa acaba la subida a
     *  medias y la borra. */
    private void resincronizar(long timeoutMs) throws IOException {
        final long CALLADO_MS = 3500L;
        long tope = System.currentTimeMillis() + Math.max(timeoutMs, 10_000L);
        for (;;) {
            long ahora = System.currentTimeMillis();
            if (ahora - lastRxMillis >= CALLADO_MS && ahora - lastTxMillis >= CALLADO_MS) break;
            if (ahora > tope) break;                       /* el cable no calla: se intenta igual */
            try {
                Thread.sleep(100);
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                throw new IOException("interrupted", ie);
            }
        }
        aprenderHello(sendRequest("HELLO", HELLO_CAMPOS, null, 5000));
    }

    /** #495 — para SubidaSmoke: lo que anunció la placa (0 = nada) y cuántas subidas necesitaron el segundo. */
    int putTrozoAnunciado()   { return putTrozo; }
    int putVentanaAnunciada() { return putVentana; }
    int segundosIntentos()    { return segundosIntentos; }

    /** Sube un fichero local al workdir de la VM. */
    public int uploadFile(java.nio.file.Path localFile, String remotePath, long timeoutMs)
            throws IOException {
        byte[] data = java.nio.file.Files.readAllBytes(localFile);
        return uploadFile(remotePath, data, timeoutMs);
    }

    /** Descarga `remotePath` del workdir de la VM. Troceada (#495, paso 4): la placa la manda a trozos, entre la
     *  salida del programa y lo demás, y aquí se junta y se comprueba con su tamaño y su CRC; un firmware anterior la
     *  manda entera. El plazo es de silencio (#454): mientras lleguen trozos, se espera. */
    public byte[] downloadFile(String remotePath, long timeoutMs) throws IOException {
        Map<String, Object> resp = sendRequest("GET",
                "\"path\":" + jsonStr(remotePath) + ",\"chunked\":true", null, timeoutMs);
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
        public final long crc;          // paso 4 cierre: CRC32 del device (-1 si el firmware no lo reporta)
        public final boolean isDirectory;
        /** V7/H8 E1 — por qué la VM NO cargaría este fichero (un módulo o un pack dañado o de un formato que no
         *  carga: lo examina el LIST), o null si está bien o no es de los que se examinan. */
        public final String mal;
        public RemoteFile(String n, long s, long c, boolean d) {
            this(n, s, c, d, null);
        }
        public RemoteFile(String n, long s, long c, boolean d, String mal) {
            this.name = n; this.size = s; this.crc = c; this.isDirectory = d;
            this.mal = (mal == null || mal.isEmpty()) ? null : mal;
        }
        @Override public String toString() {
            return (isDirectory ? "[D] " : "[F] ") + name
                    + (isDirectory ? "" : " (" + size + ")");
        }
    }

    /** Lista los ficheros bajo `remotePath` ("" o "." = raíz del workdir).
     *
     * <p>#425 — devuelve {@link DirListing} y no una lista pelada porque el
     * recorrido plano del device tiene topes (entradas por directorio,
     * directorios a visitar y —en el STM32— ficheros del FS entero) y al
     * pasarse recortaba <b>en silencio</b>: el firmware lo anotaba en su log
     * pero por el wire salía una lista corta que el árbol pintaba como si fuera
     * todo. Un listado corto que no se declara corto es una mentira, y de las
     * que se creen. Ahora el device manda `omitted` y esto lo pasa arriba —
     * mismo par (files, omitidas) que ya usaba {@link #listDir}, que es de donde
     * sale la forma. */
    public DirListing listFiles(String remotePath, long timeoutMs) throws IOException {
        /* #495 (paso 4): troceado, un directorio por vuelta de la placa (y así también durante un RUN). */
        String extra = "\"path\":" + jsonStr(remotePath == null ? "" : remotePath) + ",\"chunked\":true";
        Map<String, Object> resp = sendRequest("LIST", extra, null, timeoutMs);
        List<Object> arr = Json.getList(resp, "entries");
        List<RemoteFile> out = new ArrayList<>();
        if (arr == null) return new DirListing(out, Json.getLong(resp, "omitted", 0));
        for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new RemoteFile(
                    Json.getString(m, "name", ""),
                    Json.getLong(m, "size", 0),
                    crcSinSigno(Json.getLong(m, "crc", -1)),   // paso 4 cierre: -1 = firmware sin crc → fallback
                    Json.getBool(m, "isDir", false),
                    Json.getString(m, "mal", null)));          // V7/H8 E1: la VM no lo cargaría (y por qué)
        }
        /* Firmware anterior a #425: sin campo → 0 = "no me consta que falte
         * nada". No es lo mismo que "está completo", pero es lo único honesto
         * que se puede decir de un device que no lo cuenta. */
        return new DirListing(out, Json.getLong(resp, "omitted", 0));
    }

    /**
     * V5/H2 — las entradas de UN directorio, sin recursión y SIN CRC.
     *
     * No sustituye a {@link #listFiles}: responden a preguntas distintas y las
     * dos hacen falta. LIST trae el FS interno ENTERO con el CRC de cada
     * fichero, que es lo que necesita el Run para saltarse los PUT; esto trae
     * los hijos de un directorio, que es lo que necesita mirar.
     *
     * Con una tarjeta SD montada la diferencia deja de ser estética: LIST
     * sobre 119 GB leería la tarjeta entera byte a byte para calcular CRCs.
     *
     * Lanza IOException si el device no conoce el verbo (firmware anterior a
     * V5/H2) — que es un "no sé", no un "está vacío", y conviene no
     * confundirlos.
     */
    public DirListing listDir(String remotePath, long timeoutMs) throws IOException {
        String p = (remotePath == null || remotePath.isEmpty()) ? "/" : remotePath;
        Map<String, Object> resp = sendRequest("LIST_DIR", "\"path\":" + jsonStr(p), null, timeoutMs);
        List<Object> arr = Json.getList(resp, "entries");
        List<RemoteFile> out = new ArrayList<>();
        if (arr != null) for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new RemoteFile(
                    Json.getString(m, "name", ""),
                    Json.getLong(m, "size", 0),
                    -1,                             // aquí NO hay CRC, y es a propósito
                    Json.getBool(m, "isDir", false)));
        }
        return new DirListing(out, Json.getLong(resp, "omitidas", 0));
    }

    /**
     * #398 — EL CRC DE UN FICHERO DEL DEVICE, pedido a propósito.
     *
     * Antes venía de gorra en cada entrada del `LIST`, y por eso refrescar el
     * árbol leía el sistema de ficheros ENTERO: medido en la P4 el 15-ago, el
     * 99 % de un refresco de casi siete segundos. Ahora el listado no lo trae
     * (manda -1) y se pide aquí, de uno en uno y justo antes de subir ese
     * fichero — que es el único momento en que sirve para algo.
     *
     * Devuelve -1 si el fichero no está, si el firmware no conoce el flag o si
     * no pudo calcularlo. -1 significa siempre lo mismo: «no lo sé», y quien
     * llama debe subir en vez de arriesgarse a saltarse una subida buena.
     */
    public long fileCrc(String remotePath, long timeoutMs) {
        try {
            Map<String, Object> r = sendRequest(
                    "STAT", "\"path\":" + jsonStr(remotePath) + ",\"crc\":true", null, timeoutMs);
            return crcSinSigno(Json.getLong(r, "crc", -1));
        } catch (IOException e) {
            return -1;      // no existe, verbo desconocido, timeout… todo es "no lo sé"
        }
    }

    /** #466 — un CRC-32 del wire, SIN signo. Los firmwares anteriores lo mandaban
     *  como `long` de 32 bits (negativo con el bit 31 puesto) y este lado compara
     *  contra java.util.zip.CRC32, que es sin signo: la mitad de los «idénticos»
     *  parecían distintos y se volvían a subir. -1 sigue siendo «no lo sé». */
    static long crcSinSigno(long v) {
        if (v == -1) return -1;
        return (v < 0) ? v + 4294967296L : v;
    }

    /** #466 — lo que la placa dice de un MÓDULO por su nombre, esté donde esté. */
    public static final class ModStat {
        public final String path;   /* dónde lo tiene (/app/proj/X.mod, /lib/X.mod…) */
        public final long   size;
        public final long   crc;    /* -1 = no lo dio */
        public final String magic;  /* "MOD7"; null si no parece un módulo */
        /** V7/H9 #624 — la VERSIÓN de verdad (desde MOD8, de la cabecera): `gen`, la «version» del proyecto; y la
         *  interfaz (`ifaceCrc`). -1 = la placa no la dio (firmware anterior, o un formato sin versión). */
        public final long   gen;
        public final long   ifaceCrc;
        ModStat(String path, long size, long crc, String magic, long gen, long ifaceCrc) {
            this.path = path; this.size = size; this.crc = crc; this.magic = magic;
            this.gen = gen; this.ifaceCrc = ifaceCrc;
        }
        /** La VERSIÓN del módulo: "MOD7" → 7; 0 si no parece un módulo. Es la única
         *  que hay —los ficheros del device no tienen fecha: tienen versión y CRC. */
        public int version() { return versionOf(magic); }
        public static int versionOf(String m) {
            if (m == null || m.length() != 4 || !m.startsWith("MOD")) return 0;
            char c = m.charAt(3);
            return (c >= '0' && c <= '9') ? c - '0' : 0;
        }
    }

    /**
     * #466 — «¿tienes este módulo, donde sea?»: STAT por NOMBRE. Lo resuelve el
     * device con SU resolvedor (el del RUN: base → /app → /lib → /sys), así el IDE
     * no lleva una copia del orden de búsqueda — que es como se desincronizó la
     * lista de #463. Devuelve null si no lo tiene (NOT_FOUND), si el firmware no
     * conoce el STAT por nombre (INVALID_PARAM: firmware anterior a #466) o si no
     * se pudo preguntar: en los tres casos quien llama SUBE, que es lo que se hacía
     * siempre. Con respuesta, la regla (versión y CRC) la aplica el llamador.
     */
    public ModStat statModule(String name, String base, long timeoutMs) {
        try {
            String extra = "\"name\":" + jsonStr(name) + ",\"crc\":true";
            if (base != null && !base.isEmpty()) extra += ",\"base\":" + jsonStr(base);
            Map<String, Object> r = sendRequest("STAT", extra, null, timeoutMs);
            String path = Json.getString(r, "path", null);
            if (path == null) return null;
            return new ModStat(path, Json.getLong(r, "size", -1), crcSinSigno(Json.getLong(r, "crc", -1)),
                               Json.getString(r, "magic", null),
                               Json.getLong(r, "gen", -1), crcSinSigno(Json.getLong(r, "ifaceCrc", -1)));   // V7/H9 #624
        } catch (IOException e) {
            return null;
        }
    }

    /**
     * Un ERROR del device, con su CÓDIGO a mano.
     *
     * Sigue siendo IOException, así que todo el que ya la capturaba no se
     * entera. Lo que aporta es poder distinguir "este firmware no conoce el
     * verbo" (UNSUPPORTED) de "ese fichero no existe" (NOT_FOUND) SIN mirar
     * dentro del texto del mensaje: emparejar cadenas para tomar decisiones se
     * rompe el día que alguien reescribe el mensaje, y se rompe en silencio.
     */
    public static final class WireError extends IOException {
        public final String code;
        WireError(String code, String message) {
            super("VM error [" + code + "]: " + message);
            this.code = code;
        }
        /** El device no implementa este verbo (firmware más antiguo). */
        public boolean unsupported() { return "UNSUPPORTED".equals(code); }
    }

    /** Lo que devuelve {@link #listDir}: las entradas Y cuántas se quedaron
     *  fuera. El device tiene tope por listado, y que se trunque no puede
     *  quedarse en su log — quien mira la lista es quien tiene que enterarse.
     *  Va en el resultado y no en un campo del cliente para que no pueda
     *  quedarse a cero por descuido ni pisarse entre dos peticiones. */
    public static final class DirListing {
        public final List<RemoteFile> files;
        public final long omitidas;
        DirListing(List<RemoteFile> f, long o) { this.files = f; this.omitidas = o; }
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

    /** Borra el log RAM + flash del peer (firmware Pico). La VM Java
     *  devuelve UNSUPPORTED — los callers la tratan como soft-error y
     *  no propagan. */
    public void logClear(long timeoutMs) throws IOException {
        sendRequest("LOG_CLEAR", null, null, timeoutMs);
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

    /**
     * V5/H1 — arranca la tarjeta SD y devuelve lo que dice de sí misma.
     *
     * NO es como INFO, y conviene no confundirlos: INFO es consulta barata y
     * sin efectos; esto HABLA con el hardware, tarda (la negociación va a
     * 400 kHz por exigencia del estándar) y puede no haber tarjeta. Por eso lo
     * dispara el usuario desde la consola y no se pide solo al conectar.
     *
     * La respuesta trae SIEMPRE `ok` y `motivo`; el resto de campos sólo si
     * arrancó. `motivo` es el peldaño donde se paró — "no hay tarjeta", "no
     * contesta" y "no arranca" mandan a sitios distintos.
     *
     * Plazo generoso a propósito: el ACMD41 del estándar puede tardar 1 s, y a
     * eso hay que sumarle el viaje por el wire.
     */
    public Map<String, Object> getSdInfo(long timeoutMs) throws IOException {
        return sendRequest("SD_INFO", null, null, timeoutMs);
    }

    /**
     * V5/H2 — monta la tarjeta como sistema de ficheros bajo /sd.
     *
     * VERBO APARTE de SD_INFO, y a propósito: SD_INFO es un diagnóstico que no
     * cambia nada, y esto sí. Con las dos cosas en un solo comando no habría
     * forma de mirar una tarjeta sin montarla.
     *
     * La respuesta trae siempre `ok`, `motivo`, `prefijo` y `lba`; si montó,
     * además `etiqueta`, `kbTotal`, `kbLibres`, `entradasRaiz` y `primera`.
     * `primera` es el nombre del primer fichero de la raíz — el dato que se
     * compara con lo que enseña el PC para saber si la cadena entera (SPI →
     * bloques → FAT → nombres largos) traduce bien.
     *
     * Mismo plazo largo que SD_INFO: monta incluye arrancar la tarjeta.
     */
    public Map<String, Object> mountSd(long timeoutMs) throws IOException {
        return sendRequest("SD_MOUNT", null, null, timeoutMs);
    }

    // ============================================================
    // H9 — gestión de placa (protocolo STATE / ENV_* / PART_*).
    // Lo consume FrmBoard sobre el MISMO transporte ya conectado (comparte
    // conexión con la ventana principal). El peer es el firmware (kernel-comm)
    // o el boardsim de host (tools/boardsim.c). Modelos: clases anidadas abajo.
    // ============================================================

    /** Estado del arranque escalonado (H9): en qué capa está el device y por qué. */
    public static final class BoardState {
        public final int state;          // 0 kernel, 1 particiones, 2 fs, 3 app
        public final String name;
        public final boolean degraded;
        public final String reason;
        /** V7/H7 (Boot 2, fase 1) — el «casi» de Eduardo: la placa llega a la app pero la
         *  zona de packs no deja grabar (sin formatear, o la cadena rota). Una imagen
         *  anterior no lo manda: false. */
        public final boolean casi;
        public final String aviso;
        public BoardState(int state, String name, boolean degraded, String reason,
                          boolean casi, String aviso) {
            this.state = state; this.name = name; this.degraded = degraded; this.reason = reason;
            this.casi = casi; this.aviso = aviso;
        }
    }

    /** Una variable de entorno (par clave/valor del bloque de env). */
    public static final class EnvVar {
        public final String key, value;
        public EnvVar(String key, String value) { this.key = key; this.value = value; }
    }

    /** Una partición: nombre + región. `offset` es -1 en respuestas que solo
     *  llevan tamaños (PART_DEFAULTS); es el offset DERIVADO en PART_LS. */
    public static final class Partition {
        public final String name;
        public final long offset, size;
        public Partition(String name, long offset, long size) {
            this.name = name; this.offset = offset; this.size = size;
        }
    }

    /** Layout de particiones + contexto de flash. `missing`=true → placa virgen a
     *  nivel de particiones (el asistente ofrece defaults). */
    public static final class PartTable {
        public final boolean missing;
        public final long base, usableFlash;
        public final List<Partition> parts;
        /** V7/H7 (30-sep): > 0 si el FS es la flash que el chip tiene por encima de la zona de datos (la P4
         *  Waveshare y la C5): entonces esa placa no reparte (PART_APPLY contesta UNSUPPORTED). */
        public final long fsArriba;
        public PartTable(boolean missing, long base, long usableFlash, List<Partition> parts) {
            this(missing, base, usableFlash, parts, 0L);
        }
        public PartTable(boolean missing, long base, long usableFlash, List<Partition> parts, long fsArriba) {
            this.missing = missing; this.base = base; this.usableFlash = usableFlash; this.parts = parts;
            this.fsArriba = fsArriba;
        }
    }

    /** H3 — un pack de la zona de packs del device (una fila del PACK_LS). */
    public static final class PackInfo {
        public final String name, version;
        public final long date, size, offset;
        public final int files;
        public final boolean active, crcOk;
        public PackInfo(String name, String version, long date, long size, long offset,
                        int files, boolean active, boolean crcOk) {
            this.name = name; this.version = version; this.date = date; this.size = size;
            this.offset = offset; this.files = files; this.active = active; this.crcOk = crcOk;
        }
    }

    /** H3 — la zona de packs completa: cadena (activos Y tombstones) + espacio.
     *  `chainOk`=false → hay una cabecera corrupta que corta la cadena (lo repara
     *  una compactación desde el PC); `count` puede superar packs.size() si el
     *  device truncó el listado.
     *
     *  <p>#539 — y lo que truncó lo CUENTA: el device lista como mucho 16 (los
     *  activos primero y, en el hueco, los borrados más recientes) y manda
     *  `omitted` (cuántos de `count` no están en `packs`) y `activeOmitted`
     *  (cuántos de ésos están ACTIVOS: sólo con más activos que filas). Una imagen
     *  anterior no los manda (aquí, -1) y listaba los 16 PRIMEROS de la cadena, que
     *  en una placa con historia son borrados: la C5 enseñaba 16 borrados y
     *  escondía sus dos activos. */
    public static final class PackTable {
        public final long regionSize, free;
        public final boolean chainOk;
        public final int count;
        public final int omitted, activeOmitted;   // #539: -1 = el device no lo dice
        public final List<PackInfo> packs;
        public PackTable(long regionSize, long free, boolean chainOk, int count,
                         int omitted, int activeOmitted, List<PackInfo> packs) {
            this.regionSize = regionSize; this.free = free; this.chainOk = chainOk;
            this.count = count; this.omitted = omitted; this.activeOmitted = activeOmitted;
            this.packs = packs;
        }

        /** #539 — ¿están TODOS los activos en {@link #packs}? Con la cuenta del
         *  device se sabe; sin ella (imagen anterior), sólo si no truncó nada. */
        public boolean activosCompletos() {
            return activeOmitted >= 0 ? activeOmitted == 0 : count <= packs.size();
        }
    }

    /** STATE — estado del arranque escalonado. */
    public BoardState boardState(long timeoutMs) throws IOException {
        Map<String, Object> r = sendRequest("STATE", null, null, timeoutMs);
        return new BoardState((int) Json.getLong(r, "state", -1),
                Json.getString(r, "name", "?"),
                Json.getBool(r, "degraded", false),
                Json.getString(r, "reason", ""),
                Json.getBool(r, "casi", false),
                Json.getString(r, "aviso", ""));
    }

    /** ENV_LS — todas las variables de entorno (la tabla nombre|valor). */
    public List<EnvVar> envList(long timeoutMs) throws IOException {
        Map<String, Object> r = sendRequest("ENV_LS", null, null, timeoutMs);
        List<Object> arr = Json.getList(r, "entries");
        List<EnvVar> out = new ArrayList<>();
        if (arr == null) return out;
        for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new EnvVar(Json.getString(m, "key", ""), Json.getString(m, "value", "")));
        }
        return out;
    }

    /** ENV_GET — valor de una clave (o IOException si el peer responde NOT_FOUND). */
    public String envGet(String key, long timeoutMs) throws IOException {
        Map<String, Object> r = sendRequest("ENV_GET", "\"key\":" + jsonStr(key), null, timeoutMs);
        return Json.getString(r, "value", "");
    }

    /** ENV_SET — fija/crea `key=value`. El peer re-serializa a la copia A/B rancia. */
    public void envSet(String key, String value, long timeoutMs) throws IOException {
        sendRequest("ENV_SET", "\"key\":" + jsonStr(key) + ",\"value\":" + jsonStr(value), null, timeoutMs);
    }

    /** ENV_DEL — borra una clave. */
    public void envDel(String key, long timeoutMs) throws IOException {
        sendRequest("ENV_DEL", "\"key\":" + jsonStr(key), null, timeoutMs);
    }

    private static PartTable parsePartTable(Map<String, Object> r) {
        List<Object> arr = Json.getList(r, "parts");
        List<Partition> parts = new ArrayList<>();
        if (arr != null) {
            for (Object o : arr) {
                if (!(o instanceof Map)) continue;
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) o;
                parts.add(new Partition(Json.getString(m, "name", ""),
                        Json.getLong(m, "offset", -1), Json.getLong(m, "size", 0)));
            }
        }
        return new PartTable(Json.getBool(r, "missing", false),
                Json.getLong(r, "base", 0), Json.getLong(r, "usableFlash", 0), parts,
                Json.getLong(r, "fsArriba", 0));
    }

    /** PART_LS — layout actual (offsets DERIVADOS + tamaños), o missing si virgen. */
    public PartTable partLayout(long timeoutMs) throws IOException {
        return parsePartTable(sendRequest("PART_LS", null, null, timeoutMs));
    }

    /** PART_DEFAULTS — tamaños sugeridos para una placa virgen (offset = -1). */
    public PartTable partDefaults(long timeoutMs) throws IOException {
        return parsePartTable(sendRequest("PART_DEFAULTS", null, null, timeoutMs));
    }

    /** PART_APPLY — aplica tamaños (un campo long por partición, keyed por nombre).
     *  El peer VALIDA (offsets derivados, no-cero, alineado, cabe) y solo si es
     *  válido escribe; si no, responde ERROR (→ IOException con el detalle). */
    public void partApply(Map<String, Long> sizesByName, long timeoutMs) throws IOException {
        StringBuilder extra = new StringBuilder();
        boolean first = true;
        for (Map.Entry<String, Long> e : sizesByName.entrySet()) {
            if (!first) extra.append(',');
            extra.append(jsonStr(e.getKey())).append(':').append(e.getValue());
            first = false;
        }
        sendRequest("PART_APPLY", extra.toString(), null, timeoutMs);
    }

    /** PACK_LS — packs de la zona de packs (H3). El peer responde UNSUPPORTED si
     *  la placa no expone packs (sin layout de particiones aún) → IOException. */
    public PackTable packList(long timeoutMs) throws IOException {
        Map<String, Object> r = sendRequest("PACK_LS", null, null, timeoutMs);
        List<Object> arr = Json.getList(r, "packs");
        List<PackInfo> packs = new ArrayList<>();
        if (arr != null) {
            for (Object o : arr) {
                if (!(o instanceof Map)) continue;
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) o;
                packs.add(new PackInfo(Json.getString(m, "name", ""),
                        Json.getString(m, "version", ""),
                        Json.getLong(m, "date", 0),
                        Json.getLong(m, "size", 0),
                        Json.getLong(m, "offset", -1),
                        (int) Json.getLong(m, "files", 0),
                        Json.getBool(m, "active", false),
                        Json.getBool(m, "crcOk", false)));
            }
        }
        return new PackTable(Json.getLong(r, "regionSize", 0), Json.getLong(r, "free", 0),
                Json.getBool(r, "chainOk", false), (int) Json.getLong(r, "count", 0),
                (int) Json.getLong(r, "omitted", -1), (int) Json.getLong(r, "activeOmitted", -1),
                packs);
    }

    /** H3 — un fichero dentro de un pack (una fila del PACK_ENTRIES). */
    public static final class PackEntry {
        public final String tipo, nombre;
        public final long size;
        public PackEntry(String tipo, String nombre, long size) {
            this.tipo = tipo; this.nombre = nombre; this.size = size;
        }
    }

    /** PACK_ENTRIES — ficheros dentro del pack en `offset` (del PACK_LS previo). */
    public List<PackEntry> packEntries(long offset, long timeoutMs) throws IOException {
        Map<String, Object> r = sendRequest("PACK_ENTRIES", "\"offset\":" + offset, null, timeoutMs);
        List<Object> arr = Json.getList(r, "entries");
        List<PackEntry> out = new ArrayList<>();
        if (arr == null) return out;
        for (Object o : arr) {
            if (!(o instanceof Map)) continue;
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) o;
            out.add(new PackEntry(Json.getString(m, "tipo", ""),
                    Json.getString(m, "nombre", ""), Json.getLong(m, "size", 0)));
        }
        return out;
    }

    /** PACK_BURN_* — graba un pack en la zona de packs del device POR CHUNKS
     *  (RAM constante en el micro; sin límite de tamaño de pack). El device
     *  valida ambos CRC leyendo de su flash y solo entonces ACTIVA el pack
     *  (magic al final): un corte o un error deja la zona como estaba.
     *  Devuelve el offset donde quedó grabado. */
    public long packBurn(byte[] img, long timeoutMs) throws IOException {
        return packBurn(img, timeoutMs, null);
    }

    /** Lo llama `packBurn` tras CADA chunk confirmado por el device: (hechos,
     *  total) en bytes. Se avisa de lo ya CONFIRMADO, no de lo enviado — así el
     *  número no puede ir por delante de la placa. */
    public interface BurnProgress { void avance(int hechos, int total); }

    /** Igual, pero informando del avance. Un pack de SQLite son ~410 KB en
     *  chunks de 4 KB ≈ 100 pasos: sin esto el usuario no distingue "va" de "se
     *  ha colgado", que en una operación que BORRA flash es justo la duda que no
     *  se le puede dejar. */
    /**
     * V5/H8 — el hueco donde el IDE RELOCALIZA, entre que la placa dice dónde
     * cae el pack y que empiezan a salir los datos.
     *
     * <p>Existe porque un motor nativo (`.npk`) hay que sellarlo para la
     * dirección REAL, y ésa no se sabe hasta que `PACK_BURN_BEGIN` elige el
     * hueco. La placa la dice; aquí se aplica.
     */
    public interface AntesDeEnviar {
        /**
         * @param flashAddr dirección que verá la CPU para el pack (no el offset)
         * @param ramBase   base del bloque de RAM del motor; 0 = esta placa no da
         * @return la imagen DEFINITIVA. Debe medir lo mismo que la declarada.
         */
        byte[] transformar(long flashAddr, long ramBase, long ramSize) throws IOException;
    }

    public long packBurn(byte[] img, long timeoutMs, BurnProgress cb) throws IOException {
        return packBurn(img, timeoutMs, null, cb);
    }

    /** Igual, con el paso de relocalización entre BEGIN y los DATA. */
    public long packBurn(byte[] img, long timeoutMs, AntesDeEnviar sello, BurnProgress cb)
            throws IOException {
        Map<String, Object> r = sendRequest("PACK_BURN_BEGIN",
                "\"size\":" + img.length, null, timeoutMs);
        if (sello != null) {
            long flashAddr = Json.getLong(r, "flashAddr", 0);
            long ramBase   = Json.getLong(r, "ramBase",   0);
            long ramSize   = Json.getLong(r, "ramSize",   0);
            /* Un firmware anterior a V5/H8 no manda estos campos. Grabar
             * igualmente sellaría el motor para la direccion 0 y la placa lo
             * rechazaría MUCHO después, con un mensaje sobre el sello que no
             * apunta aquí. Mejor pararlo con el motivo verdadero. */
            if (flashAddr == 0)
                throw new IOException("el firmware de la placa no dice dónde cae el pack"
                    + " (falta `flashAddr` en PACK_BURN_BEGIN). Es anterior a V5/H8:"
                    + " no se puede grabar un pack con motor nativo hasta actualizarlo.");
            int antes = img.length;
            img = sello.transformar(flashAddr, ramBase, ramSize);
            /* El tamaño YA se declaró en BEGIN y el device ya borró para él. */
            if (img.length != antes)
                throw new IOException("la relocalización cambió el tamaño del pack ("
                    + antes + " → " + img.length + "); el device ya reservó el hueco");
        }
        int chunk = (int) Json.getLong(r, "chunkMax", 4096);
        chunk -= chunk % 16;                       // contrato: chunks múltiplos de 16
        if (chunk <= 0) chunk = 4096;
        if (cb != null) cb.avance(0, img.length);
        for (int o = 0; o < img.length; o += chunk) {
            int n = Math.min(chunk, img.length - o);
            byte[] part = java.util.Arrays.copyOfRange(img, o, o + n);
            sendRequest("PACK_BURN_DATA", null, part, timeoutMs);
            if (cb != null) cb.avance(o + n, img.length);
        }
        reiniciaTrasPack = false;
        Map<String, Object> end = sendRequest("PACK_BURN_END", null, null, timeoutMs);
        reiniciaTrasPack = Json.getBool(end, "reinicia", false);   /* V7 #671 */
        return Json.getLong(end, "offset", -1);
    }

    /** PACK_FORMAT — borra la zona de packs ENTERA (estreno de la zona o
     *  recuperación de cadena corrupta: una zona recién reparticionada lleva
     *  restos de su uso anterior). Requiere confirm=YES en el wire; la UI pide
     *  la confirmación al usuario ANTES de llamar aquí. */
    public void packFormat(long timeoutMs) throws IOException {
        reiniciaTrasPack = false;
        Map<String, Object> r = sendRequest("PACK_FORMAT", "\"confirm\":\"YES\"", null, timeoutMs);
        reiniciaTrasPack = Json.getBool(r, "reinicia", false);   /* V7 #671 */
    }

    /** V7 #671 (8-oct) — la última respuesta de PACK_BURN_END, PACK_DEL o PACK_FORMAT dijo {@code "reinicia":true}: la
     *  placa tenía motores cargados y se está reiniciando para cargar los de la zona nueva (las tablas de los de antes
     *  apuntaban a la flash de antes: la DK2 cayó así). Quien graba lo dice y vuelve a conectar. */
    private volatile boolean reiniciaTrasPack = false;
    public boolean reiniciaTrasPack() { return reiniciaTrasPack; }

    /** PACK_DEL — tombstone del pack en `offset`: se MARCA borrado (no se
     *  borra); el espacio se recupera compactando desde el PC. */
    public void packDel(long offset, long timeoutMs) throws IOException {
        reiniciaTrasPack = false;
        Map<String, Object> r = sendRequest("PACK_DEL", "\"offset\":" + offset, null, timeoutMs);
        reiniciaTrasPack = Json.getBool(r, "reinicia", false);   /* V7 #671 */
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
        if (flashDir != null && !flashDir.isEmpty()) {
            argv.add("--flash-dir=" + flashDir);   // V8 #808: la VM-Java sólo lo admite con '='
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
                this.inRaw  = conLatido(new BufferedInputStream(socket.getInputStream()));
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
        } catch (Throwable e) {
            // purejavacomm/jtermios lanza una RuntimeException (JTermiosImpl$Fail)
            // cuando el select() del puerto serie falla — típico si el device se
            // desconecta o se REINICIA a mitad de un RUN (la USB CDC cae). Antes
            // sólo capturábamos IOException, así que ese caso mataba el hilo lector
            // con un stack trace crudo y dejaba los requests pendientes colgados.
            // Lo tratamos como fin de conexión: log claro + (en el finally) fallar
            // los pending para que sus callers no se queden esperando.
            if (!closed) diag("[BpvmClient] readLoop terminó ("
                    + e.getClass().getSimpleName() + "): " + e.getMessage()
                    + " — conexión con la VM/device perdida (¿reinicio del device?)");
        } finally {
            // Falla cualquier request pendiente para no colgar a sus callers,
            // pase lo que pase (IOException, JTermios$Fail, close()).
            java.util.List<CompletableFuture<Map<String, Object>>> toFail;
            synchronized (pendingLock) {
                toFail = new ArrayList<>(pendingRequests.values());
                pendingRequests.clear();
                troceados.clear();
            }
            if (!toFail.isEmpty()) {
                IOException ex = new IOException(
                        "conexión con la VM/device perdida (reader detenido)");
                for (CompletableFuture<Map<String, Object>> f : toFail) {
                    f.completeExceptionally(ex);
                }
            }
            seHaPerdido();                                   // #803: si la cerramos desde aquí, no hace nada
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
            Map<String, Object> resp = m;
            if (TROCEADOS.contains(type)) {                 // #495 (paso 4): un trozo de una respuesta troceada
                resp = juntaTrozo(reqId, type, m);
                if (resp == null) return;                   // ...y la petición sigue esperando el siguiente
            } else if ("ERROR".equals(type)) {
                synchronized (pendingLock) { troceados.remove(reqId); }   // una troceada cortada a medias
            }
            CompletableFuture<Map<String, Object>> fut;
            synchronized (pendingLock) { fut = pendingRequests.remove(reqId); }
            if (fut != null) {
                fut.complete(resp);
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
            case "ERROR": {
                /* V8 #693 — un ERROR SIN id: la placa tiró una línea que le llegó a medias (LINE_TIMEOUT) y no sabe de
                 * qué orden era. Es la última que salió de aquí —con otra detrás, la línea no se habría estancado—: si
                 * esa orden espera respuesta, se le da este ERROR y quien la pidió la repite; si no, queda dicho. */
                String code = Json.getString(m, "code", "?");
                CompletableFuture<Map<String, Object>> fut = null;
                if (LINEA_ESTANCADA.equals(code)) {
                    synchronized (pendingLock) { fut = pendingRequests.remove(ultimoIdEscrito); }
                }
                if (fut != null) fut.complete(m);
                else diag("[BpvmClient] ERROR [" + code + "]: " + Json.getString(m, "message", ""));
                break;
            }
            case "OUTPUT": {
                senalesDePrograma++;                        // V8 #693 (sólo escribe este hilo)
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
                        // H6.b.3.b: la VM-C device-role emite el pc absoluto como
                        // `pc` (la VM-Java lo manda como `absPc`). Aceptamos ambos
                        // para que relPc = absPc - cs salga bien con cualquier device.
                        (int) Json.getLong(m, "absPc", Json.getLong(m, "pc", 0)),
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
                senalesDePrograma++;                        // V8 #693 (sólo escribe este hilo)
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
                // V6/E1 — `elapsedMs`: lo mandan los seis emisores y el IDE no
                // lo habia leido nunca. -1 = el peer no lo manda (firmware viejo).
                fire(new ExitedEvent(
                        (int) Json.getLong(m, "exitCode", 0),
                        reason,
                        Json.getLong(m, "elapsedMs", -1L),
                        Json.getString(m, "stackTrace", "")));   // V8 #582: dónde
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
