// ============================================================
// SimRunner.java
// H10 — arranque y parada del MICRO SIMULADO (bpvm-sim), la placa
// de mentira que corre en el PC. El IDE la levanta como subproceso
// y luego se conecta a ella por TCP igual que a una placa de verdad
// (mismo wire v1, mismo BpvmBackend) — por eso aquí no hay nada de
// protocolo: sólo localizar el binario, lanzarlo con la config del
// usuario, esperar a que escuche y matarlo al terminar.
//
// La "flash" y la imagen del FS viven en simDataDir y PERSISTEN entre
// arranques, como en una placa: si subes un fichero y paras el
// simulador, al volver sigue ahí. Borrar esa carpeta = placa nueva.
// ============================================================
package com.mycompany.bpide;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/** Ciclo de vida del proceso bpvm-sim. Una instancia por IDE. */
public final class SimRunner {

    private Process proc;
    private Thread  pump;
    private final Consumer<String> log;   // a la consola del IDE (puede ser null)

    public SimRunner(Consumer<String> log) { this.log = log; }

    private void say(String s) { if (log != null) log.accept(s); }

    public boolean isRunning() { return proc != null && proc.isAlive(); }

    /** Ruta del ejecutable del simulador, o null si no aparece. Sale de la MISMA
     *  raíz de bpgenvm-c que ya se configura para el AOT (una ruta, no dos). */
    public static Path locateExe(IdePrefs prefs) {
        String root = (prefs != null && prefs.aotBpgenvmDir != null && !prefs.aotBpgenvmDir.isEmpty())
                ? prefs.aotBpgenvmDir : AotBuild.autodetectBpgenvm(null);
        if (root == null) return null;
        Path base = Paths.get(root, "build");
        Path win  = base.resolve("bpvm-sim.exe");
        if (Files.isRegularFile(win)) return win;
        Path nix  = base.resolve("bpvm-sim");
        if (Files.isRegularFile(nix)) return nix;
        return null;
    }

    /** Mensaje de ayuda cuando falta el binario: decir QUÉ hacer, no sólo que falla. */
    public static String missingExeHelp(IdePrefs prefs) {
        String root = (prefs != null && prefs.aotBpgenvmDir != null && !prefs.aotBpgenvmDir.isEmpty())
                ? prefs.aotBpgenvmDir : AotBuild.autodetectBpgenvm(null);
        if (root == null) {
            return "no encuentro la raíz de bpgenvm-c; configúrala en "
                 + "Project → Micro simulado… (o en Ajustes AOT)";
        }
        return "falta " + Paths.get(root, "build", "bpvm-sim.exe")
             + " — compílalo con:  make sim   (o  make LVGL=1 sim  para tener ventana)";
    }

    /** Arranca el simulador con la configuración de `prefs` y espera a que acepte
     *  conexiones. Devuelve el endpoint "host:puerto", o lanza si no arranca. */
    public synchronized String start(IdePrefs prefs) throws IOException {
        if (isRunning()) return "127.0.0.1:" + prefs.simPort;

        Path exe = locateExe(prefs);
        if (exe == null) throw new IOException(missingExeHelp(prefs));

        Path dataDir = Paths.get(prefs.simDataDirEffective());
        Files.createDirectories(dataDir);

        List<String> cmd = new ArrayList<>();
        cmd.add(exe.toString());
        cmd.add("--port=" + prefs.simPort);
        cmd.add("--flash-file=" + dataDir.resolve("micro.flash"));
        cmd.add("--fs=" + dataDir.resolve("micro.fs"));
        cmd.add("--mem=" + (prefs.simRamKb * 1024L));
        cmd.add("--psram=" + (prefs.simPsramKb * 1024L));
        cmd.add("--flash=" + (prefs.simFlashKb * 1024L));
        if (prefs.simNoScreen) cmd.add("--no-screen");
        else cmd.add("--screen=" + prefs.simScreenW + "x" + prefs.simScreenH);

        say("[sim] " + String.join(" ", cmd) + "\n");
        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.directory(dataDir.toFile());
        pb.redirectErrorStream(true);
        proc = pb.start();

        /* Su stdout va a la consola del IDE: si el simulador se queja (FS que no
         * monta, puerto ocupado...), que se vea, en vez de morir en silencio. */
        final Process p = proc;
        pump = new Thread(() -> {
            try (BufferedReader r = new BufferedReader(
                    new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = r.readLine()) != null) say("[sim] " + line + "\n");
            } catch (IOException ignored) {
                /* el proceso murió mientras leíamos: no es noticia */
            }
        }, "bpvm-sim-output");
        pump.setDaemon(true);
        pump.start();

        /* Esperar a que ESCUCHE de verdad. Conectar "un poco después" a ciegas es
         * lo que produce el fallo intermitente de "connection refused" en máquinas
         * lentas; aquí se sondea el puerto hasta 5 s. */
        long deadline = System.currentTimeMillis() + 5000;
        while (System.currentTimeMillis() < deadline) {
            if (!p.isAlive()) {
                proc = null;
                throw new IOException("el simulador terminó nada más arrancar (exit "
                        + p.exitValue() + ") — mira la consola");
            }
            try (Socket s = new Socket()) {
                s.connect(new InetSocketAddress("127.0.0.1", prefs.simPort), 250);
                say("[sim] escuchando en 127.0.0.1:" + prefs.simPort + "\n");
                return "127.0.0.1:" + prefs.simPort;
            } catch (IOException notYet) {
                try { Thread.sleep(100); } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }
        stop();
        throw new IOException("el simulador no aceptó conexiones en 5 s (puerto "
                + prefs.simPort + " ocupado?)");
    }

    /** Para el simulador. Idempotente. */
    public synchronized void stop() {
        if (proc == null) return;
        Process p = proc;
        proc = null;
        p.destroy();
        try { p.waitFor(2, java.util.concurrent.TimeUnit.SECONDS); }
        catch (InterruptedException ie) { Thread.currentThread().interrupt(); }
        if (p.isAlive()) p.destroyForcibly();
        say("[sim] parado\n");
    }
}
