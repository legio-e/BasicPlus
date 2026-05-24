/*
 * PicoClient.java — cliente Java para hablar con el firmware bpvm-pico
 * sobre USB CDC (purejavacomm). Mirror exacto del cliente Python en
 * pico/scripts/bpvm-pico.py.
 *
 * Protocolo (line-based sobre UTF-8):
 *
 *   HELLO                              -> "OK bpvm-pico vX.Y..."
 *   LS                                 -> "OK <count>\n<name size>\n..."
 *   PUT <name> <size>                  -> "READY <size>", luego <bytes>, luego "OK <free> free"
 *   GET <name>                         -> "OK <size>\n<bytes>"
 *   DEL <name> | SAVE | FORMAT         -> "OK <msg>"
 *   RUN <name>                         -> "--- VM output ---\n<output>\n--- VM finished: STATUS ---"
 *   MEM                                -> "OK total=X used=Y free=Z count=N"
 *   LOG | LOGSAVE | LOGCLEAR           -> "OK ..." + payload
 *   RESET | BOOTSEL                    -> "OK ..." (la Pico se reinicia, el puerto se cae)
 *
 * Threading: las operaciones son SÍNCRONAS, bloquean al caller. El IDE
 * debe llamarlas desde un SwingWorker para no congelar la UI. RUN tiene
 * variante con callback para streaming del output en tiempo real.
 *
 * DTR: se asserta en open(). Sin DTR, el firmware descarta la salida.
 */
package com.mycompany.bpide;

import purejavacomm.CommPortIdentifier;
import purejavacomm.NoSuchPortException;
import purejavacomm.PortInUseException;
import purejavacomm.SerialPort;
import purejavacomm.UnsupportedCommOperationException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public final class PicoClient implements AutoCloseable {

    /** Pico Raspberry Pi VID. */
    public static final int RPI_VID = 0x2E8A;

    private SerialPort port;
    private InputStream  in;
    private OutputStream out;
    private final long defaultTimeoutMs = 5000;

    /** Lista los puertos COM disponibles. */
    public static List<String> listPorts() {
        List<String> out = new ArrayList<>();
        @SuppressWarnings("unchecked")
        Enumeration<CommPortIdentifier> e = CommPortIdentifier.getPortIdentifiers();
        while (e.hasMoreElements()) {
            CommPortIdentifier id = e.nextElement();
            if (id.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                out.add(id.getName());
            }
        }
        return out;
    }

    /** Intenta autodetectar el primer COM cuya descripción mencione "pico" o
     *  cuyo VID coincida con el de Raspberry Pi. purejavacomm 1.0.2 no
     *  expone VID/PID directamente, así que el matching se limita al nombre
     *  visible en el sistema. Devuelve null si no encuentra ninguno. */
    public static String autoDetect() {
        // purejavacomm no nos da VID/PID. Tomamos el primer COM> 1 que sea
        // serial — heurística mínima. El usuario puede pasar --port en CLI
        // o seleccionar del combo en la UI.
        List<String> ports = listPorts();
        for (String p : ports) {
            // Filtra puertos típicos: COM1, COM2, COM3 suelen ser legacy.
            // Prioriza COM >= 4.
            if (p.startsWith("COM")) {
                try {
                    int n = Integer.parseInt(p.substring(3));
                    if (n >= 4) return p;
                } catch (NumberFormatException ignore) { }
            }
        }
        return ports.isEmpty() ? null : ports.get(0);
    }

    /** Parámetros guardados de la última conexión exitosa, para
     *  reconnect() rápido sin tener que pasarlos otra vez. */
    private String lastPort = null;
    private int    lastBaud = 0;

    /** Abre el puerto con DTR activo y deja la VM lista para comandos. */
    public void connect(String portName, int baud) throws IOException {
        try {
            CommPortIdentifier id = CommPortIdentifier.getPortIdentifier(portName);
            port = (SerialPort) id.open("BpIde-PicoClient", 2000);
            port.setSerialPortParams(baud, SerialPort.DATABITS_8,
                    SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
            port.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
            port.setDTR(true);    // crítico: el firmware mira DTR
            port.setRTS(false);
            port.enableReceiveTimeout(500);  // 500 ms read timeout
            in  = port.getInputStream();
            out = port.getOutputStream();
            // Drain de banner / prompt residual.
            drain(300);
            // Guardar para reconnect() si lo necesitamos.
            lastPort = portName;
            lastBaud = baud;
        } catch (NoSuchPortException | PortInUseException
                | UnsupportedCommOperationException e) {
            throw new IOException("open " + portName + ": " + e.getMessage(), e);
        }
    }

    /** Cierra y vuelve a abrir el puerto con los mismos parámetros.
     *  Útil para forzar estado limpio cuando los buffers del driver
     *  USB CDC se ensucian tras una sesión larga de I/O (síntoma:
     *  timeout en la 2ª "Run on Pico" tras un RUN exitoso anterior).
     *
     *  Windows a veces tarda 100-300 ms en liberar el puerto tras
     *  close(); por eso hacemos retry con backoff. */
    public void reconnect() throws IOException {
        if (lastPort == null) throw new IOException("no previous connection to reconnect");
        String portName = lastPort;
        int baud = lastBaud;
        close();
        IOException last = null;
        // Hasta 5 intentos × 150 ms = ~750 ms de gracia para que Windows libere el handle.
        for (int attempt = 0; attempt < 5; attempt++) {
            try {
                Thread.sleep(150);
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                throw new IOException("reconnect interrumpido", ie);
            }
            try {
                connect(portName, baud);
                return;
            } catch (IOException e) {
                last = e;  // reintenta
            }
        }
        throw new IOException("reconnect agotó reintentos: "
                + (last != null ? last.getMessage() : "?"));
    }

    @Override
    public void close() {
        try { if (in  != null) in.close();  } catch (Exception ignore) {}
        try { if (out != null) out.close(); } catch (Exception ignore) {}
        try { if (port != null) port.close(); } catch (Exception ignore) {}
        in = null; out = null; port = null;
    }

    public boolean isConnected() { return port != null; }

    /* ========================= raw IO ============================= */

    /** Drena cualquier byte residual durante hasta `ms` milisegundos. */
    private void drain(long ms) throws IOException {
        long end = System.currentTimeMillis() + ms;
        byte[] tmp = new byte[256];
        while (System.currentTimeMillis() < end) {
            if (in.available() <= 0) {
                try { Thread.sleep(20); } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt(); return;
                }
                continue;
            }
            int n = in.read(tmp);
            if (n <= 0) break;
        }
    }

    /** Drena lo que haya en el buffer de entrada AHORA mismo (no espera
     *  más datos). Útil al inicio de cada comando para descartar
     *  cualquier residuo de comandos previos: prompts "> ", eco de
     *  caracteres tecleados, banners de boot, etc. */
    private void drainNow() throws IOException {
        byte[] tmp = new byte[256];
        while (in.available() > 0) {
            int n = in.read(tmp);
            if (n <= 0) break;
        }
    }

    /** Envía una línea (CRLF). Drena el input antes para que la respuesta
     *  del comando no quede solapada con basura previa (p.ej. el "> "
     *  que la REPL imprime tras terminar el RUN anterior). */
    private void sendLine(String line) throws IOException {
        drainNow();
        out.write((line + "\n").getBytes(StandardCharsets.UTF_8));
        out.flush();
    }

    /** Lee una línea (terminada por \n). Trim CR/LF. Timeout total ms. */
    private String readLine(long timeoutMs) throws IOException {
        long end = System.currentTimeMillis() + timeoutMs;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        while (System.currentTimeMillis() < end) {
            int c = in.read();
            if (c < 0) continue;
            if (c == '\r') continue;
            if (c == '\n') {
                return new String(buf.toByteArray(), StandardCharsets.UTF_8);
            }
            buf.write(c);
        }
        throw new IOException("readLine timeout (got " + buf.size() + " bytes)");
    }

    /** Lee exactamente N bytes con timeout. */
    private byte[] readBytes(int n, long timeoutMs) throws IOException {
        byte[] buf = new byte[n];
        int got = 0;
        long end = System.currentTimeMillis() + timeoutMs;
        while (got < n && System.currentTimeMillis() < end) {
            int avail = in.available();
            if (avail <= 0) {
                try { Thread.sleep(5); } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt(); break;
                }
                continue;
            }
            int r = in.read(buf, got, n - got);
            if (r > 0) got += r;
        }
        if (got < n) throw new IOException("readBytes timeout (" + got + "/" + n + ")");
        return buf;
    }

    /** Lee líneas hasta ver una que empiece por OK o ERR. Devuelve resto
     *  tras OK (sin el prefijo). Lanza IOException si ERR. */
    private String expectOk() throws IOException {
        for (int i = 0; i < 30; i++) {
            String line = readLine(defaultTimeoutMs);
            if (line.startsWith("OK")) {
                return line.substring(2).trim();
            }
            if (line.startsWith("ERR")) {
                throw new IOException(line);
            }
            // ignora prompts y banners
        }
        throw new IOException("no OK/ERR after 30 lines");
    }

    /* ========================= comandos ============================ */

    public String hello() throws IOException {
        sendLine("HELLO");
        return expectOk();
    }

    /** Sincroniza el RTC del Pico con el wall clock del PC.
     *
     *  Envía TIME &lt;epochsec&gt; con la hora local actual. El comando
     *  del REPL ejecuta bpvm_rtc_set_now_ms en el firmware. Tras
     *  esto, código BP que use Rtc.Clock.epochSec() ve hora real.
     *
     *  Idempotente — llamar varias veces solo refresca el offset.
     *  El usuario del IDE no necesita pulsar nada: se hace
     *  automáticamente al conectar (ver PicoExplorer.onConnect).
     */
    public String syncTime() throws IOException {
        long epochSec = System.currentTimeMillis() / 1000L;
        sendLine("TIME " + epochSec);
        return expectOk();
    }

    /** Listado de ficheros. */
    public static final class RemoteFile {
        public final String name;
        public final long   size;
        public RemoteFile(String name, long size) { this.name = name; this.size = size; }
        @Override public String toString() { return name + " (" + size + ")"; }
    }

    public List<RemoteFile> ls() throws IOException {
        sendLine("LS");
        String head = expectOk();  // "<count>"
        int n = Integer.parseInt(head.trim());
        List<RemoteFile> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            String line = readLine(defaultTimeoutMs);
            int sp = line.lastIndexOf(' ');
            if (sp < 0) continue;
            String name = line.substring(0, sp);
            long sz;
            try { sz = Long.parseLong(line.substring(sp + 1).trim()); }
            catch (NumberFormatException e) { sz = 0; }
            out.add(new RemoteFile(name, sz));
        }
        return out;
    }

    /** Stats: total=X used=Y free=Z count=N. Devuelve el string crudo. */
    public String mem() throws IOException {
        sendLine("MEM");
        return expectOk();
    }

    public void put(String remoteName, byte[] data) throws IOException {
        sendLine("PUT " + remoteName + " " + data.length);
        // Esperar "READY <size>"
        for (int i = 0; i < 10; i++) {
            String line = readLine(defaultTimeoutMs);
            if (line.startsWith("READY")) break;
            if (line.startsWith("ERR")) throw new IOException(line);
        }
        out.write(data);
        out.flush();
        expectOk();
    }

    public byte[] get(String name) throws IOException {
        sendLine("GET " + name);
        String head = expectOk();  // "<size>"
        int size = Integer.parseInt(head.trim());
        return readBytes(size, defaultTimeoutMs + size);   // +1ms/byte de margen
    }

    public void del(String name) throws IOException {
        sendLine("DEL " + name);
        expectOk();
    }

    public void save() throws IOException {
        sendLine("SAVE");
        expectOk();
    }

    public void format() throws IOException {
        sendLine("FORMAT");
        expectOk();
    }

    /** Callback para streaming del output del comando RUN. */
    public interface OutputSink {
        /** Llamada con cada línea de output (sin newline). */
        void onLine(String line);
    }

    /** Ejecuta un .mod remoto. Stream del output al sink. Devuelve el
     *  status final (texto, ej. "OK", "ERR_RUNTIME", ...). */
    public String run(String name, OutputSink sink) throws IOException {
        sendLine("RUN " + name);
        boolean inOutput = false;
        long lastActivity = System.currentTimeMillis();
        // RUN puede tardar mucho (fib(30) ~15s en Pico). Subimos timeout
        // por línea pero no por operación total.
        while (true) {
            String line;
            try {
                line = readLine(120000);   // 2 min por línea
            } catch (IOException e) {
                throw new IOException("RUN timeout: " + e.getMessage(), e);
            }
            lastActivity = System.currentTimeMillis();
            if (line.startsWith("ERR")) throw new IOException(line);
            if (line.contains("--- VM output ---")) {
                inOutput = true;
                continue;
            }
            if (line.startsWith("--- VM finished:")) {
                // "--- VM finished: STATUS ---"
                int a = line.indexOf(':') + 1;
                int b = line.lastIndexOf("---");
                String status = (b > a) ? line.substring(a, b).trim() : line;
                // Tras "VM finished" la REPL del Pico imprime "> " e
                // inicia un nuevo read_line. Si el siguiente comando del
                // IDE es un PUT y todavía no ha aparecido ese prompt, la
                // transferencia se queda colgada (síntoma: timeout en la
                // 2ª "Run on Pico"). Esperamos activamente al prompt aquí
                // con un timeout corto (1 s) para garantizar que la REPL
                // está lista. Si no aparece, seguimos igual — drainNow
                // del siguiente sendLine limpiará lo que sea.
                waitForPrompt(1000);
                return status;
            }
            if (inOutput && sink != null) sink.onLine(line);
        }
    }

    /** Lee y descarta del input hasta ver "> " (el prompt de la REPL),
     *  con timeout total en ms. Es resiliente a líneas intermedias —
     *  todo lo que llegue antes del prompt se ignora. No lanza si el
     *  timeout expira (el caller asume que drainNow del siguiente
     *  comando se encargará). */
    private void waitForPrompt(long timeoutMs) {
        long end = System.currentTimeMillis() + timeoutMs;
        int last = -1;
        try {
            while (System.currentTimeMillis() < end) {
                int c = in.read();
                if (c < 0) continue;
                // El prompt en el firmware es exactamente "> " (2 chars).
                // Reconocer cualquier '>' seguido de ' ' como prompt.
                if (last == '>' && c == ' ') return;
                last = c;
            }
        } catch (IOException ignored) {
            // sin prompt visible — drainNow del próximo sendLine lo
            // limpiará. No bloqueamos al usuario por esto.
        }
    }

    /** Dump del log persistente. Devuelve el texto completo. */
    public String log() throws IOException {
        sendLine("LOG");
        String head = expectOk();  // "<used>/<total> bytes"
        StringBuilder sb = new StringBuilder();
        sb.append("# ").append(head).append("\n");
        long end = System.currentTimeMillis() + 3000;
        // Leemos hasta ver un prompt "> " o silencio prolongado.
        while (System.currentTimeMillis() < end) {
            String line;
            try { line = readLine(800); }
            catch (IOException e) { break; }   // timeout = fin del dump
            if (line.equals(">") || line.trim().equals(">")) break;
            sb.append(line).append('\n');
        }
        return sb.toString();
    }

    public void logSave() throws IOException {
        sendLine("LOGSAVE");
        expectOk();
    }

    public void logClear() throws IOException {
        sendLine("LOGCLEAR");
        expectOk();
    }

    /** RESET — el puerto se cae tras esto. close() y reconectar a mano. */
    public void reset() throws IOException {
        sendLine("RESET");
        try { expectOk(); } catch (IOException ignore) { /* puerto se va */ }
    }

    /** BOOTSEL — el puerto se cae, la Pico aparece como mass storage. */
    public void bootsel() throws IOException {
        sendLine("BOOTSEL");
        try { expectOk(); } catch (IOException ignore) {}
    }
}
