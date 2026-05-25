/*
 * PicoExplorer.java — panel Swing que muestra los ficheros del firmware
 * bpvm-pico (Raspberry Pi Pico 2) y permite operar contra él: upload de
 * .mod desde el editor o desde disco, ejecutar, borrar, etc.
 *
 * Layout:
 *  ┌────────────────────────────────────────┐
 *  │ Toolbar (Connect | Refresh | Upload    │
 *  │   | Run | Delete | Save | Reset | LOG) │
 *  ├────────────────────────────────────────┤
 *  │ JTree con la jerarquía del dispositivo │
 *  │   📁 Pico                              │
 *  │     📁 app                             │
 *  │       📄 Hello.mod  (3519 bytes)       │
 *  │     📁 lib                             │
 *  │       📄 Math.mod   (2103 bytes)       │
 *  │       📄 Gpio.mod   (1844 bytes)       │
 *  │     📄 LegacyFile.mod  (1000 bytes)    │
 *  ├────────────────────────────────────────┤
 *  │ Status: COMxx  |  N ficheros, free=YYY │
 *  └────────────────────────────────────────┘
 *
 * El árbol se construye parseando los nombres devueltos por LS. El FS
 * subyacente del firmware es plano: los `/` son sólo namespace. Aquí
 * en el IDE los renderizamos como carpetas anidadas.
 *
 * Convención de paths al subir desde el IDE:
 *   - Si el nombre local no contiene `/`, se prefija con `/app/`.
 *   - Si ya viene con `/`, se respeta tal cual (permite avanzado).
 *
 * Threading: todas las operaciones del puerto serie se ejecutan en un
 * SwingWorker para no bloquear el EDT. La UI muestra "trabajando..." en
 * la status bar y reactiva los botones cuando termina.
 */
package com.mycompany.bpide;

import javax.swing.*;
import javax.swing.tree.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;

public final class PicoExplorer extends JPanel {

    /** Backend activo. Se construye en connect() según el radio
     *  Pico/VM-Java; null mientras no haya conexión. */
    private Backend backend;

    /** Sink al que mandar el output de RUN. Lo enchufa FrmMain a la consola. */
    private Consumer<String> outputSink;

    /* --- UI components ----------------------------------------- */
    private final JRadioButton rbSerial = new JRadioButton("Pico (serial)", true);
    private final JRadioButton rbVm     = new JRadioButton("VM Java (TCP)");
    private final JComboBox<String> portCombo = new JComboBox<>();
    private final JTextField endpointField = new JTextField("localhost:7332", 16);
    private final JPanel endpointPanel = new JPanel(new CardLayout());
    private static final String CARD_SERIAL = "SERIAL";
    private static final String CARD_VM     = "VM";
    private final JButton btnConnect = new JButton("Connect");
    private final JButton btnRefresh = new JButton("Refresh");
    private final JButton btnUpload  = new JButton("Upload…");
    private final JButton btnRun     = new JButton("Run");
    private final JButton btnGet     = new JButton("Download…");
    private final JButton btnDelete  = new JButton("Delete");
    private final JButton btnSave    = new JButton("Save");
    private final JButton btnLog     = new JButton("Log");
    private final JButton btnReset   = new JButton("Reset");

    /** Raíz del árbol. user object = String "Pico" en la raíz, String
     *  con el nombre del segmento en cada carpeta, RemoteFile en las
     *  hojas. */
    private final DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode("Pico");
    private final DefaultTreeModel treeModel = new DefaultTreeModel(rootNode);
    private final JTree fileTree = new JTree(treeModel);
    private final JLabel status = new JLabel("Disconnected");

    public PicoExplorer() {
        super(new BorderLayout());

        // Toolbar 3 filas: backend radio | endpoint (port/host) + connect |
        // acciones.
        JPanel toolbar = new JPanel(new GridLayout(3, 1));

        JPanel rowBackend = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        rowBackend.add(new JLabel("Backend:"));
        ButtonGroup bg = new ButtonGroup();
        bg.add(rbSerial);
        bg.add(rbVm);
        rowBackend.add(rbSerial);
        rowBackend.add(rbVm);

        // CardLayout intercambia portCombo / endpointField según el radio.
        JPanel serialCard = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        serialCard.add(new JLabel("Port:"));
        portCombo.setPreferredSize(new Dimension(120, 22));
        serialCard.add(portCombo);
        JPanel vmCard = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        vmCard.add(new JLabel("Endpoint:"));
        vmCard.add(endpointField);
        endpointPanel.add(serialCard, CARD_SERIAL);
        endpointPanel.add(vmCard,     CARD_VM);
        showCard(CARD_SERIAL);
        rbSerial.addActionListener(e -> showCard(CARD_SERIAL));
        rbVm.addActionListener(e     -> showCard(CARD_VM));

        JPanel row1 = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        row1.add(endpointPanel);
        row1.add(btnConnect);

        JPanel row2 = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        row2.add(btnRefresh);
        row2.add(btnUpload);
        row2.add(btnRun);
        row2.add(btnGet);
        row2.add(btnDelete);
        row2.add(btnSave);
        row2.add(btnLog);
        row2.add(btnReset);

        toolbar.add(rowBackend);
        toolbar.add(row1);
        toolbar.add(row2);
        add(toolbar, BorderLayout.NORTH);

        // Árbol de ficheros remotos con render personalizado.
        fileTree.setRootVisible(true);
        fileTree.setShowsRootHandles(true);
        fileTree.getSelectionModel().setSelectionMode(
                TreeSelectionModel.SINGLE_TREE_SELECTION);
        fileTree.setCellRenderer(new DefaultTreeCellRenderer() {
            @Override
            public Component getTreeCellRendererComponent(JTree tree,
                    Object value, boolean sel, boolean expanded, boolean leaf,
                    int row, boolean hasFocus) {
                super.getTreeCellRendererComponent(tree, value, sel, expanded,
                        leaf, row, hasFocus);
                if (value instanceof DefaultMutableTreeNode) {
                    Object uo = ((DefaultMutableTreeNode) value).getUserObject();
                    if (uo instanceof Backend.Entry) {
                        Backend.Entry f = (Backend.Entry) uo;
                        // Última componente del path como label.
                        String label = f.name;
                        int slash = label.lastIndexOf('/');
                        if (slash >= 0) label = label.substring(slash + 1);
                        setText(label + "  (" + f.size + " bytes)");
                        setIcon(UIManager.getIcon("FileView.fileIcon"));
                    } else {
                        // Carpeta (o raíz). Usa los iconos nativos.
                        Icon icon = expanded
                                ? UIManager.getIcon("FileView.directoryIcon")
                                : UIManager.getIcon("FileView.directoryIcon");
                        if (icon != null) setIcon(icon);
                    }
                }
                return this;
            }
        });
        fileTree.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2 && getSelectedEntry() != null) {
                    onRun();
                }
            }
        });
        add(new JScrollPane(fileTree), BorderLayout.CENTER);

        // Status bar.
        status.setBorder(BorderFactory.createEmptyBorder(2, 6, 2, 6));
        add(status, BorderLayout.SOUTH);

        // Refresca puertos al hacer click en el combo.
        portCombo.addPopupMenuListener(new javax.swing.event.PopupMenuListener() {
            @Override public void popupMenuWillBecomeVisible(
                    javax.swing.event.PopupMenuEvent e) {
                refreshPorts();
            }
            @Override public void popupMenuWillBecomeInvisible(
                    javax.swing.event.PopupMenuEvent e) {}
            @Override public void popupMenuCanceled(
                    javax.swing.event.PopupMenuEvent e) {}
        });

        btnConnect.addActionListener(e -> onConnect());
        btnRefresh.addActionListener(e -> onRefresh());
        btnUpload.addActionListener(e -> onUpload());
        btnRun.addActionListener(e -> onRun());
        btnGet.addActionListener(e -> onDownload());
        btnDelete.addActionListener(e -> onDelete());
        btnSave.addActionListener(e -> onSave());
        btnLog.addActionListener(e -> onLog());
        btnReset.addActionListener(e -> onReset());

        refreshPorts();
        setConnectedUI(false);
    }

    /** Llamado por FrmMain para enchufar la consola del IDE. */
    public void setOutputSink(Consumer<String> sink) { this.outputSink = sink; }

    /** ¿Hay conexión activa al dispositivo? */
    public boolean isConnected() { return backend != null && backend.isConnected(); }

    /** Helper UI: muestra el card correcto del CardLayout. */
    private void showCard(String name) {
        ((CardLayout) endpointPanel.getLayout()).show(endpointPanel, name);
    }

    /**
     * Pipeline "Run on Pico" desde el IDE: sube un .mod local al
     * dispositivo (sobreescribiendo si ya existía con ese nombre) y lo
     * ejecuta. El output stream va al sink configurado. No bloqueante.
     *
     * El nombre remoto se construye anteponiendo `/app/` si el nombre
     * local no tenía path, así los .mod de usuario quedan agrupados en
     * /app y se distinguen de la stdlib en /lib.
     */
    public void uploadAndRun(File modFile) {
        uploadAndRun(modFile, java.util.Collections.emptyList());
    }

    /** Variante deps-aware: sube primero los .mod de la lista (drivers
     *  de dispositivo) y luego el principal, y ejecuta el principal. */
    public void uploadAndRun(File modFile, java.util.List<File> depMods) {
        if (!isConnected()) {
            if (outputSink != null) outputSink.accept(
                    "[Explorer] no conectado — pulsa Connect primero");
            return;
        }
        if (modFile == null || !modFile.isFile()) {
            if (outputSink != null) outputSink.accept(
                    "[Explorer] fichero no existe: " + modFile);
            return;
        }
        final String remoteName = toAppPath(modFile.getName());
        final java.util.List<File> deps = (depMods != null)
                ? depMods : java.util.Collections.emptyList();
        final Backend b = this.backend;
        status.setText("Compile&Run: uploading "
                + (deps.size() + 1) + " file(s)...");
        if (outputSink != null) {
            outputSink.accept("[Explorer] subiendo " + remoteName + " (" + modFile.length() + " bytes)"
                    + (deps.isEmpty() ? "" : " + " + deps.size() + " dep(s)"));
        }
        runAsync(() -> {
            // 0) Sondea LS para saber qué hay ya en el FS remoto.
            java.util.Map<String, Long> remote = new java.util.HashMap<>();
            try {
                for (Backend.Entry rf : b.list()) {
                    remote.put(rf.name, rf.size);
                }
            } catch (java.io.IOException lsErr) {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] LIST falló, subiré todo: " + lsErr.getMessage()));
                }
            }

            // 1) Subir deps primero (drivers).
            //    skip-if-same-size + DEL-before-overwrite por economía
            //    (evita reescritura de flash innecesaria en Pico).
            for (File dep : deps) {
                String depRemote = toAppPath(dep.getName());
                Long sz = remote.get(depRemote);
                if (sz != null && sz == dep.length()) {
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] " + depRemote + " ya en FS (" + dep.length()
                                + " bytes), salto PUT"));
                    }
                    continue;
                }
                if (sz != null) {
                    try { b.del(depRemote); }
                    catch (java.io.IOException delErr) { /* tolerable */ }
                }
                byte[] depData = Files.readAllBytes(dep.toPath());
                b.put(depRemote, depData);
            }
            // 2) Subir el módulo principal solo si difiere.
            Long mainSz = remote.get(remoteName);
            if (mainSz != null && mainSz == modFile.length()) {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] " + remoteName + " ya en FS (" + modFile.length()
                            + " bytes), salto PUT"));
                }
            } else {
                if (mainSz != null) {
                    try { b.del(remoteName); }
                    catch (java.io.IOException delErr) { /* tolerable */ }
                }
                byte[] data = Files.readAllBytes(modFile.toPath());
                b.put(remoteName, data);
            }
            // 3) Ejecutar el principal.
            return b.run(remoteName, line -> {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(line));
                }
            });
        }, statusStr -> {
            if (outputSink != null) {
                outputSink.accept("[Explorer] VM finished: " + statusStr);
            }
            status.setText("Done: " + statusStr);
            onRefresh();
        });
    }

    /** Convierte un nombre local (e.g. "Hello.mod") en un path remoto.
     *  Si ya viene con `/`, se respeta. Si no, se prefija /app/. */
    private static String toAppPath(String localName) {
        if (localName.indexOf('/') >= 0) return localName;
        return "/app/" + localName;
    }

    /* ============================================================ */

    private void refreshPorts() {
        Object current = portCombo.getSelectedItem();
        portCombo.removeAllItems();
        for (String p : PicoClient.listPorts()) portCombo.addItem(p);
        if (current != null) portCombo.setSelectedItem(current);
        else {
            String auto = PicoClient.autoDetect();
            if (auto != null) portCombo.setSelectedItem(auto);
        }
    }

    private void setConnectedUI(boolean connected) {
        btnConnect.setText(connected ? "Disconnect" : "Connect");
        rbSerial.setEnabled(!connected);
        rbVm.setEnabled(!connected);
        portCombo.setEnabled(!connected);
        endpointField.setEnabled(!connected);
        btnRefresh.setEnabled(connected);
        btnUpload.setEnabled(connected);
        btnRun.setEnabled(connected);
        btnGet.setEnabled(connected);
        btnDelete.setEnabled(connected);
        btnSave.setEnabled(connected);
        btnLog.setEnabled(connected);
        btnReset.setEnabled(connected);
    }

    /* ============================================================ */

    private void onConnect() {
        if (isConnected()) {
            backend.close();
            backend = null;
            setConnectedUI(false);
            rootNode.removeAllChildren();
            treeModel.reload();
            status.setText("Disconnected");
            return;
        }
        final boolean serial = rbSerial.isSelected();
        final String endpoint = serial
                ? (String) portCombo.getSelectedItem()
                : endpointField.getText().trim();
        if (endpoint == null || endpoint.isEmpty()) {
            status.setText(serial ? "No port selected" : "Empty endpoint");
            return;
        }
        final Backend b = serial ? new SerialBackend() : new BpvmBackend();
        status.setText("Connecting to " + endpoint + " (" + b.displayName() + ")...");
        runAsync(() -> {
            String hello = b.connect(endpoint);
            return hello;
        }, hello -> {
            this.backend = b;
            setConnectedUI(true);
            status.setText(endpoint + " — " + hello);
            onRefresh();
        });
    }

    private void onRefresh() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        runAsync(() -> {
            List<Backend.Entry> fs = b.list();
            String mem;
            try { mem = b.mem(); }
            catch (java.io.IOException ie) { mem = "(no mem info)"; }
            return new Object[]{fs, mem};
        }, result -> {
            @SuppressWarnings("unchecked")
            List<Backend.Entry> fs = (List<Backend.Entry>) ((Object[]) result)[0];
            String mem = (String) ((Object[]) result)[1];
            rebuildTree(fs);
            status.setText(fs.size() + " files  |  " + mem);
        });
    }

    /** Reconstruye el árbol a partir de la lista plana del LS.
     *
     *  Estrategia: para cada RemoteFile, parsea su path en segmentos
     *  (/lib/Math.mod → ["lib", "Math.mod"]) y va navegando/creando
     *  nodos. La hoja lleva el RemoteFile como userObject.
     *
     *  Preserva las carpetas que estaban expandidas antes — la
     *  identificación se hace por el TreePath textual reconstruido. */
    private void rebuildTree(List<Backend.Entry> files) {
        // 1) Captura paths expandidos antes de tocar nada.
        Set<String> expandedPaths = new HashSet<>();
        Enumeration<TreePath> e = fileTree.getExpandedDescendants(
                new TreePath(rootNode));
        if (e != null) {
            while (e.hasMoreElements()) {
                expandedPaths.add(treePathToKey(e.nextElement()));
            }
        }

        // 2) Reconstruye desde cero.
        rootNode.removeAllChildren();

        // Orden estable: por path lex, para que el árbol salga
        // determinista entre listados.
        java.util.List<Backend.Entry> sorted = new java.util.ArrayList<>(files);
        sorted.sort((a, b) -> a.name.compareTo(b.name));

        for (Backend.Entry f : sorted) {
            insertFileIntoTree(f);
        }
        treeModel.reload();

        // 3) Restaura expand state, y como fallback expande root y
        //    primer nivel (/app, /lib, /sys, ...) para que el usuario
        //    vea algo útil sin clicks.
        fileTree.expandPath(new TreePath(rootNode));
        for (int i = 0; i < rootNode.getChildCount(); i++) {
            DefaultMutableTreeNode child =
                    (DefaultMutableTreeNode) rootNode.getChildAt(i);
            // Auto-expandir todas las carpetas (no son tantos ficheros).
            // El usuario puede colapsar lo que no quiera ver.
            expandAll(new TreePath(child.getPath()));
        }
        // Re-expandir lo que estaba abierto antes.
        for (String key : expandedPaths) {
            TreePath tp = keyToTreePath(key);
            if (tp != null) fileTree.expandPath(tp);
        }
    }

    private void expandAll(TreePath path) {
        DefaultMutableTreeNode node =
                (DefaultMutableTreeNode) path.getLastPathComponent();
        if (!node.isLeaf()) {
            fileTree.expandPath(path);
            for (int i = 0; i < node.getChildCount(); i++) {
                expandAll(path.pathByAddingChild(node.getChildAt(i)));
            }
        }
    }

    private String treePathToKey(TreePath tp) {
        StringBuilder sb = new StringBuilder();
        for (Object o : tp.getPath()) {
            sb.append('/');
            sb.append(((DefaultMutableTreeNode) o).getUserObject());
        }
        return sb.toString();
    }

    private TreePath keyToTreePath(String key) {
        // key es del estilo "/Pico/lib/Math.mod" — descender desde root.
        String[] segs = key.split("/");
        DefaultMutableTreeNode cur = rootNode;
        TreePath tp = new TreePath(cur);
        // segs[0] = "", segs[1] = "Pico" (root), segs[2..] = niños.
        for (int i = 2; i < segs.length; i++) {
            DefaultMutableTreeNode next = findChildByLabel(cur, segs[i]);
            if (next == null) return null;
            tp = tp.pathByAddingChild(next);
            cur = next;
        }
        return tp;
    }

    private DefaultMutableTreeNode findChildByLabel(
            DefaultMutableTreeNode parent, String label) {
        for (int i = 0; i < parent.getChildCount(); i++) {
            DefaultMutableTreeNode c =
                    (DefaultMutableTreeNode) parent.getChildAt(i);
            Object uo = c.getUserObject();
            String l;
            if (uo instanceof Backend.Entry) {
                String n = ((Backend.Entry) uo).name;
                int slash = n.lastIndexOf('/');
                l = slash >= 0 ? n.substring(slash + 1) : n;
            } else {
                l = String.valueOf(uo);
            }
            if (l.equals(label)) return c;
        }
        return null;
    }

    /** Inserta un fichero (path completo) en el árbol, creando carpetas
     *  intermedias según haga falta. */
    private void insertFileIntoTree(Backend.Entry f) {
        String name = f.name;
        // Si empieza con `/`, descártalo para que split no genere "".
        String body = name.startsWith("/") ? name.substring(1) : name;
        String[] parts = body.split("/");
        DefaultMutableTreeNode cur = rootNode;
        for (int i = 0; i < parts.length - 1; i++) {
            String seg = parts[i];
            DefaultMutableTreeNode child = findChildByLabel(cur, seg);
            if (child == null) {
                child = new DefaultMutableTreeNode(seg);
                cur.add(child);
            }
            cur = child;
        }
        // Hoja con la Entry completa.
        DefaultMutableTreeNode leaf = new DefaultMutableTreeNode(f);
        leaf.setAllowsChildren(false);
        cur.add(leaf);
    }

    /** Devuelve la Entry del nodo seleccionado, o null si no hay nada
     *  seleccionado o el nodo seleccionado es una carpeta. */
    private Backend.Entry getSelectedEntry() {
        TreePath sel = fileTree.getSelectionPath();
        if (sel == null) return null;
        Object last = sel.getLastPathComponent();
        if (!(last instanceof DefaultMutableTreeNode)) return null;
        Object uo = ((DefaultMutableTreeNode) last).getUserObject();
        if (uo instanceof Backend.Entry) {
            return (Backend.Entry) uo;
        }
        return null;
    }

    private void onUpload() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        JFileChooser fc = new JFileChooser();
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter(
                ".mod files", "mod"));
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File f = fc.getSelectedFile();
        if (f == null) return;
        // Sube a /app/<name> por convención (relevante en Pico; en VM
        // Java es un path arbitrario dentro del workdir).
        String remote = toAppPath(f.getName());
        status.setText("Uploading " + remote + "...");
        runAsync(() -> {
            byte[] data = Files.readAllBytes(f.toPath());
            b.put(remote, data);
            return data.length;
        }, n -> {
            status.setText("Uploaded " + remote + " (" + n + " bytes)");
            onRefresh();
        });
    }

    private void onRun() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        Backend.Entry sel = getSelectedEntry();
        if (sel == null) {
            status.setText("Select a file (leaf) to run");
            return;
        }
        status.setText("Running " + sel.name + "...");
        if (outputSink != null) outputSink.accept("--- RUN " + sel.name + " on " + b.displayName() + " ---");
        runAsync(() -> {
            return b.run(sel.name, line -> {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(line));
                }
            });
        }, statusStr -> {
            if (outputSink != null) {
                outputSink.accept("--- VM finished: " + statusStr + " ---");
            }
            status.setText("Done: " + statusStr);
        });
    }

    private void onDownload() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        Backend.Entry sel = getSelectedEntry();
        if (sel == null) return;
        JFileChooser fc = new JFileChooser();
        String basename = sel.name;
        int slash = basename.lastIndexOf('/');
        if (slash >= 0) basename = basename.substring(slash + 1);
        fc.setSelectedFile(new File(basename));
        if (fc.showSaveDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File out = fc.getSelectedFile();
        runAsync(() -> {
            byte[] data = b.get(sel.name);
            Files.write(out.toPath(), data);
            return data.length;
        }, n -> status.setText("Downloaded " + sel.name + " → "
                + out.getName() + " (" + n + " bytes)"));
    }

    private void onDelete() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        Backend.Entry sel = getSelectedEntry();
        if (sel == null) return;
        int rc = JOptionPane.showConfirmDialog(this,
                "Borrar " + sel.name + "?", "Confirmar",
                JOptionPane.YES_NO_OPTION);
        if (rc != JOptionPane.YES_OPTION) return;
        runAsync(() -> { b.del(sel.name); return null; },
                v -> { status.setText("Deleted " + sel.name); onRefresh(); });
    }

    private void onSave() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        status.setText("Saving FS...");
        runAsync(() -> { b.save(); return null; },
                v -> status.setText("FS saved (or no-op si VM Java)"));
    }

    private void onLog() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        runAsync(b::log, txt -> {
            JTextArea area = new JTextArea(txt);
            area.setEditable(false);
            area.setFont(new Font("Consolas", Font.PLAIN, 11));
            JScrollPane sp = new JScrollPane(area);
            sp.setPreferredSize(new Dimension(600, 400));
            JOptionPane.showMessageDialog(this, sp,
                    "Persistent log — " + b.displayName(), JOptionPane.PLAIN_MESSAGE);
        });
    }

    private void onReset() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        int rc = JOptionPane.showConfirmDialog(this,
                "Reiniciar el backend? La conexión se cerrará.",
                "Confirmar reset", JOptionPane.YES_NO_OPTION);
        if (rc != JOptionPane.YES_OPTION) return;
        runAsync(() -> { b.reset(); return null; },
                v -> {
                    backend.close();
                    backend = null;
                    rootNode.removeAllChildren();
                    treeModel.reload();
                    setConnectedUI(false);
                    status.setText("Reset sent, conexión cerrada");
                });
    }

    /* ============================================================
     * Helpers para ejecutar ops en background y resolver en EDT.
     * ============================================================ */

    private interface IOAction<T> { T run() throws IOException; }

    private <T> void runAsync(IOAction<T> task, Consumer<T> onSuccess) {
        setActionButtonsEnabled(false);
        new SwingWorker<T, Void>() {
            @Override protected T doInBackground() throws Exception {
                return task.run();
            }
            @Override protected void done() {
                try {
                    T result = get();
                    if (onSuccess != null) onSuccess.accept(result);
                } catch (Exception e) {
                    Throwable cause = e.getCause() != null ? e.getCause() : e;
                    status.setText("ERROR: " + cause.getMessage());
                    if (outputSink != null) {
                        outputSink.accept("[Pico ERROR] " + cause.getMessage());
                    }
                } finally {
                    setActionButtonsEnabled(true);
                }
            }
        }.execute();
    }

    private void setActionButtonsEnabled(boolean enabled) {
        boolean connected = isConnected();
        btnConnect.setEnabled(true);
        btnRefresh.setEnabled(enabled && connected);
        btnUpload.setEnabled(enabled && connected);
        btnRun.setEnabled(enabled && connected);
        btnGet.setEnabled(enabled && connected);
        btnDelete.setEnabled(enabled && connected);
        btnSave.setEnabled(enabled && connected);
        btnLog.setEnabled(enabled && connected);
        btnReset.setEnabled(enabled && connected);
    }
}
