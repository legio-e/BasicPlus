// ============================================================
// PacksPanel.java
// H3 — panel de gestión de PACKS de la placa (fase 1: LIST), hablando PACK_LS
// vía BpvmClient sobre la conexión YA abierta (compartida con la ventana
// principal). Autocontenido (Swing a mano, sin .form), como BoardMgrPanel.
//
// Orden de Eduardo (23-jul): 1º LIST (esto) → 2º ADD/BURN (copiar el .pack
// recién generado) → 3º REMOVE (tombstone; el hueco lo recupera una
// compactación desde el PC). Los botones de ADD/REMOVE llegarán con sus verbos.
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
import java.util.concurrent.Callable;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.SwingUtilities;
import javax.swing.table.DefaultTableModel;

public final class PacksPanel extends JPanel {

    private static final long T = 8000;    // timeout wire

    private BpvmClient client;
    private Consumer<String> log = s -> { };

    private final JLabel summaryLabel = new JLabel("(sin conexión)");
    private final DefaultTableModel model = new DefaultTableModel(
            new Object[] { "Pack", "Versión", "Tamaño (KB)", "Ficheros", "Estado", "CRC" }, 0) {
        @Override public boolean isCellEditable(int r, int c) { return false; }
    };
    private final JTable table = new JTable(model);

    public PacksPanel() {
        super(new BorderLayout(4, 4));
        setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        buildUi();
    }

    public void setLog(Consumer<String> l) { if (l != null) this.log = l; }

    public void attach(BpvmClient c) {
        this.client = c;
        if (c == null) { summaryLabel.setText("(sin conexión)"); model.setRowCount(0); }
        else refresh();
    }

    private void buildUi() {
        JPanel north = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 2));
        north.add(new JLabel("Packs de la placa:"));
        JButton refresh = new JButton("Refrescar");
        refresh.addActionListener(e -> refresh());
        north.add(refresh);
        add(north, BorderLayout.NORTH);

        JPanel center = new JPanel(new BorderLayout(4, 4));
        center.setBorder(BorderFactory.createTitledBorder("Packs (cadena completa: activos y borrados)"));
        center.add(new JScrollPane(table), BorderLayout.CENTER);
        add(center, BorderLayout.CENTER);

        add(summaryLabel, BorderLayout.SOUTH);
    }

    /** PACK_LS → repuebla la tabla. UNSUPPORTED (placa sin zona de packs aún) se
     *  muestra como estado, no como error a gritos. */
    public void refresh() {
        if (client == null) { summaryLabel.setText("(sin conexión)"); return; }
        summaryLabel.setText("consultando…");
        bg(() -> client.packList(T), this::fill);
    }

    private void fill(BpvmClient.PackTable t) {
        model.setRowCount(0);
        int alive = 0;
        for (BpvmClient.PackInfo p : t.packs) {
            model.addRow(new Object[] { p.name, p.version, p.size / 1024L, p.files,
                    p.active ? "activo" : "borrado", p.crcOk ? "OK" : "MAL" });
            if (p.active) alive++;
        }
        StringBuilder s = new StringBuilder();
        s.append(t.count).append(" packs (").append(alive).append(" activos) · libre ")
         .append(t.free / 1024L).append(" / ").append(t.regionSize / 1024L).append(" KB");
        if (!t.chainOk) s.append("  ⚠ CADENA CORRUPTA (compactar desde el PC)");
        if (t.count > t.packs.size()) s.append("  (listados ").append(t.packs.size()).append(")");
        summaryLabel.setText(s.toString());
    }

    // ---- helpers ----

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
                        model.setRowCount(0);
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
