// ============================================================
// WorkdirSmoke.java
// Valida A2.1 + A2.2 + A2.3 + A2.4: workflow IDE↔VM con workdir y
// transferencia de ficheros por el wire.
//
//   1) Crea un workdir temporal vacío.
//   2) Spawn la VM en modo daemon con --workdir.
//   3) BpvmClient conecta.
//   4) Upload del .mod local al workdir vía wire.
//   5) listFiles para confirmar lo subido.
//   6) runModule + recibir prints + ExitedEvent.
//   7) Cleanup.
//
// Uso:
//   mvn exec:java -Dexec.mainClass=com.mycompany.bpide.WorkdirSmoke \
//                 -Dexec.args="<ruta/local/Modulo.mod>"
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.DebugEvent;
import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public final class WorkdirSmoke {
    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("uso: WorkdirSmoke <local.mod>");
            System.exit(2);
        }
        Path localMod = java.nio.file.Paths.get(args[0]);
        if (!Files.isRegularFile(localMod)) {
            System.err.println("no existe: " + localMod);
            System.exit(2);
        }
        String remoteName = localMod.getFileName().toString();

        // 1) Workdir temporal vacío.
        Path workdir = Files.createTempDirectory("bpvm-smoke-");
        System.out.println("WORKDIR " + workdir);

        CountDownLatch exited = new CountDownLatch(1);

        try (BpvmClient vm = new BpvmClient()) {
            vm.setDiagSink(s -> System.err.println("DIAG  " + s));
            vm.setOutputSink(s -> System.out.print("OUT   " + s));
            vm.setEventListener((DebugEvent ev) -> {
                if (ev instanceof PausedEvent) {
                    // STEP_INTO inicial: auto-continue.
                    vm.sendCommand(StepCommand.CONTINUE);
                } else if (ev instanceof ExitedEvent) {
                    ExitedEvent e = (ExitedEvent) ev;
                    System.out.println("EXIT  code=" + e.exitCode + " " + e.reason);
                    exited.countDown();
                }
            });

            // 2) Spawn la VM en modo daemon.
            vm.startDaemon(workdir.toString(), /*waitClient=*/true);

            // 4) Upload del .mod.
            int size = vm.uploadFile(localMod, remoteName, 5000);
            System.out.println("UPLOAD " + remoteName + " (" + size + " bytes)");

            // 5) Confirmar via listFiles.
            List<BpvmClient.RemoteFile> files = vm.listFiles("", 5000);
            System.out.println("LISTED " + files.size() + " files:");
            for (BpvmClient.RemoteFile f : files) System.out.println("  " + f);

            // 6) runModule.
            vm.runModule(remoteName);

            // Esperar al ExitedEvent o timeout.
            if (!exited.await(15, TimeUnit.SECONDS)) {
                System.err.println("TIMEOUT esperando exited");
            }
        } finally {
            // 7) Cleanup workdir.
            try {
                Files.walk(workdir)
                        .sorted(java.util.Comparator.reverseOrder())
                        .forEach(p -> { try { Files.deleteIfExists(p); } catch (Exception ignored) {} });
            } catch (Exception ignored) {}
        }
        System.out.println("DONE");
    }
}
