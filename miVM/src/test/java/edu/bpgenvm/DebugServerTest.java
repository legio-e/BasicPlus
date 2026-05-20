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
import java.util.Map;
import java.util.concurrent.TimeUnit;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Integración del DebugServer: abre puerto, conecta cliente, verifica
 * mensajes en las dos direcciones. No arranca un .mod real para que el
 * test sea rápido y determinista.
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

                // 1) Mensaje hello inicial.
                String helloLine = in.readLine();
                assertNotNull(helloLine, "el server debe enviar hello al conectar");
                Map<String, Object> hello = Json.parseFlatObject(helloLine);
                assertEquals("hello", Json.getString(hello, "type", ""));
                assertEquals(DebugServer.SERVER_BANNER, Json.getString(hello, "vm", ""));

                // 2) Emitimos un PausedEvent y verificamos que llega.
                controller.addListener(server::onEventForTest);   // helper si hace falta
                // Atajamos: usamos el sink del VM (el DebugServer ya lo cambió
                // a SocketSink al accept) para emitir un chunk de print.
                OutputSink sink = vm.getProgramOut();
                assertFalse(sink instanceof StdoutSink,
                        "tras accept el sink debió cambiarse a SocketSink");
                sink.writeText("hola");
                sink.newline();

                // A1.7: chunking — writeText + newline → UNA sola línea JSON
                // con `data` = "hola\n".
                String l1 = in.readLine();
                assertNotNull(l1, "esperaba primera línea de print");
                Map<String,Object> m1 = Json.parseFlatObject(l1);
                assertEquals("print", Json.getString(m1, "type", ""));
                assertEquals("hola\n", Json.getString(m1, "data", ""));

                // 3) Enviamos un comando setBreakpoint y verificamos efecto.
                PrintWriter out = new PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);
                out.println("{\"cmd\":\"setBreakpoint\",\"file\":\"foo.bp\",\"line\":42,\"enabled\":true}");
                // Damos un breve respiro al reader thread del server.
                Thread.sleep(50);
                assertTrue(controller.isBreakpointAt("foo.bp", 42),
                        "el comando debió añadir el breakpoint");

                out.println("{\"cmd\":\"setBreakpoint\",\"file\":\"foo.bp\",\"line\":42,\"enabled\":false}");
                Thread.sleep(50);
                assertFalse(controller.isBreakpointAt("foo.bp", 42),
                        "el comando debió quitarlo");
            }
        }
    }

    @Test
    @Timeout(10)
    void chunkingDePrintAgrupaHastaNewline() throws Exception {
        // A1.7: el SocketSink consolida writeText/writeChar en un solo
        // mensaje por línea. Verificamos que un print "5\n" emite UNA línea
        // JSON (no dos), conteniendo "5\n" en `data`.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                in.readLine();   // hello, descartado

                OutputSink sink = vm.getProgramOut();
                // Caso 1: dos writeText + un newline → UN solo mensaje.
                sink.writeText("hola ");
                sink.writeText("mundo");
                sink.newline();
                String l1 = in.readLine();
                Map<String,Object> m1 = Json.parseFlatObject(l1);
                assertEquals("print", Json.getString(m1, "type", ""));
                assertEquals("hola mundo\n", Json.getString(m1, "data", ""));

                // Caso 2: writeChar repetido + newline → UN solo mensaje.
                sink.writeChar('a');
                sink.writeChar('b');
                sink.writeChar('c');
                sink.newline();
                String l2 = in.readLine();
                Map<String,Object> m2 = Json.parseFlatObject(l2);
                assertEquals("print", Json.getString(m2, "type", ""));
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
    void queryGetLocalsSinPausaDevuelveError() throws Exception {
        // A1.6: si no hay pausa, getLocals debe responder con error.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                in.readLine();   // hello

                java.io.PrintWriter out = new java.io.PrintWriter(
                        new java.io.OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8),
                        true);
                out.println("{\"req\":\"getLocals\",\"requestId\":42}");

                String resp = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(resp);
                assertEquals("error",  Json.getString(m, "resp", ""));
                assertEquals(42L,      Json.getLong(m, "requestId", -1));
                assertTrue(Json.getString(m, "message", "").contains("no está pausada"));
            }
        }
    }

    @Test
    @Timeout(10)
    void exitedEventLlegaAlCliente() throws Exception {
        // A1.7: cuando el controller emite ExitedEvent (lo hace Main al
        // terminar vm.run()), el cliente debe recibir el JSON correspondiente.
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                in.readLine();   // hello

                // Simulamos lo que hace Main.java al terminar la ejecución.
                controller.emitEvent(new edu.bpgenvm.vm.debug.ExitedEvent(0, "main returned"));

                String resp = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(resp);
                assertEquals("exited",        Json.getString(m, "type", ""));
                assertEquals(0L,              Json.getLong(m, "exitCode", -1));
                assertEquals("main returned", Json.getString(m, "reason", ""));
            }
        }
    }

    @Test
    @Timeout(10)
    void serverEnviaPausedJsonConCamposEsperados() throws Exception {
        int port = freePort();
        VirtualMachine vm = new VirtualMachine();
        DebugController controller = new DebugController();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                server.awaitClient(2, TimeUnit.SECONDS);
                BufferedReader in = new BufferedReader(
                        new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                in.readLine();    // hello, ignorado

                // Forzamos un evento: invocamos el onEvent del server directamente.
                // (El flujo "natural" pasaría por el hook + VM, pero aquí queremos
                // validar SÓLO la serialización.)
                server.onEventForTest(new PausedEvent(
                        3, 1234, 42, "C:/x/foo.bp",
                        1000, 1008, 50, 262144));

                String line = in.readLine();
                Map<String,Object> m = Json.parseFlatObject(line);
                assertEquals("paused", Json.getString(m, "type", ""));
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
