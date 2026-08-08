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

    /** N-skip-put-by-hash: CRC32 de los bytes que subimos a cada ruta remota
     *  en ESTA conexión. El skip-if-same-size (#110) servía .mod rancios
     *  cuando una edición no cambiaba el tamaño; ahora el skip se confirma
     *  por contenido. Se vacía al (re)conectar. */
    private final java.util.Map<String,Long> sentCrc =
            new java.util.concurrent.ConcurrentHashMap<>();

    /** paso 4 cierre — CRC32 REAL de cada fichero del device, leído del LS al
     *  empezar cada subida (los firmwares nuevos lo reportan; los viejos no →
     *  el mapa no tiene la entrada y putIfChanged cae al fallback tamaño+sentCrc).
     *  Es la verdad-de-terreno: salta el PUT sólo si el contenido del device
     *  coincide con el local, sin depender de la memoria de sesión. */
    private volatile java.util.Map<String,Long> deviceCrcByPath =
            java.util.Collections.emptyMap();

    /** Sink al que mandar el output de RUN. Lo enchufa FrmMain a la consola. */
    private Consumer<String> outputSink;

    /** H12 — consola: directorio actual + callback para limpiar la consola (cls). */
    private String consoleCwd = "/";
    private Runnable clearSink;

    /* --- UI components ----------------------------------------- */
    private final JRadioButton rbSerial = new JRadioButton("Placa (serial v1)", true);
    private final JRadioButton rbVm     = new JRadioButton("VM Java (TCP v1)");
    /** H10 — el micro simulado: una placa que no existe, servida por bpvm-sim en
     *  este mismo PC. Habla el MISMO wire v1 por TCP, así que reutiliza tal cual
     *  el BpvmBackend; lo único propio es arrancar/parar el proceso. */
    private final JRadioButton rbSim    = new JRadioButton("Micro simulado (VM-C)");
    private final JComboBox<String> portCombo = new JComboBox<>();
    private final JTextField endpointField = new JTextField("localhost:7332", 16);
    private final JPanel endpointPanel = new JPanel(new CardLayout());
    private static final String CARD_SERIAL = "SERIAL";
    private static final String CARD_VM     = "VM";
    private static final String CARD_SIM    = "SIM";
    private final JLabel simState = new JLabel("parado");
    private final JButton btnSim    = new JButton("Arrancar");
    private final JButton btnSimCfg = new JButton();
    private final SimRunner sim = new SimRunner(this::emitLine);
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
    private final DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode("Placa");
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
        bg.add(rbSim);
        rowBackend.add(rbSerial);
        rowBackend.add(rbVm);
        rowBackend.add(rbSim);

        // CardLayout intercambia portCombo / endpointField / estado del sim según el radio.
        JPanel serialCard = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        serialCard.add(new JLabel("Port:"));
        portCombo.setPreferredSize(new Dimension(120, 22));
        serialCard.add(portCombo);
        JPanel vmCard = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        vmCard.add(new JLabel("Endpoint:"));
        vmCard.add(endpointField);
        // H10 — el micro simulado no pide endpoint: lo arranca el IDE y sabe dónde
        // escucha. Lo que va aquí es su ESTADO y los dos botones (marcha/paro +
        // engranaje de configuración).
        JPanel simCard = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        simCard.add(new JLabel("Simulador:"));
        simCard.add(simState);
        simCard.add(btnSim);
        simCard.add(btnSimCfg);
        endpointPanel.add(serialCard, CARD_SERIAL);
        endpointPanel.add(vmCard,     CARD_VM);
        endpointPanel.add(simCard,    CARD_SIM);
        showCard(CARD_SERIAL);
        rbSerial.addActionListener(e -> showCard(CARD_SERIAL));
        rbVm.addActionListener(e     -> showCard(CARD_VM));
        rbSim.addActionListener(e    -> showCard(CARD_SIM));

        btnSim.setIcon(SimIcons.chip(14));
        btnSim.setToolTipText("Arranca el micro simulado y se conecta a él");
        btnSimCfg.setIcon(SimIcons.gear(14));
        btnSimCfg.setToolTipText("Configurar el micro simulado (RAM, flash, pantalla…)");
        btnSimCfg.setMargin(new Insets(2, 4, 2, 4));
        btnSim.addActionListener(e -> onToggleSim());
        btnSimCfg.addActionListener(e -> onSimConfig());
        /* Si el IDE se cierra con el simulador en marcha, el proceso hijo se
         * quedaría huérfano ocupando el puerto → al siguiente arranque "no
         * aceptó conexiones" sin motivo aparente. */
        Runtime.getRuntime().addShutdownHook(new Thread(sim::stop, "bpvm-sim-stop"));

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

    /** H9 — FrmMain engancha aquí el "escrutinio de primera conexión": se invoca
     *  tras conectar con éxito, con el {@link BpvmClient} compartido (null si el
     *  backend no es v1). FrmMain decide si la placa es virgen y ofrece abrir
     *  FrmBoard. */
    private Consumer<BpvmClient> connectedSink;
    public void setConnectedSink(Consumer<BpvmClient> s) { this.connectedSink = s; }

    /**
     * H11 — ARQUITECTURA del código nativo de la placa conectada (e_machine:
     * 40=ARM, 243=RISC-V; 0 = desconocida o firmware que no la publica).
     *
     * Se pide UNA vez al conectar y se guarda, porque quien la necesita es la
     * compilación AOT, que ocurre ANTES de subir nada: si sólo se preguntara al
     * pulsar el botón INFO, al compilar no la tendríamos. Y la fuente de verdad
     * es la PLACA, no un ajuste tecleado — con un `.bp` suelto ni siquiera hay
     * proyecto donde apuntarla.
     */
    private volatile int deviceArch = 0;
    public int deviceArch() { return deviceArch; }

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
                emitLine("  comandos: dir [ruta] · cd <ruta> · type <fich> · edit <fich> · new <fich> · run <fich> · del <fich>");
                emitLine("            kill · autorun [fich|off] · mem · save · log · reset · cls · help");
                emitLine("  type=volcar fichero a la consola · edit=ver/editar en ventana · new=crear fichero nuevo");
                emitLine("  kill=aborta el programa en ejecución (también menú Run → Stop, Ctrl+F2)");
                emitLine("  autorun=app que arranca al boot (/sys/auto.txt); con la app corriendo");
                emitLine("          el IDE puede conectar y pararla con kill");
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
                    case "new": {
                        if (farg.isEmpty()) { emitLine("  uso: new <fichero>"); break; }
                        String p = resolvePath(farg);
                        // No pisar un fichero existente: para eso está 'edit'. El
                        // primitivo de crear ya existe (DeviceFileEditor.doSave →
                        // backend.put crea rutas nuevas); aquí solo lo exponemos.
                        boolean exists = false;
                        for (Backend.Entry e : backend.list()) {
                            String nm = e.name.startsWith("/") ? e.name : "/" + e.name;
                            if (nm.equals(p)) { exists = true; break; }
                        }
                        if (exists) {
                            emitLine("  ya existe: " + p + " — usa 'edit' para modificarlo");
                            break;
                        }
                        SwingUtilities.invokeLater(() -> {
                            Window owner = SwingUtilities.getWindowAncestor(this);
                            new DeviceFileEditor(owner, backend, p, new byte[0], this::onRefresh)
                                    .setVisible(true);
                        });
                        emitLine("  fichero nuevo (se crea al Guardar): " + p);
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
                    case "kill": case "stop":
                        // P-run-stop (#257) — el run en curso (otro hilo
                        // bpconsole o el pipeline Run on Device) desbloquea
                        // al llegar su EXITED con status KILLED.
                        killRunning();
                        break;
                    case "sd": {
                        // V5/H1 — ¿contesta la tarjeta SD, y quién es? Esto NO
                        // monta un sistema de ficheros: es el primer peldaño de
                        // la cadena, el que dice si el bus habla. Si esto no
                        // sale, depurar FAT encima sería depurar sobre nada.
                        //
                        // Se manda a mano y no al conectar porque habla con
                        // hardware que puede no estar: la negociación va a
                        // 400 kHz por exigencia del estándar y el arranque de
                        // la tarjeta puede llevarse hasta un segundo.
                        BpvmClient dc = debugClient();
                        if (dc == null) {
                            emitLine("  sd: este backend no habla con la placa");
                            break;
                        }
                        java.util.Map<String, Object> m = dc.getSdInfo(6000);
                        if (m == null) { emitLine("  sd: sin respuesta"); break; }
                        boolean ok = "true".equalsIgnoreCase(istr(m, "ok"));
                        if (!ok) {
                            // El peldaño es el dato: dice DÓNDE se paró.
                            emitLine("  sd: NO  — " + istr(m, "motivo"));
                            String pel = istr(m, "peldano");
                            if (!pel.isEmpty()) emitLine("      (peldano " + pel + ")");
                            // La traza es el instrumento: qué mandó la tarjeta
                            // DE VERDAD. 00 repetido = linea flotante (no es la
                            // tarjeta hablando); FF = silencio; otra cosa = si
                            // habla y hay que leer su R1.
                            String tz = istr(m, "traza");
                            if (!tz.isEmpty()) {
                                emitLine("      CMD" + istr(m, "ultimoCmd") + " -> leido: " + tz);
                            }
                            break;
                        }
                        long bloques = ilong(m, "bloques");
                        emitLine("  sd: OK");
                        emitLine("      tarjeta   : " + istr(m, "producto")
                                 + " (OEM " + istr(m, "oem")
                                 + ", fabricante 0x" + Long.toHexString(ilong(m, "fabricante")) + ")");
                        emitLine("      tipo      : " + (ilong(m, "version") == 2
                                 ? "SDHC/SDXC (CSD v2)" : "SDSC (CSD v1)")
                                 + ("true".equalsIgnoreCase(istr(m, "altaCap"))
                                    ? ", direcciona por BLOQUE" : ", direcciona por BYTE"));
                        // Los bloques son de 512 B SIEMPRE en esta capa (lo que
                        // varia es como se DIRECCIONAN, no su tamano).
                        emitLine("      capacidad : " + bloques + " bloques de 512 B  ("
                                 + human(bloques * 512L) + ")");
                        emitLine("      fabricada : " + istr(m, "mes") + "/" + istr(m, "anno")
                                 + "   serie " + istr(m, "serie"));
                        // El sector 0: que se identifique prueba que responde a
                        // COMANDOS; esto prueba que entrega DATOS, que es otro
                        // camino. 55 AA es la respuesta conocida.
                        if ("true".equalsIgnoreCase(istr(m, "leeSector0"))) {
                            String f = istr(m, "firma");
                            emitLine("      sector 0  : firma " + f
                                     + ("55 AA".equals(f) ? "  ✔ es un sector de arranque"
                                                          : "  (no es 55 AA — sin formatear?)"));
                            // MBR o VBR: decide DONDE empieza el sistema de
                            // ficheros, que es lo primero que necesita H2.
                            if ("MBR".equals(istr(m, "clase"))) {
                                long tipo = ilong(m, "parteTipo");
                                String qué = tipo == 0x0B || tipo == 0x0C ? "FAT32"
                                           : tipo == 0x07 ? "exFAT/NTFS"
                                           : tipo == 0x0E || tipo == 0x06 ? "FAT16"
                                           : "desconocido";
                                emitLine("      sector 0  : es una TABLA DE PARTICIONES (MBR), "
                                         + ilong(m, "particiones") + " particion(es)");
                                emitLine("      particion : tipo 0x"
                                         + Long.toHexString(tipo).toUpperCase() + " (" + qué + ")"
                                         + "  empieza en el bloque " + ilong(m, "parteLba")
                                         + ", " + human(ilong(m, "parteSectores") * 512L));
                            }
                            emitLine("      formato   : \"" + istr(m, "oemFs") + "\""
                                     + "   (quien la formateo)");
                        } else {
                            emitLine("      sector 0  : NO se pudo leer — " + istr(m, "motivoSector0"));
                        }
                        break;
                    }
                    case "autorun": {
                        // P-autorun (#256) — gestiona /sys/auto.txt del device.
                        //   autorun           → muestra el actual
                        //   autorun <fich>    → lo establece (conveniencia .mod)
                        //   autorun off       → lo borra
                        if (farg.isEmpty()) {
                            try {
                                String cur = new String(backend.get("/sys/auto.txt"),
                                        java.nio.charset.StandardCharsets.UTF_8).trim();
                                emitLine("  autorun actual: " + cur);
                            } catch (Exception nf) {
                                emitLine("  no hay autorun (no existe /sys/auto.txt)");
                            }
                            break;
                        }
                        if (farg.equalsIgnoreCase("off")) {
                            backend.del("/sys/auto.txt");
                            // Persistir el borrado: sin SAVE, la copia en flash
                            // de la Pico resucitaría el autorun al siguiente boot.
                            try { backend.save(); } catch (Exception ignored) { }
                            emitLine("  autorun desactivado (/sys/auto.txt borrado)");
                            SwingUtilities.invokeLater(this::onRefresh);
                            break;
                        }
                        String mod = farg.endsWith(".mod") ? farg : farg + ".mod";
                        String p = resolvePath(mod);
                        backend.put("/sys/auto.txt",
                                (p + "\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
                        emitLine("  autorun establecido: " + p);
                        // En la Pico el PUT va al FS RAM: sin SAVE, el reset
                        // se lo come. Persistimos aquí mismo (best effort —
                        // VM Java y ESP32/STM32 persisten solos en el PUT).
                        try { backend.save(); emitLine("  FS guardado en flash."); }
                        catch (Exception ignored) { }
                        emitLine("  (arrancará al reiniciar la placa — prueba con 'reset';");
                        emitLine("   para pararlo: 'kill'; para quitarlo: 'autorun off')");
                        SwingUtilities.invokeLater(this::onRefresh);
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
    /** P-run-stop (#257) — aborta el programa que corre en la placa: manda
     *  KILL por la conexión activa. El uploadAndRun en curso desbloquea al
     *  llegar el EXITED (status KILLED). Sin conexión, solo avisa. */
    public void killRunning() {
        Backend b = this.backend;
        if (b == null || !b.isConnected()) {
            emitLine("[Explorer] Stop: no hay placa conectada");
            return;
        }
        try {
            // #256 — escenario "attach" (programa arrancado por autorun u
            // otra sesión): ningún run() local va a consumir el EXITED del
            // kill, así que sin esto el éxito y el fracaso se ven IGUAL
            // (consola muda). Listener efímero que pinta el cierre y
            // recarga el árbol (la placa vuelve a atender FILES: el
            // connect-time refresh había chocado con el BUSY).
            if (b instanceof AbstractBpvmBackend
                    && !((AbstractBpvmBackend) b).isRunActive()) {
                ((AbstractBpvmBackend) b).onNextExited(line -> {
                    emitLine(line);
                    SwingUtilities.invokeLater(this::onRefresh);
                });
            }
            b.kill();
            emitLine("[Explorer] Stop: KILL enviado a la placa");
        } catch (Exception ex) {
            emitLine("[Explorer] Stop falló: " + ex.getMessage());
        }
    }

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

    /** H6.b.3.b — variante "Debug on Device": si {@code debugHook} != null,
     *  tras subir los ficheros NO ejecuta en bloqueante; cede el
     *  {@link BpvmClient} ya conectado al hook (en EDT), que arranca la
     *  sesión de debug por eventos. Con null = comportamiento normal (run). */
    public void uploadAndRun(File modFile, java.util.List<File> depMods,
                             java.util.Set<String> libDepNames,
                             java.util.function.Consumer<BpvmClient> debugHook) {
        uploadAndRun(modFile, depMods, libDepNames, debugHook, null);
    }

    /** CRC32 de un buffer, como long (para confirmar contenido idéntico
     *  antes de saltarse un PUT). */
    private static long crc32(byte[] data) {
        java.util.zip.CRC32 c = new java.util.zip.CRC32();
        c.update(data);
        return c.getValue();
    }

    /** Sube `file` a `remote` salvo que YA esté allí con el MISMO contenido.
     *  paso 4 cierre — verdad-de-terreno: si el firmware reportó el CRC del
     *  fichero en el LS (deviceCrcByPath), saltamos el PUT sólo si coincide con
     *  el CRC local → detecta rancios de CUALQUIER fuente, no solo de lo que
     *  subió esta sesión. Sin crc del device (firmware viejo) cae al heurístico
     *  anterior (#110/#111: tamaño + sentCrc). DEL-before-PUT se conserva (#111).
     *  @return true si (re)subió; false si lo saltó. */
    private boolean putIfChanged(Backend b, File file, String remote,
                                 Long deviceSize) throws java.io.IOException {
        if (remote.length() > 39 && outputSink != null) {   // F2: FS_NAME_LEN=40 (39 chars + NUL)
            final String r = remote;
            SwingUtilities.invokeLater(() -> outputSink.accept(
                    "[Explorer] AVISO: ruta " + r.length() + " chars > 39 (FS_NAME_LEN) — "
                    + "el device puede truncarla/rechazarla: " + r));
        }
        byte[] data = Files.readAllBytes(file.toPath());
        long crc = crc32(data);
        Long devCrc = deviceCrcByPath.get(remote);
        if (devCrc != null) {
            if (devCrc.longValue() == crc) return false;   // contenido REAL del device == local
        } else {
            // Fallback (firmware sin crc en el LS): tamaño del device + CRC de
            // lo último que mandó ESTA sesión.
            Long sent = sentCrc.get(remote);
            if (deviceSize != null && deviceSize.longValue() == data.length
                    && sent != null && sent.longValue() == crc) {
                return false;
            }
        }
        if (deviceSize != null) {
            try { b.del(remote); }
            catch (java.io.IOException delErr) { /* tolerable */ }
        }
        b.put(remote, data);
        sentCrc.put(remote, crc);
        return true;
    }

    /** H12 (#260) — variante con resources: pares (ruta remota → fichero
     *  local) de la carpeta resources/ del proyecto, de cualquier tipo. Se
     *  suben con skip-if-idéntico (tamaño + CRC32), como las deps.
     *  Delegado: prefijo de APP por defecto "/app" (modo fichero-suelto). */
    public void uploadAndRun(File modFile, java.util.List<File> depMods,
                             java.util.Set<String> libDepNames,
                             java.util.function.Consumer<BpvmClient> debugHook,
                             java.util.Map<String, File> resourceFiles) {
        uploadAndRun(modFile, depMods, libDepNames, debugHook, resourceFiles, "/app");
    }

    /** H19-F2 — variante con prefijo de proyecto: {@code appPrefix} es la
     *  carpeta del proyecto en el device ("/app/&lt;proj&gt;") o "/app" en
     *  fichero-suelto. Los ficheros de APP (entry .mod/.mdn, deps no-lib + su
     *  .mdn, resources) aterrizan bajo ese prefijo; las libs (core stdlib +
     *  Gui) siguen en /lib. En modo proyecto, al final borra de
     *  {@code appPrefix}/ las claves del device que ya no despliega (huérfanos
     *  = LS bajo el prefijo − lo desplegado este run). */
    public void uploadAndRun(File modFile, java.util.List<File> depMods,
                             java.util.Set<String> libDepNames,
                             java.util.function.Consumer<BpvmClient> debugHook,
                             java.util.Map<String, File> resourceFiles,
                             String appPrefix) {
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
        final String prefix = (appPrefix == null || appPrefix.isEmpty()) ? "/app" : appPrefix;
        final boolean projectMode = !"/app".equals(prefix);   // F2: huérfanos sólo con proyecto
        final String remoteName = appPath(prefix, modFile.getName());
        final java.util.List<File> deps = (depMods != null)
                ? depMods : java.util.Collections.emptyList();
        final java.util.Set<String> libNames = (libDepNames != null)
                ? libDepNames : java.util.Collections.emptySet();
        final java.util.Map<String, File> resources = (resourceFiles != null)
                ? resourceFiles : java.util.Collections.emptyMap();
        final Backend b = this.backend;
        status.setText("Compile&Run: uploading "
                + (deps.size() + 1) + " file(s)...");
        if (outputSink != null) {
            outputSink.accept("[Explorer] subiendo " + remoteName + " (" + modFile.length() + " bytes)"
                    + (deps.isEmpty() ? "" : " + " + deps.size() + " dep(s)"));
        }
        runAsync(() -> {
            // 0) Sondea LS para saber qué hay ya en el FS remoto (tamaño + CRC real).
            java.util.Map<String, Long> remote = new java.util.HashMap<>();
            java.util.Map<String, Long> remoteCrc = new java.util.HashMap<>();
            try {
                for (Backend.Entry rf : b.list()) {
                    remote.put(rf.name, rf.size);
                    if (rf.crc >= 0) remoteCrc.put(rf.name, rf.crc);   // paso 4: -1 = firmware sin crc
                }
            } catch (java.io.IOException lsErr) {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] LIST falló, subiré todo: " + lsErr.getMessage()));
                }
            }
            this.deviceCrcByPath = remoteCrc;   // putIfChanged lo consulta por ruta

            // F2 (H19) — claves bajo el prefijo del proyecto que ESTE run deja
            // presentes (subidas O saltadas por CRC). Al final borramos del
            // device las claves bajo prefix/ que NO estén aquí (huérfanos).
            final java.util.Set<String> deployed = new java.util.HashSet<>();
            deployed.add(remoteName);

            // 1) Subir deps primero (drivers).
            //    skip-if-IDÉNTICO (tamaño + CRC32, vía putIfChanged) +
            //    DEL-before-overwrite por economía (evita reescritura de
            //    flash innecesaria en Pico).
            for (File dep : deps) {
                // stdlib core → /lib (como en la Pico); driver/app → /app.
                boolean isLib = libNames.contains(dep.getName());
                String depRemote = isLib ? ("/lib/" + dep.getName())
                                         : appPath(prefix, dep.getName());
                if (!isLib) deployed.add(depRemote);   // F2: bajo el prefijo del proyecto
                Long sz = remote.get(depRemote);
                // ANTES: para stdlib en /lib confiábamos en la copia
                // "pre-instalada" (embebida del firmware) y saltábamos el PUT.
                // PELIGRO (bug cazado 2026-06-13): si el blob embebido viene de
                // un frontend ANTERIOR, su layout de vtable de clase no casa con
                // la app recién compilada → `INVOKE_VIRTUAL slot N no resoluble`
                // → RuntimeError en CUALQUIER método OO de stdlib (I2c.Bus,
                // Spi.Bus, Uart.Port, Rtc.Clock, ...). Top-level se salvaba
                // (resuelve por índice de función), por eso el bug era sutil.
                // AHORA: /lib pasa por el MISMO content-check (CRC32) que la app,
                // de modo que la copia en FS siempre case con lo que compiló la
                // app, auto-curando blobs embebidos rancios sin reflashear.
                boolean up = putIfChanged(b, dep, depRemote, sz);
                if (!up && outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] " + depRemote + " ya en FS (" + dep.length()
                            + " bytes, contenido idéntico), salto PUT"));
                }
                // Si el dep tiene .mdn alongside, subirlo también.
                final File depMdn = mdnSiblingOf(dep);
                boolean depMdnStale = depMdn != null && depMdn.isFile() && mdnIsStale(depMdn, dep);
                if (depMdnStale && outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] AOT " + depMdn.getName() + " es MÁS VIEJO que su .mod"
                            + " — NO se sube (regenéralo activando AOT en el proyecto)"));
                }
                if (depMdn != null && depMdn.isFile() && !depMdnStale) {
                    String depMdnRemote = isLib ? toAppPath(depMdn.getName())
                                                : appPath(prefix, depMdn.getName());
                    if (!isLib) deployed.add(depMdnRemote);   // F2: junto a su .mod
                    boolean upMdn = putIfChanged(b, depMdn, depMdnRemote, remote.get(depMdnRemote));
                    if (upMdn && outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] subido AOT " + depMdnRemote + " ("
                                + depMdn.length() + " bytes)"));
                    }
                }
            }
            // 1c) H12 (#260) — resources/ del proyecto: ficheros arbitrarios a
            //     su ruta remota (/app/<rel>), con skip-if-idéntico.
            for (java.util.Map.Entry<String, File> res : resources.entrySet()) {
                final String rRemote = res.getKey();
                deployed.add(rRemote);   // F2: ya viene con el prefijo del proyecto
                final File rFile = res.getValue();
                boolean up = putIfChanged(b, rFile, rRemote, remote.get(rRemote));
                if (outputSink != null) {
                    if (up) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] subido resource " + rRemote + " ("
                                + rFile.length() + " bytes)"));
                    } else {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Explorer] " + rRemote + " ya en FS (" + rFile.length()
                                + " bytes, contenido idéntico), salto PUT"));
                    }
                }
            }
            // 2) Subir el módulo principal solo si su CONTENIDO difiere.
            boolean upMain = putIfChanged(b, modFile, remoteName, remote.get(remoteName));
            if (!upMain && outputSink != null) {
                SwingUtilities.invokeLater(() -> outputSink.accept(
                        "[Explorer] " + remoteName + " ya en FS (" + modFile.length()
                        + " bytes, contenido idéntico), salto PUT"));
            }
            // 2b) Si hay .mdn alongside del .mod, subirlo también (H3 #158
            //     fase D). El firmware al hacer RUN escanea el FS por
            //     <mod>.mdn y registra los thunks AOT zero-copy. Si el
            //     .mdn no existe localmente, sin problema — BP corre
            //     interpretado normal.
            final File mdnFile = mdnSiblingOf(modFile);
            boolean mdnStale = mdnFile != null && mdnFile.isFile() && mdnIsStale(mdnFile, modFile);
            if (mdnStale && outputSink != null) {
                SwingUtilities.invokeLater(() -> outputSink.accept(
                        "[Explorer] AOT " + mdnFile.getName() + " es MÁS VIEJO que su .mod"
                        + " — NO se sube (regenéralo activando AOT en el proyecto)"));
            }
            if (mdnFile != null && mdnFile.isFile() && !mdnStale) {
                String mdnRemote = appPath(prefix, mdnFile.getName());
                deployed.add(mdnRemote);   // F2
                boolean up = putIfChanged(b, mdnFile, mdnRemote, remote.get(mdnRemote));
                if (up && outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Explorer] subido AOT " + mdnRemote + " ("
                            + mdnFile.length() + " bytes)"));
                }
            }
            // 2b-bis) F2 (H19) — huérfanos: en modo proyecto, borra de prefix/
            //   las claves que el device tiene pero este run ya no despliega
            //   (módulos/resources quitados del proyecto). El prefijo ES el
            //   manifest (FS plano enumerado por LS); nunca toca /lib ni otros
            //   /app/<otro>/.
            if (projectMode) {
                final String pfx = prefix + "/";
                for (String key : new java.util.ArrayList<>(remote.keySet())) {
                    if (key.startsWith(pfx) && !deployed.contains(key)) {
                        try {
                            b.del(key);
                            if (outputSink != null) {
                                final String k = key;
                                SwingUtilities.invokeLater(() -> outputSink.accept(
                                        "[Explorer] huérfano borrado: " + k));
                            }
                        } catch (java.io.IOException delErr) {
                            /* best-effort: si falla el DEL, seguimos */
                        }
                    }
                }
            }
            // 2c) Refrescar el árbol del FS AQUÍ — tras subir, ANTES de ejecutar.
            //     Si el programa es GUI (Gui.run() no retorna) el wire queda
            //     OCUPADO y el onRefresh() del done-callback (tras b.run()) no
            //     llega nunca. Esta es la única ventana con el wire libre. El
            //     LIST es síncrono (mismo hilo, wire libre); el repintado al EDT.
            try {
                final java.util.List<Backend.Entry> postFs = b.list();
                SwingUtilities.invokeLater(() -> {
                    rebuildTree(postFs);
                    status.setText(postFs.size() + " files (tras subir)");
                });
            } catch (java.io.IOException refErr) {
                // best-effort: si el LIST falla, seguimos al run igual.
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

    /** F2 (H19) — como toAppPath pero con prefijo de proyecto explícito
     *  ("/app/&lt;proj&gt;" o "/app"). Si el nombre ya trae '/', se respeta. */
    private static String appPath(String prefix, String localName) {
        if (localName.indexOf('/') >= 0) return localName;
        return prefix + "/" + localName;
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

    /**
     * ¿Es este `.mdn` MÁS VIEJO que su `.mod`? Entonces es de otra compilación y
     * NO se sube.
     *
     * Por qué existe esta comprobación: el `.mdn` sólo se regenera si el
     * proyecto tiene el AOT activado, pero el subidor cogía cualquiera que
     * hubiera al lado del `.mod`. Un `.mdn` de hace dos meses viajaba a la placa
     * como si fuera de este build — y como lleva código NATIVO compilado contra
     * una versión concreta de los helpers AOT, ejecutarlo tras cambiar esos
     * helpers es corrupción de memoria y reset (pasó: .mdn de mayo, helpers
     * v1→v2 en julio).
     *
     * Es la cuarta vez que un artefacto rancio muerde en este proyecto (el
     * fat-jar del IDE, los .mod versionados de la stdlib, los .o sin memoria de
     * sus flags). La regla barata que los caza a todos: si el fuente se
     * recompiló y el derivado no, el derivado NO vale.
     *
     * La VM tiene además su propio gate de ABI, que rechaza el `.mdn`
     * incompatible aunque llegue. Dos redes independientes, a propósito.
     */
    private static boolean mdnIsStale(File mdnFile, File modFile) {
        return mdnFile.lastModified() < modFile.lastModified();
    }

    /* ===== H10 — micro simulado: arranque, parada y configuración ===== */

    /** Refresca el estado visible del simulador (texto + color del chip). */
    private void updateSimUI() {
        boolean run = sim.isRunning();
        btnSim.setText(run ? "Parar" : "Arrancar");
        btnSim.setIcon(SimIcons.chip(14, run ? new Color(0x1B, 0x8A, 0x3C) : null));
        simState.setText(run ? ("en marcha :" + IdePrefs.load().simPort) : "parado");
        /* Parar mientras está conectado desconecta primero, así que el botón nunca
         * se bloquea; lo que sí se bloquea es cambiar la configuración en caliente
         * (el diálogo lo avisa). */
        btnSim.setEnabled(true);
    }

    /** Botón del chip: arranca el simulador y se conecta, o desconecta y lo para.
     *  Worker propio en vez de runAsync porque aquí SÍ hace falta atender el fallo
     *  (dejar el botón usable y explicar qué pasó); runAsync no expone ese punto. */
    private void onToggleSim() {
        if (!btnSim.isEnabled()) return;         // ya hay un arranque en curso
        if (sim.isRunning()) {
            if (isConnected()) onConnect();      // toggle: aquí desconecta
            sim.stop();
            updateSimUI();
            status.setText("simulador parado");
            return;
        }
        final IdePrefs prefs = IdePrefs.load();
        rbSim.setSelected(true);
        showCard(CARD_SIM);
        status.setText("arrancando el simulador…");
        btnSim.setEnabled(false);
        new SwingWorker<String, Void>() {
            @Override protected String doInBackground() throws Exception {
                return sim.start(prefs);
            }
            @Override protected void done() {
                try {
                    String endpoint = get();
                    updateSimUI();
                    status.setText("simulador en " + endpoint + " — conectando…");
                    if (!isConnected()) onConnect();   // ya escucha: conectar de verdad
                } catch (Exception ex) {
                    Throwable cause = ex.getCause() != null ? ex.getCause() : ex;
                    String msg = cause.getMessage() == null ? cause.toString() : cause.getMessage();
                    status.setText("no arrancó el simulador");
                    emitLine("[sim ERROR] " + msg + "\n");
                    updateSimUI();
                }
            }
        }.execute();
    }

    /** Botón del engranaje: la ventana de configuración del micro simulado. */
    private void onSimConfig() {
        IdePrefs prefs = IdePrefs.load();
        if (SimConfigDialog.show(this, prefs, sim.isRunning())) {
            /* La biblioteca de packs se aplica YA (el compilador es in-process):
             * si el usuario acaba de apuntarla, el siguiente Compile la usa. */
            basicplus.frontend.Main.setPacksDir(prefs.packsDirEffective());
            emitLine("[sim] configuración guardada: RAM " + prefs.simRamKb + "K, PSRAM "
                    + prefs.simPsramKb + "K, flash " + prefs.simFlashKb + "K, pantalla "
                    + (prefs.simNoScreen ? "ninguna"
                                         : prefs.simScreenW + "x" + prefs.simScreenH) + "\n");
            if (sim.isRunning())
                emitLine("[sim] para y arranca el simulador para aplicarla\n");
            updateSimUI();
        }
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
        rbSim.setEnabled(!connected);
        portCombo.setEnabled(!connected);
        endpointField.setEnabled(!connected);
        updateSimUI();
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
            deviceArch = 0;              // H11: la arch se va con la conexión
            setConnectedUI(false);
            rootNode.removeAllChildren();
            treeModel.reload();
            status.setText("Disconnected");
            return;
        }
        final boolean serial = rbSerial.isSelected();
        final boolean simulado = rbSim.isSelected();
        if (simulado && !sim.isRunning()) {
            status.setText("el simulador no está en marcha — pulsa Arrancar");
            return;
        }
        /* El micro simulado habla el mismo wire por TCP: su "endpoint" no lo teclea
         * nadie, sale de la configuración con la que lo arrancamos. */
        final String endpoint = serial
                ? (String) portCombo.getSelectedItem()
                : simulado ? ("127.0.0.1:" + IdePrefs.load().simPort)
                           : endpointField.getText().trim();
        if (endpoint == null || endpoint.isEmpty()) {
            status.setText(serial ? "No port selected" : "Empty endpoint");
            return;
        }
        // El nombre que se enseña sale de a QUÉ nos conectamos, no de la clase:
        // el mismo backend TCP sirve a la VM Java y al micro simulado.
        final Backend b = serial ? new SerialBackend()
                : new BpvmBackend(simulado ? rbSim.getText() : rbVm.getText());
        status.setText("Connecting to " + endpoint + " (" + b.displayName() + ")...");
        runAsync(() -> {
            String hello = b.connect(endpoint);
            return hello;
        }, hello -> {
            this.backend = b;
            sentCrc.clear();   // nueva conexión → cache de subidas limpia
            setConnectedUI(true);
            status.setText(endpoint + " — " + hello);
            onRefresh();
            fetchDeviceArch();           // H11: la arch, ANTES de que haga falta compilar
            if (connectedSink != null) connectedSink.accept(debugClient());   // H9 escrutinio
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
        // key es del estilo "/Placa/lib/Math.mod" — descender desde root.
        String[] segs = key.split("/");
        DefaultMutableTreeNode cur = rootNode;
        TreePath tp = new TreePath(cur);
        // segs[0] = "", segs[1] = label del root, segs[2..] = niños.
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

    /**
     * H11 — pregunta al micro por su arquitectura y la guarda. En segundo plano
     * y sin ruido: un firmware viejo no manda el campo y se queda en 0, con lo
     * que el AOT simplemente no se ofrece (y el módulo corre interpretado, que
     * es correcto). No falla nada por no saberlo.
     */
    private void fetchDeviceArch() {
        final BpvmClient dc = debugClient();
        if (dc == null) return;
        runAsync(() -> dc.getInfo(4000), m -> {
            if (m == null) return;
            long a = ilong(m, "arch");
            deviceArch = (int) a;
            if (a != 0 && outputSink != null) {
                outputSink.accept("[Explorer] la placa ejecuta nativo "
                        + archName((int) a) + " (arch=" + a + ")");
            }
        });
    }

    /** Nombre legible de la arquitectura del nativo (e_machine del ELF). */
    public static String archName(int arch) {
        switch (arch) {
            case 40:  return "arm";      /* EM_ARM   — Cortex-M Thumb-2 */
            case 243: return "riscv";    /* EM_RISCV — RV32, ESP32-P4   */
            /* EM_XTENSA (ESP32-S3). Sin toolchain AOT todavía: se devuelve igual
             * para que AotBuild lo rechace con un aviso claro ("target no
             * soportado") en vez de caer al ajuste del proyecto y compilar ARM
             * para una placa Xtensa. Decir la verdad y fallar limpio. */
            case 94:  return "xtensa";   /* EM_XTENSA — ESP32-S3        */
            default:  return "";
        }
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
        // H9 — reparto de la memoria de la VM (heap BP + región de stacks, tope
        // 512K). Solo si el firmware lo manda (tolerante con firmwares viejos).
        long vh = ilong(m, "vmHeapBytes"), vs = ilong(m, "vmStackBytes");
        if (vh > 0 && vs > 0)
            sb.append("VM          : heap ").append(human(vh))
              .append(" + stack ").append(human(vs)).append('\n');
        // V5/H — el bloque de la BD y lo que el IDE necesita para PRE-ENLAZAR un
        // pack de código nativo. Tolerante con firmwares viejos (si no vienen, no
        // se pintan) — pero OJO: mientras no se pinten, este panel NO distingue un
        // firmware nuevo de uno viejo. Por eso la línea de la BD sale SIEMPRE que
        // el firmware mande el campo, aunque valga 0: "(no activada)" es una
        // respuesta, "nada" es un instrumento mudo.
        if (m.containsKey("sqliteBytes")) {
            long   sqb = ilong(m, "sqliteBytes"), sqa = ilong(m, "sqliteBase");
            long   ask = ilong(m, "sqliteAskedMb"), min = ilong(m, "sqliteMinMb");
            String st  = istr(m, "sqliteStatus");
            sb.append("BD (SQLite) : ");
            // El AVISO va con el MOTIVO, no con los bytes: 0 bytes significa
            // "no se pidió" Y "se pidió mal", y son cosas distintas. Y el
            // remedio (el mínimo) lo dice la PLACA — si algún día cambia,
            // cambia en un sitio y este panel se entera solo.
            switch (st) {
                case "ok":
                    sb.append(human(sqb)).append(" @ ").append(hex(sqa));
                    break;
                case "low":
                    sb.append("SQLite=").append(ask).append(" es MENOS del mínimo (")
                      .append(min).append(" MB) → no se activa. Pon SQLite=").append(min)
                      .append(" o más y reinicia");
                    break;
                case "nofit":
                    sb.append("SQLite=").append(ask)
                      .append(" no cabe: la VM se quedaría sin memoria. Baja el valor");
                    break;
                default:   // "off" y firmwares que no manden el motivo
                    sb.append("no activada — añade SQLite=").append(min > 0 ? min : 2)
                      .append(" al entorno de la placa y reinicia");
            }
            sb.append('\n');
        }
        long pxb = ilong(m, "packsXipBase"), pln = ilong(m, "packsBytes");
        if (pxb > 0)
            sb.append("Zona packs  : ").append(human(pln))
              .append(" @ ").append(hex(pxb)).append("  (dirección que ve la CPU)\n");
        String fabi = istr(m, "floatAbi");
        if (!fabi.isEmpty())
            sb.append("Coma flot.  : ").append(fabi)
              .append("  (sello del pack nativo; arch sola NO distingue hard de softfp)\n");
        // #354 — lo que FreeRTOS sabe y nunca le habíamos preguntado: cuánto de
        // lo reservado llegó a usarse. SOLO DIAGNÓSTICO (no se recorta nada
        // hasta ver los números con carga real; en la duda se deja como está).
        // Ojo al leerlos: "RTOS libre" es el mínimo histórico de ucHeap (32 KB en
        // la Pico), que hospeda las tareas de FreeRTOS — vm_task, comms, workers.
        //
        // CORREGIDO 3-ago: aquí ponía "= techo de threads" y era FALSO. Los Thread
        // de BP no crean tareas de FreeRTOS: sus pilas salen de la región de pilas
        // de la propia VM (src/threading.c, `vm->next_thread_stack`, 2 KB cada
        // una), así que este número NO baja por más threads que cree el programa.
        // Lo midió Eduardo en placa: ejecutó T (hilos) + JsonDemo y el mínimo se
        // quedó clavado en los mismos 15 KB. Quien depurara un "no me caben más
        // threads" mirando esta línea se iba de cabeza a la pista equivocada.
        //
        // Una marca tomada sin haber ejecutado nada no dice nada: mirar DESPUÉS
        // de correr algo con carga. Tolerante con firmwares que no lo manden.
        long rh = ilong(m, "rtosHeapMinFreeBytes"), vt = ilong(m, "vmTaskStackFreeBytes");
        if (rh > 0) sb.append("RTOS libre  : ").append(human(rh))
                      .append(" (mínimo histórico de ucHeap)\n");
        if (vt > 0) sb.append("Pila VM     : ").append(human(vt))
                      .append(" sin usar de 16 KB (marca de agua)\n");
        sb.append("FS          : ").append(human(ilong(m, "fsUsedBytes"))).append(" / ")
          .append(human(ilong(m, "fsTotalBytes"))).append('\n');
        long up = ilong(m, "uptimeMs");
        if (up > 0) sb.append("Uptime      : ").append(up / 1000L).append(" s\n");
        String reset = istr(m, "resetReason");   // paso 4 cierre — causa del último reset
        if (!reset.isEmpty()) sb.append("Reset       : ").append(reset).append('\n');
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
    /** V5/H — dirección en hexadecimal de 32 bits. Las direcciones se leen mal en
     *  decimal: 0x11000000 (ventana PSRAM) y 0x10180000 (XIP de flash) se
     *  reconocen de un vistazo, y 285212672 no. */
    private static String hex(long addr) {
        return String.format("0x%08X", addr & 0xFFFFFFFFL);
    }

    private static String human(long bytes) {
        if (bytes <= 0) return "0";
        // MB con un decimal: un FS de 2080768 B es "1.98 MB", no "1 MB"
        // (la división entera perdía casi 1 MB de golpe en el redondeo).
        if (bytes >= 1024L * 1024L)
            return String.format(java.util.Locale.US, "%.1f MB", bytes / (1024.0 * 1024.0));
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
                    String msg = cause.getMessage() == null ? "" : cause.getMessage();
                    // #256 — un BUSY aquí no es un fallo: la placa está
                    // ejecutando un programa (p.ej. un autorun) y durante un
                    // run solo atiende HELLO/KILL. Mensaje accionable en vez
                    // del error crudo del wire.
                    if (msg.contains("BUSY")) {
                        status.setText("placa ocupada (programa en ejecución)");
                        if (outputSink != null) {
                            outputSink.accept("[Placa] ocupada ejecutando un programa "
                                    + "— 'kill' para abortarlo y recuperar el control");
                        }
                    } else {
                        status.setText("ERROR: " + msg);
                        if (outputSink != null) {
                            outputSink.accept("[Placa ERROR] " + msg);
                        }
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
