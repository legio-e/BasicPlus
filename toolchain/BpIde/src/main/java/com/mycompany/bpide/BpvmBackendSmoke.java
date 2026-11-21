// ============================================================
// BpvmBackendSmoke.java
// Valida BpvmBackend end-to-end: arranca un daemon VM-Java como
// subproceso, conecta via TCP wire v1, lista ficheros, sube un .mod,
// lo ejecuta y captura el output.
//
// Uso:
//   mvn -pl BpIde exec:java -Dexec.mainClass=com.mycompany.bpide.BpvmBackendSmoke \
//                           -Dexec.args="<ruta/local/Modulo.mod>"
// ============================================================
package com.mycompany.bpide;

import java.io.File;
import java.io.IOException;
import java.net.ServerSocket;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public final class BpvmBackendSmoke {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("uso: BpvmBackendSmoke <local.mod>");
            System.exit(2);
        }
        Path localMod = java.nio.file.Paths.get(args[0]);
        if (!Files.isRegularFile(localMod)) {
            System.err.println("no existe: " + localMod);
            System.exit(2);
        }
        String remoteName = localMod.getFileName().toString();

        // Workdir temporal + puerto libre para el daemon.
        Path workdir = Files.createTempDirectory("bpvm-backend-smoke-");
        System.out.println("WORKDIR " + workdir);
        int port;
        try (ServerSocket s = new ServerSocket(0)) { port = s.getLocalPort(); }
        System.out.println("PORT    " + port);

        // Spawnear daemon: java -cp <cp> edu.bpgenvm.Main --listen P
        //                  --workdir W --wait-client
        Process daemon = spawnDaemon(port, workdir);
        System.out.println("DAEMON  arrancado (esperando cliente)");

        try {
            // Conectar con BpvmBackend.
            Backend b = new BpvmBackend();
            String hello = b.connect("localhost:" + port);
            System.out.println("HELLO   " + hello);

            // Subir el .mod.
            byte[] data = Files.readAllBytes(localMod);
            b.put(remoteName, data);
            System.out.println("PUT     " + remoteName + " (" + data.length + " bytes)");

            // Listar.
            List<Backend.Entry> files = b.list();
            System.out.println("LIST    " + files.size() + " entradas:");
            for (Backend.Entry f : files) System.out.println("  " + f);

            // Ejecutar.
            System.out.println("RUN     " + remoteName);
            String status = b.run(remoteName, line -> System.out.print("OUT     " + line));
            System.out.println("DONE    " + status);

            b.close();
        } finally {
            // Cleanup.
            if (daemon.isAlive()) {
                daemon.destroy();
                daemon.waitFor();
            }
            try {
                Files.walk(workdir)
                        .sorted(java.util.Comparator.reverseOrder())
                        .forEach(p -> { try { Files.deleteIfExists(p); } catch (Exception ignored) {} });
            } catch (Exception ignored) {}
        }
        System.out.println("CLEANUP ok");
    }

    private static Process spawnDaemon(int port, Path workdir) throws IOException {
        String javaBin = System.getProperty("java.home") + File.separator + "bin"
                + File.separator + "java";
        if (System.getProperty("os.name", "").toLowerCase().contains("win")) {
            javaBin += ".exe";
        }
        // Misma estrategia que BpvmClient.spawnVmProcess: localizar el
        // origen físico de edu.bpgenvm.Main y añadirlo al classpath.
        String mainCp;
        try {
            java.security.CodeSource cs = edu.bpgenvm.Main.class
                    .getProtectionDomain().getCodeSource();
            mainCp = new File(cs.getLocation().toURI()).getAbsolutePath();
        } catch (Exception e) {
            throw new IOException("no localizo el jar de edu.bpgenvm.Main: " + e.getMessage(), e);
        }
        String runtimeCp = System.getProperty("java.class.path", "");
        String cp = runtimeCp.isEmpty() ? mainCp
                : (runtimeCp.contains(mainCp) ? runtimeCp
                                              : runtimeCp + File.pathSeparator + mainCp);
        ProcessBuilder pb = new ProcessBuilder(
                javaBin, "-cp", cp,
                "edu.bpgenvm.Main",
                "--listen", Integer.toString(port),
                "--workdir", workdir.toString(),
                "--wait-client");
        pb.inheritIO();
        return pb.start();
    }
}
