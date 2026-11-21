// ============================================================
// EnvDialogShot.java — #435: ¿el diálogo del entorno ABRE y trae datos?
//
// Eduardo: «no sé cómo acceder al environment». El botón está y el código
// compila, así que la pregunta no se contesta leyendo: hay que abrirlo. Esto lo
// abre CONTRA EL MICRO SIMULADO (bpvm-sim, que habla el mismo wire v1 que la
// placa), le pide sus variables y lo pinta a un PNG.
//
// Si sale con la tabla llena, el diálogo funciona y lo que falla es llegar a él.
// Si sale vacío o revienta, el fallo es del diálogo y aquí está.
//
//   mvn -f BpIde/pom.xml exec:java \
//       -Dexec.mainClass=com.mycompany.bpide.EnvDialogShot -Dexec.args="<salida.png>"
// ============================================================
package com.mycompany.bpide;

import java.awt.image.BufferedImage;
import java.io.File;

import javax.imageio.ImageIO;

public final class EnvDialogShot {

    private static final int PORT = 5133;

    public static void main(String[] args) throws Exception {
        File out = new File(args.length > 0 ? args[0] : "envdlg.png");

        File sim = new File("bpgenvm-c/build/bpvm-sim.exe");
        if (!sim.isFile()) sim = new File("bpgenvm-c/build/bpvm-sim");
        if (!sim.isFile()) { System.out.println("(skip: no está bpvm-sim)"); return; }

        File tmp = File.createTempFile("envshot", "");
        tmp.delete(); tmp.mkdirs();
        Process p = new ProcessBuilder(sim.getAbsolutePath(), "--port=" + PORT,
                "--flash-file=" + new File(tmp, "sim.flash").getAbsolutePath())
                .redirectErrorStream(true)
                .redirectOutput(new File(tmp, "sim.log"))
                .start();
        try {
            BpvmClient cli = new BpvmClient();
            boolean conectado = false;
            for (int i = 0; i < 60 && !conectado; i++) {
                try { cli.connectRemote("127.0.0.1", PORT); conectado = true; }
                catch (Exception e) { Thread.sleep(150); }
            }
            if (!conectado) { System.out.println("FAIL: el sim no aceptó conexión"); System.exit(1); }
            System.out.println("conectado al simulador");

            // Un par de variables, para que la tabla tenga algo que enseñar.
            cli.envSet("psram", "1", 8000);
            cli.envSet("log", "1", 8000);

            final EnvDialog[] dlg = new EnvDialog[1];
            javax.swing.SwingUtilities.invokeAndWait(() ->
                    dlg[0] = new EnvDialog(null, cli, s -> System.out.println("  log> " + s), () -> { }));
            Thread.sleep(1500);   // el refresh va en hilo de fondo

            javax.swing.SwingUtilities.invokeAndWait(() -> {
                try {
                    java.awt.Container cp = dlg[0].getContentPane();
                    cp.setSize(dlg[0].getWidth(), dlg[0].getHeight());
                    validarTodo(cp);
                    BufferedImage img = new BufferedImage(Math.max(cp.getWidth(), 100),
                            Math.max(cp.getHeight(), 100), BufferedImage.TYPE_INT_RGB);
                    java.awt.Graphics2D g = img.createGraphics();
                    g.setColor(java.awt.Color.WHITE);
                    g.fillRect(0, 0, img.getWidth(), img.getHeight());
                    cp.printAll(g);
                    g.dispose();
                    ImageIO.write(img, "png", out);
                    System.out.println("foto: " + out.getAbsolutePath()
                            + " (" + img.getWidth() + "x" + img.getHeight() + ")");
                } catch (Exception e) { e.printStackTrace(); }
            });
            cli.close();
        } finally {
            p.destroy();
        }
        System.exit(0);
    }

    private static void validarTodo(java.awt.Container c) {
        c.doLayout();
        for (java.awt.Component h : c.getComponents())
            if (h instanceof java.awt.Container) validarTodo((java.awt.Container) h);
    }
}
