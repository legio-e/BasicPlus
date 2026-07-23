// ============================================================
// PacksPanel.java
// H3 — panel de gestión de PACKS de la placa (fase 1: LIST + interior),
// hablando PACK_LS / PACK_ENTRIES vía BpvmClient sobre la conexión YA abierta
// (compartida con la ventana principal). Autocontenido (Swing a mano, sin
// .form), como BoardMgrPanel.
//
// Retoques (Eduardo, 23-jul tras probar la v1):
//   - al seleccionar un pack se ven los FICHEROS de dentro (PACK_ENTRIES).
//   - columna FECHA (la fecha de build del pack: sirve para comparar si el
//     grabado en placa es más viejo que el recién construido en el PC).
//   - ocupación de la zona en bytes y % (la partición se ajusta una vez;
//     el día a día es "¿cuánto me queda?") → barra + resumen.
//
// Orden de fases: 1º LIST (esto) → 2º ADD/BURN → 3º REMOVE (tombstone; el
// hueco lo recupera una compactación desde el PC).
//
// La tabla muestra la CADENA completa (activos Y tombstones): el tombstone
// sigue ocupando flash hasta compactar, y verlo cuenta la verdad del espacio.
//
// Todas las llamadas al wire van en un thread de fondo; la UI vuelve al EDT.
// ============================================================
package com.mycompany.bpide;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.io.IOException;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.table.DefaultTableModel;

public final class PacksPanel extends JPanel {

    private static final long T = 8000;    // timeout wire
    private static final DateTimeFormatter FECHA_FMT =
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm").withZone(ZoneId.systemDefault());

    private BpvmClient client;
    private Consumer<String> log = s -> { };

    private final JLabel summaryLabel = new JLabel("(sin conexión)");
    private final JProgressBar usageBar = new JProgressBar(0, 100);

    private final DefaultTableModel packModel = new DefaultTableModel(
            new Object[] { "Pack", "Versión", "Fecha", "Tamaño (KB)", "Fich.", "Estado", "CRC" }, 0) {
        @Override public boolean isCellEditable(int r, int c) { return false; }
    };
    private final JTable packTable = new JTable(packModel);
    /** offsets de los packs listados, alineados con las filas de packModel
     *  (el offset identifica al pack en PACK_ENTRIES — y en DEL/READ futuros). */
    private final List<Long> packOffsets = new ArrayList<>();

    private final DefaultTableModel fileModel = new DefaultTableModel(
            new Object[] { "Fichero", "Tipo", "Tamaño (B)" }, 0) {
        @Override public boolean isCellEditable(int r, int c) { return false; }
    };
    private final JTable fileTable = new JTable(fileModel);
    private final JPanel filePanel = new JPanel(new BorderLayout(4, 4));

    public PacksPanel() {
        super(new BorderLayout(4, 4));
        setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        buildUi();
    }

    public void setLog(Consumer<String> l) { if (l != null) this.log = l; }

    public void attach(BpvmClient c) {
        this.client = c;
        if (c == null) { summaryLabel.setText("(sin conexión)"); clearAll(); }
        else refresh();
    }

    private void buildUi() {
        JPanel north = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 2));
        north.add(new JLabel("Packs de la placa:"));
        JButton refresh = new JButton("Refrescar");
        refresh.addActionListener(e -> refresh());
        north.add(refresh);
        add(north, BorderLayout.NORTH);

        JPanel packPanel = new JPanel(new BorderLayout(4, 4));
        packPanel.setBorder(BorderFactory.createTitledBorder("Packs (cadena completa: activos y borrados)"));
        packTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        packTable.getSelectionModel().addListSelectionListener(e -> {
            if (!e.getValueIsAdjusting()) onPackSelected();
        });
        packPanel.add(new JScrollPane(packTable), BorderLayout.CENTER);

        filePanel.setBorder(BorderFactory.createTitledBorder("Ficheros del pack"));
        filePanel.add(new JScrollPane(fileTable), BorderLayout.CENTER);

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT, packPanel, filePanel);
        split.setResizeWeight(0.55);
        add(split, BorderLayout.CENTER);

        // resumen de ocupación: barra (con % pintado encima) + detalle en bytes
        JPanel south = new JPanel(new BorderLayout(4, 2));
        usageBar.setStringPainted(true);
        usageBar.setString("");
        south.add(usageBar, BorderLayout.NORTH);
        south.add(summaryLabel, BorderLayout.SOUTH);
        add(south, BorderLayout.SOUTH);
    }

    private void clearAll() {
        packModel.setRowCount(0);
        packOffsets.clear();
        fileModel.setRowCount(0);
        setFilesTitle(null);
        usageBar.setValue(0);
        usageBar.setString("");
    }

    private void setFilesTitle(String packName) {
        filePanel.setBorder(BorderFactory.createTitledBorder(
                packName == null ? "Ficheros del pack (selecciona uno arriba)"
                                 : "Ficheros de '" + packName + "'"));
        filePanel.repaint();
    }

    /** PACK_LS → repuebla la tabla + ocupación. UNSUPPORTED (placa sin zona de
     *  packs aún) se muestra como estado, no como error a gritos. */
    public void refresh() {
        if (client == null) { summaryLabel.setText("(sin conexión)"); return; }
        summaryLabel.setText("consultando…");
        bg(() -> client.packList(T), this::fill);
    }

    private void fill(BpvmClient.PackTable t) {
        clearAll();
        int alive = 0;
        for (BpvmClient.PackInfo p : t.packs) {
            packModel.addRow(new Object[] { p.name, p.version,
                    p.date > 0 ? FECHA_FMT.format(Instant.ofEpochSecond(p.date)) : "—",
                    p.size / 1024L, p.files,
                    p.active ? "activo" : "borrado", p.crcOk ? "OK" : "MAL" });
            packOffsets.add(p.offset);
            if (p.active) alive++;
        }
        long used = t.regionSize - t.free;   // ocupado = cadena entera (tombstones incluidos)
        int pct = t.regionSize > 0 ? (int) Math.round(used * 100.0 / t.regionSize) : 0;
        usageBar.setValue(t.chainOk ? pct : 100);
        usageBar.setString(t.chainOk ? ("ocupado " + pct + "%") : "CADENA CORRUPTA");
        StringBuilder s = new StringBuilder();
        s.append(t.count).append(" packs (").append(alive).append(" activos) · ocupado ")
         .append(kb(used)).append(" · libre ").append(kb(t.free))
         .append(" de ").append(kb(t.regionSize));
        if (!t.chainOk) s.append("  ⚠ compactar desde el PC");
        if (t.count > t.packs.size()) s.append("  (listados ").append(t.packs.size()).append(")");
        summaryLabel.setText(s.toString());
    }

    /** Selección en la tabla de packs → PACK_ENTRIES del offset de esa fila. */
    private void onPackSelected() {
        int row = packTable.getSelectedRow();
        fileModel.setRowCount(0);
        if (row < 0 || row >= packOffsets.size() || client == null) { setFilesTitle(null); return; }
        final String name = String.valueOf(packModel.getValueAt(row, 0));
        final long off = packOffsets.get(row);
        setFilesTitle(name);
        bg(() -> client.packEntries(off, T), entries -> {
            fileModel.setRowCount(0);
            for (BpvmClient.PackEntry e : entries)
                fileModel.addRow(new Object[] { e.nombre + "." + e.tipo, e.tipo, e.size });
        });
    }

    // ---- helpers ----

    private static String kb(long bytes) { return (bytes / 1024L) + " KB"; }

    private <R> void bg(Callable<R> work, Consumer<R> ok) {
        new Thread(() -> {
            try {
                R res = work.call();
                SwingUtilities.invokeLater(() -> ok.accept(res));
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> {
                    String m = e instanceof IOException ? e.getMessage() : String.valueOf(e);
                    // sin zona de packs (estado < particiones) es un estado normal, no un pop-up
                    if (m != null && m.contains("sin zona de packs")) {
                        clearAll();
                        summaryLabel.setText("la placa no expone packs (configura las particiones primero)");
                        return;
                    }
                    log.accept("[Packs] error: " + m);
                    summaryLabel.setText("error");
                    JOptionPane.showMessageDialog(this, m, "Packs de la placa",
                            JOptionPane.ERROR_MESSAGE);
                });
            }
        }, "packs-wire").start();
    }
}
