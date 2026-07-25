// ============================================================
// SimConfigDialog.java
// H10 — la ventana del engranaje: cómo es el micro simulado.
// Aquí se decide el "silicio" que verá el programa BP — RAM, PSRAM,
// flash y pantalla — más dónde vive el binario del simulador y las
// carpetas (datos de la placa simulada y librería de packs).
//
// Los cambios se guardan en IdePrefs y se aplican al SIGUIENTE arranque
// del simulador: como en una placa, el tamaño de la RAM no se cambia en
// caliente. Si el simulador está en marcha, el diálogo lo avisa.
// ============================================================
package com.mycompany.bpide;

import javax.swing.*;
import java.awt.*;
import java.io.File;

public final class SimConfigDialog {

    private SimConfigDialog() { }

    /** Presets de las placas reales del proyecto: lo más cómodo es decir "quiero
     *  que se comporte como la DK2" y que rellene los cuatro números. */
    private static final Object[][] PRESETS = {
        /* nombre                       RAM KB  PSRAM KB  flash KB   W     H  */
        { "Pico 2 / RP2350A",             512,        0,     4096,  480,  320 },
        { "Metro RP2350B (PSRAM 8M)",     512,     8192,    16384,  480,  320 },
        { "STM32 DK2 (pantalla 480x272)", 512,        0,     2048,  480,  272 },
        { "ESP32-S3 (sin pantalla)",      512,     8192,     8192,    0,    0 },
        { "ESP32-P4 EK79007 1024x600",   1024,    32768,    16384, 1024,  600 },
        { "ESP32-P4 Waveshare 480x800",  1024,    32768,    16384,  480,  800 },
    };

    /** Abre el diálogo. Devuelve true si el usuario aceptó (prefs ya guardadas). */
    public static boolean show(Component parent, IdePrefs prefs, boolean simRunning) {
        JSpinner spPort   = intSpinner(prefs.simPort,     1024, 65535, 1);
        JSpinner spRam    = intSpinner(prefs.simRamKb,      64, 65536, 64);
        JSpinner spPsram  = intSpinner(prefs.simPsramKb,     0, 65536, 512);
        JSpinner spFlash  = intSpinner(prefs.simFlashKb,   256, 65536, 256);
        JSpinner spW      = intSpinner(prefs.simScreenW,    64,  4096, 16);
        JSpinner spH      = intSpinner(prefs.simScreenH,    64,  4096, 16);
        JCheckBox cbNoScr = new JCheckBox("sin pantalla (placa sin panel)", prefs.simNoScreen);
        cbNoScr.addActionListener(e -> {
            spW.setEnabled(!cbNoScr.isSelected());
            spH.setEnabled(!cbNoScr.isSelected());
        });
        spW.setEnabled(!prefs.simNoScreen);
        spH.setEnabled(!prefs.simNoScreen);

        JTextField tfData  = new JTextField(prefs.simDataDirEffective(), 26);
        JTextField tfPacks = new JTextField(prefs.packsDir != null ? prefs.packsDir : "", 26);
        JTextField tfVmc   = new JTextField(
                prefs.aotBpgenvmDir != null ? prefs.aotBpgenvmDir : "", 26);

        JComboBox<String> cbPreset = new JComboBox<>();
        cbPreset.addItem("(elige una placa…)");
        for (Object[] p : PRESETS) cbPreset.addItem((String) p[0]);
        cbPreset.addActionListener(e -> {
            int i = cbPreset.getSelectedIndex() - 1;
            if (i < 0) return;
            Object[] p = PRESETS[i];
            spRam.setValue(p[1]); spPsram.setValue(p[2]); spFlash.setValue(p[3]);
            int w = (Integer) p[4], h = (Integer) p[5];
            boolean sinPantalla = (w == 0 || h == 0);
            cbNoScr.setSelected(sinPantalla);
            spW.setEnabled(!sinPantalla); spH.setEnabled(!sinPantalla);
            if (!sinPantalla) { spW.setValue(w); spH.setValue(h); }
        });

        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(3, 4, 3, 4);
        c.anchor = GridBagConstraints.WEST;
        int row = 0;

        if (simRunning) {
            c.gridx = 0; c.gridy = row++; c.gridwidth = 3;
            panel.add(new JLabel("<html><b>El simulador está en marcha.</b> "
                    + "Los cambios se aplican cuando lo pares y lo vuelvas a arrancar.</html>"), c);
            c.gridwidth = 1;
        }

        c.gridx = 0; c.gridy = row; panel.add(new JLabel("Parecerse a:"), c);
        c.gridx = 1; c.gridy = row++; c.gridwidth = 2;
        c.fill = GridBagConstraints.HORIZONTAL;
        panel.add(cbPreset, c);
        c.fill = GridBagConstraints.NONE; c.gridwidth = 1;

        row = addSep(panel, c, row, "Memoria");
        row = addSpin(panel, c, row, "RAM (KB):",   spRam);
        row = addSpin(panel, c, row, "PSRAM (KB):", spPsram);
        row = addSpin(panel, c, row, "Flash (KB):", spFlash);
        c.gridx = 1; c.gridy = row++; c.gridwidth = 2;
        panel.add(new JLabel("<html><i>De la flash salen las particiones "
                + "(FS + packs), como en la placa.</i></html>"), c);
        c.gridwidth = 1;

        row = addSep(panel, c, row, "Pantalla");
        c.gridx = 1; c.gridy = row++; c.gridwidth = 2;
        panel.add(cbNoScr, c);
        c.gridwidth = 1;
        JPanel res = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        res.add(spW); res.add(new JLabel("×")); res.add(spH);
        c.gridx = 0; c.gridy = row; panel.add(new JLabel("Resolución:"), c);
        c.gridx = 1; c.gridy = row++; panel.add(res, c);

        row = addSep(panel, c, row, "Rutas");
        row = addPath(panel, c, row, "Datos de la placa:", tfData, true);
        row = addPath(panel, c, row, "Librería de packs:", tfPacks, true);
        row = addPath(panel, c, row, "Raíz de bpgenvm-c:", tfVmc, true);
        c.gridx = 1; c.gridy = row++; c.gridwidth = 2;
        panel.add(new JLabel("<html><i>El binario del simulador es "
                + "&lt;bpgenvm-c&gt;/build/bpvm-sim.exe — la misma raíz que usa el AOT.</i></html>"), c);
        c.gridwidth = 1;

        row = addSep(panel, c, row, "Conexión");
        row = addSpin(panel, c, row, "Puerto TCP:", spPort);

        int r = JOptionPane.showConfirmDialog(parent, panel, "Micro simulado (VM-C)",
                JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
        if (r != JOptionPane.OK_OPTION) return false;

        prefs.simPort     = (Integer) spPort.getValue();
        prefs.simRamKb    = (Integer) spRam.getValue();
        prefs.simPsramKb  = (Integer) spPsram.getValue();
        prefs.simFlashKb  = (Integer) spFlash.getValue();
        prefs.simScreenW  = (Integer) spW.getValue();
        prefs.simScreenH  = (Integer) spH.getValue();
        prefs.simNoScreen = cbNoScr.isSelected();
        prefs.simDataDir  = blankToNull(tfData.getText());
        prefs.packsDir    = blankToNull(tfPacks.getText());
        prefs.aotBpgenvmDir = blankToNull(tfVmc.getText());
        prefs.save();
        return true;
    }

    /* ---------- helpers de maquetación ---------- */

    private static String blankToNull(String s) {
        if (s == null) return null;
        s = s.trim();
        return s.isEmpty() ? null : s;
    }

    private static JSpinner intSpinner(int val, int min, int max, int step) {
        if (val < min) val = min;
        if (val > max) val = max;
        JSpinner sp = new JSpinner(new SpinnerNumberModel(val, min, max, step));
        ((JSpinner.DefaultEditor) sp.getEditor()).getTextField().setColumns(7);
        return sp;
    }

    private static int addSep(JPanel p, GridBagConstraints c, int row, String title) {
        c.gridx = 0; c.gridy = row++; c.gridwidth = 3;
        c.insets = new Insets(10, 4, 2, 4);
        JLabel l = new JLabel(title);
        l.setFont(l.getFont().deriveFont(Font.BOLD));
        p.add(l, c);
        c.insets = new Insets(3, 4, 3, 4);
        c.gridwidth = 1;
        return row;
    }

    private static int addSpin(JPanel p, GridBagConstraints c, int row, String label, JSpinner sp) {
        c.gridx = 0; c.gridy = row; p.add(new JLabel(label), c);
        c.gridx = 1; c.gridy = row++; p.add(sp, c);
        return row;
    }

    private static int addPath(JPanel p, GridBagConstraints c, int row,
                               String label, JTextField tf, boolean dirs) {
        c.gridx = 0; c.gridy = row; p.add(new JLabel(label), c);
        c.gridx = 1; c.gridy = row; c.fill = GridBagConstraints.HORIZONTAL; c.weightx = 1;
        p.add(tf, c);
        c.fill = GridBagConstraints.NONE; c.weightx = 0;
        JButton browse = new JButton("…");
        browse.setMargin(new Insets(1, 4, 1, 4));
        browse.addActionListener(e -> {
            JFileChooser fc = new JFileChooser();
            fc.setFileSelectionMode(dirs ? JFileChooser.DIRECTORIES_ONLY
                                         : JFileChooser.FILES_ONLY);
            String cur = tf.getText().trim();
            if (!cur.isEmpty()) {
                File f = new File(cur);
                if (f.exists()) fc.setCurrentDirectory(dirs ? f : f.getParentFile());
            }
            if (fc.showOpenDialog(p) == JFileChooser.APPROVE_OPTION)
                tf.setText(fc.getSelectedFile().getAbsolutePath());
        });
        c.gridx = 2; c.gridy = row++; p.add(browse, c);
        return row;
    }
}
