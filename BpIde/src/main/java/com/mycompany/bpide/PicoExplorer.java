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

    /** H12 — consola: directorio actual + callback para limpiar la consola (cls). */
    private String consoleCwd = "/";
    private Runnable clearSink;

    /* --- UI components ----------------------------------------- */
    private final JRadioButton rbSerial = new JRadioButton("Pico (serial v1)", true);
    private final JRadioButton rbVm     = new JRadioButton("VM Java (TCP v1)");
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
    private final JButton btnEdit    = new JButton("Edit");
    private final JButton btnDelete  = new JButton("Delete");
    private final JButton btnSave    = new JButton("Save");
    private final JButton btnLog     = new JButton("Log");
    private final JButton btnLogClr  = new JButton("Clr Log");
    private final JButton btnReset   = new JButton("Reset");
    private final JButton btnInfo    = new JButton("Info");

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
        row2.add(btnEdit);
        row2.add(btnDelete);
        row2.add(btnSave);
        row2.add(btnLog);
        row2.add(btnLogClr);
        row2.add(btnReset);
        row2.add(btnInfo);

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
                if (e.getClickCount() == 2) {
                    Backend.Entry sel = getSelectedEntry();
                    if (sel == null) return;
                    // .mod = ejecutable → run (como siempre); cualquier
                    // otro fichero (config, log, .txt…) → ver/editar.
                    if (sel.name.toLowerCase().endsWith(".mod")) onRun();
                    else onEdit();
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
        btnEdit.addActionListener(e -> onEdit());
        btnDelete.addActionListener(e -> onDelete());
        btnSave.addActionListener(e -> onSave());
        btnLog.addActionListener(e -> onLog());
        btnLogClr.addActionListener(e -> onLogClear());
        btnReset.addActionListener(e -> onReset());
        btnInfo.addActionListener(e -> onInfo());

        refreshPorts();
        setConnectedUI(false);
    }

    /** Llamado por FrmMain para enchufar la consola del IDE. */
    public void setOutputSink(Consumer<String> sink) { this.outputSink = sink; }

    /** ¿Hay conexión activa al dispositivo? */
    public boolean isConnected() { return backend != null && backend.isConnected(); }

    /* ============================================================
     * H12 — Consola de comandos (línea de comandos estilo terminal).
     * La UI (JTextField + prompt) vive en FrmMain; aquí la lógica: parser +
     * dispatch sobre el backend conectado + estado de directorio (cwd). El
     * output va al outputSink (la consolaArea). Comandos:
     *   dir [ruta] · cd <ruta> · type <fich> · edit <fich> · run <fich> ·
     *   del <fich> · mem · save · log · reset · cls · help
     * (type=ver, edit=ver/editar en ventana; ambos sobre Backend.get/put.
     *  mkdir aún no está en la interfaz Backend.)
     * ============================================================ */

    /** FrmMain enchufa aquí el "limpiar consola" (comando cls). */
    public void setClearSink(Runnable r) { this.clearSink = r; }

    /** Prompt actual: ruta del device + "> ". */
    public String consolePrompt() { return consoleCwd + "> "; }

    /** Emite una línea a la consola de forma segura desde cualquier hilo. */
    private void emitLine(String s) {
        if (outputSink != null) {
            SwingUtilities.invokeLater(() -> outputSink.accept(s));
        }
    }

    /** Normaliza un path absoluto: colapsa // y resuelve . / .. */
    private static String normalizePath(String p) {
        java.util.Deque<String> st = new java.util.ArrayDeque<>();
        for (String seg : p.split("/")) {
            if (seg.isEmpty() || seg.equals(".")) continue;
            if (seg.equals("..")) { if (!st.isEmpty()) st.removeLast(); continue; }
            st.addLast(seg);
        }
        if (st.isEmpty()) return "/";
        StringBuilder sb = new StringBuilder();
        for (String seg : st) sb.append('/').append(seg);
        return sb.toString();
    }

    /** Resuelve un argumento de path contra el cwd (absoluto si empieza por /). */
    private String resolvePath(String arg) {
        if (arg == null || arg.isEmpty()) return consoleCwd;
        if (arg.startsWith("/")) return normalizePath(arg);
        String base = consoleCwd.endsWith("/") ? consoleCwd : consoleCwd + "/";
        return normalizePath(base + arg);
    }

    /** Ejecuta una línea de comandos. Eco + dispatch; las ops de I/O van a un
     *  hilo de fondo (no bloquear el EDT). cd/cls/help son síncronos. */
    public void executeConsoleCommand(String rawLine) {
        String line = (rawLine == null) ? "" : rawLine.trim();
        emitLine(consoleCwd + "> " + line);                 // eco estilo terminal
        if (line.isEmpty()) return;

        String[] parts = line.split("\\s+", 2);
        String cmd = parts[0].toLowerCase();
        String arg = (parts.length > 1) ? parts[1].trim() : "";

        switch (cmd) {
            case "help": case "?":
                emitLine("  comandos: dir [ruta] · cd <ruta> · type <fich> · edit <fich> · run <fich> · del <fich>");
                emitLine("            mem · save · log · reset · cls · help");
                emitLine("  type=volcar fichero a la consola · edit=ver/editar en ventana");
                emitLine("  (doble-clic en el árbol: .mod ejecuta, el resto abre el editor)");
                return;
            case "cls":
                if (clearSink != null) SwingUtilities.invokeLater(clearSink);
                return;
            case "cd":
                consoleCwd = arg.isEmpty() ? "/" : resolvePath(arg);
                return;                                     // el prompt lo refresca FrmMain
            default:
                break;
        }

        if (!isConnected()) { emitLine("  no conectado — pulsa Connect."); return; }

        final String fcmd = cmd, farg = arg, fcwd = consoleCwd;
        new Thread(() -> {
            try {
                switch (fcmd) {
                    case "dir": {
                        java.util.List<Backend.Entry> all = backend.list();
                        String base = fcwd.equals("/") ? "/" : fcwd + "/";
                        int n = 0;
                        for (Backend.Entry e : all) {
                            String nm = e.name.startsWith("/") ? e.name : "/" + e.name;
                            String rel;
                            if (fcwd.equals("/")) rel = nm.substring(1);
                            else if (nm.startsWith(base)) rel = nm.substring(base.length());
                            else continue;                  // fuera del cwd
                            emitLine(String.format("  %-32s %10d", rel + (e.isDir ? "/" : ""), e.size));
                            n++;
                        }
                        emitLine("  " + n + " entrada(s).");
                        break;
                    }
                    case "type": case "cat": {
                        if (farg.isEmpty()) { emitLine("  uso: type <fichero>"); break; }
                        String p = resolvePath(farg);
                        byte[] data = backend.get(p);
                        if (data.length > 32768) {
                            emitLine("  " + p + ": " + data.length + " bytes — demasiado "
                                    + "grande para 'type'. Usa 'edit' o Download.");
                            break;
                        }
                        String[] lns = new String(data,
                                java.nio.charset.StandardCharsets.UTF_8).split("\\R", -1);
                        int cap = Math.min(lns.length, 400);
                        for (int i = 0; i < cap; i++) emitLine("  " + lns[i]);
                        if (cap < lns.length) emitLine("  … (" + (lns.length - cap) + " líneas más)");
                        emitLine("  (" + data.length + " bytes)");
                        break;
                    }
                    case "edit": {
                        if (farg.isEmpty()) { emitLine("  uso: edit <fichero>"); break; }
                        String p = resolvePath(farg);
                        byte[] data = backend.get(p);
                        SwingUtilities.invokeLater(() -> {
                            Window owner = SwingUtilities.getWindowAncestor(this);
                            new DeviceFileEditor(owner, backend, p, data, this::onRefresh)
                                    .setVisible(true);
                        });
                        emitLine("  abriendo editor: " + p);
                        break;
                    }
                    case "del":
                        if (farg.isEmpty()) { emitLine("  uso: del <fichero>"); break; }
                        backend.del(resolvePath(farg));
                        emitLine("  borrado: " + resolvePath(farg));
                        SwingUtilities.invokeLater(this::onRefresh);   // el árbol refleja el borrado
                        break;
                    case "run": {
                        if (farg.isEmpty()) { emitLine("  uso: run <fichero>"); break; }
                        // conveniencia: 'run Blink' = 'run Blink.mod' (los ejecutables son .mod)
                        String mod = farg.endsWith(".mod") ? farg : farg + ".mod";
                        emitLine("  (" + backend.run(resolvePath(mod), this::emitLine) + ")");
                        SwingUtilities.invokeLater(this::onRefresh);   // run puede crear ficheros
                        break;
                    }
                    case "mem": case "df":
                        emitLine(backend.mem());
                        break;
                    case "save":
                        backend.save();
                        emitLine("  FS guardado en flash.");
                        break;
                    case "log":
                        emitLine(backend.log());
                        break;
                    case "reset":
                        backend.reset();
                        emitLine("  reset enviado.");
                        break;
                    default:
                        emitLine("  comando no reconocido: '" + fcmd + "' (prueba 'help').");
                }
            } catch (Exception ex) {
                emitLine("  error: " + ex.getMessage());
            }
        }, "bpconsole").start();
    }

    /** H6.b.3.b — el {@link BpvmClient} de la conexión activa (serie/TCP) para
     *  que "Debug on Pico" le enganche una DebugSession sobre el MISMO puerto
     *  (acceso único). null si el backend no es BpvmClient-based o sin conexión. */
    public BpvmClient debugClient() {
        return (backend instanceof AbstractBpvmBackend)
                ? ((AbstractBpvmBackend) backend).debugClient() : null;
    }

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

    /** Variante deps-aware (2-arg): todas las deps a /app. */
    public void uploadAndRun(File modFile, java.util.List<File> depMods) {
        uploadAndRun(modFile, depMods, java.util.Collections.emptySet());
    }

    /** Variante deps-aware: sube primero los .mod de la lista y luego el
     *  principal, y ejecuta el principal. `libDepNames` = basenames (.mod)
     *  que son stdlib core → van a /lib/ (y NO se pisan si ya están: la
     *  versión del firmware/FS es la garantizada-compatible). El resto va
     *  a /app/ con skip-if-same-size. */
    public void uploadAndRun(File modFile, java.util.List<File> depMods,
                             java.util.Set<String> libDepNames) {
        uploadAndRun(modFile, depMods, libDepNames, null);
    }

    /** H6.b.3.b — variante "Debug on Pico": si {@code debugHook} != null, tras
     *  subir los ficheros NO ejecuta en bloqueante; cede el {@link BpvmClient}
     *  ya conectado al hook (en EDT), que arranca la sesión de debug por
     *  eventos (PAUSE/RUN/STEP/locals). Con null = comportamiento normal (run). */
    public void uploadAndRun(File modFile, java.util.List<File> depMods,
                             java.util.Set<String> libDepNames,
                             java.util.function.Consumer<BpvmClient> debugHook) {
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
        final java.util.Set<String> libNames = (libDepNames != null)
                ? libDepNames : java.util.Collections.emptySet();
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
                // stdlib core → /lib (como en la Pico); driver/app → /app.
                boolean isLib = libNames.contains(dep.getName());
                String depRemote = isLib ? ("/lib/" + dep.getName())
                                         : toAppPath(dep.getName());
                Long sz = remote.get(depRemote);
                if (isLib && sz != null) {
                    // stdlib ya en /lib (embebida del firmware o subida
                    // antes) → NO la pisamos; es la compatible.
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] " + depRemote + " pre-instalada, salto PUT"));
                    }
                } else if (!isLib && sz != null && sz == dep.length()) {
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] " + depRemote + " ya en FS (" + dep.length()
                                + " bytes), salto PUT"));
                    }
                } else {
                    if (sz != null) {
                        try { b.del(depRemote); }
                        catch (java.io.IOException delErr) { /* tolerable */ }
                    }
                    byte[] depData = Files.readAllBytes(dep.toPath());
                    b.put(depRemote, depData);
                }
                // Si el dep tiene .mdn alongside, subirlo también.
                File depMdn = mdnSiblingOf(dep);
                if (depMdn != null && depMdn.isFile()) {
                    String depMdnRemote = toAppPath(depMdn.getName());
                    Long mz = remote.get(depMdnRemote);
                    if (mz != null && mz == depMdn.length()) {
                        // skip
                    } else {
                        if (mz != null) {
                            try { b.del(depMdnRemote); }
                            catch (java.io.IOException delErr) { /* tolerable */ }
                        }
                        byte[] mdnData = Files.readAllBytes(depMdn.toPath());
                        b.put(depMdnRemote, mdnData);
                        if (outputSink != null) {
                            SwingUtilities.invokeLater(() -> outputSink.accept(
                                    "[Explorer] subido AOT " + depMdnRemote + " ("
                                    + depMdn.length() + " bytes)"));
                        }
                    }
                }
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
            // 2b) Si hay .mdn alongside del .mod, subirlo también (H3 #158
            //     fase D). El firmware al hacer RUN escanea el FS por
            //     <mod>.mdn y registra los thunks AOT zero-copy. Si el
            //     .mdn no existe localmente, sin problema — BP corre
            //     interpretado normal.
            File mdnFile = mdnSiblingOf(modFile);
            if (mdnFile != null && mdnFile.isFile()) {
                String mdnRemote = toAppPath(mdnFile.getName());
                Long mdnSz = remote.get(mdnRemote);
                if (mdnSz != null && mdnSz == mdnFile.length()) {
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] " + mdnRemote + " ya en FS ("
                                + mdnFile.length() + " bytes), salto PUT"));
                    }
                } else {
                    if (mdnSz != null) {
                        try { b.del(mdnRemote); }
                        catch (java.io.IOException delErr) { /* tolerable */ }
                    }
                    byte[] mdnData = Files.readAllBytes(mdnFile.toPath());
                    b.put(mdnRemote, mdnData);
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] subido AOT " + mdnRemote + " ("
                                + mdnFile.length() + " bytes)"));
                    }
                }
            }
            // 3) Ejecutar el principal — salvo en modo DEBUG, donde cedemos el
            //    client ya conectado a la sesión de debug, que conduce
            //    PAUSE/RUN/STEP por eventos (no bloqueante).
            if (debugHook != null) {
                final BpvmClient dc = debugClient();
                SwingUtilities.invokeLater(() -> debugHook.accept(dc));
                return "(debug)";
            }
            return b.run(remoteName, line -> {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(line));
                }
            });
        }, statusStr -> {
            if ("(debug)".equals(statusStr)) {
                // Sesión de debug: el device queda PAUSADO esperando comandos de
                // debug. NO hacemos onRefresh() — haría un LIST y el pause_cb del
                // firmware respondería UNSUPPORTED ("no valido en pausa"). El árbol
                // de ficheros se refresca al detach/fin de la sesión.
                status.setText("Debug: sesión iniciada (device pausado)");
                return;
            }
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

    /** Para "Foo.mod" devuelve "Foo.mdn" en el mismo directorio (o null
     *  si el nombre no acaba en .mod). El fichero puede no existir —
     *  el caller chequea con .isFile(). H3 #158 fase D. */
    private static File mdnSiblingOf(File modFile) {
        if (modFile == null) return null;
        String name = modFile.getName();
        if (!name.toLowerCase().endsWith(".mod")) return null;
        String base = name.substring(0, name.length() - 4);
        return new File(modFile.getParentFile(), base + ".mdn");
    }

    /* ============================================================ */

    private void refreshPorts() {
        Object current = portCombo.getSelectedItem();
        portCombo.removeAllItems();
        for (String p : SerialPorts.listPorts()) portCombo.addItem(p);
        if (current != null) portCombo.setSelectedItem(current);
        else {
            String auto = SerialPorts.autoDetect();
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
        btnEdit.setEnabled(connected);
        btnDelete.setEnabled(connected);
        btnSave.setEnabled(connected);
        btnLog.setEnabled(connected);
        btnLogClr.setEnabled(connected);
        btnReset.setEnabled(connected);
        btnInfo.setEnabled(connected);
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
        // IDE-4 — arrancar el chooser en la última carpeta de upload usada.
        IdePrefs prefs = IdePrefs.load();
        JFileChooser fc = new JFileChooser();
        if (prefs.lastUploadDir != null) {
            File d = new File(prefs.lastUploadDir);
            if (d.isDirectory()) fc.setCurrentDirectory(d);
        }
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter(
                ".mod files", "mod"));
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File f = fc.getSelectedFile();
        if (f == null) return;
        // Recordar la carpeta para la próxima vez.
        File parent = f.getParentFile();
        if (parent != null) {
            prefs.lastUploadDir = parent.getAbsolutePath();
            prefs.save();
        }
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

    /** H12 / #231 — abre el fichero seleccionado en una ventana visor-editor.
     *  Lee el contenido con get() en hilo de fondo y, ya en el EDT, abre el
     *  DeviceFileEditor. Texto (config, log, .txt…) editable y reescribible
     *  con put(); binario (.mod…) en volcado hex de solo lectura. Al guardar
     *  refresca el árbol (el tamaño puede cambiar). */
    private void onEdit() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        Backend.Entry sel = getSelectedEntry();
        if (sel == null) {
            status.setText("Selecciona un fichero (hoja) para ver/editar");
            return;
        }
        final String path = sel.name;
        status.setText("Abriendo " + path + "...");
        runAsync(() -> b.get(path), data -> {
            Window owner = SwingUtilities.getWindowAncestor(this);
            new DeviceFileEditor(owner, b, path, data, this::onRefresh)
                    .setVisible(true);
            status.setText("Ver/editar " + path + " (" + data.length + " bytes)");
        });
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

    /** Borra el log persistente del backend (RAM + flash en el Pico).
     *  Útil cuando estamos bisecting instrumentación y queremos partir
     *  de un buffer limpio. */
    private void onLogClear() {
        if (!isConnected()) return;
        final Backend b = this.backend;
        runAsync(() -> { b.clearLog(); return null; },
                v -> status.setText("Log borrado en " + b.displayName()));
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

    /** H7 / #230 — botón INFO: micro, flash, RAM y PSRAM del dispositivo. */
    private void onInfo() {
        if (!isConnected()) return;
        final BpvmClient dc = debugClient();
        if (dc == null) {
            JOptionPane.showMessageDialog(this,
                    "INFO no disponible para este backend.",
                    "Device INFO", JOptionPane.WARNING_MESSAGE);
            return;
        }
        final Backend b = this.backend;
        runAsync(() -> dc.getInfo(4000), m -> {
            if (m == null) { status.setText("INFO sin respuesta"); return; }
            JTextArea area = new JTextArea(formatInfo(m));
            area.setEditable(false);
            area.setFont(new Font("Consolas", Font.PLAIN, 12));
            JOptionPane.showMessageDialog(this, new JScrollPane(area),
                    "Device INFO — " + b.displayName(), JOptionPane.PLAIN_MESSAGE);
        });
    }

    /** Formatea el Map del INFO_REPLY (H7: micro/flash/RAM/PSRAM). */
    private static String formatInfo(java.util.Map<String, Object> m) {
        StringBuilder sb = new StringBuilder();
        String variant = istr(m, "variant");
        sb.append("Micro       : ").append(istr(m, "boardName"));
        if (!variant.isEmpty()) sb.append("  (RP2350").append(variant).append(")");
        sb.append('\n');
        sb.append("Serial      : ").append(istr(m, "uniqueId")).append('\n');
        long hz = ilong(m, "cpuFreqHz");
        if (hz > 0) sb.append("CPU         : ").append(hz / 1_000_000L).append(" MHz\n");
        long mtc = ilong(m, "tempMilliC");
        if (mtc != 0) sb.append("Temp        : ")
                        .append(String.format(java.util.Locale.US, "%.1f", mtc / 1000.0))
                        .append(" °C\n");
        sb.append("GPIO        : ").append(ilong(m, "gpioCount")).append('\n');
        sb.append("PIO/PWM/ADC : ").append(ilong(m, "pioCount")).append(" / ")
          .append(ilong(m, "pwmSlices")).append(" / ").append(ilong(m, "adcChannels")).append('\n');
        sb.append("Flash       : ").append(human(ilong(m, "flashBytes"))).append('\n');
        sb.append("SRAM        : ").append(human(ilong(m, "sramBytes"))).append('\n');
        long ps = ilong(m, "psramBytes");
        sb.append("PSRAM       : ").append(ps > 0 ? human(ps) : "(ninguna)").append('\n');
        sb.append("FS          : ").append(human(ilong(m, "fsUsedBytes"))).append(" / ")
          .append(human(ilong(m, "fsTotalBytes"))).append('\n');
        long up = ilong(m, "uptimeMs");
        if (up > 0) sb.append("Uptime      : ").append(up / 1000L).append(" s\n");
        return sb.toString();
    }

    private static String istr(java.util.Map<String, Object> m, String k) {
        Object v = m.get(k);
        return v == null ? "" : v.toString();
    }
    private static long ilong(java.util.Map<String, Object> m, String k) {
        Object v = m.get(k);
        if (v instanceof Number) return ((Number) v).longValue();
        try { return v == null ? 0 : Long.parseLong(v.toString().trim()); }
        catch (NumberFormatException e) { return 0; }
    }
    private static String human(long bytes) {
        if (bytes <= 0) return "0";
        if (bytes >= 1024L * 1024L) return (bytes / (1024L * 1024L)) + " MB";
        if (bytes >= 1024L)         return (bytes / 1024L) + " KB";
        return bytes + " B";
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
        btnEdit.setEnabled(enabled && connected);
        btnDelete.setEnabled(enabled && connected);
        btnSave.setEnabled(enabled && connected);
        btnLog.setEnabled(enabled && connected);
        btnLogClr.setEnabled(enabled && connected);
        btnReset.setEnabled(enabled && connected);
        btnInfo.setEnabled(enabled && connected);
    }
}
