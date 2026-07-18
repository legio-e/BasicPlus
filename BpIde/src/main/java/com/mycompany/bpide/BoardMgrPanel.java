// ============================================================
// BoardMgrPanel.java
// H9 — panel funcional de gestión de placa (entorno + particiones + STATE),
// hablando el protocolo ENV_*/PART_* vía BpvmClient sobre la conexión YA abierta
// (compartida con la ventana principal). Autocontenido (Swing a mano, sin .form)
// para que Eduardo lo coloque donde quiera dentro de FrmBoard; de momento FrmBoard
// lo embebe entero. El "device" puede ser el firmware (kernel-comm) o el boardsim
// de host (tools/boardsim.c) — sin placa.
//
// Todas las llamadas al wire van en un thread de fondo (bloquean); las
// actualizaciones de tabla vuelven al EDT con invokeLater.
// ============================================================
package com.mycompany.bpide;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.table.DefaultTableModel;

public final class BoardMgrPanel extends JPanel {

    private static final long T = 8000;   // timeout wire

    private BpvmClient client;
    private Consumer<String> log = s -> { };

    private final JLabel stateLabel = new JLabel("(sin conexión)");
    private final JLabel flashLabel = new JLabel(" ");

    private final DefaultTableModel envModel =
            new DefaultTableModel(new Object[] { "Clave", "Valor" }, 0) {
                @Override public boolean isCellEditable(int r, int c) { return false; }
            };
    private final JTable envTable = new JTable(envModel);

    // col 1 (Tamaño KB) editable; el resto no
    private final DefaultTableModel partModel =
            new DefaultTableModel(new Object[] { "Partición", "Tamaño (KB)", "Offset" }, 0) {
                @Override public boolean isCellEditable(int r, int c) { return c == 1; }
            };
    private final JTable partTable = new JTable(partModel);

    public BoardMgrPanel() {
        super(new BorderLayout(4, 4));
        setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        buildUi();
    }

    /** Sink opcional para volcar mensajes a la consola del IDE. */
    public void setLog(Consumer<String> l) { if (l != null) this.log = l; }

    /** Engancha (o re-engancha) el cliente ya conectado y refresca. null = desconectado. */
    public void attach(BpvmClient c) {
        this.client = c;
        if (c == null) { stateLabel.setText("(sin conexión)"); clearTables(); }
        else refresh();
    }

    // ---- UI ----

    private void buildUi() {
        JPanel north = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 2));
        north.add(new JLabel("Estado:"));
        north.add(stateLabel);
        JButton refresh = new JButton("Refrescar");
        refresh.addActionListener(e -> refresh());
        north.add(refresh);
        add(north, BorderLayout.NORTH);

        // --- entorno ---
        JPanel envPanel = new JPanel(new BorderLayout(4, 4));
        envPanel.setBorder(BorderFactory.createTitledBorder("Variables de entorno"));
        envPanel.add(new JScrollPane(envTable), BorderLayout.CENTER);
        JPanel envBtns = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        JButton envSet = new JButton("Añadir / editar…");
        JButton envDel = new JButton("Borrar");
        envSet.addActionListener(e -> onEnvSet());
        envDel.addActionListener(e -> onEnvDel());
        envBtns.add(envSet);
        envBtns.add(envDel);
        envPanel.add(envBtns, BorderLayout.SOUTH);

        // --- particiones ---
        JPanel partPanel = new JPanel(new BorderLayout(4, 4));
        partPanel.setBorder(BorderFactory.createTitledBorder("Particiones (el usuario solo edita tamaños)"));
        partPanel.add(new JScrollPane(partTable), BorderLayout.CENTER);
        JPanel partSouth = new JPanel(new BorderLayout());
        partSouth.add(flashLabel, BorderLayout.NORTH);
        JPanel partBtns = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        JButton defaults = new JButton("Proponer defaults");
        JButton apply = new JButton("Aplicar tamaños");
        defaults.addActionListener(e -> onDefaults());
        apply.addActionListener(e -> onApply());
        partBtns.add(defaults);
        partBtns.add(apply);
        partSouth.add(partBtns, BorderLayout.SOUTH);
        partPanel.add(partSouth, BorderLayout.SOUTH);

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT, envPanel, partPanel);
        split.setResizeWeight(0.55);
        add(split, BorderLayout.CENTER);
    }

    private void clearTables() {
        envModel.setRowCount(0);
        partModel.setRowCount(0);
        flashLabel.setText(" ");
    }

    // ---- acciones ----

    /** Lee STATE + ENV_LS + PART_LS y repuebla las tablas. */
    public void refresh() {
        if (client == null) { stateLabel.setText("(sin conexión)"); return; }
        stateLabel.setText("consultando…");
        bg(() -> {
            BpvmClient.BoardState st = client.boardState(T);
            List<BpvmClient.EnvVar> env = client.envList(T);
            BpvmClient.PartTable pt = client.partLayout(T);
            return new Object[] { st, env, pt };
        }, arr -> {
            BpvmClient.BoardState st = (BpvmClient.BoardState) arr[0];
            @SuppressWarnings("unchecked")
            List<BpvmClient.EnvVar> env = (List<BpvmClient.EnvVar>) arr[1];
            BpvmClient.PartTable pt = (BpvmClient.PartTable) arr[2];

            stateLabel.setText("estado " + st.state + " — " + st.name
                    + (st.degraded ? "  ⚠ DEGRADADO: " + st.reason : ""));
            envModel.setRowCount(0);
            for (BpvmClient.EnvVar e : env) envModel.addRow(new Object[] { e.key, e.value });
            partModel.setRowCount(0);
            for (BpvmClient.Partition p : pt.parts) {
                partModel.addRow(new Object[] { p.name, p.size / 1024L,
                        p.offset < 0 ? "—" : hex(p.offset) });
            }
            flashLabel.setText("Flash usable: " + (pt.usableFlash / 1024L) + " KB · base "
                    + hex(pt.base) + (pt.missing ? "  ·  SIN PARTICIONES → propón defaults" : ""));
        });
    }

    private void onEnvSet() {
        int row = envTable.getSelectedRow();
        String defKey = row >= 0 ? String.valueOf(envModel.getValueAt(row, 0)) : "";
        String defVal = row >= 0 ? String.valueOf(envModel.getValueAt(row, 1)) : "";
        JTextField keyF = new JTextField(defKey, 16);
        JTextField valF = new JTextField(defVal, 16);
        JPanel form = new JPanel(new java.awt.GridLayout(2, 2, 4, 4));
        form.add(new JLabel("Clave:"));  form.add(keyF);
        form.add(new JLabel("Valor:"));  form.add(valF);
        int ok = JOptionPane.showConfirmDialog(this, form, "Variable de entorno",
                JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
        if (ok != JOptionPane.OK_OPTION) return;
        String key = keyF.getText().trim();
        if (key.isEmpty()) return;
        String val = valF.getText();
        bg(() -> { client.envSet(key, val, T); return null; }, r -> {
            log.accept("[FrmBoard] ENV_SET " + key + "=" + val);
            refresh();
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
        });
    }

    private void onDefaults() {
        if (client == null) return;
        bg(() -> client.partDefaults(T), pt -> {
            // rellena la columna de tamaños con la propuesta (el usuario la ajusta y aplica)
            for (BpvmClient.Partition p : pt.parts) {
                for (int r = 0; r < partModel.getRowCount(); r++) {
                    if (p.name.equals(partModel.getValueAt(r, 0))) {
                        partModel.setValueAt(p.size / 1024L, r, 1);
                    }
                }
            }
            if (partModel.getRowCount() == 0) {   // placa virgen: aún no hay filas
                for (BpvmClient.Partition p : pt.parts) {
                    partModel.addRow(new Object[] { p.name, p.size / 1024L, "—" });
                }
            }
            info("Tamaños por defecto propuestos. Ajusta si quieres y pulsa «Aplicar tamaños».");
        });
    }

    private void onApply() {
        if (client == null) return;
        if (partModel.getRowCount() == 0) { info("No hay particiones que aplicar (pulsa «Proponer defaults»)."); return; }
        if (partTable.isEditing()) partTable.getCellEditor().stopCellEditing();
        Map<String, Long> sizes = new LinkedHashMap<>();
        try {
            for (int r = 0; r < partModel.getRowCount(); r++) {
                String name = String.valueOf(partModel.getValueAt(r, 0));
                long kb = Long.parseLong(String.valueOf(partModel.getValueAt(r, 1)).trim());
                sizes.put(name, kb * 1024L);
            }
        } catch (NumberFormatException nfe) {
            info("Los tamaños deben ser números (en KB).");
            return;
        }
        bg(() -> { client.partApply(sizes, T); return null; }, r -> {
            log.accept("[FrmBoard] PART_APPLY " + sizes);
            info("Tamaños aplicados. El device se reiniciará y subirá con el layout nuevo.");
            refresh();
        });
    }

    // ---- helpers ----

    private static String hex(long v) { return String.format("0x%X", v); }

    private void info(String msg) {
        JOptionPane.showMessageDialog(this, msg, "Gestión de placa", JOptionPane.INFORMATION_MESSAGE);
    }

    /** Ejecuta `work` en un thread de fondo (el wire bloquea) y entrega el resultado
     *  en el EDT a `ok`; los errores (IOException del peer, etc.) se muestran. */
    private <R> void bg(Callable<R> work, Consumer<R> ok) {
        new Thread(() -> {
            try {
                R res = work.call();
                SwingUtilities.invokeLater(() -> ok.accept(res));
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> {
                    String m = e instanceof IOException ? e.getMessage() : String.valueOf(e);
                    log.accept("[FrmBoard] error: " + m);
                    stateLabel.setText("error");
                    JOptionPane.showMessageDialog(this, m, "Error de gestión de placa",
                            JOptionPane.ERROR_MESSAGE);
                });
            }
        }, "boardmgr-wire").start();
    }
}
