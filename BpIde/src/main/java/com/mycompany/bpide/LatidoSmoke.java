// ============================================================
// LatidoSmoke.java
// V8/H3 (#803): el latido del IDE, sin placa. Una placa de mentira por TCP que contesta, se calla sin cerrar o cierra,
// y el mismo camino que la de verdad: BpvmClient y el run() del backend, con plazos cortos (0,3 s + 0,3 s).
//
// Uso: java -cp BpIde/target/classes;<miVM>;<frontend> com.mycompany.bpide.LatidoSmoke
// ============================================================
package com.mycompany.bpide;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class LatidoSmoke {

    private static final long CALLADO_MS = 300;
    private static final long PLAZO_MS   = 300;

    private static int fallos = 0;

    private static void mira(boolean cond, String que) {
        System.out.println((cond ? "  ok   : " : "  FALLO: ") + que);
        if (!cond) fallos++;
    }

    /** La placa de mentira: un cliente cada vez, una línea JSON por mensaje. */
    private static final class Placa implements AutoCloseable {
        final ServerSocket ss = new ServerSocket(0);
        final AtomicInteger pings = new AtomicInteger();
        volatile boolean callada;        // no contesta a nada, y no cierra
        volatile boolean enRun;          // al PING, un ERROR: la de verdad contesta PONG (#803), pero vale cualquier respuesta
        volatile long retrasoInfoMs;     // el INFO tarda esto en contestar
        volatile Socket s;
        private static final Pattern TIPO = Pattern.compile("\"type\":\"([A-Z_]+)\"");
        private static final Pattern ID   = Pattern.compile("\"id\":(\\d+)");

        Placa() throws IOException {
            Thread t = new Thread(this::sirve, "placa-de-mentira");
            t.setDaemon(true);
            t.start();
        }

        int puerto() { return ss.getLocalPort(); }

        private void manda(String linea) throws IOException {
            OutputStream o = s.getOutputStream();
            o.write((linea + "\n").getBytes(StandardCharsets.UTF_8));
            o.flush();
        }

        void exited() throws IOException {
            manda("{\"type\":\"EXITED\",\"session\":1,\"status\":\"OK\",\"exitCode\":0,\"elapsedMs\":5}");
        }

        private void sirve() {
            try {
                for (;;) {
                    s = ss.accept();
                    BufferedReader in = new BufferedReader(
                            new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                    String l;
                    while ((l = in.readLine()) != null) {
                        Matcher mt = TIPO.matcher(l);
                        Matcher mi = ID.matcher(l);
                        if (!mt.find() || !mi.find()) continue;
                        String tipo = mt.group(1);
                        String id = mi.group(1);
                        if ("PING".equals(tipo)) pings.incrementAndGet();
                        if (callada) continue;
                        switch (tipo) {
                            case "HELLO":
                                manda("{\"type\":\"HELLO_REPLY\",\"id\":" + id
                                        + ",\"serverName\":\"mentira\",\"serverBuild\":\"0\",\"protoVersion\":1}");
                                break;
                            case "PING":
                                manda(enRun
                                        ? "{\"type\":\"ERROR\",\"id\":" + id + ",\"code\":\"BUSY\",\"message\":\"en run\"}"
                                        : "{\"type\":\"PONG\",\"id\":" + id + "}");
                                break;
                            case "RUN":
                                enRun = true;
                                manda("{\"type\":\"RUN_REPLY\",\"id\":" + id + ",\"session\":1}");
                                break;
                            case "INFO":
                                final String idInfo = id;
                                final long espera = retrasoInfoMs;
                                new Thread(() -> {
                                    try {
                                        Thread.sleep(espera);
                                        manda("{\"type\":\"INFO_REPLY\",\"id\":" + idInfo + "}");
                                    } catch (Exception ignored) { }
                                }).start();
                                break;
                            default:
                                manda("{\"type\":\"" + tipo + "_REPLY\",\"id\":" + id + "}");
                        }
                    }
                }
            } catch (IOException fin) { /* el ServerSocket, cerrado */ }
        }

        @Override public void close() throws IOException {
            ss.close();
            if (s != null) s.close();
        }
    }

    /** La placa por la red, con los plazos cortos. */
    private static final class RedRapida extends SerialBackend {
        @Override protected void openTransport(String endpoint, BpvmClient c) throws IOException {
            c.connectRemote("127.0.0.1", Integer.parseInt(endpoint));
        }
        @Override protected void postConnect(BpvmClient c) {
            c.vigilarConexion(CALLADO_MS, PLAZO_MS);
        }
    }

    private static BpvmClient cliente(Placa p, AtomicInteger avisos) throws IOException {
        BpvmClient c = new BpvmClient();
        c.connectRemote("127.0.0.1", p.puerto());
        c.alPerderse(avisos::incrementAndGet);
        c.vigilarConexion(CALLADO_MS, PLAZO_MS);
        return c;
    }

    public static void main(String[] args) throws Exception {
        System.out.println("-- la placa contesta: no se pierde");
        try (Placa p = new Placa()) {
            AtomicInteger avisos = new AtomicInteger();
            try (BpvmClient c = cliente(p, avisos)) {
                Thread.sleep(1600);
                mira(p.pings.get() >= 3, "manda el PING, y sólo con el cable callado (" + p.pings.get() + ")");
                mira(!c.conexionPerdida() && avisos.get() == 0, "no se da por perdida");
            }
            Thread.sleep(100);
            mira(avisos.get() == 0, "cerrar desde aquí no es perderse");
        }

        System.out.println("-- la placa se calla sin cerrar");
        try (Placa p = new Placa()) {
            AtomicInteger avisos = new AtomicInteger();
            try (BpvmClient c = cliente(p, avisos)) {
                long t0 = System.currentTimeMillis();
                p.callada = true;
                while (!c.conexionPerdida() && System.currentTimeMillis() - t0 < 5000) Thread.sleep(20);
                long ms = System.currentTimeMillis() - t0;
                mira(c.conexionPerdida(), "se da por perdida, en " + ms + " ms");
                mira(ms >= PLAZO_MS && ms < 2000, "ni antes del plazo ni mucho después");
                Thread.sleep(300);
                mira(avisos.get() == 1, "avisa una vez (" + avisos.get() + ")");
            }
        }

        System.out.println("-- la placa cierra");
        try (Placa p = new Placa()) {
            AtomicInteger avisos = new AtomicInteger();
            try (BpvmClient c = cliente(p, avisos)) {
                p.close();
                long t0 = System.currentTimeMillis();
                while (!c.conexionPerdida() && System.currentTimeMillis() - t0 < 3000) Thread.sleep(20);
                mira(c.conexionPerdida(), "se da por perdida");
                Thread.sleep(100);
                mira(avisos.get() == 1, "avisa una vez (" + avisos.get() + ")");
            }
        }

        System.out.println("-- una petición en vuelo tiene su plazo: sin PING");
        try (Placa p = new Placa()) {
            AtomicInteger avisos = new AtomicInteger();
            p.retrasoInfoMs = 1500;
            try (BpvmClient c = cliente(p, avisos)) {
                Thread.sleep(100);
                int antes = p.pings.get();
                c.getInfo(5000);
                mira(p.pings.get() - antes <= 1, "el cable callado 1,5 s, y PINGs " + (p.pings.get() - antes));
                mira(!c.conexionPerdida(), "no se da por perdida");
            }
        }

        System.out.println("-- un programa callado: cualquier respuesta vale, también un ERROR");
        try (Placa p = new Placa()) {
            RedRapida b = new RedRapida();
            b.connect(Integer.toString(p.puerto()));
            String[] fin = new String[1];
            Thread t = new Thread(() -> {
                try { fin[0] = b.run("/app/X.mod", null, l -> { }); }
                catch (IOException e) { fin[0] = "ERROR: " + e.getMessage(); }
            });
            t.start();
            Thread.sleep(1600);
            mira(p.pings.get() >= 2, "manda el PING en marcha (" + p.pings.get() + ")");
            mira(b.isRunActive(), "el programa sigue en marcha");
            p.exited();
            t.join(2000);
            mira(fin[0] != null && fin[0].startsWith("exit 0"), "acaba bien: " + fin[0]);
            b.close();
        }

        System.out.println("-- la placa se calla con un programa en marcha");
        try (Placa p = new Placa()) {
            RedRapida b = new RedRapida();
            b.connect(Integer.toString(p.puerto()));
            String[] fin = new String[1];
            Thread t = new Thread(() -> {
                try { fin[0] = b.run("/app/X.mod", null, l -> { }); }
                catch (IOException e) { fin[0] = "ERROR: " + e.getMessage(); }
            });
            t.start();
            Thread.sleep(200);
            long t0 = System.currentTimeMillis();
            p.callada = true;
            t.join(5000);
            long ms = System.currentTimeMillis() - t0;
            mira(!t.isAlive(), "el run() no se queda esperando (" + ms + " ms)");
            mira(fin[0] != null && fin[0].contains("se ha perdido la conexión"), "lo dice: " + fin[0]);
            b.close();
        }

        System.out.println("-- Disconnect con un programa en marcha");
        try (Placa p = new Placa()) {
            RedRapida b = new RedRapida();
            b.connect(Integer.toString(p.puerto()));
            String[] fin = new String[1];
            Thread t = new Thread(() -> {
                try { fin[0] = b.run("/app/X.mod", null, l -> { }); }
                catch (IOException e) { fin[0] = "ERROR: " + e.getMessage(); }
                catch (RuntimeException e) { fin[0] = "CAIDA: " + e; }
            });
            t.start();
            Thread.sleep(200);
            b.close();
            t.join(3000);
            mira(!t.isAlive(), "el run() acaba");
            mira(fin[0] != null && fin[0].contains("desconectado"), "lo dice: " + fin[0]);
        }

        System.out.println(fallos == 0 ? "LATIDO OK" : "LATIDO: " + fallos + " FALLO(S)");
        System.exit(fallos == 0 ? 0 : 1);
    }
}
