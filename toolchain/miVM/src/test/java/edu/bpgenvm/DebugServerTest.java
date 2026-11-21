package edu.bpgenvm;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.OutputSink;
import edu.bpgenvm.vm.StdoutSink;
import edu.bpgenvm.vm.VirtualMachine;
import edu.bpgenvm.vm.debug.DebugController;
import edu.bpgenvm.vm.debug.DebugServer;
import edu.bpgenvm.vm.debug.PausedEvent;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Integración del DebugServer: abre puerto, conecta cliente, verifica
 * mensajes en las dos direcciones. No arranca un .mod real para que el
 * test sea rápido y determinista. Ahora habla el protocolo v1 (PR-1).
 */
class DebugServerTest {

    /** Reserva un puerto libre, lo cierra inmediatamente y lo devuelve.
     *  Hay una pequeña race entre el close y que el server abra de nuevo
     *  el mismo puerto, pero en práctica es fiable en localhost. */
    private static int freePort() throws IOException {
        try (ServerSocket s = new ServerSocket(0)) {
            return s.getLocalPort();
        }
    }

    @Test
    @Timeout(10)
    void handshakeYEventosLleganAlCliente() throws Exception {
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();

        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);

            // Conectamos como cliente.
            try (Socket s = new Socket("localhost", port)) {
                assertTrue(server.awaitClient(2, TimeUnit.SECONDS),
                        "el servidor debió aceptar nuestra conexión");

                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                // 1) HELLO handshake (v1 §5.1): cliente manda HELLO, server
                //    responde con HELLO_REPLY incluyendo protoVersion,
                //    serverName y capabilities.
                out.println("{\"type\":\"HELLO\",\"id\":1,\"protoVersion\":1,\"clientName\":\"test\"}");
                String helloLine = in.readLine();
                assertNotNull(helloLine, "el server debe responder al HELLO");
                Map<String, Object> hello = Json.parseFlatObject(helloLine);
                assertEquals("HELLO_REPLY", Json.getString(hello, "type", ""));
                assertEquals(1L, Json.getLong(hello, "id", -1));
                assertEquals(1L, Json.getLong(hello, "protoVersion", -1));
                assertEquals(DebugServer.SERVER_NAME, Json.getString(hello, "serverName", ""));

                // 2) Emitimos un OUTPUT desde el sink y verificamos que llega.
                OutputSink sink = vm.getProgramOut();
                assertFalse(sink instanceof StdoutSink,
                        "tras accept el sink debió cambiarse a SocketSink");
                sink.writeText("hola");
                sink.newline();

                // PR-1: chunking — writeText + newline → UNA sola línea JSON
                // OUTPUT con `data` = "hola\n".
                String l1 = in.readLine();
                assertNotNull(l1, "esperaba primera línea de OUTPUT");
                Map<String,Object> m1 = Json.parseFlatObject(l1);
                assertEquals("OUTPUT", Json.getString(m1, "type", ""));
                assertEquals("hola\n", Json.getString(m1, "data", ""));

                // 3) Enviamos un SET_BP y verificamos efecto + reply.
                out.println("{\"type\":\"SET_BP\",\"id\":2,\"file\":\"foo.bp\",\"line\":42,\"enabled\":true}");
                String r1 = in.readLine();
                Map<String,Object> mr1 = Json.parseFlatObject(r1);
                assertEquals("SET_BP_REPLY", Json.getString(mr1, "type", ""));
                assertEquals(2L, Json.getLong(mr1, "id", -1));
                assertTrue(controller.isBreakpointAt("foo.bp", 42),
                        "el SET_BP debió añadir el breakpoint");

                out.println("{\"type\":\"SET_BP\",\"id\":3,\"file\":\"foo.bp\",\"line\":42,\"enabled\":false}");
                String r2 = in.readLine();
                Map<String,Object> mr2 = Json.parseFlatObject(r2);
                assertEquals("SET_BP_REPLY", Json.getString(mr2, "type", ""));
                assertEquals(3L, Json.getLong(mr2, "id", -1));
                assertFalse(controller.isBreakpointAt("foo.bp", 42),
                        "el SET_BP enabled=false debió quitarlo");
            }
        }
    }

    @Test
    @Timeout(10)
    void chunkingDeOutputAgrupaHastaNewline() throws Exception {
        // PR-1: el SocketSink consolida writeText/writeChar en un solo
        // mensaje OUTPUT por línea. Verificamos que un print "5\n" emite UNA
        // línea JSON (no dos), conteniendo "5\n" en `data`.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));

                OutputSink sink = vm.getProgramOut();
                // Caso 1: dos writeText + un newline → UN solo mensaje.
                sink.writeText("hola ");
                sink.writeText("mundo");
                sink.newline();
                String l1 = in.readLine();
                Map<String,Object> m1 = Json.parseFlatObject(l1);
                assertEquals("OUTPUT", Json.getString(m1, "type", ""));
                assertEquals("hola mundo\n", Json.getString(m1, "data", ""));

                // Caso 2: writeChar repetido + newline → UN solo mensaje.
                sink.writeChar('a');
                sink.writeChar('b');
                sink.writeChar('c');
                sink.newline();
                String l2 = in.readLine();
                Map<String,Object> m2 = Json.parseFlatObject(l2);
                assertEquals("OUTPUT", Json.getString(m2, "type", ""));
                assertEquals("abc\n", Json.getString(m2, "data", ""));

                // Caso 3: flush() explícito sin newline emite lo pendiente.
                sink.writeText("parcial");
                sink.flush();
                String l3 = in.readLine();
                Map<String,Object> m3 = Json.parseFlatObject(l3);
                assertEquals("parcial", Json.getString(m3, "data", ""));
            }
        }
    }

    @Test
    @Timeout(10)
    void queryLocalsSinPausaDevuelveError() throws Exception {
        // Si no hay pausa, LOCALS debe responder con ERROR.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));

                java.io.PrintWriter out = new java.io.PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);
                out.println("{\"type\":\"LOCALS\",\"id\":42}");

                String resp = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(resp);
                assertEquals("ERROR", Json.getString(m, "type", ""));
                assertEquals(42L,     Json.getLong(m, "id", -1));
                assertTrue(Json.getString(m, "message", "").contains("no está pausada"));
            }
        }
    }

    @Test
    @Timeout(10)
    void exitedEventLlegaAlCliente() throws Exception {
        // Cuando el controller emite ExitedEvent (lo hace Main al
        // terminar vm.run()), el cliente debe recibir EXITED en v1.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));

                // Simulamos lo que hace Main.java al terminar la ejecución.
                controller.emitEvent(new edu.bpgenvm.vm.debug.ExitedEvent(0, "main returned"));

                String resp = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(resp);
                // PR-align-vms: EXITED ahora habla canon v1 — status+errorMessage
                // en lugar de reason. exitCode==0 mapea a status="OK" sin
                // errorMessage. Para preservar el detalle "main returned" del
                // ExitedEvent, lo emitimos como errorMessage cuando reason!=""
                // (ver DebugServer.onEvent).
                assertEquals("EXITED",        Json.getString(m, "type", ""));
                assertEquals(0L,              Json.getLong(m, "exitCode", -1));
                assertEquals("OK",            Json.getString(m, "status", ""));
                assertEquals("main returned", Json.getString(m, "errorMessage", ""));
            }
        }
    }

    @Test
    @Timeout(10)
    void exitedDeEnlaceEsLinkErrorComoEnElDevice() throws Exception {
        // V7/H2 (#523, 17-sep) — Main pone exitReason = "link: <texto del cargador>" cuando
        // falla el enlace; por el wire tiene que salir como en el device (repl_v1.c):
        // status LINK_ERROR y errorMessage con el texto PELADO, sin el prefijo.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                controller.emitEvent(new edu.bpgenvm.vm.debug.ExitedEvent(1,
                        "link: 'Lib' es MOD8 y 'App' necesita su tabla de clases: recompila 'Lib'"));
                Map<String,Object> m = Json.parseFlatObject(in.readLine());
                assertEquals("EXITED",     Json.getString(m, "type", ""));
                assertEquals(1L,           Json.getLong(m, "exitCode", -1));
                assertEquals("LINK_ERROR", Json.getString(m, "status", ""));
                assertEquals("'Lib' es MOD8 y 'App' necesita su tabla de clases: recompila 'Lib'",
                             Json.getString(m, "errorMessage", ""));
            }
        }
    }

    @Test
    @Timeout(10)
    void serverEnviaBpHitJsonConCamposEsperados() throws Exception {
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));

                // Forzamos un evento: invocamos el onEvent del server directamente.
                // (El flujo "natural" pasaría por el hook + VM, pero aquí queremos
                // validar SÓLO la serialización v1.)
                server.onEventForTest(new PausedEvent(
                        3, 1234, 42, "C:/x/foo.bp",
                        1000, 1008, 50, 262144));

                String line = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(line);
                assertEquals("BP_HIT", Json.getString(m, "type", ""));
                // PR-3: session=0 sin RUN previo. PR-5: file/line dentro
                // del subobjeto `frame`; los campos planos restantes son
                // extensión Java-only (tid/absPc/bp/sp/cs/stackBase).
                assertEquals(0L,    Json.getLong(m, "session", -1));
                assertEquals(0L,    Json.getLong(m, "bpId", -1));
                assertEquals(3L,    Json.getLong(m, "tid", -1));
                Map<String,Object> frame = Json.getMap(m, "frame");
                assertEquals(42L,   Json.getLong(frame, "line", -1));
                assertEquals("C:/x/foo.bp", Json.getString(frame, "file", ""));
                assertEquals(1234L, Json.getLong(m, "absPc", -1));
                assertEquals(1000L, Json.getLong(m, "bp", -1));
                assertEquals(1008L, Json.getLong(m, "sp", -1));
            }
        }
    }

    @Test
    @Timeout(10)
    void pingDevuelvePong() throws Exception {
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                out.println("{\"type\":\"PING\",\"id\":11}");
                Map<String,Object> m = Json.parseFlatObject(in.readLine());
                assertEquals("PONG", Json.getString(m, "type", ""));
                assertEquals(11L,    Json.getLong(m, "id", -1));
            }
        }
    }

    @Test
    @Timeout(10)
    void infoDevuelveCamposEsperados() throws Exception {
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                out.println("{\"type\":\"INFO\",\"id\":12}");
                Map<String,Object> m = Json.parseFlatObject(in.readLine());
                assertEquals("INFO_REPLY", Json.getString(m, "type", ""));
                assertEquals(12L,          Json.getLong(m, "id", -1));
                assertEquals("java-host",  Json.getString(m, "boardName", ""));
                assertTrue(Json.getString(m, "uniqueId", "").startsWith("j"));
                // uptimeMs siempre >= 0.
                assertTrue(Json.getLong(m, "uptimeMs", -1) >= 0);
            }
        }
    }

    @Test
    @Timeout(10)
    void bootselDevuelveUnsupportedEnJava() throws Exception {
        // BOOTSEL/SAVE/LOG_DUMP son sólo para el firmware Pico. En la VM
        // Java el server debe responder con ERROR code=UNSUPPORTED.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                out.println("{\"type\":\"BOOTSEL\",\"id\":13}");
                Map<String,Object> m = Json.parseFlatObject(in.readLine());
                assertEquals("ERROR",       Json.getString(m, "type", ""));
                assertEquals(13L,           Json.getLong(m, "id", -1));
                assertEquals("UNSUPPORTED", Json.getString(m, "code", ""));
            }
        }
    }

    @Test
    @Timeout(10)
    void jsonInvalidoProduceFatalYCierraConexion() throws Exception {
        // PR-6 / v1 §8.3: JSON inválido → FATAL + cierre.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                // JSON roto (} cerrando sin contenido).
                out.println("{esto no es json}");
                String line = in.readLine();
                assertNotNull(line);
                Map<String,Object> m = Json.parseFlatObject(line);
                assertEquals("FATAL", Json.getString(m, "type", ""));
                assertEquals("PROTOCOL_ERROR", Json.getString(m, "code", ""));

                // El siguiente readLine debe ver EOF — el server cerró.
                String next = in.readLine();
                assertNull(next, "tras FATAL el server debió cerrar la conexión");
            }
        }
    }

    @Test
    @Timeout(10)
    void setBpDevuelveRealBpIdYListBpLoEnumera() throws Exception {
        // PR-5: SET_BP_REPLY devuelve un bpId real (no más 0 placeholder).
        // LIST_BP enumera los breakpoints registrados.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                // SET_BP en a.bp:10
                out.println("{\"type\":\"SET_BP\",\"id\":1,\"file\":\"a.bp\",\"line\":10,\"enabled\":true}");
                Map<String,Object> r1 = Json.parseFlatObject(in.readLine());
                assertEquals("SET_BP_REPLY", Json.getString(r1, "type", ""));
                long bp1 = Json.getLong(r1, "bpId", -1);
                assertTrue(bp1 > 0, "bpId debe ser > 0; obtuvo " + bp1);

                // SET_BP en b.bp:20 — bpId distinto.
                out.println("{\"type\":\"SET_BP\",\"id\":2,\"file\":\"b.bp\",\"line\":20,\"enabled\":true}");
                Map<String,Object> r2 = Json.parseFlatObject(in.readLine());
                long bp2 = Json.getLong(r2, "bpId", -1);
                assertNotEquals(bp1, bp2);

                // LIST_BP devuelve ambos.
                out.println("{\"type\":\"LIST_BP\",\"id\":3}");
                Map<String,Object> r3 = Json.parseFlatObject(in.readLine());
                assertEquals("LIST_BP_REPLY", Json.getString(r3, "type", ""));
                @SuppressWarnings("unchecked")
                java.util.List<Object> bps = (java.util.List<Object>) r3.get("breakpoints");
                assertEquals(2, bps.size());

                // CLR_BP por bpId borra solo el indicado.
                out.println("{\"type\":\"CLR_BP\",\"id\":4,\"bpId\":" + bp1 + "}");
                Map<String,Object> r4 = Json.parseFlatObject(in.readLine());
                assertEquals("CLR_BP_REPLY", Json.getString(r4, "type", ""));
                assertFalse(controller.isBreakpointAt("a.bp", 10));
                assertTrue(controller.isBreakpointAt("b.bp", 20));
            }
        }
    }

    // ---- V7/H8 #617: la subida por trozos, la regla de las placas, y volver a escuchar ----

    /** Un mensaje del wire —la línea y, si anuncia bulk, sus bytes— leído del flujo crudo (como el lector del IDE). */
    private static Map<String, Object> mensaje(java.io.InputStream in) throws IOException {
        Map<String, Object> m = Json.parseFlatObject(edu.bpgenvm.util.WireFraming.recvLine(in));
        long n = Json.getLong(m, "bulk", 0);
        if (n > 0) m.put("__bulk", edu.bpgenvm.util.WireFraming.recvBulk(in, (int) n));
        return m;
    }

    private static void manda(java.io.OutputStream out, String linea, byte[] bulk) throws IOException {
        edu.bpgenvm.util.WireFraming.sendLine(out, linea);
        if (bulk != null) edu.bpgenvm.util.WireFraming.sendBulk(out, bulk);
        out.flush();
    }

    private static VirtualMachine vmConWorkdir(java.nio.file.Path dir) {
        VirtualMachine vm = new VirtualMachine();
        edu.bpgenvm.vm.ModuleManager mm = new edu.bpgenvm.vm.ModuleManager(vm);   // como Main con --workdir
        mm.setWorkdir(dir);
        vm.setModuleManager(mm);
        return vm;
    }

    /** V7/H9 #624 — el STAT de un módulo dice su formato (MAGIC) y, desde MOD8, su versión (`gen`, +36) y su interfaz
     *  (`ifaceCrc`, +40), como la placa: con eso decide quien sube (formato compatible y versión >= la mínima). */
    @Test
    @Timeout(15)
    void statDeUnModuloDaSuVersion(@org.junit.jupiter.api.io.TempDir java.nio.file.Path dir) throws Exception {
        int port = freePort();
        java.nio.ByteBuffer cab = java.nio.ByteBuffer.allocate(52);   // big-endian, como el .mod
        cab.put(new byte[] {'M', 'O', 'D', '9'}).position(36);
        cab.putInt(7).putInt(0xDEADBEEF);
        byte[] mod9 = cab.array();
        java.nio.file.Files.createDirectories(dir.resolve("app"));
        java.nio.file.Files.write(dir.resolve("app/Ver.mod"), mod9);
        java.nio.file.Files.write(dir.resolve("app/raw.bin"), new byte[] {1, 2, 3});
        java.util.zip.CRC32 crc = new java.util.zip.CRC32();
        crc.update(mod9);
        try (DebugServer server = new DebugServer(vmConWorkdir(dir), new DebugController())) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                java.io.InputStream in = new java.io.BufferedInputStream(s.getInputStream());
                java.io.OutputStream out = s.getOutputStream();
                manda(out, "{\"type\":\"STAT\",\"id\":1,\"path\":\"app/Ver.mod\",\"crc\":true}", null);
                Map<String, Object> r = mensaje(in);
                assertEquals("STAT_REPLY", Json.getString(r, "type", ""));
                assertEquals("MOD9", Json.getString(r, "magic", null));
                assertEquals(7L, Json.getLong(r, "gen", -1));
                assertEquals(0xDEADBEEFL, Json.getLong(r, "ifaceCrc", -1));
                assertEquals(crc.getValue(), Json.getLong(r, "crc", -1));
                manda(out, "{\"type\":\"STAT\",\"id\":2,\"path\":\"app/raw.bin\"}", null);
                Map<String, Object> r2 = mensaje(in);
                assertEquals(3L, Json.getLong(r2, "size", -1));
                assertNull(Json.getString(r2, "magic", null), "lo que no es módulo, sin magic");
                assertEquals(-1L, Json.getLong(r2, "gen", -1));
            }
        }
    }

    @Test
    @Timeout(15)
    void subidaPorTrozosYBajadaTroceada(@org.junit.jupiter.api.io.TempDir java.nio.file.Path dir) throws Exception {
        int port = freePort();
        byte[] datos = new byte[20000];
        new java.util.Random(617).nextBytes(datos);
        try (DebugServer server = new DebugServer(vmConWorkdir(dir), new DebugController())) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                java.io.InputStream in = new java.io.BufferedInputStream(s.getInputStream());
                java.io.OutputStream out = s.getOutputStream();
                manda(out, "{\"type\":\"PUT_BEGIN\",\"id\":1,\"path\":\"app/a.bin\",\"size\":20000}", null);
                assertEquals("PUT_BEGIN_REPLY", Json.getString(mensaje(in), "type", ""));
                long recibido = 0;
                for (int k = 0, id = 2; k < datos.length; k += 8192, id++) {
                    byte[] t = java.util.Arrays.copyOfRange(datos, k, Math.min(k + 8192, datos.length));
                    manda(out, "{\"type\":\"PUT_DATA\",\"id\":" + id + ",\"bulk\":" + t.length + "}", t);
                    recibido = Json.getLong(mensaje(in), "received", -1);
                }
                assertEquals(20000L, recibido);
                manda(out, "{\"type\":\"PUT_END\",\"id\":9,\"size\":20000}", null);
                Map<String, Object> fin = mensaje(in);
                assertEquals("PUT_END_REPLY", Json.getString(fin, "type", ""));
                assertArrayEquals(datos, java.nio.file.Files.readAllBytes(dir.resolve("app/a.bin")));

                // y bajada troceada (#495 paso 4): GET_PART seguidos por off y GET_END con el CRC-32
                manda(out, "{\"type\":\"GET\",\"id\":10,\"path\":\"app/a.bin\",\"chunked\":true}", null);
                java.io.ByteArrayOutputStream junto = new java.io.ByteArrayOutputStream();
                Map<String, Object> m;
                while ("GET_PART".equals(Json.getString(m = mensaje(in), "type", ""))) {
                    assertEquals((long) junto.size(), Json.getLong(m, "off", -1));
                    junto.write((byte[]) m.get("__bulk"));
                }
                assertEquals("GET_END", Json.getString(m, "type", ""));
                java.util.zip.CRC32 crc = new java.util.zip.CRC32();
                crc.update(datos);
                assertEquals(crc.getValue(), Json.getLong(m, "crc", -1));
                assertArrayEquals(datos, junto.toByteArray());
            }
        }
    }

    @Test
    @Timeout(15)
    void laSubidaQueNoTerminaSeBorra(@org.junit.jupiter.api.io.TempDir java.nio.file.Path dir) throws Exception {
        int port = freePort();
        try (DebugServer server = new DebugServer(vmConWorkdir(dir), new DebugController())) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                java.io.InputStream in = new java.io.BufferedInputStream(s.getInputStream());
                java.io.OutputStream out = s.getOutputStream();
                // a medias y llega un HELLO: lo subido, borrado, y no queda sesión
                manda(out, "{\"type\":\"PUT_BEGIN\",\"id\":1,\"path\":\"app/r.bin\",\"size\":4096}", null);
                mensaje(in);
                manda(out, "{\"type\":\"PUT_DATA\",\"id\":2,\"bulk\":512}", new byte[512]);
                assertEquals(512L, Json.getLong(mensaje(in), "received", -1));
                assertTrue(java.nio.file.Files.exists(dir.resolve("app/r.bin")));
                manda(out, "{\"type\":\"HELLO\",\"id\":3,\"protoVersion\":1}", null);
                assertEquals("HELLO_REPLY", Json.getString(mensaje(in), "type", ""));
                assertFalse(java.nio.file.Files.exists(dir.resolve("app/r.bin")), "la subida a medias se borra");
                manda(out, "{\"type\":\"PUT_DATA\",\"id\":4,\"bulk\":8}", new byte[8]);
                assertEquals("NO_SESSION", Json.getString(mensaje(in), "code", ""));
                // un PUT_END que no cuadra: SIZE_MISMATCH, y lo subido no se queda
                manda(out, "{\"type\":\"PUT_BEGIN\",\"id\":5,\"path\":\"app/c.bin\",\"size\":100}", null);
                mensaje(in);
                manda(out, "{\"type\":\"PUT_DATA\",\"id\":6,\"bulk\":50}", new byte[50]);
                mensaje(in);
                manda(out, "{\"type\":\"PUT_END\",\"id\":7,\"size\":100}", null);
                assertEquals("SIZE_MISMATCH", Json.getString(mensaje(in), "code", ""));
                assertFalse(java.nio.file.Files.exists(dir.resolve("app/c.bin")), "lo que no está entero no se queda");
                // y otra a medias cuando el cliente se va (abajo, al cerrar el socket)
                manda(out, "{\"type\":\"PUT_BEGIN\",\"id\":8,\"path\":\"app/v.bin\",\"size\":100}", null);
                mensaje(in);
                assertTrue(java.nio.file.Files.exists(dir.resolve("app/v.bin")));
            }
            long t0 = System.currentTimeMillis();
            while (java.nio.file.Files.exists(dir.resolve("app/v.bin")) && System.currentTimeMillis() - t0 < 3000)
                Thread.sleep(20);
            assertFalse(java.nio.file.Files.exists(dir.resolve("app/v.bin")), "el cliente se fue: la subida a medias, borrada");
        }
    }

    @Test
    @Timeout(15)
    void elServidorVuelveAEscucharCuandoSeVaElCliente() throws Exception {
        int port = freePort();
        try (DebugServer server = new DebugServer(new VirtualMachine(), new DebugController())) {
            server.start(port);
            for (int vez = 1; vez <= 3; vez++) {
                try (Socket s = new Socket("localhost", port)) {
                    s.setSoTimeout(5000);
                    java.io.InputStream in = new java.io.BufferedInputStream(s.getInputStream());
                    manda(s.getOutputStream(), "{\"type\":\"HELLO\",\"id\":" + vez + ",\"protoVersion\":1}", null);
                    Map<String, Object> r = mensaje(in);
                    assertEquals("HELLO_REPLY", Json.getString(r, "type", ""), "el cliente " + vez + " tiene que tener respuesta");
                    assertEquals((long) vez, Json.getLong(r, "id", -1));
                }
            }
        }
    }

    @Test
    @Timeout(10)
    void stepEmiteStepDoneNoBpHit() throws Exception {
        // PR-3: tras un STEP, la siguiente PausedEvent del controller se
        // serializa como STEP_DONE (no BP_HIT). Sin STEP previo es BP_HIT.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);

                // 1) Cliente manda STEP — server responde STEP_REPLY y marca
                //    stepInProgress=true para la siguiente pausa.
                out.println("{\"type\":\"STEP\",\"id\":7,\"mode\":\"into\"}");
                String r1 = in.readLine();
                Map<String,Object> mr1 = Json.parseFlatObject(r1);
                assertEquals("STEP_REPLY", Json.getString(mr1, "type", ""));
                assertEquals(7L, Json.getLong(mr1, "id", -1));

                // 2) Se emite PausedEvent — debe salir como STEP_DONE.
                // PR-5: line/file dentro del subobjeto `frame` (v1 puro).
                server.onEventForTest(new PausedEvent(
                        0, 100, 5, "x.bp", 1000, 1004, 0, 65536));
                String l1 = in.readLine();
                Map<String,Object> m1 = Json.parseFlatObject(l1);
                assertEquals("STEP_DONE", Json.getString(m1, "type", ""));
                assertEquals(5L, Json.getLong(Json.getMap(m1, "frame"), "line", -1));

                // 3) Siguiente PausedEvent — ya no es step → BP_HIT.
                server.onEventForTest(new PausedEvent(
                        0, 200, 6, "x.bp", 1000, 1004, 0, 65536));
                String l2 = in.readLine();
                Map<String,Object> m2 = Json.parseFlatObject(l2);
                assertEquals("BP_HIT", Json.getString(m2, "type", ""));
                assertEquals(6L, Json.getLong(Json.getMap(m2, "frame"), "line", -1));
            }
        }
    }
}
