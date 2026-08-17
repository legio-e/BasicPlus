// ============================================================
// ProjectDialog.java — #436: editar el fichero de proyecto desde el IDE.
//
// Eduardo (17-ago): «necesitamos poder editar el fichero del proyecto (es algo
// parecido a editar el pom de Maven), para cambiar el tipo de salida, los
// ficheros incluidos, las familias nativas, etc.»
//
// Sustituye al viejo «Project Properties…», que sólo llevaba out:pack, el check
// de AOT y UN target en un campo de texto suelto. Lo que faltaba y ahora está:
// los `sources` del proyecto, las FAMILIAS (varias, no una) y los datos del pack.
//
// DOS DECISIONES QUE CONVIENE NO PERDER
//
//  1. NO se validan aquí las reglas del `.bpbuild`. El cargador ya las tiene, y
//     buenas: un `.bp` que no existe se dice al leer, `pack.version` no pasa de
//     16 bytes UTF-8 porque es lo que cabe en la cabecera, `pack.name` es un
//     nombre y no una ruta. Reescribirlas aquí sería una segunda copia que se
//     quedaría atrás. Lo que se hace es GUARDAR y VOLVER A CARGAR: si el
//     cargador protesta, se restaura el fichero tal y como estaba y se enseña
//     su mensaje — el de verdad, no una paráfrasis.
//
//  2. Las familias se editan como LISTA con casillas, no como texto. Antes
//     había un campo suelto que escribía `aotTarget` (singular) mientras el
//     proyecto podía tener `aotTargets` (la lista de V5/H8): guardar desde el
//     IDE podía dejar el fichero diciendo una familia y el build haciendo otra.
//     `BpBuild.save()` ya respeta la forma declarada; esto le da la lista.
// ============================================================
package com.mycompany.bpide;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Window;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import basicplus.frontend.BpBuild;

public final class ProjectDialog extends JDialog {

    /** Las familias que el proyecto puede declarar. Mismo juego que valida el
     *  cargador (`NpackReloc.DESTINOS`); si algún día crece, crece allí y aquí
     *  se ve el hueco — mejor que aceptar texto libre y fallar al leer. */
    private static final String[] FAMILIAS = { "arm", "riscv", "xtensa" };

    private final Path fichero;
    private final BpBuild proyecto;
    private final Consumer<String> log;
    private boolean guardado = false;

    private final JCheckBox cbPack = new JCheckBox("Empaquetar en un pack al construir (out: pack)");
    private final JCheckBox cbAot  = new JCheckBox("AOT: compilar las funciones 'native' a código máquina");
    private final List<JCheckBox> cbFamilias = new ArrayList<>();
    private final DefaultListModel<String> fuentesModel = new DefaultListModel<>();
    private final JList<String> fuentesList = new JList<>(fuentesModel);
    private final JTextField tfPackName     = new JTextField(16);
    private final JTextField tfPackVersion  = new JTextField(10);
    private final JTextField tfPackProvides = new JTextField(6);
    private final JTextField tfPackNotas    = new JTextField(24);

    public ProjectDialog(Window duenyo, Path fichero, BpBuild proyecto, Consumer<String> log) {
        super(duenyo, "Proyecto — " + fichero.getFileName(), ModalityType.APPLICATION_MODAL);
        this.fichero = fichero;
        this.proyecto = proyecto;
        this.log = (log != null) ? log : s -> { };
        buildUi();
        cargarDeProyecto();
        pack();
        setLocationRelativeTo(duenyo);
    }

    /** ¿se guardó algo? El llamador recarga si sí. */
    public boolean seGuardo() { return guardado; }

    private void buildUi() {
        JPanel centro = new JPanel();
        centro.setLayout(new BoxLayout(centro, BoxLayout.Y_AXIS));
        centro.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));

        centro.add(cabecera());
        centro.add(Box.createVerticalStrut(6));
        centro.add(salidaYaot());
        centro.add(Box.createVerticalStrut(6));
        centro.add(fuentes());
        centro.add(Box.createVerticalStrut(6));
        centro.add(datosPack());

        JPanel sur = new JPanel(new FlowLayout(FlowLayout.RIGHT, 6, 6));
        JButton ok = new JButton("Guardar");
        JButton cancelar = new JButton("Cancelar");
        ok.addActionListener(e -> onGuardar());
        cancelar.addActionListener(e -> dispose());
        sur.add(ok); sur.add(cancelar);
        getRootPane().setDefaultButton(ok);

        setLayout(new BorderLayout());
        add(new JScrollPane(centro), BorderLayout.CENTER);
        add(sur, BorderLayout.SOUTH);
    }

    private JPanel cabecera() {
        JPanel p = new JPanel(new GridBagLayout());
        p.setBorder(BorderFactory.createTitledBorder("Fichero"));
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(2, 4, 2, 4);
        c.anchor = GridBagConstraints.WEST;
        c.gridx = 0; c.gridy = 0;
        p.add(new JLabel(fichero.toAbsolutePath().toString()), c);
        c.gridy = 1;
        /* Los caminos NO se editan aquí a propósito: cambiar sourceDir/outDir o
         * el `main` desde un formulario rompe el proyecto de formas que no se
         * ven hasta el siguiente build. Se enseñan para saber dónde se está. */
        p.add(new JLabel("main: " + nvl(proyecto.main) + "   ·   sourceDir: "
                + nvl(proyecto.sourceDir) + "   ·   outDir: " + nvl(proyecto.outDir)), c);
        return p;
    }

    private JPanel salidaYaot() {
        JPanel p = new JPanel();
        p.setLayout(new BoxLayout(p, BoxLayout.Y_AXIS));
        p.setBorder(BorderFactory.createTitledBorder("Salida y código nativo"));
        p.add(cbPack);
        p.add(cbAot);
        JPanel fam = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        fam.add(new JLabel("Familias:"));
        for (String f : FAMILIAS) {
            JCheckBox cb = new JCheckBox(f);
            cbFamilias.add(cb);
            fam.add(cb);
        }
        p.add(fam);
        JLabel nota = new JLabel("Sin ninguna marcada se compila para la familia por defecto (arm).");
        nota.setEnabled(false);
        p.add(nota);
        cbAot.addActionListener(e -> actualizarFamilias());
        return p;
    }

    private void actualizarFamilias() {
        for (JCheckBox cb : cbFamilias) cb.setEnabled(cbAot.isSelected());
    }

    private JPanel fuentes() {
        JPanel p = new JPanel(new BorderLayout(4, 4));
        p.setBorder(BorderFactory.createTitledBorder(
                "Ficheros incluidos (sources) — vacío = sólo el módulo 'main' y su cierre"));
        fuentesList.setVisibleRowCount(6);
        p.add(new JScrollPane(fuentesList), BorderLayout.CENTER);
        JPanel btns = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        JButton anyadir = new JButton("Añadir .bp…");
        JButton quitar  = new JButton("Quitar");
        anyadir.addActionListener(e -> onAnyadirFuente());
        quitar.addActionListener(e -> {
            int i = fuentesList.getSelectedIndex();
            if (i >= 0) fuentesModel.remove(i);
        });
        btns.add(anyadir); btns.add(quitar);
        p.add(btns, BorderLayout.SOUTH);
        return p;
    }

    private void onAnyadirFuente() {
        javax.swing.JFileChooser fc = new javax.swing.JFileChooser(
                proyecto.projectDir != null ? new java.io.File(proyecto.projectDir) : null);
        fc.setDialogTitle("Añadir fuente al proyecto");
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Fuentes BP (*.bp)", "bp"));
        if (fc.showOpenDialog(this) != javax.swing.JFileChooser.APPROVE_OPTION) return;
        java.io.File f = fc.getSelectedFile();
        /* Se guarda RELATIVA al proyecto si se puede: un .bpbuild con rutas
         * absolutas deja de funcionar en otra máquina, y sin dar ningún error. */
        String ruta = f.getAbsolutePath();
        if (proyecto.projectDir != null) {
            try {
                Path base = java.nio.file.Paths.get(proyecto.projectDir).toAbsolutePath().normalize();
                Path rel = base.relativize(f.toPath().toAbsolutePath().normalize());
                String r = rel.toString().replace('\\', '/');
                if (!r.startsWith("..")) ruta = r.startsWith("./") ? r : "./" + r;
            } catch (IllegalArgumentException ex) {
                /* Otra unidad de disco: no hay relativa posible. Se queda la
                 * absoluta, que al menos funciona en ESTA máquina. */
            }
        }
        if (!fuentesModel.contains(ruta)) fuentesModel.addElement(ruta);
    }

    private JPanel datosPack() {
        JPanel p = new JPanel(new GridBagLayout());
        p.setBorder(BorderFactory.createTitledBorder("Pack (sólo si la salida es un pack)"));
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(2, 4, 2, 4);
        c.anchor = GridBagConstraints.WEST;
        int y = 0;
        c.gridx = 0; c.gridy = y;   p.add(new JLabel("Nombre:"), c);
        c.gridx = 1;                p.add(tfPackName, c);
        c.gridx = 2;                p.add(new JLabel("Versión (≤16 bytes):"), c);
        c.gridx = 3;                p.add(tfPackVersion, c);
        c.gridx = 0; c.gridy = ++y; p.add(new JLabel("Publica (fourcc):"), c);
        c.gridx = 1;                p.add(tfPackProvides, c);
        c.gridx = 0; c.gridy = ++y; p.add(new JLabel("Notas:"), c);
        c.gridx = 1; c.gridwidth = 3; p.add(tfPackNotas, c);
        return p;
    }

    private void cargarDeProyecto() {
        cbPack.setSelected("pack".equals(proyecto.out));
        cbAot.setSelected(proyecto.aotEnabled);
        List<String> declaradas = !proyecto.aotTargets.isEmpty()
                ? proyecto.aotTargets
                : java.util.Collections.singletonList(proyecto.aotTarget);
        for (JCheckBox cb : cbFamilias) cb.setSelected(declaradas.contains(cb.getText()));
        actualizarFamilias();

        fuentesModel.clear();
        for (String s : proyecto.sourcesDeclarados) fuentesModel.addElement(s);

        tfPackName.setText(nvl(proyecto.packName));
        tfPackVersion.setText(nvl(proyecto.packVersion));
        tfPackProvides.setText(nvl(proyecto.packProvides));
        tfPackNotas.setText(nvl(proyecto.packNotas));
    }

    private void onGuardar() {
        List<String> familias = new ArrayList<>();
        for (JCheckBox cb : cbFamilias) if (cb.isSelected()) familias.add(cb.getText());

        // Copia de seguridad ANTES de tocar: si el cargador rechaza lo nuevo, el
        // fichero vuelve a estar como estaba. Sin esto, un error de edición
        // dejaría el proyecto ilegible y sin vuelta atrás.
        byte[] antes;
        try {
            antes = Files.readAllBytes(fichero);
        } catch (IOException ex) {
            error("No se pudo leer el proyecto: " + ex.getMessage());
            return;
        }

        proyecto.out = cbPack.isSelected() ? "pack" : "normal";
        proyecto.aotEnabled = cbAot.isSelected();
        if (familias.size() > 1) {
            proyecto.aotTargets = familias;                 // se escribe `targets`
        } else {
            proyecto.aotTargets = new ArrayList<>();        // se escribe `target`
            if (!familias.isEmpty()) proyecto.aotTarget = familias.get(0);
        }
        proyecto.sourcesDeclarados = new ArrayList<>();
        for (int i = 0; i < fuentesModel.size(); i++) proyecto.sourcesDeclarados.add(fuentesModel.get(i));
        proyecto.packName     = vacioANull(tfPackName.getText());
        proyecto.packNameDeclarado = proyecto.packName != null;
        proyecto.packVersion  = vacioANull(tfPackVersion.getText());
        proyecto.packProvides = vacioANull(tfPackProvides.getText());
        proyecto.packNotas    = vacioANull(tfPackNotas.getText());

        try {
            proyecto.save(fichero);
        } catch (IOException ex) {
            error("No se pudo escribir el .bpbuild: " + ex.getMessage());
            return;
        }

        /* LA COMPROBACIÓN: que lo escrito lo acepte QUIEN LO VA A LEER. Las
         * reglas viven en el cargador y ahí se quedan; aquí sólo se le pregunta. */
        try {
            BpBuild.load(fichero);
        } catch (Exception ex) {
            try { Files.write(fichero, antes); } catch (IOException ignore) { }
            error("El proyecto no habría podido cargarse, así que NO se ha guardado:\n\n"
                  + ex.getMessage());
            return;
        }

        guardado = true;
        log.accept("[ide] proyecto guardado: " + fichero.getFileName()
                + "  out=" + proyecto.out
                + "  aot=" + (proyecto.aotEnabled
                              ? (familias.isEmpty() ? proyecto.aotTarget : familias.toString())
                              : "off")
                + "  sources=" + proyecto.sourcesDeclarados.size() + "\n");
        dispose();
    }

    private void error(String msg) {
        JOptionPane.showMessageDialog(this, msg, "Proyecto", JOptionPane.ERROR_MESSAGE);
    }

    private static String nvl(String s) { return (s == null) ? "" : s; }

    private static String vacioANull(String s) {
        String t = (s == null) ? "" : s.trim();
        return t.isEmpty() ? null : t;
    }
}
