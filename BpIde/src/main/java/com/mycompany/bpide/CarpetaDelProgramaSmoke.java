// ============================================================
// CarpetaDelProgramaSmoke.java — V8 #808 (13-nov): la VM local corre el programa en la flash del PC, en la carpeta
// que le daría la placa, y la carpeta se queda al acabar (CarpetaDelPrograma).
//
//   1) la carpeta, con el nombre de la placa: "/app/Termostato" es <flash>/app/Termostato, y "/app", <flash>/app;
//   2) lo que sobra: de lo que sube el IDE (.mod, .bpi y .dbg), lo que este Run no ha subido; lo que escribió el
//      programa y lo de sus subcarpetas, nunca (Eduardo: «La b»); los nombres, sin mirar mayúsculas;
//   3) con la VM-Java de verdad, arrancada como la arranca el IDE (setFlashDir, y startDaemon con la carpeta): un
//      programa que guarda "cuenta.txt" (ruta relativa) y "/prueba808.txt" (absoluta), dos Run seguidos, cada uno
//      con su VM. Lo relativo va a la carpeta del programa y lo absoluto a la flash; el segundo Run ve lo que dejó el
//      primero («vez 2»); y entre los dos, un .mod que sobra se borra y la cuenta se queda.
//
// Desde la raíz del repo (la stdlib sale de su BpVM.cfg, como en el IDE):
//   java -cp BpIde/target/BpIde-7.0.jar com.mycompany.bpide.CarpetaDelProgramaSmoke
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public final class CarpetaDelProgramaSmoke {

    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    private static final String PROGRAMA =
            "module Guarda808\n"
          + "  import Core\n"
          + "\n"
          + "  function Main()\n"
          + "    var n: integer := 0\n"
          + "    if fileExists(\"cuenta.txt\") then\n"
          + "      n := parseInt(readFile(\"cuenta.txt\"))\n"
          + "    endif\n"
          + "    n := n + 1\n"
          + "    writeFile(\"cuenta.txt\", intToString(n))\n"
          + "    writeFile(\"/prueba808.txt\", \"vez \" + intToString(n))\n"
          + "    print \"vez \" + intToString(n)\n"
          + "  end Main\n"
          + "end Guarda808\n";

    private static void escribe(Path p, String s) throws Exception {
        Files.createDirectories(p.getParent());
        Files.write(p, s.getBytes(StandardCharsets.UTF_8));
    }

    private static String lee(Path p) throws Exception {
        return Files.isRegularFile(p) ? new String(Files.readAllBytes(p), StandardCharsets.UTF_8) : null;
    }

    /** Un Run como el del IDE con la VM local (FrmMain.runOnVmRemote): la VM con la carpeta y la flash, se sube el
     *  programa, lo que sobra se borra, y a correr. Devuelve lo que imprimió. */
    private static String corre(Path flash, Path carpeta, Path mod, String stdlib) throws Exception {
        StringBuilder salida = new StringBuilder();
        CountDownLatch fin = new CountDownLatch(1);
        try (BpvmClient vm = new BpvmClient()) {
            vm.setOutputSink(salida::append);
            vm.setEventListener(ev -> {
                if (ev instanceof PausedEvent) vm.sendCommand(StepCommand.CONTINUE);
                else if (ev instanceof ExitedEvent) {
                    ExitedEvent e = (ExitedEvent) ev;
                    if (e.exitCode != 0) salida.append("[exit ").append(e.exitCode).append(": ").append(e.reason).append("]");
                    fin.countDown();
                }
            });
            vm.setFlashDir(flash.toString());
            vm.startDaemon(carpeta.toString(), /*waitClient=*/true, stdlib);
            Set<String> subidos = new HashSet<>();
            vm.uploadFile(mod, mod.getFileName().toString(), 10_000);
            subidos.add(mod.getFileName().toString());
            StringBuilder dicho = new StringBuilder();
            CarpetaDelPrograma.borraLosQueSobran(carpeta, subidos, dicho::append);
            if (dicho.length() > 0) System.out.print("    " + dicho.toString().replace("\n", "\n    ").trim() + "\n");
            vm.runModuleConRespuesta(mod.getFileName().toString(), null, 10_000L);
            if (!fin.await(20, TimeUnit.SECONDS)) salida.append("[sin EXITED en 20 s]");
        }
        return salida.toString();
    }

    public static void main(String[] args) throws Exception {
        System.out.println("--- #808: la carpeta del programa, en la flash del PC ---");

        // 1) la carpeta
        Path f = Paths.get(System.getProperty("java.io.tmpdir"), "flash");
        check(CarpetaDelPrograma.de(f, "/app/Termostato").equals(f.resolve("app").resolve("Termostato")),
              "\"/app/Termostato\" es <flash>/app/Termostato");
        check(CarpetaDelPrograma.de(f, "/app").equals(f.resolve("app")), "\"/app\" (sin proyecto) es <flash>/app");
        check(CarpetaDelPrograma.de(f, null).equals(f.resolve("app")), "sin prefijo, también <flash>/app");
        check(CarpetaDelPrograma.flashDelPc().equals(
                      Paths.get(System.getProperty("user.home"), ".basicplus", "flash")),
              "la flash del PC, la de las dos VMs por defecto (~/.basicplus/flash)");

        Path tmp = Files.createTempDirectory("bpide808-");
        try {
            // 2) lo que sobra
            Path c = tmp.resolve("sobra");
            for (String n : Arrays.asList("A.mod", "A.dbg", "B.mod", "b.BPI", "termostato.mod", "datos.txt",
                                          "lecturas.csv", "sub/C.mod")) {
                escribe(c.resolve(n), "x");
            }
            Set<String> subidos = new HashSet<>(Arrays.asList("A.mod", "A.dbg", "Termostato.mod"));
            Set<String> h = new HashSet<>();
            for (Path p : CarpetaDelPrograma.huerfanos(c, subidos)) h.add(p.getFileName().toString());
            check(h.equals(new HashSet<>(Arrays.asList("B.mod", "b.BPI"))),
                  "sobran B.mod y b.BPI: lo que sube el IDE y este Run no ha subido (sale " + h + ")");
            StringBuilder dicho = new StringBuilder();
            int n = CarpetaDelPrograma.borraLosQueSobran(c, subidos, dicho::append);
            check(n == 2 && !Files.exists(c.resolve("B.mod")) && !Files.exists(c.resolve("b.BPI"))
                  && dicho.toString().contains("sobraba B.mod: borrado"),
                  "se borran, y se dice");
            check(Files.exists(c.resolve("datos.txt")) && Files.exists(c.resolve("lecturas.csv")),
                  "lo que escribió el programa se queda (Eduardo: «La b»)");
            check(Files.exists(c.resolve("sub").resolve("C.mod")), "y lo de sus subcarpetas, también");
            check(Files.exists(c.resolve("termostato.mod")),
                  "termostato.mod no sobra si se ha subido Termostato.mod (en Windows, el mismo fichero)");
            check(Files.exists(c.resolve("A.mod")) && Files.exists(c.resolve("A.dbg")), "y lo subido, claro");

            // 3) con la VM-Java de verdad
            edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(null);
            String stdlib = cfg.stdlibDir;
            check(stdlib != null && Files.isDirectory(Paths.get(stdlib)), "la stdlib del BpVM.cfg: " + stdlib);
            Path src = tmp.resolve("src");
            escribe(src.resolve("BpVM.cfg"), "{ \"stdlibDir\": \"" + stdlib.replace('\\', '/') + "\" }\n");
            escribe(src.resolve("Guarda808.bp"), PROGRAMA);
            Path out = src.resolve("out");
            Files.createDirectories(out);
            boolean ok = basicplus.frontend.Main.compileFile(src.resolve("Guarda808.bp"), out, "mivm", true);
            Path mod = out.resolve("Guarda808.mod");
            check(ok && Files.isRegularFile(mod), "compila Guarda808");

            Path flash = tmp.resolve("flash");
            Path carpeta = CarpetaDelPrograma.de(flash, "/app/Prueba808");
            Files.createDirectories(carpeta);
            String s1 = corre(flash, carpeta, mod, stdlib);
            check(s1.contains("vez 1"), "el primer Run dice «vez 1» (sale " + s1.trim() + ")");
            check("1".equals(lee(carpeta.resolve("cuenta.txt"))),
                  "lo relativo, en la carpeta del programa: <flash>/app/Prueba808/cuenta.txt");
            check("vez 1".equals(lee(flash.resolve("prueba808.txt"))),
                  "lo absoluto, en la flash: <flash>/prueba808.txt");

            escribe(carpeta.resolve("Viejo808.mod"), "x");   // lo que sobra de un Run anterior
            String s2 = corre(flash, carpeta, mod, stdlib);
            check(s2.contains("vez 2"), "el segundo Run ve lo que guardó el primero: «vez 2» (sale " + s2.trim() + ")");
            check(!Files.exists(carpeta.resolve("Viejo808.mod")), "entre los dos, el .mod que sobraba, borrado");
            check("2".equals(lee(carpeta.resolve("cuenta.txt"))), "y la cuenta, que es del programa, se queda");
        } finally {
            try (java.util.stream.Stream<Path> w = Files.walk(tmp)) {
                w.sorted(java.util.Comparator.reverseOrder())
                 .forEach(p -> { try { Files.deleteIfExists(p); } catch (Exception ignored) { } });
            } catch (Exception ignored) { }
        }

        System.out.println("[status=" + (fallos == 0 ? "OK" : "FAIL: " + fallos) + "]");
        if (fallos != 0) System.exit(1);
    }
}
