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
                assertEquals("EXITED",        Json.getString(m, "type", ""));
                assertEquals(0L,              Json.getLong(m, "exitCode", -1));
                assertEquals("main returned", Json.getString(m, "reason", ""));
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
                // PR-1: bpId placeholder = 0; PR-5 lo hará único.
                assertEquals(0L,    Json.getLong(m, "bpId", -1));
                assertEquals(3L,    Json.getLong(m, "tid", -1));
                assertEquals(42L,   Json.getLong(m, "line", -1));
                assertEquals("C:/x/foo.bp", Json.getString(m, "file", ""));
                assertEquals(1234L, Json.getLong(m, "absPc", -1));
                assertEquals(1000L, Json.getLong(m, "bp", -1));
                assertEquals(1008L, Json.getLong(m, "sp", -1));
            }
        }
    }
}
