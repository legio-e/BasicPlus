// ============================================================
// TrazaVmLocalSmoke.java — V8 #788 (13-nov): con la VM local (Run en el PC), la consola dice DÓNDE falló el programa,
// con sus líneas, como con la placa: «en IdeFallaLib.bp:9», «llamado desde IdeFalla.bp:7». Antes sólo salía la línea
// en bruto de la VM («[vm] === traza: IdeFallaLib+43 IdeFalla+56 … ===»): la traducción estaba sólo en el camino de la
// placa (AbstractBpvmBackend.run).
//
// Por el camino real, sin darle las carpetas a TrazaError: se compila como el IDE (Main.compileFile y
// TrazaError.conoce), se corre en la VM-Java como la arranca el IDE (BpvmClient en modo daemon), y la traza la dice
// TrazaDeLaVmLocal, lo mismo que usa FrmMain.runOnVmRemote. La VM-Java manda DOS EXITED con un error (la excepción
// plegada y el fin): la traza tiene que salir una vez.
//
// Desde la raíz del repo (la stdlib sale de su BpVM.cfg, como en el IDE):
//   java -cp BpIde/target/BpIde-7.0.jar com.mycompany.bpide.TrazaVmLocalSmoke
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public final class TrazaVmLocalSmoke {

    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    // La división por cero, en la línea 7 de la librería; la llamada, en la línea 7 del programa, dentro de `paso`,
    // que llama Main en la línea 11.
    private static final String LIB =
            "module IdeFallaLib\n"                                   // 1
          + "  import Core\n"                                        // 2
          + "\n"                                                     // 3
          + "  public function divide(a: integer, b: integer): integer\n"   // 4
          + "    var r: integer := 0\n"                              // 5
          + "    print \"dividiendo\"\n"                             // 6
          + "    r := a / b\n"                                       // 7
          + "    return r\n"                                         // 8
          + "  end divide\n"                                         // 9
          + "end IdeFallaLib\n";
    private static final String PROG =
            "module IdeFalla\n"                                      // 1
          + "  import Core\n"                                        // 2
          + "  import IdeFallaLib\n"                                 // 3
          + "\n"                                                     // 4
          + "  function paso(n: integer): integer\n"                 // 5
          + "    var x: integer := 0\n"                              // 6
          + "    x := IdeFallaLib.divide(10, n)\n"                   // 7
          + "    return x\n"                                         // 8
          + "  end paso\n"                                           // 9
          + "  function Main()\n"                                    // 10
          + "    print paso(0)\n"                                    // 11
          + "  end Main\n"                                           // 12
          + "end IdeFalla\n";

    private static void escribe(Path p, String s) throws Exception {
        Files.createDirectories(p.getParent());
        Files.write(p, s.getBytes(StandardCharsets.UTF_8));
    }

    public static void main(String[] args) throws Exception {
        System.out.println("--- #788: la traza de la VM local, con sus líneas ---");
        Path tmp = Files.createTempDirectory("bpide788-");
        try {
            edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(null);
            String stdlib = cfg.stdlibDir;
            check(stdlib != null && Files.isDirectory(Paths.get(stdlib)), "la stdlib del BpVM.cfg: " + stdlib);
            Path src = tmp.resolve("src");
            escribe(src.resolve("BpVM.cfg"), "{ \"stdlibDir\": \"" + stdlib.replace('\\', '/') + "\" }\n");
            escribe(src.resolve("IdeFallaLib.bp"), LIB);
            escribe(src.resolve("IdeFalla.bp"), PROG);
            Path out = src.resolve("out");
            Files.createDirectories(out);
            boolean ok = basicplus.frontend.Main.compileFile(src.resolve("IdeFalla.bp"), out, "mivm", true);
            check(ok && Files.isRegularFile(out.resolve("IdeFalla.mod")) && Files.isRegularFile(out.resolve("IdeFallaLib.mod")),
                  "compilan IdeFalla e IdeFallaLib");
            TrazaError.conoce(out);   // lo que hace el IDE al compilar (FrmMain): sus .dbg, ahí

            // Como FrmMain.runOnVmRemote con Run: la VM-Java en modo daemon, en una carpeta, con lo compilado subido.
            Path flash = tmp.resolve("flash");
            Path carpeta = CarpetaDelPrograma.de(flash, "/app/IdeFalla");
            Files.createDirectories(carpeta);
            List<String> consola = Collections.synchronizedList(new ArrayList<>());
            List<String> trazas = Collections.synchronizedList(new ArrayList<>());
            CountDownLatch fin = new CountDownLatch(1);
            TrazaDeLaVmLocal traza = new TrazaDeLaVmLocal(l -> consola.add(l));
            try (BpvmClient vm = new BpvmClient()) {
                vm.setOutputSink(s -> consola.add("OUT " + s));
                vm.setEventListener(ev -> {
                    if (ev instanceof PausedEvent) vm.sendCommand(StepCommand.CONTINUE);
                    else if (ev instanceof ExitedEvent) {
                        trazas.add(((ExitedEvent) ev).stackTrace);
                        traza.alAcabar((ExitedEvent) ev);
                        fin.countDown();
                    }
                });
                vm.setFlashDir(flash.toString());
                vm.startDaemon(carpeta.toString(), /*waitClient=*/true, stdlib);
                for (String n : new String[] { "IdeFalla.mod", "IdeFalla.dbg", "IdeFallaLib.mod", "IdeFallaLib.dbg" })
                    vm.uploadFile(out.resolve(n), n, 10_000);
                vm.runModuleConRespuesta("IdeFalla.mod", null, 10_000L);
                check(fin.await(20, TimeUnit.SECONDS), "el programa acaba");
                Thread.sleep(500);   // por si llega un segundo EXITED
            }
            System.out.println("    EXITED recibidos: " + trazas.size() + " " + trazas);
            System.out.println("    la consola: " + consola);
            List<String> donde = new ArrayList<>();
            for (String l : consola) if (l.startsWith("  en ") || l.startsWith("  llamado desde ")) donde.add(l);
            check(donde.equals(java.util.Arrays.asList("  en IdeFallaLib.bp:7", "  llamado desde IdeFalla.bp:7",
                                                       "  llamado desde IdeFalla.bp:11")),
                  "dónde, con sus líneas y una vez: " + donde);
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
