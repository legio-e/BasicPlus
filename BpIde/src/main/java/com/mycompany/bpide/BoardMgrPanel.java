// ============================================================
// BoardMgrPanel.java
// H9 — panel funcional de gestión de placa (entorno + particiones + STATE),
// hablando el protocolo ENV_*/PART_* vía BpvmClient sobre la conexión YA abierta
// (compartida con la ventana principal). Autocontenido (Swing a mano, sin .form)
// para que Eduardo lo coloque donde quiera dentro de FrmBoard.
//
// Retoques (Eduardo, tras probar):
//   - partición "fs" se muestra como "File System" (no todo el mundo sabe qué es FS).
//   - los cambios de TAMAÑO se hacen ABAJO (campos por partición), no en la tabla.
//   - la tabla de entorno NO muestra las claves de partición (part.*.size); las
//     gestiona la sección de particiones.
//   - checkbox PSRAM (por defecto 0; marcado = 1).
//   - (pendiente placa) Flash.Size: se decidirá lectura/escritura según autodetección.
//
// Todas las llamadas al wire van en un thread de fondo (bloquean); las
// actualizaciones de UI vuelven al EDT con invokeLater.
// ============================================================
package com.mycompany.bpide;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
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

    private static final long T = 8000;    // timeout wire
    /* Clave CANÓNICA del env (minúsculas): el firmware lee "psram" EXACTO
     * (bpvm_env_get es case-sensitive). Escribir "PSRAM" fue el bug que dejó
     * la PSRAM de la Metro apagada con el checkbox marcado (19-jul). La
     * lectura de abajo usa equalsIgnoreCase a propósito: tolera una entrada
     * vieja en mayúsculas para MOSTRARLA, pero aquí siempre se escribe en
     * minúsculas. */
    private static final String PSRAM_KEY = "psram";

    private BpvmClient client;
    private Consumer<String> log = s -> { };
    private boolean updatingUi = false;    // suprime listeners durante un refresh programático

    private final JLabel stateLabel = new JLabel("(sin conexión)");
    private final JLabel flashLabel = new JLabel(" ");
    private final JCheckBox psramCheck = new JCheckBox("PSRAM presente (0/1)");

    private final DefaultTableModel envModel =
            new DefaultTableModel(new Object[] { "Clave", "Valor" }, 0) {
                @Override public boolean isCellEditable(int r, int c) { return false; }
            };
    private final JTable envTable = new JTable(envModel);

    // tabla de particiones AHORA es solo-lectura (los tamaños se editan abajo)
    private final DefaultTableModel partModel =
            new DefaultTableModel(new Object[] { "Partición", "Tamaño (KB)", "Offset" }, 0) {
                @Override public boolean isCellEditable(int r, int c) { return false; }
            };
    private final JTable partTable = new JTable(partModel);

    // editor de tamaños "abajo": un campo por partición (nombre interno → campo)
    private final JPanel sizeForm = new JPanel();
    private final Map<String, JTextField> sizeFields = new LinkedHashMap<>();

    public BoardMgrPanel() {
        super(new BorderLayout(4, 4));
        setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        buildUi();
    }

    public void setLog(Consumer<String> l) { if (l != null) this.log = l; }

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
        // PSRAM como checkbox de conveniencia (arriba)
        JPanel envTop = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        psramCheck.addActionListener(e -> onPsramToggle());
        envTop.add(psramCheck);
        envPanel.add(envTop, BorderLayout.NORTH);
        envPanel.add(new JScrollPane(envTable), BorderLayout.CENTER);
        JPanel envBtns = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        JButton envSet = new JButton("Añadir / editar…");
        JButton envDel = new JButton("Borrar");
        envSet.addActionListener(e -> onEnvSet());
        envDel.addActionListener(e -> onEnvDel());
        envBtns.add(envSet);
        envBtns.add(envDel);
        envPanel.add(envBtns, BorderLayout.SOUTH);

        // --- particiones (tabla read-only arriba, edición de tamaños abajo) ---
        JPanel partPanel = new JPanel(new BorderLayout(4, 4));
        partPanel.setBorder(BorderFactory.createTitledBorder("Particiones (edita los tamaños abajo)"));
        partPanel.add(new JScrollPane(partTable), BorderLayout.CENTER);

        JPanel partSouth = new JPanel(new BorderLayout(4, 4));
        partSouth.add(flashLabel, BorderLayout.NORTH);
        sizeForm.setLayout(new BoxLayout(sizeForm, BoxLayout.Y_AXIS));
        partSouth.add(sizeForm, BorderLayout.CENTER);
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
        split.setResizeWeight(0.45);
        add(split, BorderLayout.CENTER);
    }

    private void clearTables() {
        envModel.setRowCount(0);
        partModel.setRowCount(0);
        sizeFields.clear();
        sizeForm.removeAll();
        sizeForm.revalidate();
        sizeForm.repaint();
        flashLabel.setText(" ");
    }

    /** "fs" → "File System"; el resto capitalizado (packs → Packs). */
    private static String displayName(String part) {
        if ("fs".equals(part)) return "File System";
        return part.isEmpty() ? part : Character.toUpperCase(part.charAt(0)) + part.substring(1);
    }

    /** ¿es una clave gestionada por la sección de particiones? (part.<x>.size) */
    private static boolean isPartKey(String k) {
        return k != null && k.startsWith("part.") && k.endsWith(".size");
    }

    // ---- acciones ----

    /** STATE + ENV_LS + PART_LS (+ defaults si virgen) → repuebla todo. */
    public void refresh() {
        if (client == null) { stateLabel.setText("(sin conexión)"); return; }
        stateLabel.setText("consultando…");
        bg(() -> {
            BpvmClient.BoardState st = client.boardState(T);
            List<BpvmClient.EnvVar> env = client.envList(T);
            BpvmClient.PartTable lay = client.partLayout(T);
            // en placa virgen la tabla de particiones viene vacía; usamos DEFAULTS
            // para conocer el conjunto fijo y proponer tamaños en el editor de abajo.
            BpvmClient.PartTable defs = lay.missing ? client.partDefaults(T) : null;
            return new Object[] { st, env, lay, defs };
        }, arr -> {
            BpvmClient.BoardState st = (BpvmClient.BoardState) arr[0];
            @SuppressWarnings("unchecked")
            List<BpvmClient.EnvVar> env = (List<BpvmClient.EnvVar>) arr[1];
            BpvmClient.PartTable lay = (BpvmClient.PartTable) arr[2];
            BpvmClient.PartTable defs = (BpvmClient.PartTable) arr[3];
            fill(st, env, lay, defs);
        });
    }

    private void fill(BpvmClient.BoardState st, List<BpvmClient.EnvVar> env,
                      BpvmClient.PartTable lay, BpvmClient.PartTable defs) {
        updatingUi = true;
        try {
            stateLabel.setText("estado " + st.state + " — " + st.name
                    + (st.degraded ? "  ⚠ DEGRADADO: " + st.reason : ""));

            // entorno: sin las claves de partición; PSRAM alimenta también el checkbox
            envModel.setRowCount(0);
            boolean psram = false;
            for (BpvmClient.EnvVar e : env) {
                if (isPartKey(e.key)) continue;                 // gestionada abajo
                envModel.addRow(new Object[] { e.key, e.value });
                if (PSRAM_KEY.equalsIgnoreCase(e.key)) psram = "1".equals(e.value.trim());
            }
            psramCheck.setSelected(psram);

            // tabla de particiones (solo lectura): el layout actual
            partModel.setRowCount(0);
            for (BpvmClient.Partition p : lay.parts) {
                partModel.addRow(new Object[] { displayName(p.name), p.size / 1024L,
                        p.offset < 0 ? "—" : hex(p.offset) });
            }
            flashLabel.setText("Flash usable: " + (lay.usableFlash / 1024L) + " KB · base "
                    + hex(lay.base) + (lay.missing ? "  ·  SIN PARTICIONES → propón defaults" : ""));

            // editor de tamaños (abajo): conjunto fijo, pre-relleno del layout o de defaults
            List<BpvmClient.Partition> set = lay.missing && defs != null ? defs.parts : lay.parts;
            rebuildSizeForm(set);
        } finally {
            updatingUi = false;
        }
    }

    /** Reconstruye el formulario de tamaños: una fila «[nombre]: [campo] KB» por partición. */
    private void rebuildSizeForm(List<BpvmClient.Partition> parts) {
        sizeFields.clear();
        sizeForm.removeAll();
        for (BpvmClient.Partition p : parts) {
            JPanel row = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 1));
            row.add(new JLabel(displayName(p.name) + ":"));
            JTextField f = new JTextField(String.valueOf(p.size / 1024L), 8);
            row.add(f);
            row.add(new JLabel("KB"));
            sizeForm.add(row);
            sizeFields.put(p.name, f);                          // clave INTERNA (fs/packs)
        }
        sizeForm.revalidate();
        sizeForm.repaint();
    }

    private void onPsramToggle() {
        if (updatingUi || client == null) return;
        final String val = psramCheck.isSelected() ? "1" : "0";
        bg(() -> { client.envSet(PSRAM_KEY, val, T); return null; }, r -> {
            log.accept("[FrmBoard] " + PSRAM_KEY + "=" + val);
            refresh();
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
        if (isPartKey(key)) { info("Los tamaños de partición se editan en la sección de particiones."); return; }
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
        bg(() -> client.partDefaults(T), defs -> {
            // rellena los campos de abajo con la propuesta (el usuario ajusta y aplica)
            for (BpvmClient.Partition p : defs.parts) {
                JTextField f = sizeFields.get(p.name);
                if (f == null) { rebuildSizeForm(defs.parts); f = sizeFields.get(p.name); }
                if (f != null) f.setText(String.valueOf(p.size / 1024L));
            }
            info("Tamaños por defecto propuestos abajo. Ajusta si quieres y pulsa «Aplicar tamaños».");
        });
    }

    private void onApply() {
        if (client == null) return;
        if (sizeFields.isEmpty()) { info("No hay particiones que aplicar."); return; }
        Map<String, Long> sizes = new LinkedHashMap<>();
        try {
            for (Map.Entry<String, JTextField> e : sizeFields.entrySet()) {
                long kb = Long.parseLong(e.getValue().getText().trim());
                sizes.put(e.getKey(), kb * 1024L);
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
