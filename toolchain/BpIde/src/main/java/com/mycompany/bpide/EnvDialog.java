// ============================================================
// EnvDialog.java — #435: las variables de entorno, en su propio diálogo.
//
// Vivían en la mitad de arriba de BoardMgrPanel, compartiendo un split vertical
// con las particiones. Se mudan enteras (tabla + checkbox de PSRAM + añadir /
// editar / borrar) por decisión de Eduardo (17-ago): la ventana de la placa se
// queda con particiones y packs, y el hueco lo ocupa el panel de carpetas desde
// el que se añaden packs.
//
// No cambia NADA del comportamiento: mismo protocolo (ENV_LS / ENV_SET /
// ENV_DEL), mismas reglas y mismos mensajes. Es una mudanza, y en las mudanzas
// lo que se pierde son las razones — por eso las que había viajan con el código.
// ============================================================
package com.mycompany.bpide;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.Window;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.table.DefaultTableModel;

public final class EnvDialog extends JDialog {

    private static final long T = 8000;    // timeout wire

    /* Clave CANÓNICA del env (minúsculas): el firmware lee "psram" EXACTO
     * (bpvm_env_get es case-sensitive). Escribir "PSRAM" fue el bug que dejó
     * la PSRAM de la Metro apagada con el checkbox marcado (19-jul). La
     * lectura de abajo usa equalsIgnoreCase a propósito: tolera una entrada
     * vieja en mayúsculas para MOSTRARLA, pero aquí siempre se escribe en
     * minúsculas. */
    private static final String PSRAM_KEY = "psram";

    private final BpvmClient client;
    private final Consumer<String> log;
    private final Runnable alCambiar;      // el panel de placa se repinta (part.* comparte env)
    private boolean updatingUi = false;

    private final JCheckBox psramCheck = new JCheckBox("PSRAM presente (0/1)");
    private final DefaultTableModel envModel =
            new DefaultTableModel(new Object[] { "Clave", "Valor" }, 0) {
                @Override public boolean isCellEditable(int r, int c) { return false; }
            };
    private final JTable envTable = new JTable(envModel);

    public EnvDialog(Window duenyo, BpvmClient client, Consumer<String> log, Runnable alCambiar) {
        super(duenyo, "Variables de entorno", ModalityType.MODELESS);
        this.client = client;
        this.log = (log != null) ? log : s -> { };
        this.alCambiar = (alCambiar != null) ? alCambiar : () -> { };
        buildUi();
        setSize(460, 340);
        setLocationRelativeTo(duenyo);
        refresh();
    }

    private void buildUi() {
        JPanel raiz = new JPanel(new BorderLayout(4, 4));
        raiz.setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));

        JPanel top = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        psramCheck.addActionListener(e -> onPsramToggle());
        top.add(psramCheck);
        raiz.add(top, BorderLayout.NORTH);

        raiz.add(new JScrollPane(envTable), BorderLayout.CENTER);

        JPanel btns = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        JButton set = new JButton("Añadir / editar…");
        JButton del = new JButton("Borrar");
        JButton ref = new JButton("Refrescar");
        JButton cerrar = new JButton("Cerrar");
        set.addActionListener(e -> onEnvSet());
        del.addActionListener(e -> onEnvDel());
        ref.addActionListener(e -> refresh());
        cerrar.addActionListener(e -> dispose());
        btns.add(set); btns.add(del); btns.add(ref); btns.add(cerrar);
        raiz.add(btns, BorderLayout.SOUTH);

        setContentPane(raiz);
    }

    /** ¿es una clave gestionada por la sección de particiones? (part.<x>.size) */
    private static boolean isPartKey(String k) {
        return k != null && k.startsWith("part.") && k.endsWith(".size");
    }

    public void refresh() {
        if (client == null) return;
        bg(() -> client.envList(T), env -> {
            updatingUi = true;
            try {
                envModel.setRowCount(0);
                boolean psram = false;
                for (BpvmClient.EnvVar e : env) {
                    if (isPartKey(e.key)) continue;                 // gestionada en particiones
                    envModel.addRow(new Object[] { e.key, e.value });
                    if (PSRAM_KEY.equalsIgnoreCase(e.key)) psram = "1".equals(e.value.trim());
                }
                psramCheck.setSelected(psram);
            } finally {
                updatingUi = false;
            }
        });
    }

    private void onPsramToggle() {
        if (updatingUi || client == null) return;
        final String val = psramCheck.isSelected() ? "1" : "0";
        bg(() -> { client.envSet(PSRAM_KEY, val, T); return null; }, r -> {
            log.accept("[FrmBoard] " + PSRAM_KEY + "=" + val);
            refresh();
            alCambiar.run();
        });
    }

    private void onEnvSet() {
        int row = envTable.getSelectedRow();
        String defKey = row >= 0 ? String.valueOf(envModel.getValueAt(row, 0)) : "";
        String defVal = row >= 0 ? String.valueOf(envModel.getValueAt(row, 1)) : "";
        JTextField keyF = new JTextField(defKey, 16);
        JTextField valF = new JTextField(defVal, 16);
        JPanel form = new JPanel(new GridLayout(2, 2, 4, 4));
        form.add(new JLabel("Clave:"));  form.add(keyF);
        form.add(new JLabel("Valor:"));  form.add(valF);
        int ok = JOptionPane.showConfirmDialog(this, form, "Variable de entorno",
                JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
        if (ok != JOptionPane.OK_OPTION) return;
        String key = keyF.getText().trim();
        if (key.isEmpty()) return;
        if (isPartKey(key)) {
            info("Los tamaños de partición se editan en la sección de particiones.");
            return;
        }
        String val = valF.getText();
        bg(() -> { client.envSet(key, val, T); return null; }, r -> {
            log.accept("[FrmBoard] ENV_SET " + key + "=" + val);
            refresh();
            alCambiar.run();
        });
    }

    private void onEnvDel() {
        int row = envTable.getSelectedRow();
        if (row < 0) { info("Selecciona una variable para borrar."); return; }
        String key = String.valueOf(envModel.getValueAt(row, 0));
        if (JOptionPane.showConfirmDialog(this, "¿Borrar la variable '" + key + "'?",
                "Confirmar", JOptionPane.YES_NO_OPTION) != JOptionPane.YES_OPTION) return;
        bg(() -> { client.envDel(key, T); return null; }, r -> {
            log.accept("[FrmBoard] ENV_DEL " + key);
            refresh();
            alCambiar.run();
        });
    }

    private void info(String msg) {
        JOptionPane.showMessageDialog(this, msg, "Entorno", JOptionPane.INFORMATION_MESSAGE);
    }

    /** El wire NO se toca desde el EDT. Copia literal del de BoardMgrPanel:
     *  mismo hilo con nombre, mismo trato del error y mismo canal de log. */
    private <R> void bg(Callable<R> work, Consumer<R> ok) {
        new Thread(() -> {
            try {
                R res = work.call();
                SwingUtilities.invokeLater(() -> ok.accept(res));
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> {
                    String m = (e instanceof java.io.IOException) ? e.getMessage() : String.valueOf(e);
                    log.accept("[FrmBoard] error: " + m);
                    JOptionPane.showMessageDialog(this, m, "Error de gestión de placa",
                            JOptionPane.ERROR_MESSAGE);
                });
            }
        }, "envdlg-wire").start();
    }
}
