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

    /** V5/H8 — de dónde sale la arquitectura de la placa (la publica el INFO y
     *  la guarda el explorador al conectar). Se pide en vez de guardarla: entre
     *  que se conectó y que se graba puede haber cambiado de placa. */
    private java.util.function.IntSupplier archPlaca = () -> 0;
    public void setArchPlaca(java.util.function.IntSupplier s) {
        if (s != null) this.archPlaca = s;
    }

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
        JButton copy = new JButton("Copiar pack a la placa…");
        copy.addActionListener(e -> onBurn());
        north.add(copy);
        JButton del = new JButton("Borrar");
        del.addActionListener(e -> onDelete());
        north.add(del);
        JButton fmt = new JButton("Formatear zona…");
        fmt.addActionListener(e -> onFormat());
        north.add(fmt);
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
        // cadena corrupta con 0 packs = zona SIN ESTRENAR (restos del uso anterior
        // de esa flash: littlefs/FS viejo) → la salida es «Formatear zona…»
        if (!t.chainOk) s.append(t.count == 0
                ? "  ⚠ zona sin formatear (restos previos) → «Formatear zona…»"
                : "  ⚠ cadena corrupta → compactar desde el PC o «Formatear zona…»");
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

    /** "Copiar pack a la placa…": elige un .pack (por defecto el último dir) y
     *  lo graba por chunks (PACK_BURN_*). El device verifica en flash antes de
     *  activar; aquí solo se informa del resultado y se refresca la lista. */
    private void onBurn() {
        if (client == null) { summaryLabel.setText("(sin conexión)"); return; }
        javax.swing.JFileChooser fc = new javax.swing.JFileChooser(lastBurnDir);
        fc.setDialogTitle("Copiar pack a la placa");
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Packs (*.pack)", "pack"));
        if (fc.showOpenDialog(this) != javax.swing.JFileChooser.APPROVE_OPTION) return;
        final java.io.File f = fc.getSelectedFile();
        lastBurnDir = f.getParentFile();
        summaryLabel.setText("grabando " + f.getName() + "…");
        bg(() -> {
            byte[] img = java.nio.file.Files.readAllBytes(f.toPath());

            /* ── V5/H8: EL PACK SE PREPARA PARA ESTA PLACA ──────────────────
             *
             * Un pack universal trae el motor de VARIAS familias. Aquí se poda
             * a la de esta placa, y el motor que quede se relocaliza para la
             * direccion REAL — que no se sabe hasta que el device elige el
             * hueco, o sea DENTRO de packBurn, entre el BEGIN y los datos.
             *
             * Si el pack no lleva motor nativo esto no hace nada: `podar`
             * devuelve el mismo contenido y `sellar` lo pasa tal cual. */
            final basicplus.frontend.PackBurn.Preparado prep = prepararParaLaPlaca(img);
            byte[] aGrabar = (prep != null) ? prep.bytes : img;

            // El avance va a la MISMA etiqueta que ya dice "grabando…": es donde
            // el usuario está mirando. Y se pinta en el hilo de Swing, que es de
            // donde NO viene esta llamada (esto corre en el hilo de fondo).
            long off = client.packBurn(aGrabar, 30000,
                (flashAddr, ramBase, ramSize) -> {
                    if (prep == null || !prep.necesitaDireccion) return aGrabar;
                    int codigo = basicplus.frontend.PackBurn.baseDelCodigo(prep, (int) flashAddr);
                    log.accept(String.format(
                        "[Packs] pack en 0x%08X · motor en 0x%08X · RAM 0x%08X (%d KB)"
                        + " — relocalizando %d sitio(s)%n",
                        flashAddr, codigo, ramBase, ramSize / 1024, prep.relocalizaciones));
                    try {
                        return basicplus.frontend.PackBurn.sellar(prep, codigo, (int) ramBase);
                    } catch (basicplus.frontend.PackBurn.BurnException be) {
                        throw new java.io.IOException(be.getMessage(), be);
                    }
                },
                (hechos, total) -> {   // T holgado: borra+programa
                final int pct = total > 0 ? (int) ((hechos * 100L) / total) : 0;
                javax.swing.SwingUtilities.invokeLater(() -> summaryLabel.setText(
                        "grabando " + f.getName() + "… " + pct + " %"
                        + "  (" + (hechos / 1024) + "/" + (total / 1024) + " KB)"));
            });
            return new Object[] { img.length, off };
        }, r -> {
            log.accept("[Packs] grabado " + f.getName() + " (" + r[0] + " B) en offset " + r[1]);
            refresh();
        });
    }

    /**
     * V5/H8 — deja el pack con lo de ESTA placa: poda las familias que no son y
     * prepara el motor para sellarlo cuando se sepa la dirección.
     *
     * <p>Devuelve null si no hay que tocar nada (pack sin familias). Un fallo NO
     * se traga: grabar un pack a medias es peor que no grabarlo, y el mensaje
     * de `PackBurn` ya dice qué pasa y qué hacer.
     */
    private basicplus.frontend.PackBurn.Preparado prepararParaLaPlaca(byte[] img)
            throws java.io.IOException {
        /* La familia la dice la PLACA, igual que en la pasada AOT: un desplegable
         * del IDE podría estar puesto en otra cosa, y el motor equivocado no da
         * error — da un salto a un sitio que no es.
         *
         * Y si el explorador todavía no la tiene, SE LA PEDIMOS. El valor se
         * cachea al conectar mediante una petición ASÍNCRONA, así que puede no
         * haber llegado —o venir de otra placa anterior—. Depender de ese
         * cacheo es una carrera; preguntar aquí no lo es, y estamos en el hilo
         * de fondo con la conexión abierta, que es el sitio para hacerlo. */
        int a = archPlaca.getAsInt();
        if (a == 0 && client != null) {
            java.util.Map<String, Object> inf;
            try {
                inf = client.getInfo(T);
            } catch (java.io.IOException io) {
                /* NO es lo mismo «no lo tengo cacheado» que «la placa no
                 * contesta», y mandan a sitios distintos: reconectar el
                 * explorador no arregla un wire caído. Decir cuál es. */
                throw new java.io.IOException("la placa no responde al INFO ("
                    + io.getMessage() + "). Sin eso no se puede saber qué motor"
                    + " grabar. Resetea la placa y vuelve a conectar; comprueba"
                    + " con el botón INFO antes de grabar.", io);
            }
            Object v = (inf != null) ? inf.get("arch") : null;
            /* Número O cadena, igual que `PicoExplorer.ilong`: el wire manda
             * JSON y según el camino un entero puede llegar de las dos formas.
             * Aceptar sólo una lo dejaría a 0 EN SILENCIO. */
            if (v instanceof Number) a = ((Number) v).intValue();
            else if (v != null) {
                try { a = Integer.parseInt(v.toString().trim()); }
                catch (NumberFormatException ignore) { }
            }
            if (a != 0) log.accept("[Packs] la placa dice arch=" + a
                    + " (" + PicoExplorer.archName(a) + ")\n");
        }
        String arch = PicoExplorer.archName(a);
        basicplus.frontend.NpackReloc.Destino d =
                basicplus.frontend.NpackReloc.porTargetAot(arch);
        if (d == null)
            throw new java.io.IOException("no sé la arquitectura de la placa"
                + (arch.isEmpty() ? "" : " ('" + arch + "')")
                + ". Desconecta y vuelve a conectar el explorador: sin eso no se"
                + " puede elegir qué motor grabar.");
        try {
            basicplus.frontend.PackBurn.Preparado prep =
                    basicplus.frontend.PackBurn.podar(img, d);
            for (String s : prep.detalle) log.accept("[Packs] " + s + "\n");
            return prep;
        } catch (basicplus.frontend.PackBurn.BurnException be) {
            throw new java.io.IOException(be.getMessage(), be);
        }
    }

    /** "Borrar" = fase 3, tombstone del pack seleccionado: se MARCA borrado
     *  (no se borra); el espacio se recupera compactando desde el PC. */
    private void onDelete() {
        int row = packTable.getSelectedRow();
        if (row < 0 || row >= packOffsets.size() || client == null) {
            JOptionPane.showMessageDialog(this, "Selecciona un pack de la lista.",
                    "Packs de la placa", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        final String name = String.valueOf(packModel.getValueAt(row, 0));
        if ("borrado".equals(String.valueOf(packModel.getValueAt(row, 5)))) {
            JOptionPane.showMessageDialog(this, "'" + name + "' ya está borrado.",
                    "Packs de la placa", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        final long off = packOffsets.get(row);
        int ok = JOptionPane.showConfirmDialog(this,
                "¿Marcar '" + name + "' como borrado?\n"
                + "(No se borra de la flash: el espacio se recupera compactando desde el PC.)",
                "Borrar pack", JOptionPane.YES_NO_OPTION);
        if (ok != JOptionPane.YES_OPTION) return;
        bg(() -> { client.packDel(off, 30000); return null; }, r -> {
            log.accept("[Packs] '" + name + "' marcado como borrado (offset " + off + ")");
            refresh();
        });
    }

    /** "Formatear zona…": borra la zona de packs ENTERA. Es el estreno de una
     *  zona recién reparticionada (lleva restos de littlefs/FS viejo y el scan
     *  la ve corrupta, correctamente) o la recuperación de una cadena rota.
     *  Confirmación explícita SIEMPRE — borra todos los packs. */
    private void onFormat() {
        if (client == null) { summaryLabel.setText("(sin conexión)"); return; }
        int ok = JOptionPane.showConfirmDialog(this,
                "Esto BORRA TODOS los packs de la placa (la zona entera).\n"
                + "Es lo correcto para estrenar una zona recién particionada\n"
                + "(contiene restos de su uso anterior) o recuperar una cadena rota.\n\n"
                + "¿Formatear la zona de packs?",
                "Formatear zona de packs", JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE);
        if (ok != JOptionPane.YES_OPTION) return;
        summaryLabel.setText("formateando zona de packs…");
        bg(() -> { client.packFormat(30000); return null; }, r -> {
            log.accept("[Packs] zona de packs formateada");
            refresh();
        });
    }

    // ---- helpers ----

    private static java.io.File lastBurnDir = null;   // recordar carpeta entre usos

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
