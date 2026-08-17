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
import javax.swing.DefaultListModel;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JList;
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

    private BpvmClient client;
    private Consumer<String> log = s -> { };
    private boolean updatingUi = false;    // suprime listeners durante un refresh programático

    private final JLabel stateLabel = new JLabel("(sin conexión)");
    private final JLabel flashLabel = new JLabel(" ");
    /* #435 — EL PANEL DE CARPETAS, donde estaba el cuadro del entorno. Enseña
     * una carpeta del PC (por defecto la de packs) y deja navegar; con un
     * `.pack` seleccionado, «Añadir» lo graba en la placa sin pasar por el
     * chooser. La grabación NO vive aquí: la hace PacksPanel y las une
     * FrmBoard, que es quien tiene los dos paneles — así no se acoplan entre
     * sí ni se duplica el camino de grabado. */
    private final DefaultListModel<java.io.File> carpetaModel = new DefaultListModel<>();
    private final JList<java.io.File> carpetaList = new JList<>(carpetaModel);
    private final JLabel carpetaLabel = new JLabel(" ");
    private final JButton anyadirBtn = new JButton("Añadir a la placa");
    private java.io.File carpetaActual;
    private Consumer<java.io.File> alAnyadir = f -> { };
    private Runnable alAbrirEnv = () -> { };

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
        /* #435 — la entrada al entorno, que ya no vive en esta ventana. Va en
         * la barra que ya estaba, junto a las demás acciones de placa; quien
         * abre el diálogo es FrmBoard, que tiene la conexión. */
        JButton envBtn = new JButton("Variables de entorno…");
        envBtn.addActionListener(e -> alAbrirEnv.run());
        north.add(envBtn);
        add(north, BorderLayout.NORTH);

        JPanel carpetaPanel = construirPanelCarpeta();

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

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT, partPanel, carpetaPanel);
        split.setResizeWeight(0.55);
        add(split, BorderLayout.CENTER);
    }

    private void clearTables() {
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

    /** STATE + PART_LS (+ defaults si virgen) → repuebla todo.
     *
     *  <p>#435 — ya NO pide ENV_LS: el entorno se fue a {@link EnvDialog} y con
     *  él su consulta. Una petición menos por refresco, que en placa se nota. */
    public void refresh() {
        if (client == null) { stateLabel.setText("(sin conexión)"); return; }
        stateLabel.setText("consultando…");
        bg(() -> {
            BpvmClient.BoardState st = client.boardState(T);
            BpvmClient.PartTable lay = client.partLayout(T);
            // en placa virgen la tabla de particiones viene vacía; usamos DEFAULTS
            // para conocer el conjunto fijo y proponer tamaños en el editor de abajo.
            BpvmClient.PartTable defs = lay.missing ? client.partDefaults(T) : null;
            return new Object[] { st, lay, defs };
        }, arr -> {
            BpvmClient.BoardState st = (BpvmClient.BoardState) arr[0];
            BpvmClient.PartTable lay = (BpvmClient.PartTable) arr[1];
            BpvmClient.PartTable defs = (BpvmClient.PartTable) arr[2];
            fill(st, lay, defs);
        });
    }

    private void fill(BpvmClient.BoardState st,
                      BpvmClient.PartTable lay, BpvmClient.PartTable defs) {
        updatingUi = true;
        try {
            stateLabel.setText("estado " + st.state + " — " + st.name
                    + (st.degraded ? "  ⚠ DEGRADADO: " + st.reason : ""));

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

    // ---- #435: el panel de carpetas ----

    /** Enseña una carpeta del PC y deja navegar. Reemplaza al chooser para
     *  añadir packs: el fichero ya está a la vista y basta seleccionarlo.
     *
     *  <p>Enseña TODO (carpetas y ficheros) y habilita «Añadir» sólo con un
     *  `.pack` seleccionado, en vez de filtrar. Filtrando, un pack con el
     *  nombre mal escrito «no está» y no se ve por qué; así se ve que está y
     *  que no vale. */
    private JPanel construirPanelCarpeta() {
        JPanel panel = new JPanel(new BorderLayout(4, 4));
        panel.setBorder(BorderFactory.createTitledBorder("Carpeta (doble clic para entrar)"));

        JPanel top = new JPanel(new BorderLayout(4, 0));
        JButton arriba = new JButton("↑");
        arriba.setToolTipText("Carpeta superior");
        arriba.addActionListener(e -> {
            if (carpetaActual != null && carpetaActual.getParentFile() != null)
                verCarpeta(carpetaActual.getParentFile());
        });
        JButton otra = new JButton("Otra…");
        otra.addActionListener(e -> elegirCarpeta());
        JPanel topBtns = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        topBtns.add(arriba); topBtns.add(otra);
        carpetaLabel.setToolTipText("Carpeta que se está mostrando");
        top.add(topBtns, BorderLayout.WEST);
        top.add(carpetaLabel, BorderLayout.CENTER);
        panel.add(top, BorderLayout.NORTH);

        carpetaList.setSelectionMode(javax.swing.ListSelectionModel.SINGLE_SELECTION);
        carpetaList.setCellRenderer(new javax.swing.DefaultListCellRenderer() {
            @Override public java.awt.Component getListCellRendererComponent(
                    javax.swing.JList<?> l, Object v, int i, boolean sel, boolean foco) {
                super.getListCellRendererComponent(l, v, i, sel, foco);
                java.io.File f = (java.io.File) v;
                setText(f.isDirectory() ? "[" + f.getName() + "]" : f.getName());
                return this;
            }
        });
        carpetaList.addListSelectionListener(e -> actualizarAnyadir());
        carpetaList.addMouseListener(new java.awt.event.MouseAdapter() {
            @Override public void mouseClicked(java.awt.event.MouseEvent e) {
                if (e.getClickCount() != 2) return;
                java.io.File f = carpetaList.getSelectedValue();
                if (f != null && f.isDirectory()) verCarpeta(f);
            }
        });
        panel.add(new JScrollPane(carpetaList), BorderLayout.CENTER);

        JPanel sur = new JPanel(new FlowLayout(FlowLayout.LEFT, 6, 2));
        anyadirBtn.setEnabled(false);
        anyadirBtn.addActionListener(e -> {
            java.io.File f = carpetaList.getSelectedValue();
            if (f != null && f.isFile()) alAnyadir.accept(f);
        });
        sur.add(anyadirBtn);
        panel.add(sur, BorderLayout.SOUTH);

        verCarpeta(carpetaPorDefecto());
        return panel;
    }

    /** Qué carpeta se enseña al abrir: LA DE PACKS, que es lo que se viene a
     *  buscar aquí (Eduardo). Y no se inventa cuál es: `packsDirEffective()` ya
     *  resuelve la configurada → la que viaja junto al jar → `packs/` del
     *  directorio de trabajo, que es la misma cadena que usa el compilador. Si
     *  no hay ninguna, el directorio de trabajo — con la navegación a mano para
     *  ir a otra parte. */
    private static java.io.File carpetaPorDefecto() {
        String packs = IdePrefs.load().packsDirEffective();
        if (packs != null) {
            java.io.File d = new java.io.File(packs);
            if (d.isDirectory()) return d;
        }
        return new java.io.File(".").getAbsoluteFile();
    }

    private void verCarpeta(java.io.File dir) {
        if (dir == null || !dir.isDirectory()) return;
        carpetaActual = dir;
        carpetaLabel.setText(dir.getAbsolutePath());
        carpetaModel.clear();
        java.io.File[] hijos = dir.listFiles();
        if (hijos == null) {
            // Sin permisos o desconectada: se DICE. Una lista vacía se leería
            // como "aquí no hay nada", que es otra cosa.
            carpetaLabel.setText(dir.getAbsolutePath() + "   (no se puede leer)");
            return;
        }
        java.util.Arrays.sort(hijos, (a, b) -> {
            if (a.isDirectory() != b.isDirectory()) return a.isDirectory() ? -1 : 1;
            return a.getName().compareToIgnoreCase(b.getName());
        });
        for (java.io.File f : hijos) carpetaModel.addElement(f);
        actualizarAnyadir();
    }

    private void elegirCarpeta() {
        javax.swing.JFileChooser fc = new javax.swing.JFileChooser(carpetaActual);
        fc.setFileSelectionMode(javax.swing.JFileChooser.DIRECTORIES_ONLY);
        fc.setDialogTitle("Ver carpeta");
        if (fc.showOpenDialog(this) == javax.swing.JFileChooser.APPROVE_OPTION)
            verCarpeta(fc.getSelectedFile());
    }

    private void actualizarAnyadir() {
        java.io.File f = carpetaList.getSelectedValue();
        boolean esPack = f != null && f.isFile() && esPack(f.getName());
        anyadirBtn.setEnabled(esPack);
        anyadirBtn.setToolTipText(esPack
                ? "Graba " + f.getName() + " en la placa"
                : "Selecciona un fichero .pack de la lista");
    }

    /** ¿es un pack? ⚠️ Esta regla está escrita a mano en varios sitios más
     *  (`bpvm.c`, `Main.java`, `FrmMain` ×2, `SimRunner`) y está fichada como
     *  riesgo en ESTADO.md. Aquí NO se inventa una sexta: se usa el mismo
     *  criterio —extensión `.pack`, sin distinguir mayúsculas— y se deja dicho,
     *  para que el día que se unifique aparezca en el grep. */
    private static boolean esPack(String nombre) {
        return nombre != null && nombre.toLowerCase().endsWith(".pack");
    }

    /** #435 — qué hacer con el fichero que se «añade». Lo pone FrmBoard, que es
     *  quien tiene también el panel de packs: así este panel no sabe grabar y
     *  el de packs no sabe de carpetas. */
    public void setAlAnyadir(Consumer<java.io.File> h) {
        if (h != null) this.alAnyadir = h;
    }

    /** #435 — qué hace el botón «Variables de entorno…». Lo pone FrmBoard, que
     *  es quien tiene la conexión y el diálogo. */
    public void setAlAbrirEnv(Runnable h) {
        if (h != null) this.alAbrirEnv = h;
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
