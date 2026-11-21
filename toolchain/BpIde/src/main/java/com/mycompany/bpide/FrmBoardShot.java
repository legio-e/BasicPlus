// ============================================================
// FrmBoardShot.java — #435: una FOTO de la ventana de placa, sin placa.
//
// El código Swing compila igual aunque la ventana salga mal: al reordenar
// #435 el botón del entorno acabó en un `add(..., NORTH)` sobre un
// GridLayout(1,1), que compila y rompe el layout. Eso se ve mirando, no
// compilando — así que esto la construye, la pinta a un PNG y se acabó la
// discusión.
//
// No necesita placa: los paneles arrancan "(sin conexión)", que es justo el
// estado en el que hay que comprobar que la ventana se ve bien.
//
//   mvn -f BpIde/pom.xml exec:java \
//       -Dexec.mainClass=com.mycompany.bpide.FrmBoardShot -Dexec.args="<salida.png>"
// ============================================================
package com.mycompany.bpide;

import java.awt.image.BufferedImage;
import java.io.File;

import javax.imageio.ImageIO;
import javax.swing.SwingUtilities;

public final class FrmBoardShot {

    /** Los hijos no tienen tamaño hasta que su padre los coloca; sin esto sale
     *  todo a 0x0 y el PNG queda en blanco. */
    private static void validarTodo(java.awt.Container c) {
        c.doLayout();
        for (java.awt.Component h : c.getComponents()) {
            if (h instanceof java.awt.Container) validarTodo((java.awt.Container) h);
        }
    }

    public static void main(String[] args) throws Exception {
        final String destino = (args.length > 0) ? args[0] : "frmboard.png";
        final File out = new File(destino);

        SwingUtilities.invokeAndWait(() -> {
            try {
                /* #436 — la misma foto sirve para el diálogo del proyecto:
                 * `-Dexec.args="salida.png proyecto <ruta.bpbuild>"`. */
                java.awt.Window f;
                if (args.length >= 3 && "proyecto".equals(args[1])) {
                    java.nio.file.Path pf = java.nio.file.Paths.get(args[2]);
                    f = new ProjectDialog(null, pf,
                            basicplus.frontend.BpBuild.load(pf), s -> { });
                } else {
                    f = new FrmBoard();
                }
                f.setSize(f instanceof FrmBoard ? 1100 : 760, f instanceof FrmBoard ? 620 : 640);
                // pack() no: queremos el tamaño real de uso, que es donde se ve
                // si algo se come el sitio de otro.
                f.addNotify();                 // crea los peers -> los layouts corren
                f.validate();
                /* printAll y NO paintAll: sobre una ventana que no se ha
                 * mostrado, paintAll deja el lienzo NEGRO (los componentes no
                 * se consideran pintables). printAll es el camino de impresión
                 * y sí dibuja sin display. Se pinta el contentPane —lo que hay
                 * dentro del marco—, que es lo que interesa mirar. */
                java.awt.Container cp = (f instanceof javax.swing.RootPaneContainer)
                        ? ((javax.swing.RootPaneContainer) f).getContentPane()
                        : f;
                cp.setSize(f.getWidth(), f.getHeight());
                cp.doLayout();
                validarTodo(cp);
                BufferedImage img = new BufferedImage(cp.getWidth(), cp.getHeight(),
                                                      BufferedImage.TYPE_INT_RGB);
                java.awt.Graphics2D g = img.createGraphics();
                g.setColor(java.awt.Color.WHITE);
                g.fillRect(0, 0, img.getWidth(), img.getHeight());
                cp.printAll(g);
                g.dispose();
                ImageIO.write(img, "png", out);
                System.out.println("foto: " + out.getAbsolutePath()
                                   + " (" + img.getWidth() + "x" + img.getHeight() + ")");
            } catch (Exception e) {
                e.printStackTrace();
                System.exit(1);
            }
        });
        System.exit(0);
    }
}
