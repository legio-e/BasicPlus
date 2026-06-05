/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.mycompany.bpide;

import edu.bpgenvm.vm.DebugContext;
import edu.bpgenvm.vm.debug.StepCommand;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Desktop;
import java.awt.Font;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.swing.BorderFactory;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.KeyStroke;
import org.fife.ui.rsyntaxtextarea.AbstractTokenMakerFactory;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.TokenMakerFactory;
import org.fife.ui.rsyntaxtextarea.folding.FoldParserManager;
import org.fife.ui.rtextarea.RTextScrollPane;
import javax.swing.SwingUtilities;
import javax.swing.SwingWorker;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.swing.table.AbstractTableModel;
import javax.swing.text.DefaultHighlighter;
import javax.swing.text.Element;
import javax.swing.text.Highlighter;

/**
 * IDE minimal de BasicPlus. MVP: cargar/guardar un .bp, compilar y ejecutar
 * spawneando frontend + VM en sub-procesos, capturando stdout/stderr a la
 * pestaña "Consola" y los errores semánticos parseados a la pestaña
 * "Errores" (double-click salta a la línea).
 *
 * @author Eduardo
 */
public class FrmMain extends javax.swing.JFrame
{
    static FrmMain form;

    // -------- Estado MVP (fuera del bloque GEN para que NetBeans no toque) --------
    /** Editor con syntax highlighting del tab ACTIVO. Se actualiza desde
     *  el ChangeListener de jTabbedPane1 cuando el usuario cambia de tab.
     *  Todas las acciones (save, compile, debug, run) operan sobre este. */
    private RSyntaxTextArea editorArea;
    /** TextArea de salida del proceso (Consola). */
    private JTextArea consolaArea;
    /** Lista observable de errores; única fuente de verdad para la tabla y para futuros consumers. */
    private final ObservableList<CompileError> errors = new ObservableList<>();
    /** Fichero del tab ACTIVO. Igual que editorArea, se actualiza desde el
     *  ChangeListener. null si el tab activo todavía no se ha guardado. */
    private Path currentFile = null;

    /** Estado de cada tab abierto en jTabbedPane1. Indexado por su
     *  componente (JScrollPane) porque los tabs pueden reordenarse,
     *  insertar/quitar y los índices no son estables. */
    private static final class OpenFile {
        final RSyntaxTextArea editor;
        final JScrollPane scroll;
        Path file;        // null = buffer sin guardar (p.ej. tab inicial)
        OpenFile(RSyntaxTextArea editor, JScrollPane scroll, Path file) {
            this.editor = editor; this.scroll = scroll; this.file = file;
        }
    }
    private final java.util.Map<java.awt.Component, OpenFile> openFiles = new java.util.HashMap<>();
    /** Patrón de líneas de diagnóstico del frontend BP: "[L:C] error/aviso categoría: msg".
     *  Importante: la categoría puede contener acentos ("sintáctico", "semántico");
     *  usamos \\S+? (no-greedy non-whitespace) en vez de \\w+ porque \\w no acepta
     *  caracteres no-ASCII por defecto en Java. */
    private static final Pattern ERROR_RE = Pattern.compile(
            "\\[(\\d+):(\\d+)\\]\\s+(error|aviso)\\s+(\\S+?):\\s+(.*)");

    // ---- Estado de depuración ----
    /** Sesión de depuración compartida entre runs. Breakpoints persisten. */
    private final DebugSession debug = new DebugSession();
    /** Highlight de la línea pausada (amarillo). Se quita al continuar. */
    private Object pausedLineHighlight = null;
    /** Highlights de las líneas con breakpoint (rojo claro). */
    private final java.util.Map<Integer, Object> bpHighlights = new java.util.HashMap<>();
    /** Modelo de la tabla "Variables" (slots locales como i32 crudos). */
    private LocalsTableModel localsModel;
    private ModulePropsTableModel modulePropsModel;
    /** Modelo de la tabla "Call Stack" (frames pc/bp). */
    private StackTableModel  stackModel;
    /** Indica si la sesión actual es "Run" normal o "Debug Run". */
    private volatile boolean debuggingActive = false;

    // ---- Proyecto (.bpbuild) — árbol del panel superior izquierdo ----
    /** Configuración del proyecto activo, o null si trabajamos con un .bp suelto. */
    private basicplus.frontend.BpBuild currentProject;
    /** Path al .bpbuild del proyecto activo (informativo, para etiquetar). */
    private java.nio.file.Path currentProjectFile;
    /** Árbol estilo NetBeans con Sources/Output/Dependencies del proyecto. */
    private javax.swing.JTree projectTree;
    /** Modelo del árbol; lo reconstruimos al cargar/refrescar el proyecto. */
    private javax.swing.tree.DefaultTreeModel projectTreeModel;
    /** IDE-3 — submenús de recientes: ficheros .bp (en File) y proyectos (en Project). */
    private javax.swing.JMenu recentFilesMenu;
    private javax.swing.JMenu recentProjectsMenu;
    /** FP4 — panel inferior izquierdo: ficheros remotos del dispositivo Pico. */
    private PicoExplorer picoExplorer;

    // ---- Status bar (jPanel1 al PAGE_END del form) ----
    /** Lado izquierdo: nombre/ruta del fichero actual o "(sin fichero)". */
    private JLabel lblStatusFile;
    /** Lado derecho: posición del caret en formato "Línea N, Col M". */
    private JLabel lblStatusCaret;

    /**
     * Creates new form FrmMain
     */
    public FrmMain() {
        initComponents();
        this.setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        FrmMain.form=this;
        setupMvp();
    }

    /**
     * This method is called from within the constructor to initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is always
     * regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jPanel1 = new javax.swing.JPanel();
        jSplitPane1 = new javax.swing.JSplitPane();
        jSplitPane2 = new javax.swing.JSplitPane();
        jSplitPane3 = new javax.swing.JSplitPane();
        jTabbedPane1 = new javax.swing.JTabbedPane();
        jTabbedPane2 = new javax.swing.JTabbedPane();
        jPanel2 = new javax.swing.JPanel();
        jPanel3 = new javax.swing.JPanel();
        jScrollPane1 = new javax.swing.JScrollPane();
        jTable1 = new javax.swing.JTable();
        jMenuBar1 = new javax.swing.JMenuBar();
        jMenu1 = new javax.swing.JMenu();
        jMenuItem1 = new javax.swing.JMenuItem();
        jMenuItem2 = new javax.swing.JMenuItem();
        jSeparator1 = new javax.swing.JPopupMenu.Separator();
        jMenuItem3 = new javax.swing.JMenuItem();
        jMenu2 = new javax.swing.JMenu();
        jMenu3 = new javax.swing.JMenu();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        jPanel1.setPreferredSize(new java.awt.Dimension(935, 40));

        javax.swing.GroupLayout jPanel1Layout = new javax.swing.GroupLayout(jPanel1);
        jPanel1.setLayout(jPanel1Layout);
        jPanel1Layout.setHorizontalGroup(
            jPanel1Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 935, Short.MAX_VALUE)
        );
        jPanel1Layout.setVerticalGroup(
            jPanel1Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 40, Short.MAX_VALUE)
        );

        getContentPane().add(jPanel1, java.awt.BorderLayout.PAGE_END);

        jSplitPane1.setDividerLocation(200);

        jSplitPane2.setDividerLocation(400);
        jSplitPane2.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
        jSplitPane1.setLeftComponent(jSplitPane2);

        jSplitPane3.setDividerLocation(400);
        jSplitPane3.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
        jSplitPane3.setLeftComponent(jTabbedPane1);

        javax.swing.GroupLayout jPanel2Layout = new javax.swing.GroupLayout(jPanel2);
        jPanel2.setLayout(jPanel2Layout);
        jPanel2Layout.setHorizontalGroup(
            jPanel2Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 722, Short.MAX_VALUE)
        );
        jPanel2Layout.setVerticalGroup(
            jPanel2Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 162, Short.MAX_VALUE)
        );

        jTabbedPane2.addTab("Consola", jPanel2);

        jPanel3.setLayout(new java.awt.GridLayout(1, 1));

        jTable1.setModel(new javax.swing.table.DefaultTableModel(
            new Object [][] {
                {null, null, null, null},
                {null, null, null, null},
                {null, null, null, null},
                {null, null, null, null}
            },
            new String [] {
                "Title 1", "Title 2", "Title 3", "Title 4"
            }
        ));
        jScrollPane1.setViewportView(jTable1);

        jPanel3.add(jScrollPane1);

        jTabbedPane2.addTab("Errores", jPanel3);

        jSplitPane3.setRightComponent(jTabbedPane2);

        jSplitPane1.setRightComponent(jSplitPane3);

        getContentPane().add(jSplitPane1, java.awt.BorderLayout.CENTER);

        jMenu1.setText("File");

        jMenuItem1.setText("Load");
        jMenu1.add(jMenuItem1);

        jMenuItem2.setText("Save");
        jMenu1.add(jMenuItem2);
        jMenu1.add(jSeparator1);

        jMenuItem3.setText("Exit");
        jMenuItem3.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jMenuItem3ActionPerformed(evt);
            }
        });
        jMenu1.add(jMenuItem3);

        jMenuBar1.add(jMenu1);

        jMenu2.setText("Edit");
        jMenuBar1.add(jMenu2);

        jMenu3.setText("Run");
        jMenuBar1.add(jMenu3);

        setJMenuBar(jMenuBar1);

        pack();
    }// </editor-fold>//GEN-END:initComponents

    private void jMenuItem3ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jMenuItem3ActionPerformed
        // TODO add your handling code here:
        if (FrmMain.form!=null)  FrmMain.form.dispose();
    }//GEN-LAST:event_jMenuItem3ActionPerformed

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) 
    {
        // IDE-2 — Look & Feel NATIVO del SO (Windows/GTK/Aqua) en vez del "Metal"
        //         de Java que venía forzado. Si no está disponible, Swing se
        //         queda con el por defecto. Se fija antes de crear la ventana.
        try {
            javax.swing.UIManager.setLookAndFeel(
                    javax.swing.UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ex) {
            java.util.logging.Logger.getLogger(FrmMain.class.getName())
                    .log(java.util.logging.Level.WARNING, "L&F del sistema no disponible", ex);
        }

        /* Create and display the form */
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                new FrmMain().setVisible(true);
            }
        });
    }

    // ================================================================
    // MVP — todo lo siguiente está fuera del bloque GEN; NetBeans no toca
    // ================================================================

    /**
     * Configura todo el comportamiento MVP: instala editor en jTabbedPane1,
     * consola en jPanel2, modelo de errores en jTable1, cablea menús (Load,
     * Save, Run-Compile, Run-Run) y double-click de tabla de errores.
     */
    private void setupMvp() {
        setTitle("BpIde — Untitled");
        setSize(1100, 720);
        setLocationRelativeTo(null);

        // -- RSyntaxTextArea: registrar el TokenMaker de BP UNA vez, antes de
        //    crear ningún editor. El estilo "text/bp" lo usa newEditorPane(). --
        AbstractTokenMakerFactory atmf =
                (AbstractTokenMakerFactory) TokenMakerFactory.getDefaultInstance();
        atmf.putMapping("text/bp", "com.mycompany.bpide.BpTokenMaker");
        // IDE-5 — folding por bloques de BP (function/class/if/while/try/...).
        FoldParserManager.get().addFoldParserMapping("text/bp", new BpFoldParser());

        // -- Editor inicial: tab "(sin abrir)" reciclable.
        //    El primer openFileInEditor() reusará este tab si su Path
        //    sigue siendo null (no se le ha cargado nada todavía).
        //    Los siguientes ficheros crean tabs nuevos. --
        editorArea = newEditorPane();
        JScrollPane initialScroll = new RTextScrollPane(editorArea);
        jTabbedPane1.addTab("(sin abrir)", initialScroll);
        OpenFile initial = new OpenFile(editorArea, initialScroll, null);
        openFiles.put(initialScroll, initial);
        installTabCloseButton(initialScroll, "(sin abrir)");

        // -- Árbol del proyecto en el cuadro superior izquierdo (jSplitPane2). --
        setupProjectTree();

        // Caret listener inicial — los tabs nuevos también lo instalan en
        // newEditorPane() para que la status bar refleje el activo.
        editorArea.addCaretListener(e -> updateCaretStatus());

        // ChangeListener: cuando el usuario cambia de tab, redirige
        // editorArea y currentFile al buffer del tab activo. De este modo,
        // las ~27 referencias a editorArea en el resto del IDE siguen
        // funcionando sin cambios — siempre apuntan al editor visible.
        jTabbedPane1.addChangeListener(e -> refreshActiveTab());

        // Ctrl+W cierra el tab activo.
        javax.swing.KeyStroke ctrlW = javax.swing.KeyStroke.getKeyStroke(
                KeyEvent.VK_W, java.awt.event.InputEvent.CTRL_DOWN_MASK);
        getRootPane().getInputMap(javax.swing.JComponent.WHEN_IN_FOCUSED_WINDOW)
                .put(ctrlW, "closeActiveTab");
        getRootPane().getActionMap().put("closeActiveTab", new javax.swing.AbstractAction() {
            @Override public void actionPerformed(java.awt.event.ActionEvent e) {
                int idx = jTabbedPane1.getSelectedIndex();
                if (idx >= 0) closeTabAt(idx);
            }
        });

        // -- Status bar (jPanel1: ya estaba en BorderLayout.PAGE_END del form). --
        setupStatusBar();

        // -- Consola: jPanel2 estaba vacío; le metemos un JTextArea no editable. --
        consolaArea = new JTextArea();
        consolaArea.setEditable(false);
        consolaArea.setFont(new Font("Consolas", Font.PLAIN, 12));
        jPanel2.setLayout(new BorderLayout());
        jPanel2.add(new JScrollPane(consolaArea), BorderLayout.CENTER);

        // -- Errores: bindeamos un ErrorTableModel a la ObservableList. --
        ErrorTableModel errorTableModel = new ErrorTableModel(errors);
        jTable1.setModel(errorTableModel);
        // Anchos preferentes (cols: Fichero, Línea, Columna, Tipo, Categoría, Mensaje).
        int[] widths = {140, 60, 60, 70, 100, 600};
        for (int i = 0; i < widths.length && i < jTable1.getColumnModel().getColumnCount(); i++) {
            jTable1.getColumnModel().getColumn(i).setPreferredWidth(widths[i]);
        }
        // N7 — renderer que pinta filas según severidad: rojo para "error",
        // amarillo suave para "aviso". Aplicado a todas las columnas para
        // que la fila entera quede tintada y los avisos no se confundan con
        // errores reales.
        javax.swing.table.DefaultTableCellRenderer severityRenderer =
                new javax.swing.table.DefaultTableCellRenderer() {
            @Override
            public java.awt.Component getTableCellRendererComponent(
                    javax.swing.JTable tbl, Object value, boolean sel,
                    boolean focus, int row, int col) {
                java.awt.Component c = super.getTableCellRendererComponent(
                        tbl, value, sel, focus, row, col);
                if (!sel && row >= 0 && row < errors.size()) {
                    CompileError ce = errors.get(row);
                    if ("aviso".equalsIgnoreCase(ce.kind)) {
                        c.setBackground(new java.awt.Color(255, 248, 200));  // amarillo suave
                        c.setForeground(java.awt.Color.BLACK);
                    } else {
                        c.setBackground(new java.awt.Color(255, 220, 220));  // rojo suave
                        c.setForeground(java.awt.Color.BLACK);
                    }
                }
                return c;
            }
        };
        for (int i = 0; i < jTable1.getColumnModel().getColumnCount(); i++) {
            jTable1.getColumnModel().getColumn(i).setCellRenderer(severityRenderer);
        }
        jTable1.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    int row = jTable1.getSelectedRow();
                    if (row >= 0 && row < errors.size()) {
                        CompileError ce = errors.get(row);
                        jumpToLine(ce.line);
                    }
                }
            }
        });

        // UX: mantener el título de la pestaña "Errores" con el conteo en vivo,
        // y al primer error que llegue tras un Run/Compile, saltar a esa
        // pestaña para que el usuario lo vea sin tener que cambiar a mano.
        // Localizamos el índice una vez (asumimos: tab 1 = Errores).
        final int erroresTabIdx = findTabByTitlePrefix(jTabbedPane2, "Errores");
        errors.addListener(new ObservableList.Listener<CompileError>() {
            @Override public void onAdded(int idx, CompileError item) {
                updateErroresTabTitle(erroresTabIdx);
                if (idx == 0 && erroresTabIdx >= 0) {
                    jTabbedPane2.setSelectedIndex(erroresTabIdx);
                }
            }
            @Override public void onRemoved(int idx, CompileError item) {
                updateErroresTabTitle(erroresTabIdx);
            }
            @Override public void onCleared() {
                updateErroresTabTitle(erroresTabIdx);
            }
        });

        // -- Menú File: Load / Save / Recent Files. --
        jMenuItem1.addActionListener(e -> onLoad());
        jMenuItem2.addActionListener(e -> onSave());
        recentFilesMenu = new javax.swing.JMenu("Recent Files");
        jMenu1.insert(recentFilesMenu, 2);   // Load, Save, [Recent], sep, Exit
        rebuildRecentFilesMenu();

        // -- Menú Project: New Project, Open Project, Close Project. --
        setupProjectMenu();

        // -- Menú Run: Compile, Run, Clear. --
        JMenuItem miCompile = new JMenuItem("Compile");
        miCompile.addActionListener(e -> doRun(false));
        jMenu3.add(miCompile);

        JMenuItem miRun = new JMenuItem("Run");
        miRun.addActionListener(e -> doRun(true));
        jMenu3.add(miRun);

        JMenuItem miRunOnPico = new JMenuItem("Run on Pico");
        miRunOnPico.addActionListener(e -> doRunOnPico());
        jMenu3.add(miRunOnPico);

        jMenu3.addSeparator();
        JMenuItem miClear = new JMenuItem("Clear console");
        miClear.addActionListener(e -> {
            consolaArea.setText("");
            errors.clear();
        });
        jMenu3.add(miClear);

        setupDebug();
        setupHelp();
    }

    // ================================================================
    // PROYECTO: árbol estilo NetBeans (Sources / Output / Dependencies)
    // y menú "Project" con New / Open / Close.
    // ================================================================

    /**
     * Construye el JTree con un placeholder "(sin proyecto)" y lo monta
     * en el cuadro superior izquierdo (jSplitPane2.setTopComponent).
     * Doble-click en una hoja de tipo .bp abre el fichero en el editor.
     */
    private void setupProjectTree() {
        javax.swing.tree.DefaultMutableTreeNode root =
                new javax.swing.tree.DefaultMutableTreeNode("(sin proyecto)");
        projectTreeModel = new javax.swing.tree.DefaultTreeModel(root);
        projectTree = new javax.swing.JTree(projectTreeModel);
        projectTree.setRootVisible(true);
        projectTree.setShowsRootHandles(true);
        projectTree.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    javax.swing.tree.TreePath p = projectTree.getPathForLocation(e.getX(), e.getY());
                    if (p == null) return;
                    Object last = ((javax.swing.tree.DefaultMutableTreeNode) p.getLastPathComponent())
                            .getUserObject();
                    if (last instanceof ProjectNode) {
                        ProjectNode pn = (ProjectNode) last;
                        if (pn.openableFile != null) {
                            openFileInEditor(pn.openableFile);
                        }
                    }
                }
            }
        });
        jSplitPane2.setTopComponent(new JScrollPane(projectTree));

        // FP4 — explorador del dispositivo Pico en el panel inferior izquierdo.
        // Envía el output del comando RUN remoto a la consola del IDE.
        picoExplorer = new PicoExplorer();
        picoExplorer.setOutputSink(line -> {
            if (consolaArea != null) {
                consolaArea.append(line);
                consolaArea.append("\n");
                consolaArea.setCaretPosition(consolaArea.getDocument().getLength());
            }
        });
        jSplitPane2.setBottomComponent(picoExplorer);
    }

    /** Userdata adjunta a cada nodo del JTree: label visible + path opcional
     *  al fichero a abrir si el usuario hace doble-click. */
    private static final class ProjectNode {
        final String label;
        final java.nio.file.Path openableFile;
        ProjectNode(String label, java.nio.file.Path openableFile) {
            this.label = label;
            this.openableFile = openableFile;
        }
        @Override public String toString() { return label; }
    }

    private void setupProjectMenu() {
        JMenu menuProject = new JMenu("Project");
        JMenuItem miNew   = new JMenuItem("New Project...");
        JMenuItem miOpen  = new JMenuItem("Open Project...");
        JMenuItem miClose = new JMenuItem("Close Project");
        JMenuItem miEndpoint = new JMenuItem("VM Endpoint...");
        miNew.addActionListener(e -> onNewProject());
        miOpen.addActionListener(e -> onOpenProject());
        miClose.addActionListener(e -> onCloseProject());
        miEndpoint.addActionListener(e -> onConfigureVmEndpoint());
        menuProject.add(miNew);
        menuProject.add(miOpen);
        recentProjectsMenu = new javax.swing.JMenu("Recent Projects");
        menuProject.add(recentProjectsMenu);
        rebuildRecentProjectsMenu();
        menuProject.addSeparator();
        menuProject.add(miClose);
        menuProject.addSeparator();
        menuProject.add(miEndpoint);
        // Insertar entre File y Edit. El JMenuBar tiene File, Edit, Run; queremos
        // File, Project, Edit, Run.
        jMenuBar1.add(menuProject, 1);
    }

    // ---- IDE-3: submenús de recientes (ficheros .bp y proyectos .bpbuild) ----

    /** Reconstruye un submenú de recientes desde una lista de paths. Cada
     *  entrada abre el fichero/proyecto; al final, "Clear". */
    private void rebuildRecentMenu(javax.swing.JMenu menu, java.util.List<String> paths, boolean isProject) {
        menu.removeAll();
        if (paths.isEmpty()) {
            JMenuItem empty = new JMenuItem("(vacío)");
            empty.setEnabled(false);
            menu.add(empty);
            return;
        }
        int i = 1;
        for (String p : paths) {
            final java.nio.file.Path path = java.nio.file.Paths.get(p);
            JMenuItem item = new JMenuItem(i + "  " + path.getFileName());
            item.setToolTipText(p);
            item.addActionListener(e -> {
                if (isProject) loadProjectFrom(path);
                else           openFileInEditor(path);
            });
            menu.add(item);
            i++;
        }
        menu.addSeparator();
        JMenuItem clear = new JMenuItem("Clear");
        clear.addActionListener(e -> {
            IdePrefs pr = IdePrefs.load();
            if (isProject) { pr.recentProjects.clear(); pr.save(); rebuildRecentProjectsMenu(); }
            else           { pr.recentFiles.clear();    pr.save(); rebuildRecentFilesMenu(); }
        });
        menu.add(clear);
    }

    private void rebuildRecentFilesMenu() {
        rebuildRecentMenu(recentFilesMenu, IdePrefs.load().recentFiles, false);
    }

    private void rebuildRecentProjectsMenu() {
        rebuildRecentMenu(recentProjectsMenu, IdePrefs.load().recentProjects, true);
    }

    /** A2.6 — Diálogo simple para configurar host:port de la VM remota.
     *  Vacío = comportamiento por defecto (spawn local). Se persiste en
     *  un fichero JSON al lado del IDE para reuso entre sesiones. */
    private void onConfigureVmEndpoint() {
        IdePrefs prefs = IdePrefs.load();
        String current = (prefs.vmHost == null || prefs.vmHost.isEmpty())
                ? "" : (prefs.vmHost + ":" + prefs.vmPort);
        String input = (String) javax.swing.JOptionPane.showInputDialog(
                this,
                "Endpoint de la VM remota (host:puerto). Vacío = lanzar VM local.\n"
                        + "El daemon remoto debe estar corriendo: bpgenvm --listen <port> --workdir <dir>",
                "VM Endpoint",
                javax.swing.JOptionPane.PLAIN_MESSAGE,
                null, null, current);
        if (input == null) return;   // cancel
        input = input.trim();
        if (input.isEmpty()) {
            prefs.vmHost = null;
            prefs.vmPort = 0;
        } else {
            int colon = input.indexOf(':');
            if (colon <= 0 || colon == input.length() - 1) {
                javax.swing.JOptionPane.showMessageDialog(this,
                        "Formato esperado: host:puerto", "VM Endpoint",
                        javax.swing.JOptionPane.ERROR_MESSAGE);
                return;
            }
            int p;
            try { p = Integer.parseInt(input.substring(colon + 1).trim()); }
            catch (NumberFormatException ex) {
                javax.swing.JOptionPane.showMessageDialog(this,
                        "Puerto inválido: " + input.substring(colon + 1),
                        "VM Endpoint", javax.swing.JOptionPane.ERROR_MESSAGE);
                return;
            }
            prefs.vmHost = input.substring(0, colon).trim();
            prefs.vmPort = p;
        }
        prefs.save();
        appendConsola("[ide] VM endpoint actualizado: "
                + (prefs.vmHost == null ? "local (spawn)" : prefs.vmHost + ":" + prefs.vmPort) + "\n");
    }

    /** Pregunta al usuario: directorio del proyecto + nombre. Crea estructura:
     *  <projectDir>/<name>.bpbuild + src/<name>.bp + out/. */
    private void onNewProject() {
        javax.swing.JFileChooser fc = new javax.swing.JFileChooser();
        fc.setDialogTitle("Selecciona el directorio donde crear el proyecto");
        fc.setFileSelectionMode(javax.swing.JFileChooser.DIRECTORIES_ONLY);
        if (fc.showOpenDialog(this) != javax.swing.JFileChooser.APPROVE_OPTION) return;
        java.io.File dir = fc.getSelectedFile();

        String name = javax.swing.JOptionPane.showInputDialog(this,
                "Nombre del proyecto (será el módulo main):",
                "Nuevo proyecto", javax.swing.JOptionPane.PLAIN_MESSAGE);
        if (name == null) return;
        name = name.trim();
        if (name.isEmpty() || !isValidModuleName(name)) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "Nombre inválido: usa letras, dígitos y guiones bajos (no empieza por dígito).",
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
            return;
        }
        java.io.File srcDir = new java.io.File(dir, "src");
        java.io.File outDir = new java.io.File(dir, "out");
        if (!srcDir.exists() && !srcDir.mkdirs()) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "No se pudo crear " + srcDir,
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
            return;
        }
        outDir.mkdirs();

        // Scaffolding del .bp principal.
        java.io.File mainBp = new java.io.File(srcDir, name + ".bp");
        try (java.io.PrintWriter pw = new java.io.PrintWriter(mainBp, "UTF-8")) {
            pw.println("module " + name);
            pw.println("  function main(arg: string)");
            pw.println("    print \"hola desde " + name + "\"");
            pw.println("  end main");
            pw.println("end " + name);
        } catch (java.io.IOException ex) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "No se pudo crear " + mainBp + ": " + ex.getMessage(),
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
            return;
        }

        // .bpbuild
        java.io.File buildFile = new java.io.File(dir, name + ".bpbuild");
        try (java.io.PrintWriter pw = new java.io.PrintWriter(buildFile, "UTF-8")) {
            pw.println("// Proyecto BasicPlus generado por BpIde.");
            pw.println("{");
            pw.println("  \"sourceDir\":    \"src\",");
            pw.println("  \"outDir\":       \"out\",");
            pw.println("  \"main\":         \"" + name + "\",");
            pw.println("  \"dependencies\": []");
            pw.println("}");
        } catch (java.io.IOException ex) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "No se pudo crear " + buildFile + ": " + ex.getMessage(),
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
            return;
        }

        loadProjectFrom(buildFile.toPath());
        openFileInEditor(mainBp.toPath());
    }

    /** Abre un FileChooser filtrando por .bpbuild y lo carga. */
    private void onOpenProject() {
        javax.swing.JFileChooser fc = new javax.swing.JFileChooser();
        fc.setDialogTitle("Abrir proyecto (.bpbuild)");
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Proyecto BP", "bpbuild"));
        if (fc.showOpenDialog(this) != javax.swing.JFileChooser.APPROVE_OPTION) return;
        loadProjectFrom(fc.getSelectedFile().toPath());
    }

    /** Vuelve al modo "fichero suelto": vacía el árbol y olvida currentProject. */
    private void onCloseProject() {
        currentProject = null;
        currentProjectFile = null;
        javax.swing.tree.DefaultMutableTreeNode root =
                new javax.swing.tree.DefaultMutableTreeNode("(sin proyecto)");
        projectTreeModel.setRoot(root);
        setTitle("BpIde — Untitled");
    }

    private static boolean isValidModuleName(String s) {
        if (s.isEmpty()) return false;
        if (!Character.isLetter(s.charAt(0)) && s.charAt(0) != '_') return false;
        for (int i = 1; i < s.length(); i++) {
            char c = s.charAt(i);
            if (!Character.isLetterOrDigit(c) && c != '_') return false;
        }
        return true;
    }

    /** Carga el .bpbuild en disco y rellena el árbol. Si hay error, muestra
     *  diálogo y se queda en el estado anterior. */
    private void loadProjectFrom(java.nio.file.Path bpbuild) {
        try {
            basicplus.frontend.BpBuild proj = basicplus.frontend.BpBuild.load(bpbuild);
            this.currentProject     = proj;
            this.currentProjectFile = bpbuild;
            refreshProjectTree();
            setTitle("BpIde — " + bpbuild.getFileName());
            // IDE-3 — registrar en "Recent Projects".
            IdePrefs prP = IdePrefs.load();
            IdePrefs.pushRecent(prP.recentProjects, bpbuild.toAbsolutePath().toString(), IdePrefs.MAX_RECENT);
            prP.save();
            rebuildRecentProjectsMenu();
        } catch (java.io.IOException ex) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "Error cargando proyecto:\n" + ex.getMessage(),
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * Reconstruye el árbol estilo NetBeans desde currentProject:
     *   <ProjectName> (root)
     *     ├─ Sources    (todos los .bp de sourceDir)
     *     ├─ Output     (todos los .mod/.bpi de outDir, si existen)
     *     └─ Dependencies (cada entrada de dependencies; si es dir, sus .mod/.bpi)
     */
    private void refreshProjectTree() {
        if (currentProject == null) return;
        String projName = currentProjectFile != null
                ? currentProjectFile.getFileName().toString().replaceFirst("\\.bpbuild$", "")
                : "Proyecto";
        javax.swing.tree.DefaultMutableTreeNode root =
                new javax.swing.tree.DefaultMutableTreeNode(new ProjectNode(projName, null));

        // Sources
        java.nio.file.Path srcDir = java.nio.file.Paths.get(currentProject.sourceDir);
        javax.swing.tree.DefaultMutableTreeNode sources =
                new javax.swing.tree.DefaultMutableTreeNode(new ProjectNode("Sources", null));
        addFilesUnder(sources, srcDir, ".bp");
        root.add(sources);

        // Output
        java.nio.file.Path outDir = java.nio.file.Paths.get(currentProject.outDir);
        javax.swing.tree.DefaultMutableTreeNode output =
                new javax.swing.tree.DefaultMutableTreeNode(new ProjectNode("Output", null));
        if (java.nio.file.Files.isDirectory(outDir)) {
            addFilesUnder(output, outDir, ".mod");
            addFilesUnder(output, outDir, ".bpi");
        }
        root.add(output);

        // Dependencies
        javax.swing.tree.DefaultMutableTreeNode deps =
                new javax.swing.tree.DefaultMutableTreeNode(new ProjectNode("Dependencies", null));
        for (String d : currentProject.dependencies) {
            java.nio.file.Path dp = java.nio.file.Paths.get(d);
            String label = dp.getFileName() != null ? dp.getFileName().toString() : d;
            javax.swing.tree.DefaultMutableTreeNode dn =
                    new javax.swing.tree.DefaultMutableTreeNode(new ProjectNode(label, null));
            if (java.nio.file.Files.isDirectory(dp)) {
                addFilesUnder(dn, dp, ".mod");
                addFilesUnder(dn, dp, ".bpi");
            }
            deps.add(dn);
        }
        root.add(deps);

        projectTreeModel.setRoot(root);
        // Expandir los nodos principales para que el usuario vea Sources/Output/Deps abiertos.
        for (int i = 0; i < projectTree.getRowCount(); i++) {
            projectTree.expandRow(i);
        }
    }

    /** Añade como hijos del nodo todos los ficheros del dir cuyo nombre acaba en ext.
     *  Sin recursión a subdirectorios — keep simple v1. */
    private void addFilesUnder(javax.swing.tree.DefaultMutableTreeNode parent,
                               java.nio.file.Path dir, String ext) {
        if (!java.nio.file.Files.isDirectory(dir)) return;
        try (java.util.stream.Stream<java.nio.file.Path> s = java.nio.file.Files.list(dir)) {
            s.filter(java.nio.file.Files::isRegularFile)
             .filter(p -> p.getFileName().toString().endsWith(ext))
             .sorted()
             .forEach(p -> {
                 // Solo hacemos clickable los .bp (los .mod/.bpi se abren con disasm,
                 // futura mejora).
                 boolean openable = p.getFileName().toString().endsWith(".bp");
                 parent.add(new javax.swing.tree.DefaultMutableTreeNode(
                         new ProjectNode(p.getFileName().toString(),
                                 openable ? p : null)));
             });
        } catch (java.io.IOException ignored) { }
    }

    /**
     * Crea un JTextPane nuevo con el syntax highlighter de BP y el caret
     * listener instalados. Usado por openFileInEditor() para cada tab
     * nuevo. El tab inicial también lo crea con este método.
     */
    private RSyntaxTextArea newEditorPane() {
        RSyntaxTextArea ed = new RSyntaxTextArea();
        ed.setSyntaxEditingStyle("text/bp");        // resaltado BP (BpTokenMaker)
        ed.setCodeFoldingEnabled(true);             // plegado (IDE-5); fold parser → siguiente paso
        ed.setFont(new Font("Consolas", Font.PLAIN, 13));
        ed.setTabSize(2);
        ed.setTabsEmulated(true);                   // tabs → espacios
        ed.setAntiAliasingEnabled(true);
        ed.setHighlightCurrentLine(true);
        ed.addCaretListener(e -> {
            // Solo actualizamos la status bar si este es el editor activo.
            if (ed == editorArea) updateCaretStatus();
        });
        return ed;
    }

    /**
     * Instala un componente custom en la pestaña del JTabbedPane con
     * label + botón "×". El click en el botón cierra el tab respectivo.
     * El componente del tab se identifica por su JScrollPane (no por
     * índice, que es inestable si el usuario reordena/cierra tabs).
     */
    private void installTabCloseButton(JScrollPane scroll, String title) {
        JLabel lbl = new JLabel(title);
        lbl.setBorder(BorderFactory.createEmptyBorder(0, 0, 0, 6));
        javax.swing.JButton close = new javax.swing.JButton("×");
        close.setFont(new Font("Dialog", Font.BOLD, 12));
        close.setMargin(new java.awt.Insets(0, 4, 0, 4));
        close.setFocusable(false);
        close.setBorder(BorderFactory.createEmptyBorder(0, 4, 0, 4));
        close.setContentAreaFilled(false);
        close.addActionListener(e -> {
            int idx = jTabbedPane1.indexOfComponent(scroll);
            if (idx >= 0) closeTabAt(idx);
        });
        javax.swing.JPanel pnl = new javax.swing.JPanel(
                new java.awt.FlowLayout(java.awt.FlowLayout.LEFT, 0, 0));
        pnl.setOpaque(false);
        pnl.add(lbl);
        pnl.add(close);
        int idx = jTabbedPane1.indexOfComponent(scroll);
        if (idx >= 0) jTabbedPane1.setTabComponentAt(idx, pnl);
    }

    /**
     * Sincroniza editorArea, currentFile, status bar y título de
     * ventana con el tab actualmente seleccionado. Lo llama el
     * ChangeListener cuando el usuario cambia de tab; también se
     * llama manualmente desde openFileInEditor() porque cuando
     * solo hay un tab y se "selecciona" el ya activo,
     * setSelectedComponent NO dispara el ChangeListener (no hay
     * cambio de índice). Síntoma de no llamarlo: primer Load no
     * actualiza currentFile y compile falla con "no se ha guardado".
     */
    private void refreshActiveTab() {
        java.awt.Component sel = jTabbedPane1.getSelectedComponent();
        OpenFile of = openFiles.get(sel);
        if (of != null) {
            editorArea = of.editor;
            currentFile = of.file;
            if (lblStatusFile != null) {
                lblStatusFile.setText(of.file != null ? of.file.toString() : "(sin guardar)");
            }
            setTitle("BpIde" + (of.file != null ? " — " + of.file : ""));
            updateCaretStatus();
        }
    }

    /** Cambia el título mostrado en el componente custom del tab. */
    private void setTabTitle(JScrollPane scroll, String title) {
        int idx = jTabbedPane1.indexOfComponent(scroll);
        if (idx < 0) return;
        java.awt.Component tabComp = jTabbedPane1.getTabComponentAt(idx);
        if (tabComp instanceof javax.swing.JPanel) {
            javax.swing.JPanel pnl = (javax.swing.JPanel) tabComp;
            if (pnl.getComponentCount() > 0 && pnl.getComponent(0) instanceof JLabel) {
                ((JLabel) pnl.getComponent(0)).setText(title);
            }
        }
        jTabbedPane1.setTitleAt(idx, title);  // fallback por si no hay tabComponent
    }

    /**
     * Cierra el tab en el índice indicado. Si es el último tab, lo
     * sustituye por uno nuevo vacío para no dejar el IDE sin editor
     * (mantiene jTabbedPane1 con al menos un tab visible).
     */
    private void closeTabAt(int idx) {
        if (idx < 0 || idx >= jTabbedPane1.getTabCount()) return;
        java.awt.Component comp = jTabbedPane1.getComponentAt(idx);
        openFiles.remove(comp);
        jTabbedPane1.remove(idx);

        // Si quedamos sin tabs, abrimos uno vacío para no dejar al usuario
        // sin editor donde escribir.
        if (jTabbedPane1.getTabCount() == 0) {
            RSyntaxTextArea ed = newEditorPane();
            JScrollPane sc = new RTextScrollPane(ed);
            jTabbedPane1.addTab("(sin abrir)", sc);
            openFiles.put(sc, new OpenFile(ed, sc, null));
            installTabCloseButton(sc, "(sin abrir)");
            jTabbedPane1.setSelectedComponent(sc);
        } else {
            // Selecciona el tab siguiente (o el anterior si era el último).
            int newIdx = Math.min(idx, jTabbedPane1.getTabCount() - 1);
            jTabbedPane1.setSelectedIndex(newIdx);
        }
    }

    /**
     * Abre el fichero indicado en un tab del editor.
     *
     * Política de tabs:
     *   1. Si el fichero YA está abierto en algún tab → selecciona ese
     *      tab y vuelve. No duplicamos buffers del mismo fichero.
     *   2. Si solo existe el tab inicial vacío "(sin abrir)" sin Path
     *      asignado → lo recicla cargándole el contenido. Útil para que
     *      el primer fichero que abras no acumule dos tabs.
     *   3. En otro caso → crea un tab nuevo con su propio JTextPane.
     */
    private void openFileInEditor(java.nio.file.Path file) {
        try {
            // 1) ¿ya abierto?
            for (java.util.Map.Entry<java.awt.Component, OpenFile> e : openFiles.entrySet()) {
                if (file.equals(e.getValue().file)) {
                    jTabbedPane1.setSelectedComponent(e.getKey());
                    refreshActiveTab();
                    return;
                }
            }
            String content = new String(java.nio.file.Files.readAllBytes(file),
                    java.nio.charset.StandardCharsets.UTF_8);

            // 2) ¿reciclar el tab inicial vacío?
            OpenFile recycle = null;
            if (jTabbedPane1.getTabCount() == 1) {
                OpenFile only = openFiles.get(jTabbedPane1.getComponentAt(0));
                if (only != null && only.file == null && only.editor.getText().isEmpty()) {
                    recycle = only;
                }
            }

            if (recycle != null) {
                recycle.file = file;
                recycle.editor.setText(content);
                recycle.editor.setCaretPosition(0);
                setTabTitle(recycle.scroll, file.getFileName().toString());
                jTabbedPane1.setSelectedComponent(recycle.scroll);
            } else {
                // 3) tab nuevo
                RSyntaxTextArea ed = newEditorPane();
                ed.setText(content);
                ed.setCaretPosition(0);
                JScrollPane sc = new RTextScrollPane(ed);
                String title = file.getFileName().toString();
                jTabbedPane1.addTab(title, sc);
                openFiles.put(sc, new OpenFile(ed, sc, file));
                installTabCloseButton(sc, title);
                jTabbedPane1.setSelectedComponent(sc);
            }
            // Sincroniza explícitamente editorArea/currentFile/etc con
            // el tab activo. Crítico para el caso "reciclar el tab
            // inicial": setSelectedComponent NO dispara ChangeListener
            // si el tab ya estaba seleccionado (es el único). Sin esto,
            // el primer Load no actualiza currentFile y compile falla.
            refreshActiveTab();
            // IDE-3 — registrar en "Recent Files".
            IdePrefs prF = IdePrefs.load();
            IdePrefs.pushRecent(prF.recentFiles, file.toAbsolutePath().toString(), IdePrefs.MAX_RECENT);
            prF.save();
            rebuildRecentFilesMenu();
        } catch (java.io.IOException ex) {
            javax.swing.JOptionPane.showMessageDialog(this,
                    "No se pudo abrir " + file + ": " + ex.getMessage(),
                    "Error", javax.swing.JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * Menú Help con un único item "Manual (F1)" que extrae el HTML
     * empotrado en el jar a un fichero temporal y lo abre con el
     * navegador del sistema (vía {@link Desktop#browse}). El recurso
     * vive en {@code /manual.html} dentro del jar.
     */
    private void setupHelp() {
        JMenu menuHelp = new JMenu("Help");
        JMenuItem miManual = mi("Manual", KeyEvent.VK_F1, 0, e -> showManual());
        menuHelp.add(miManual);
        jMenuBar1.add(menuHelp);
    }

    /** Path al fichero temporal extraído (cacheado tras la 1ª extracción). */
    private Path cachedManualPath = null;

    private void showManual() {
        try {
            if (cachedManualPath == null || !Files.isRegularFile(cachedManualPath)) {
                try (InputStream in = getClass().getResourceAsStream("/manual.html")) {
                    if (in == null) {
                        JOptionPane.showMessageDialog(this,
                                "manual.html no encontrado en los resources del jar.",
                                "Error", JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    Path tmpDir = Files.createTempDirectory("bpide-help-");
                    tmpDir.toFile().deleteOnExit();
                    cachedManualPath = tmpDir.resolve("manual.html");
                    Files.copy(in, cachedManualPath, StandardCopyOption.REPLACE_EXISTING);
                    cachedManualPath.toFile().deleteOnExit();
                }
            }
            if (Desktop.isDesktopSupported()
                    && Desktop.getDesktop().isSupported(Desktop.Action.BROWSE)) {
                Desktop.getDesktop().browse(cachedManualPath.toUri());
            } else {
                JOptionPane.showMessageDialog(this,
                        "Esta plataforma no soporta abrir URLs con el navegador.\n"
                                + "El manual está en:\n" + cachedManualPath,
                        "Manual", JOptionPane.INFORMATION_MESSAGE);
            }
        } catch (IOException ex) {
            JOptionPane.showMessageDialog(this,
                    "No se pudo abrir el manual: " + ex.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * Cablea menú "Debug" + sus paneles (Variables / Call Stack /
     * Breakpoints) y los handlers de teclas (F5/F9/F10/F11).
     */
    private void setupDebug() {
        // Menú Debug con sus items y atajos.
        JMenu menuDebug = new JMenu("Debug");
        JMenuItem miDebugRun    = mi("Debug",            KeyEvent.VK_F5,  KeyEvent.SHIFT_DOWN_MASK,  e -> doDebug());
        JMenuItem miDebugOnPico = new JMenuItem("Debug on Pico");
        miDebugOnPico.addActionListener(e -> doDebugOnPico());
        JMenuItem miContinue    = mi("Continue",         KeyEvent.VK_F5,  0,                         e -> debug.sendCommand(StepCommand.CONTINUE));
        JMenuItem miStepOver    = mi("Step Over",        KeyEvent.VK_F10, 0,                         e -> debug.sendCommand(StepCommand.STEP_OVER));
        JMenuItem miStepInto    = mi("Step Into",        KeyEvent.VK_F11, 0,                         e -> debug.sendCommand(StepCommand.STEP_INTO));
        JMenuItem miStepOut     = mi("Step Out",         KeyEvent.VK_F11, KeyEvent.SHIFT_DOWN_MASK,  e -> debug.sendCommand(StepCommand.STEP_OUT));
        JMenuItem miStop        = mi("Stop",             KeyEvent.VK_F5,  KeyEvent.CTRL_DOWN_MASK,   e -> debug.sendCommand(StepCommand.STOP));
        JMenuItem miToggleBP    = mi("Toggle Breakpoint",KeyEvent.VK_F9,  0,                         e -> onToggleBreakpoint());
        menuDebug.add(miDebugRun);
        menuDebug.add(miDebugOnPico);
        menuDebug.addSeparator();
        menuDebug.add(miContinue);
        menuDebug.add(miStepOver);
        menuDebug.add(miStepInto);
        menuDebug.add(miStepOut);
        menuDebug.add(miStop);
        menuDebug.addSeparator();
        menuDebug.add(miToggleBP);
        jMenuBar1.add(menuDebug);

        // Panels de depuración (3 nuevas pestañas en el panel inferior).
        localsModel = new LocalsTableModel();
        JTable localsTable = new JTable(localsModel);
        jTabbedPane2.addTab("Variables", new JScrollPane(localsTable));

        modulePropsModel = new ModulePropsTableModel();
        JTable modulePropsTable = new JTable(modulePropsModel);
        jTabbedPane2.addTab("Properties módulo", new JScrollPane(modulePropsTable));

        stackModel = new StackTableModel();
        JTable stackTable = new JTable(stackModel);
        jTabbedPane2.addTab("Call Stack", new JScrollPane(stackTable));

        JTable bpsTable = new JTable(new BreakpointsAdapter(debug.breakpoints()));
        jTabbedPane2.addTab("Breakpoints", new JScrollPane(bpsTable));
        bpsTable.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    int row = bpsTable.getSelectedRow();
                    if (row >= 0 && row < debug.breakpoints().size()) {
                        Breakpoint bp = debug.breakpoints().get(row);
                        jumpToLine(bp.line);
                    }
                }
            }
        });

        // Cuando se añade/quita un breakpoint, repinta los highlights.
        debug.breakpoints().addListener(new ObservableList.Listener<Breakpoint>() {
            @Override public void onAdded(int idx, Breakpoint b)   { refreshBreakpointHighlights(); }
            @Override public void onRemoved(int idx, Breakpoint b) { refreshBreakpointHighlights(); }
            @Override public void onCleared()                       { refreshBreakpointHighlights(); }
        });

        // Listener de la sesión (A1.9): los eventos llegan en el thread
        // reader del BpvmClient. Para paused, ya en ese thread hacemos las
        // queries de locals/frames/properties (es seguro bloquear ahí)
        // y luego invokeLater para pintar la UI con el snapshot completo.
        debug.addListener(e -> {
            if (e instanceof edu.bpgenvm.vm.debug.PausedEvent) {
                edu.bpgenvm.vm.debug.PausedEvent pe = (edu.bpgenvm.vm.debug.PausedEvent) e;
                // H6.b.3.b — 1ª pausa de una sesión DEVICE (pausa de entrada):
                // capturamos cs y aplicamos los breakpoints del editor como pc
                // (pc = relPc + cs). Si hay alguno, CONTINUE (run-to-breakpoint)
                // sin pintar la entrada; si no, caemos a mostrar la pausa inicial.
                if (deviceDbg != null && !deviceEntryHandled) {
                    deviceEntryHandled = true;
                    deviceCs = pe.cs;
                    int applied = applyDeviceBreakpoints(debug.client(), pe.cs);
                    if (applied > 0) {
                        debug.sendCommand(edu.bpgenvm.vm.debug.StepCommand.CONTINUE);
                        return;
                    }
                }
                int[] locals = debug.getLocals(2000);
                List<BpvmClient.NamedLocal> named0 = debug.getNamedLocals(2000);   // H6.a.1
                // H6.b.3.b — si el server es DEVICE-role (VM-C / Pico) no manda la
                // sección `named[]`; el host resuelve los nombres con el .dbg que
                // tiene (regla de oro de H6). Aditivo y gated: sólo cuando `named`
                // viene vacío y hay un .dbg de device cargado para esta sesión.
                final List<BpvmClient.NamedLocal> named =
                        named0.isEmpty() ? resolveDeviceNamedLocals(pe) : named0;
                List<int[]> frames = debug.getStackFrames(2000);
                // MODULE_PROPERTIES es extensión Java-only (la VM-C / Pico no la
                // soporta: el pause_cb del device responde UNSUPPORTED). La
                // pedimos SÓLO cuando el server manda `named[]` (= VM-Java); si
                // `named0` viene vacío es device-role → la saltamos para no
                // disparar el error "no valido en pausa".
                List<edu.bpgenvm.vm.ModuleManager.PropertyView> props =
                        named0.isEmpty()
                            ? java.util.Collections.emptyList()
                            : debug.getModuleProperties(2000);
                SwingUtilities.invokeLater(() -> onDebugPaused(pe, locals, named, frames, props));
            } else if (e instanceof edu.bpgenvm.vm.debug.ResumedEvent) {
                SwingUtilities.invokeLater(this::onDebugResumed);
            } else if (e instanceof edu.bpgenvm.vm.debug.ExitedEvent) {
                SwingUtilities.invokeLater(this::onDebugResumed);
            }
        });
    }

    /** H6.b.3.b — `.dbg` del módulo en depuración cuando la sesión es contra una
     *  VM DEVICE-role (VM-C host / Pico): el server no manda símbolos, así que el
     *  host los resuelve con esto. null = sesión Java-VM (manda `named[]`) o sin
     *  .dbg. Lo fija {@link #runOnVmRemote} al hacer attach y lo limpia al detach. */
    private volatile edu.bpgenvm.vm.debug.DbgFile deviceDbg;

    /** H6.b.3.b — `cs` (code start) del device, capturado del 1er BP_HIT de la
     *  sesión. Necesario para traducir línea→pc (pc = relPc + cs). */
    private volatile int deviceCs = 0;
    /** H6.b.3.b — true cuando ya se procesó la pausa de ENTRADA de la sesión
     *  device (capturado cs + aplicados los breakpoints del editor). */
    private volatile boolean deviceEntryHandled = false;

    /** H6.b.3.b — aplica los breakpoints del editor (del fichero en depuración)
     *  al device como SET_BP por pc = relPc + cs, traduciendo línea→relPc con el
     *  {@link #deviceDbg}. Devuelve cuántos se aplicaron. Single-module: sólo los
     *  breakpoints del .bp actual (multi-módulo necesitaría el .dbg+cs de cada uno). */
    private int applyDeviceBreakpoints(BpvmClient client, int cs) {
        if (deviceDbg == null || client == null) return 0;
        String fname = (currentFile != null) ? currentFile.getFileName().toString() : null;
        int n = 0;
        for (int i = 0; i < debug.breakpoints().size(); i++) {
            Breakpoint bp = debug.breakpoints().get(i);
            if (fname != null && !bp.file.equals(fname)) continue;
            int relPc = deviceDbg.relPcForLine(bp.line);
            if (relPc < 0) continue;
            try { client.setBreakpointPc(relPc + cs, 2000); n++; }
            catch (java.io.IOException ignored) {}
        }
        return n;
    }

    /** H6.b.3.b — resuelve los locales POR NOMBRE de un frame DEVICE-role usando
     *  el {@link #deviceDbg} que el host tiene, vía
     *  {@link BpvmClient#resolveDeviceFrame}. Convierte
     *  {@code DeviceFrameResolver.Local} → {@link BpvmClient.NamedLocal} para
     *  alimentar el MISMO modelo de la tabla de Variables. Devuelve [] si no hay
     *  sesión device, .dbg, o no resuelve (el caller cae al array crudo). */
    private List<BpvmClient.NamedLocal> resolveDeviceNamedLocals(
            edu.bpgenvm.vm.debug.PausedEvent pe) {
        edu.bpgenvm.vm.debug.DbgFile dbg = this.deviceDbg;
        BpvmClient c = debug.client();
        if (dbg == null || c == null) return java.util.Collections.emptyList();
        try {
            edu.bpgenvm.vm.debug.DeviceFrameResolver.Frame f =
                    c.resolveDeviceFrame(dbg, pe, 2000);
            if (f == null || f.locals.isEmpty()) return java.util.Collections.emptyList();
            List<BpvmClient.NamedLocal> out = new java.util.ArrayList<>(f.locals.size());
            for (edu.bpgenvm.vm.debug.DeviceFrameResolver.Local l : f.locals) {
                int size = ("long".equals(l.type) || "double".equals(l.type)) ? 8 : 4;
                out.add(new BpvmClient.NamedLocal(
                        l.name, l.value, size, /*isArray*/ false, l.offset, l.type, l.display));
            }
            return out;
        } catch (Throwable t) {
            return java.util.Collections.emptyList();
        }
    }

    /** Crea un JMenuItem con accelerator y listener. */
    private static JMenuItem mi(String text, int key, int modifiers,
                                java.awt.event.ActionListener al) {
        JMenuItem item = new JMenuItem(text);
        if (key != 0) item.setAccelerator(KeyStroke.getKeyStroke(key, modifiers));
        item.addActionListener(al);
        return item;
    }

    /** F9 / menú: alterna breakpoint en la línea donde está el caret. */
    private void onToggleBreakpoint() {
        if (currentFile == null) {
            JOptionPane.showMessageDialog(this,
                    "Guarda primero el fichero para poner breakpoints.",
                    "Aviso", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        int line = caretLine(editorArea);
        debug.toggleBreakpoint(currentFile.getFileName().toString(), line);
    }

    /** Devuelve la línea 1-based del caret en el editor. */
    private int caretLine(RSyntaxTextArea tp) {
        Element root = tp.getDocument().getDefaultRootElement();
        int caret = tp.getCaretPosition();
        return root.getElementIndex(caret) + 1;
    }

    /** Repinta los highlights de breakpoints (rojo claro) en el editor. */
    private void refreshBreakpointHighlights() {
        SwingUtilities.invokeLater(() -> {
            Highlighter hl = editorArea.getHighlighter();
            // Quitar viejos.
            for (Object tag : bpHighlights.values()) {
                hl.removeHighlight(tag);
            }
            bpHighlights.clear();
            // Reañadir.
            if (currentFile == null) return;
            String fname = currentFile.getFileName().toString();
            DefaultHighlighter.DefaultHighlightPainter painter =
                    new DefaultHighlighter.DefaultHighlightPainter(new Color(255, 200, 200));
            for (int i = 0; i < debug.breakpoints().size(); i++) {
                Breakpoint bp = debug.breakpoints().get(i);
                if (!bp.file.equals(fname)) continue;
                try {
                    Element root = editorArea.getDocument().getDefaultRootElement();
                    if (bp.line - 1 < 0 || bp.line - 1 >= root.getElementCount()) continue;
                    Element el = root.getElement(bp.line - 1);
                    Object tag = hl.addHighlight(el.getStartOffset(), el.getEndOffset(), painter);
                    bpHighlights.put(bp.line, tag);
                } catch (Exception ex) { /* ignore */ }
            }
        });
    }

    /** Llamado en EDT cuando la sesión pausa: actualiza paneles + highlight.
     *  Los datos (locals, frames, props) llegan precargados desde el thread
     *  reader del BpvmClient — aquí NO se hace I/O. */
    private void onDebugPaused(edu.bpgenvm.vm.debug.PausedEvent pe,
                               int[] locals,
                               List<BpvmClient.NamedLocal> named,
                               List<int[]> frames,
                               List<edu.bpgenvm.vm.ModuleManager.PropertyView> props) {
        // Highlight de línea actual (amarillo).
        Highlighter hl = editorArea.getHighlighter();
        if (pausedLineHighlight != null) {
            hl.removeHighlight(pausedLineHighlight);
            pausedLineHighlight = null;
        }
        if (pe.line > 0) {
            try {
                Element root = editorArea.getDocument().getDefaultRootElement();
                if (pe.line - 1 < root.getElementCount()) {
                    Element el = root.getElement(pe.line - 1);
                    DefaultHighlighter.DefaultHighlightPainter painter =
                            new DefaultHighlighter.DefaultHighlightPainter(new Color(255, 240, 150));
                    pausedLineHighlight = hl.addHighlight(el.getStartOffset(), el.getEndOffset(), painter);
                    editorArea.setCaretPosition(el.getStartOffset());
                }
            } catch (Exception ex) { /* ignore */ }
        }
        localsModel.update(locals, named);
        modulePropsModel.update(props);
        // Call Stack: hoy mostramos PC raw — describePc requiere el
        // ModuleManager in-process; cuando la VM corre como subproceso ya
        // no lo tenemos. Si en algún momento queremos labels legibles,
        // añadir un query "describePc(pc)" al wire.
        List<String> labels = new java.util.ArrayList<>(frames.size());
        for (int[] f : frames) {
            labels.add("PC=" + f[0]);
        }
        stackModel.update(frames, labels);
        // Saltar a la pestaña Variables para que se vea.
        int tabVars = findTabByTitlePrefix(jTabbedPane2, "Variables");
        if (tabVars >= 0) jTabbedPane2.setSelectedIndex(tabVars);
    }

    /** Llamado en EDT cuando la sesión reanuda: quita highlight de pausa. */
    private void onDebugResumed() {
        Highlighter hl = editorArea.getHighlighter();
        if (pausedLineHighlight != null) {
            hl.removeHighlight(pausedLineHighlight);
            pausedLineHighlight = null;
        }
        localsModel.update(new int[0], java.util.Collections.emptyList());
        modulePropsModel.update(java.util.Collections.emptyList());
        stackModel.update(java.util.Collections.emptyList(),
                          java.util.Collections.emptyList());
    }

    /** Mantenemos referencia al ModuleManager actual para describePc en la stack table. */
    private volatile edu.bpgenvm.vm.ModuleManager currentModuleManager = null;

    /**
     * Debug Run: similar a Run pero instala el DebugHook ANTES de
     * vm.run(). El hook bloquea en cada cambio de línea y la UI controla
     * los pasos.
     */
    private void doDebug() {
        if (editorArea.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this, "Editor vacío.",
                    "Aviso", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        onSave();
        if (currentFile == null) return;

        consolaArea.setText("");
        errors.clear();
        debug.reset();
        debuggingActive = true;

        // Modo proyecto: el .bp a compilar es <sourceDir>/<main>.bp y el
        // output va a <outDir>. Si no hay proyecto, comportamiento histórico
        // (compilar el fichero actualmente abierto y dejar el .mod en su lado).
        final Path bpFile;
        final Path outDir;
        if (currentProject != null) {
            bpFile = java.nio.file.Paths.get(currentProject.sourceDir, currentProject.main + ".bp");
            outDir = java.nio.file.Paths.get(currentProject.outDir);
        } else {
            bpFile = currentFile;
            outDir = bpFile.getParent().resolve("out");
        }

        new SwingWorker<Void, String>() {
            @Override
            protected Void doInBackground() {
                publish("== compilando " + bpFile.getFileName() + " ==\n");
                boolean ok = invokeWithCapture(() ->
                        basicplus.frontend.Main.compileFile(bpFile, outDir, "mivm"),
                        this::publish);
                if (!ok) {
                    publish("== compilación falló ==\n");
                    return null;
                }
                String moduleName = inferModuleName(bpFile);
                Path mod = outDir.resolve(moduleName + ".mod");
                if (!Files.isRegularFile(mod)) {
                    publish("== no se encontró " + mod.getFileName() + " ==\n");
                    return null;
                }
                publish("== debug " + mod.getFileName() + " ==\n");
                final String mainModName = moduleName + ".mod";

                // A2.5 — mismo flow que doRun pero adjuntando DebugSession:
                // la VM en daemon, recibe los .mod via upload, runModule
                // tras el handshake. La pausa inicial NO se libera —
                // el usuario controla con Continue/Step.
                runOnVmRemote(outDir, mainModName, /*pauseInitially=*/true, this::publish);
                publish("== fin debug ==\n");
                return null;
            }
            @Override
            protected void process(List<String> chunks) {
                for (String s : chunks) {
                    appendConsola(s);
                    parseAndAddError(s);
                }
            }
            @Override
            protected void done() {
                debuggingActive = false;
                onDebugResumed();
                currentModuleManager = null;
            }
        }.execute();
    }

    /**
     * Adapter ObservableList&lt;Breakpoint&gt; → TableModel para la pestaña
     * "Breakpoints". Read-only; columnas (Fichero, Línea).
     */
    private static final class BreakpointsAdapter extends AbstractTableModel
            implements ObservableList.Listener<Breakpoint> {
        private final ObservableList<Breakpoint> source;
        BreakpointsAdapter(ObservableList<Breakpoint> s) { this.source = s; s.addListener(this); }
        @Override public int getRowCount()       { return source.size(); }
        @Override public int getColumnCount()    { return 2; }
        @Override public String getColumnName(int c) { return new String[]{"Fichero", "Línea"}[c]; }
        @Override public boolean isCellEditable(int r, int c) { return false; }
        @Override public Class<?> getColumnClass(int c) {
            return c == 1 ? Integer.class : String.class;
        }
        @Override public Object getValueAt(int r, int c) {
            Breakpoint bp = source.get(r);
            return c == 0 ? bp.file : (Object) bp.line;
        }
        @Override public void onAdded(int i, Breakpoint b)   { SwingUtilities.invokeLater(() -> fireTableRowsInserted(i, i)); }
        @Override public void onRemoved(int i, Breakpoint b) { SwingUtilities.invokeLater(() -> fireTableRowsDeleted(i, i)); }
        @Override public void onCleared()                     { SwingUtilities.invokeLater(this::fireTableDataChanged); }
    }

    /**
     * Devuelve el directorio inicial que debe usar el JFileChooser:
     *   1. Carpeta del fichero del tab activo, si lo hay.
     *   2. Última carpeta usada (persistida en .bpide-prefs).
     *   3. null (deja que Swing use Documents — fallback).
     */
    private File initialChooserDir() {
        if (currentFile != null && currentFile.getParent() != null) {
            return currentFile.getParent().toFile();
        }
        IdePrefs prefs = IdePrefs.load();
        if (prefs.lastDir != null && !prefs.lastDir.isEmpty()) {
            File f = new File(prefs.lastDir);
            if (f.isDirectory()) return f;
        }
        return null;
    }

    /** Persiste la carpeta como last-used. Se llama tras open/save exitoso. */
    private void rememberLastDir(File parent) {
        if (parent == null || !parent.isDirectory()) return;
        IdePrefs prefs = IdePrefs.load();
        prefs.lastDir = parent.getAbsolutePath();
        prefs.save();
    }

    /**
     * File → Load: pide un .bp, lo abre en un tab (nuevo o reciclando
     * el "(sin abrir)" inicial si está vacío). Arranca el chooser en la
     * última carpeta usada para que el usuario no tenga que navegar.
     */
    private void onLoad() {
        JFileChooser fc = new JFileChooser();
        fc.setFileFilter(new FileNameExtensionFilter("BasicPlus (*.bp)", "bp"));
        File initial = initialChooserDir();
        if (initial != null) fc.setCurrentDirectory(initial);
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File chosen = fc.getSelectedFile();
        rememberLastDir(chosen.getParentFile());
        openFileInEditor(chosen.toPath());
    }

    /**
     * File → Save: escribe al fichero del tab activo. Si el tab no tiene
     * Path asociado (buffer nuevo sin guardar), pide uno con JFileChooser
     * y lo asocia al OpenFile del tab. Actualiza el título del tab.
     */
    private void onSave() {
        java.awt.Component sel = jTabbedPane1.getSelectedComponent();
        OpenFile of = openFiles.get(sel);
        if (of == null) return;   // no debería pasar — siempre hay tab activo

        if (of.file == null) {
            JFileChooser fc = new JFileChooser();
            fc.setFileFilter(new FileNameExtensionFilter("BasicPlus (*.bp)", "bp"));
            File initial = initialChooserDir();
            if (initial != null) fc.setCurrentDirectory(initial);
            if (fc.showSaveDialog(this) != JFileChooser.APPROVE_OPTION) return;
            File chosen = fc.getSelectedFile();
            if (!chosen.getName().toLowerCase().endsWith(".bp")) {
                chosen = new File(chosen.getAbsolutePath() + ".bp");
            }
            of.file = chosen.toPath();
            currentFile = of.file;
            rememberLastDir(chosen.getParentFile());
        }
        try {
            Files.write(of.file, of.editor.getText().getBytes(StandardCharsets.UTF_8));
            setTabTitle(of.scroll, of.file.getFileName().toString());
            setTitle("BpIde — " + of.file);
            if (lblStatusFile != null) lblStatusFile.setText(of.file.toString());
        } catch (IOException ex) {
            JOptionPane.showMessageDialog(this, "No se pudo guardar: " + ex.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * Run → Compile (execute=false) o Run → Run (execute=true).
     * Ejecuta in-process en un SwingWorker para no bloquear la UI. Captura
     * stdout/stderr del frontend y de la VM redirigiéndolos a un
     * {@link PrintStream} que vuelca línea a línea al panel "Consola"
     * (también parsea diagnósticos a la tabla "Errores").
     */
    private void doRun(boolean execute) {
        if (editorArea.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this, "Editor vacío. Carga o escribe un fichero primero.",
                    "Aviso", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        onSave();
        if (currentFile == null) return;

        consolaArea.setText("");
        errors.clear();

        // Modo proyecto (.bpbuild): compila <sourceDir>/<main>.bp a <outDir>.
        // Sin proyecto: compila el fichero abierto a <suDir>/out (comportamiento histórico).
        final Path bpFile;
        final Path outDir;
        if (currentProject != null) {
            bpFile = java.nio.file.Paths.get(currentProject.sourceDir, currentProject.main + ".bp");
            outDir = java.nio.file.Paths.get(currentProject.outDir);
        } else {
            bpFile = currentFile;
            outDir = bpFile.getParent().resolve("out");
        }

        new SwingWorker<Void, String>() {
            @Override
            protected Void doInBackground() {
                publish("== compilando " + bpFile.getFileName() + " ==\n");
                boolean ok = invokeWithCapture(() ->
                        basicplus.frontend.Main.compileFile(bpFile, outDir, "mivm"),
                        this::publish);
                if (!ok) {
                    publish("== compilación falló ==\n");
                    return null;
                }
                if (!execute) {
                    publish("== compilación OK ==\n");
                    return null;
                }
                String moduleName = inferModuleName(bpFile);
                Path mod = outDir.resolve(moduleName + ".mod");
                if (!Files.isRegularFile(mod)) {
                    publish("== no se encontró " + mod.getFileName() + " ==\n");
                    return null;
                }
                publish("== ejecutando " + mod.getFileName() + " ==\n");
                final String mainModName = moduleName + ".mod";

                // A2.5 — flow remote-friendly: workdir temporal, VM en modo
                // daemon, subir todos los .mod/.bpi/.dbg del outDir local
                // por el wire, runModule del main. El stdlib NO se sube:
                // se asume preinstalado en el dispositivo (--stdlibDir
                // del BpVM.cfg del lado VM).
                runOnVmRemote(outDir, mainModName, /*pauseInitially=*/false, this::publish);
                publish("== fin ==\n");
                return null;
            }
            @Override
            protected void process(List<String> chunks) {
                for (String s : chunks) {
                    appendConsola(s);
                    parseAndAddError(s);
                }
            }
        }.execute();
    }

    /**
     * FP4.c — Compile & Run on Pico: compila el .bp actual y delega el
     * upload + ejecución al panel PicoExplorer (que usa su conexión
     * USB CDC ya abierta). El output llega a la consola del IDE vía el
     * outputSink que enchufamos al panel en setupProjectTree().
     */
    private void doRunOnPico() {
        if (editorArea.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this,
                    "Editor vacío. Carga o escribe un fichero primero.",
                    "Aviso", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        if (picoExplorer == null || !picoExplorer.isConnected()) {
            JOptionPane.showMessageDialog(this,
                    "Conecta primero el dispositivo desde el panel Pico\n" +
                    "(ventana inferior izquierda → Connect).",
                    "Pico no conectado", JOptionPane.WARNING_MESSAGE);
            return;
        }
        onSave();
        if (currentFile == null) return;

        consolaArea.setText("");
        errors.clear();

        final Path bpFile;
        final Path outDir;
        if (currentProject != null) {
            bpFile = java.nio.file.Paths.get(currentProject.sourceDir,
                    currentProject.main + ".bp");
            outDir = java.nio.file.Paths.get(currentProject.outDir);
        } else {
            bpFile = currentFile;
            outDir = bpFile.getParent().resolve("out");
        }

        new SwingWorker<Boolean, String>() {
            Path modPath = null;
            @Override
            protected Boolean doInBackground() {
                publish("== compilando " + bpFile.getFileName() + " para Pico ==\n");
                boolean ok = invokeWithCapture(() ->
                        basicplus.frontend.Main.compileFile(bpFile, outDir, "mivm"),
                        this::publish);
                if (!ok) {
                    publish("== compilación falló ==\n");
                    return false;
                }
                String moduleName = inferModuleName(bpFile);
                modPath = outDir.resolve(moduleName + ".mod");
                if (!Files.isRegularFile(modPath)) {
                    publish("== no se encontró " + modPath.getFileName() + " ==\n");
                    return false;
                }
                publish("== compilación OK: " + modPath.getFileName() + " ==\n");
                return true;
            }
            @Override
            protected void process(List<String> chunks) {
                for (String s : chunks) {
                    appendConsola(s);
                    parseAndAddError(s);
                }
            }
            @Override
            protected void done() {
                try {
                    if (Boolean.TRUE.equals(get()) && modPath != null) {
                        // Resolver TODAS las deps (incl. stdlib core) a subir
                        // antes del main. Las stdlib core van a /lib, el resto
                        // a /app (lo decide uploadAndRun con este set).
                        java.util.List<java.io.File> deps =
                                resolveDeviceDeps(bpFile, outDir);
                        java.util.Set<String> libDeps = new java.util.HashSet<>();
                        for (java.io.File d : deps) {
                            String n = d.getName();
                            String mod = n.endsWith(".mod")
                                    ? n.substring(0, n.length() - 4) : n;
                            if (EMBEDDED_CORE_MODS.contains(mod)) libDeps.add(n);
                        }
                        if (!deps.isEmpty()) {
                            appendConsola("[deps] " + deps.size()
                                    + " módulo(s) a subir:\n");
                            for (java.io.File d : deps) {
                                appendConsola("  - " + d.getName()
                                        + (libDeps.contains(d.getName()) ? " → /lib" : " → /app")
                                        + "\n");
                            }
                        }
                        picoExplorer.uploadAndRun(modPath.toFile(), deps, libDeps);
                    }
                } catch (Exception ex) {
                    appendConsola("[ide] error: " + ex.getMessage() + "\n");
                }
            }
        }.execute();
    }

    /**
     * Debug on Pico — análogo a Run on Pico pero arrancando la VM del
     * firmware en modo debug, con breakpoints, step y locals contra el
     * source actual.
     *
     * Estado v1 (FINAL para v1, deferido a v2):
     *
     * El firmware ya habla wire v1 sobre USB CDC (#150 P-pico-wire-v1)
     * y la VM-C lleva el plumbing del debug hook en el inner loop
     * (#139 P-interp-debug-hook, no-op por default). Lo que NO tiene
     * v1 es el back-end real del debug sobre el Pico:
     *
     *   - [v2] #138 P-cdc-multiplex: separar el canal de debug del
     *     stdout del programa BP sobre el mismo USB CDC.
     *   - [v2] #140 P-debug-pico-impl: cablear DebugController/Server
     *     sobre el wire v1 del firmware y reportar capability DEBUG
     *     en HELLO_REPLY.
     *
     * Decisión de v1: el workflow combinado Shift+F5 (Debug Run en
     * VM-Java local con breakpoints, step, locals, frames) + "Run on
     * Pico" (ejecutar lógica ya verificada sobre el HW real) cubre
     * el 95% del valor de "debug-on-Pico". Bugs que sólo se
     * manifiesten sobre HW se cazan con `print` instrumentado.
     *
     * Cuando v2 traiga #140, este método se sustituye por la llamada
     * análoga a doDebug() — el transporte ya existe (SerialBackend
     * sobre BpvmClient.connectSerial), sólo hay que dejar de
     * cortocircuitar y pasar `waitClient=true`.
     */
    private void doDebugOnPico() {
        if (editorArea.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this,
                    "Editor vacío. Carga o escribe un fichero primero.",
                    "Aviso", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        if (picoExplorer == null || !picoExplorer.isConnected()) {
            JOptionPane.showMessageDialog(this,
                    "Conecta primero el dispositivo desde el panel Pico\n" +
                    "(ventana inferior izquierda → Connect).",
                    "Pico no conectado", JOptionPane.WARNING_MESSAGE);
            return;
        }
        onSave();
        if (currentFile == null) return;

        consolaArea.setText("");
        errors.clear();

        final Path bpFile;
        final Path outDir;
        if (currentProject != null) {
            bpFile = java.nio.file.Paths.get(currentProject.sourceDir,
                    currentProject.main + ".bp");
            outDir = java.nio.file.Paths.get(currentProject.outDir);
        } else {
            bpFile = currentFile;
            outDir = bpFile.getParent().resolve("out");
        }

        new SwingWorker<Boolean, String>() {
            Path modPath = null;
            String base = null;
            @Override
            protected Boolean doInBackground() {
                publish("== compilando " + bpFile.getFileName() + " para Debug on Pico ==\n");
                boolean ok = invokeWithCapture(() ->
                        basicplus.frontend.Main.compileFile(bpFile, outDir, "mivm"),
                        this::publish);
                if (!ok) { publish("== compilación falló ==\n"); return false; }
                base = inferModuleName(bpFile);
                modPath = outDir.resolve(base + ".mod");
                if (!Files.isRegularFile(modPath)) {
                    publish("== no se encontró " + modPath.getFileName() + " ==\n");
                    return false;
                }
                publish("== compilación OK: " + modPath.getFileName() + " ==\n");
                return true;
            }
            @Override
            protected void process(List<String> chunks) {
                for (String s : chunks) { appendConsola(s); parseAndAddError(s); }
            }
            @Override
            protected void done() {
                try {
                    if (!Boolean.TRUE.equals(get()) || modPath == null) return;
                    // .dbg para resolver locales por nombre en el device (regla de oro H6).
                    deviceDbg = edu.bpgenvm.vm.debug.DbgFile.load(
                            outDir.resolve(base + ".dbg").toString());
                    deviceEntryHandled = false;   // nueva sesión: re-procesar la pausa de entrada
                    deviceCs = 0;
                    if (deviceDbg == null) {
                        appendConsola("[debug] aviso: no hay " + base + ".dbg; locales saldrán crudos\n");
                    }
                    // Deps (igual que Run on Pico): stdlib core → /lib, resto → /app.
                    java.util.List<java.io.File> deps = resolveDeviceDeps(bpFile, outDir);
                    java.util.Set<String> libDeps = new java.util.HashSet<>();
                    for (java.io.File d : deps) {
                        String n = d.getName();
                        String mod = n.endsWith(".mod") ? n.substring(0, n.length() - 4) : n;
                        if (EMBEDDED_CORE_MODS.contains(mod)) libDeps.add(n);
                    }
                    final String remoteMain = "/app/" + base + ".mod";
                    appendConsola("[debug] subiendo + enganchando sesión de debug en la Pico...\n");
                    // uploadAndRun con debugHook: sube los ficheros y, en vez de
                    // ejecutar en bloqueante, cede el client serie ya conectado.
                    picoExplorer.uploadAndRun(modPath.toFile(), deps, libDeps, client -> {
                        if (client == null) {
                            appendConsola("[debug] no se pudo obtener el client serie\n");
                            return;
                        }
                        debug.attach(client);            // el listener (onDebugPaused) ya está puesto
                        client.requestPause();           // rompe en el 1er opcode → BP_HIT(cs)
                        client.runModule(remoteMain);    // arranca; cada pausa pinta locales por nombre
                        appendConsola("[debug] sesión activa. Breakpoints del editor → run-to-breakpoint;"
                                + " si no hay, pausa en la entrada. Continue/Step en el menú Debug.\n");
                    });
                } catch (Exception ex) {
                    appendConsola("[debug] error: " + ex.getMessage() + "\n");
                }
            }
        }.execute();
    }

    /** Nombres de módulos stdlib core que ya están pre-instalados en el
     *  firmware. No se suben — están allí desde el boot.
     *
     *  Política: cuando se modifica la API de un módulo stdlib (p.ej.
     *  añadiéndole una clase pública), la versión empotrada en flash
     *  queda obsoleta y al hacer Run on Pico aparece:
     *
     *    [bpvm-c link] símbolo no resuelto: 'X.__cls_new_<Cls>'
     *
     *  Soluciones:
     *    a) Sacar el módulo de esta lista temporalmente para que el IDE
     *       lo suba fresco en cada run (sobreescribe el del FS). Lento
     *       pero permite iterar sin reflashear.
     *    b) Regenerar el array empotrado con `xxd -i -n X_mod X.mod >
     *       bpgenvm-c/pico/X_mod.c`, recompilar el firmware
     *       (`bpgenvm-c/pico/build/ ninja`) y reflashear el .uf2.
     *       Después puede volver a entrar en esta lista.
     *
     *  Gpio entró aquí (con la clase Pin) tras reflashear el firmware
     *  con el array empotrado actualizado el 23-may-2026. */
    private static final java.util.Set<String> EMBEDDED_CORE_MODS =
            new java.util.HashSet<>(java.util.Arrays.asList(
                    "Math", "IO", "Gpio", "I2c", "Spi", "Uart",
                    "Pulse", "Pwm", "Pico", "Rtc", "Adc", "Wdt", "Timer"
            ));

    /**
     * Resuelve los drivers de dispositivo necesarios para ejecutar
     * {@code bpFile}. Parsea las líneas `import X` del fuente, filtra
     * los stdlib core, y busca el `.mod` correspondiente en:
     *   1) outDir (si el frontend lo emitió como dep)
     *   2) {bpdevices/} via devicesDir del BpVM.cfg
     *   3) {bpstdlib/} via stdlibDir (fallback)
     *   4) directorio del bpFile
     * Devuelve lista de .mod existentes a subir, en orden de
     * descubrimiento. (No recursivo en deps-de-deps para v1.)
     */
    private java.util.List<java.io.File> resolveDeviceDeps(Path bpFile, Path outDir) {
        java.util.List<java.io.File> result = new java.util.ArrayList<>();
        java.util.Set<String> imports = parseImports(bpFile);
        if (imports.isEmpty()) return result;

        java.util.List<Path> searchDirs = new java.util.ArrayList<>();
        if (outDir != null) searchDirs.add(outDir);
        try {
            edu.bpgenvm.config.VmConfig cfg =
                    edu.bpgenvm.config.VmConfig.loadDefaultFor(bpFile);
            if (cfg.devicesDir != null && !cfg.devicesDir.isEmpty())
                searchDirs.add(java.nio.file.Paths.get(cfg.devicesDir));
            if (cfg.stdlibDir != null && !cfg.stdlibDir.isEmpty())
                searchDirs.add(java.nio.file.Paths.get(cfg.stdlibDir));
        } catch (Throwable ignored) { }
        if (bpFile.getParent() != null) searchDirs.add(bpFile.getParent());

        // Resolvemos TODOS los imports, incluidos los stdlib core. Antes
        // se saltaban asumiéndolos embebidos en el firmware; pero eso solo
        // vale para la Pico. En dispositivos que NO embeben stdlib (ESP32)
        // hay que subirlos. La decisión final de subir o no la toma
        // uploadAndRun según lo que YA exista en el device (/lib = stdlib
        // embebida → no se pisa; ausente → se sube a /app). Así funciona
        // para ambos targets sin asumir nada. (EMBEDDED_CORE_MODS se queda
        // como referencia documental de qué mods suele traer un firmware.)
        for (String imp : imports) {
            for (Path dir : searchDirs) {
                Path candidate = dir.resolve(imp + ".mod");
                if (Files.isRegularFile(candidate)) {
                    result.add(candidate.toFile());
                    break;
                }
            }
        }
        return result;
    }

    /** Parser ligero de imports del .bp. Devuelve los nombres a la
     *  derecha de `import` (sin alias, sin `from`). */
    private static java.util.Set<String> parseImports(Path bpFile) {
        java.util.LinkedHashSet<String> out = new java.util.LinkedHashSet<>();
        try {
            java.util.List<String> lines = Files.readAllLines(bpFile,
                    java.nio.charset.StandardCharsets.UTF_8);
            java.util.regex.Pattern pat = java.util.regex.Pattern.compile(
                    "^\\s*import\\s+([A-Za-z_][A-Za-z0-9_]*)");
            for (String line : lines) {
                java.util.regex.Matcher m = pat.matcher(line);
                if (m.find()) out.add(m.group(1));
            }
        } catch (Exception ignored) { }
        return out;
    }

    /**
     * A2.5 — Ejecuta un módulo en una VM REMOTE-FRIENDLY:
     *   1) Crea un workdir temporal local.
     *   2) Lanza la VM en modo daemon con `--workdir <tmp>`.
     *   3) Sube TODOS los .mod / .bpi / .dbg que el outDir local
     *      contenga (la app del usuario y sus dependencias compiladas).
     *      El stdlib NO se sube: vive preinstalado en el dispositivo.
     *   4) Si {@code pauseInitially} es true, hace {@code debug.attach}
     *      ANTES del runModule, así la sesión de debug recibe el primer
     *      PausedEvent. Si es false (Run), libera el paused inicial
     *      automáticamente.
     *   5) Manda {@code runModule(mainModName)}, espera al ExitedEvent
     *      / fin del subproceso.
     *   6) Limpia el workdir.
     */
    private void runOnVmRemote(Path outDir, String mainModName,
                               boolean pauseInitially,
                               java.util.function.Consumer<String> publish) {
        Path workdir = null;
        try {
            workdir = Files.createTempDirectory("bpide-vm-");
            publish.accept("[ide] workdir temporal: " + workdir + "\n");

            try (BpvmClient client = new BpvmClient()) {
                client.setDiagSink(s -> publish.accept("[vm] " + s + "\n"));
                client.setOutputSink(publish::accept);
                // N20 — IO.prompt(spec) del programa BP abre un JDialog
                // construido a partir del spec JSON. Las respuestas
                // serializadas regresan al thread BP bloqueado.
                client.setPromptHandler((reqId, spec) -> {
                    final String[] resp = {"{}"};
                    try {
                        SwingUtilities.invokeAndWait(() -> {
                            resp[0] = PromptDialog.showAndCollect(FrmMain.this, spec);
                        });
                    } catch (Throwable t) {
                        publish.accept("[ide] PromptDialog falló: " + t.getMessage() + "\n");
                    }
                    client.respondToPrompt(reqId, resp[0]);
                });

                if (pauseInitially) {
                    // Debug: dejamos que la sesión gestione todo
                    // (incluido el primer paused) tras attach().
                    // No ponemos listener aquí; debug.attach pone el suyo.
                } else {
                    // Run: auto-continue al primer paused.
                    client.setEventListener(ev -> {
                        if (ev instanceof edu.bpgenvm.vm.debug.PausedEvent) {
                            client.sendCommand(edu.bpgenvm.vm.debug.StepCommand.CONTINUE);
                        } else if (ev instanceof edu.bpgenvm.vm.debug.ExceptionEvent) {
                            edu.bpgenvm.vm.debug.ExceptionEvent e =
                                    (edu.bpgenvm.vm.debug.ExceptionEvent) ev;
                            publish.accept("[exception tid=" + e.tid + "] " + e.message + "\n");
                        }
                    });
                }

                // A2.6 — si el usuario configuró un endpoint remoto en las
                // prefs del IDE, NO lanzamos VM local: nos conectamos al
                // daemon que vive en el dispositivo. El workdir local sigue
                // creándose pero queda vacío (los .mod viajan por el wire,
                // como en el flow local).
                IdePrefs prefs = IdePrefs.load();
                if (prefs.vmHost != null && !prefs.vmHost.isEmpty()) {
                    publish.accept("[ide] conectando a VM remota "
                            + prefs.vmHost + ":" + prefs.vmPort + "\n");
                    client.connectRemote(prefs.vmHost, prefs.vmPort);
                } else {
                    // Resolvemos stdlibDir desde BpVM.cfg para propagarlo al
                    // subproceso VM. El cwd de la VM normalmente coincidirá
                    // con el del IDE (sin BpVM.cfg si el IDE lo lanza
                    // NetBeans/etc.), así que caminamos hacia arriba desde
                    // el .mod del usuario — análogo al fix del frontend.
                    String stdlibDir = resolveStdlibDir(outDir);
                    if (stdlibDir != null) {
                        publish.accept("[ide] stdlibDir → " + stdlibDir + "\n");
                    }
                    client.startDaemon(workdir.toString(), /*waitClient=*/true, stdlibDir);
                }

                // Sube los artefactos de la app compilada.
                int nUploaded = uploadAppArtifacts(client, outDir, publish);
                publish.accept("[ide] subidos " + nUploaded + " ficheros\n");

                if (pauseInitially) {
                    // H6.b.3.b — precarga el .dbg del módulo principal (host-side)
                    // por si el server es device-role (VM-C / Pico) y no manda la
                    // sección named[]: entonces resolveDeviceNamedLocals lo usa.
                    // Inocuo en sesiones Java-VM (named[] no viene vacío).
                    String base = mainModName.endsWith(".mod")
                            ? mainModName.substring(0, mainModName.length() - 4) : mainModName;
                    this.deviceDbg = edu.bpgenvm.vm.debug.DbgFile.load(
                            outDir.resolve(base + ".dbg").toString());
                    debug.attach(client);
                }
                try {
                    client.runModule(mainModName);
                    client.waitForExit();
                } finally {
                    if (pauseInitially) { debug.detach(); this.deviceDbg = null; }
                }
            } catch (Throwable t) {
                publish.accept("[VM error] " + t.getMessage() + "\n");
            }
        } catch (java.io.IOException ie) {
            publish.accept("[IDE error creando workdir] " + ie.getMessage() + "\n");
        } finally {
            if (workdir != null) {
                deleteRecursively(workdir);
            }
        }
    }

    /** Resuelve stdlibDir desde BpVM.cfg. Mira en este orden:
     *   1) Si hay currentProject: project.outDir → walk up.
     *   2) cwd del proceso IDE.
     *   3) outDir del compile → walk up.
     *  Devuelve null si no encuentra cfg o si no define stdlibDir.
     *  Usado para propagar el dir al subproceso VM via --stdlibDir. */
    private String resolveStdlibDir(Path outDirHint) {
        try {
            // 1) cwd primero (compatibilidad histórica)
            edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(null);
            if (cfg.sourcePath != null && cfg.stdlibDir != null && !cfg.stdlibDir.isEmpty()) {
                return cfg.stdlibDir;
            }
            // 2) caminamos desde outDirHint si la tenemos
            if (outDirHint != null) {
                cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(outDirHint.toAbsolutePath());
                if (cfg.sourcePath != null && cfg.stdlibDir != null && !cfg.stdlibDir.isEmpty()) {
                    return cfg.stdlibDir;
                }
            }
            // 3) si hay proyecto activo, intentamos desde su outDir
            if (currentProject != null && currentProject.outDir != null) {
                cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(
                        java.nio.file.Paths.get(currentProject.outDir).toAbsolutePath());
                if (cfg.sourcePath != null && cfg.stdlibDir != null && !cfg.stdlibDir.isEmpty()) {
                    return cfg.stdlibDir;
                }
            }
        } catch (Throwable ignored) { /* sin cfg, devuelve null */ }
        return null;
    }

    /** Sube .mod/.bpi/.dbg del outDir local a la raíz del workdir del BpvmClient.
     *  No recurre: asume que outDir está plano (lo está hoy).
     *
     *  Filtro importante: NO sube ningún artefacto cuyo basename coincida
     *  con un módulo de la stdlib core. Esos los resuelve la VM por su
     *  cuenta vía --stdlibDir (que ya recibe). Subir una copia obsoleta
     *  desde outDir (residuos de compilaciones anteriores cuando la
     *  stdlib aún no había crecido con clases) hace que la VM use el
     *  fichero del workdir y falle con
     *  "Símbolo no resuelto: X.__cls_new_<Cls>". */
    private static final java.util.Set<String> STDLIB_BASENAMES =
            new java.util.HashSet<>(java.util.Arrays.asList(
                    "Math", "IO", "Gpio", "I2c", "Spi", "Uart",
                    "Pulse", "Pwm", "Pico", "Rtc", "Adc", "Wdt", "Timer",
                    "Json", "L2Lib"
            ));

    private int uploadAppArtifacts(BpvmClient client, Path outDir,
                                   java.util.function.Consumer<String> publish) throws java.io.IOException {
        if (!Files.isDirectory(outDir)) return 0;
        int n = 0;
        int skipped = 0;
        try (java.util.stream.Stream<Path> s = Files.list(outDir)) {
            java.util.List<Path> files = s
                    .filter(p -> {
                        String name = p.getFileName().toString().toLowerCase();
                        return name.endsWith(".mod") || name.endsWith(".bpi") || name.endsWith(".dbg");
                    })
                    .sorted()
                    .collect(java.util.stream.Collectors.toList());
            for (Path p : files) {
                String name = p.getFileName().toString();
                int dot = name.lastIndexOf('.');
                String base = (dot > 0) ? name.substring(0, dot) : name;
                if (STDLIB_BASENAMES.contains(base)) {
                    skipped++;
                    continue;   // la VM lo resuelve desde --stdlibDir
                }
                try {
                    int size = client.uploadFile(p, name, 10_000);
                    publish.accept("[ide] up " + name + " (" + size + " bytes)\n");
                    n++;
                } catch (java.io.IOException ue) {
                    publish.accept("[ide] upload " + name + " falló: " + ue.getMessage() + "\n");
                }
            }
        }
        if (skipped > 0) {
            publish.accept("[ide] omitidos " + skipped
                    + " artefactos stdlib (los resuelve la VM via --stdlibDir)\n");
        }
        return n;
    }

    private static void deleteRecursively(Path root) {
        if (!Files.exists(root)) return;
        try {
            Files.walk(root)
                    .sorted(java.util.Comparator.reverseOrder())
                    .forEach(p -> {
                        try { Files.deleteIfExists(p); }
                        catch (java.io.IOException ignored) {}
                    });
        } catch (java.io.IOException ignored) {}
    }

    /**
     * Ejecuta {@code action} con System.out/err redirigidos a un buffer.
     * Cada línea producida se entrega a {@code onLine} en streaming (no se
     * espera al final). Restaura System.out/err al terminar.
     *
     * @return el valor devuelto por la acción (true/false en nuestro caso),
     *         o false si lanzó excepción.
     */
    private boolean invokeWithCapture(java.util.concurrent.Callable<Boolean> action,
                                      java.util.function.Consumer<String> onLine) {
        PrintStream origOut = System.out;
        PrintStream origErr = System.err;
        LineCapturingStream cap = new LineCapturingStream(onLine);
        PrintStream ps;
        try {
            ps = new PrintStream(cap, true, "UTF-8");
        } catch (java.io.UnsupportedEncodingException ue) {
            ps = new PrintStream(cap, true);  // imposible: UTF-8 siempre disponible
        }
        System.setOut(ps);
        System.setErr(ps);
        try {
            Boolean r = action.call();
            cap.flushPartial();
            return Boolean.TRUE.equals(r);
        } catch (Throwable t) {
            ps.println("[exception] " + t.getClass().getSimpleName() + ": " + t.getMessage());
            cap.flushPartial();
            return false;
        } finally {
            System.setOut(origOut);
            System.setErr(origErr);
        }
    }

    /**
     * OutputStream que junta bytes en líneas y llama a {@code onLine} al
     * encontrar '\n'. Si al final del proceso quedó un buffer parcial,
     * {@link #flushPartial()} lo descarga.
     */
    private static final class LineCapturingStream extends OutputStream {
        private final ByteArrayOutputStream buf = new ByteArrayOutputStream();
        private final java.util.function.Consumer<String> onLine;
        LineCapturingStream(java.util.function.Consumer<String> onLine) {
            this.onLine = onLine;
        }
        @Override public synchronized void write(int b) {
            buf.write(b);
            if (b == '\n') {
                onLine.accept(new String(buf.toByteArray(), StandardCharsets.UTF_8));
                buf.reset();
            }
        }
        synchronized void flushPartial() {
            if (buf.size() > 0) {
                onLine.accept(new String(buf.toByteArray(), StandardCharsets.UTF_8) + "\n");
                buf.reset();
            }
        }
    }

    /** Append thread-safe a la consola. */
    private void appendConsola(String s) {
        SwingUtilities.invokeLater(() -> {
            consolaArea.append(s);
            consolaArea.setCaretPosition(consolaArea.getDocument().getLength());
        });
    }

    /**
     * Si la línea parece un diagnóstico del frontend, construye un
     * {@link CompileError} y lo añade a la {@link ObservableList} —
     * el {@link ErrorTableModel} bindeado refresca la tabla
     * automáticamente.
     */
    private void parseAndAddError(String line) {
        Matcher m = ERROR_RE.matcher(line);
        if (!m.find()) return;
        int ln       = Integer.parseInt(m.group(1));
        int col      = Integer.parseInt(m.group(2));
        String kind  = m.group(3);            // "error" / "aviso"
        String cat   = m.group(4);            // "sintáctico" / "semántico"
        String msg   = m.group(5).trim();
        String fname = currentFile != null ? currentFile.getFileName().toString() : "?";
        CompileError ce = new CompileError(fname, ln, col, kind, cat, msg);
        SwingUtilities.invokeLater(() -> errors.add(ce));
    }

    /**
     * Lee el .bp y busca la declaración {@code module Nombre} para saber
     * cómo se llamará el .mod. Fallback al basename del fichero.
     */
    private String inferModuleName(Path bp) {
        try {
            for (String line : Files.readAllLines(bp, StandardCharsets.UTF_8)) {
                String t = line.trim();
                if (t.isEmpty() || t.startsWith("//")) continue;
                if (t.startsWith("library")) continue;
                if (t.startsWith("module")) {
                    String rest = t.substring("module".length()).trim();
                    int end = 0;
                    while (end < rest.length()
                            && (Character.isJavaIdentifierPart(rest.charAt(end))
                                || rest.charAt(end) == '.')) end++;
                    if (end > 0) return rest.substring(0, end);
                }
            }
        } catch (IOException ignored) { }
        String fn = bp.getFileName().toString();
        int dot = fn.lastIndexOf('.');
        return (dot > 0) ? fn.substring(0, dot) : fn;
    }

    /**
     * Configura la barra de estado (jPanel1 al PAGE_END del form). La
     * dejamos con BorderLayout para colocar fichero a la izquierda y
     * posición del caret a la derecha. NetBeans había puesto un
     * GroupLayout placeholder; lo sobreescribimos.
     */
    private void setupStatusBar() {
        lblStatusFile  = new JLabel("(sin fichero)");
        lblStatusCaret = new JLabel("Línea 1, Col 1");
        lblStatusFile.setBorder(BorderFactory.createEmptyBorder(0, 8, 0, 8));
        lblStatusCaret.setBorder(BorderFactory.createEmptyBorder(0, 8, 0, 8));
        // Una fuente algo más pequeña; el JTextPane usa Consolas 13.
        Font smaller = new Font(Font.SANS_SERIF, Font.PLAIN, 11);
        lblStatusFile.setFont(smaller);
        lblStatusCaret.setFont(smaller);

        jPanel1.setLayout(new BorderLayout());
        jPanel1.setBorder(BorderFactory.createMatteBorder(1, 0, 0, 0, Color.GRAY));
        jPanel1.setPreferredSize(new java.awt.Dimension(0, 22));
        jPanel1.add(lblStatusFile,  BorderLayout.WEST);
        jPanel1.add(lblStatusCaret, BorderLayout.EAST);
    }

    /**
     * Refresca el label de "Línea N, Col M" a partir del caret actual.
     * Se llama desde el CaretListener (ya en EDT — addCaretListener
     * dispara siempre en EDT).
     */
    private void updateCaretStatus() {
        if (lblStatusCaret == null) return;
        Element root = editorArea.getDocument().getDefaultRootElement();
        int caret = editorArea.getCaretPosition();
        int lineIdx = root.getElementIndex(caret);
        Element el = root.getElement(lineIdx);
        int col = caret - el.getStartOffset() + 1;
        lblStatusCaret.setText("Línea " + (lineIdx + 1) + ", Col " + col);
    }

    /**
     * Busca el índice de la pestaña cuyo título empieza por {@code prefix}.
     * Devuelve -1 si no se encuentra. Robusto a que más adelante alguien
     * cambie "Errores" por "Errores (3)" etc.
     */
    private int findTabByTitlePrefix(javax.swing.JTabbedPane tabs, String prefix) {
        for (int i = 0; i < tabs.getTabCount(); i++) {
            String t = tabs.getTitleAt(i);
            if (t != null && t.startsWith(prefix)) return i;
        }
        return -1;
    }

    /**
     * Refleja el conteo actual de la {@link ObservableList} en el título
     * de la pestaña: "Errores" si vacía, "Errores (N)" si tiene N. Llamado
     * desde el listener tras add/remove/clear.
     */
    private void updateErroresTabTitle(int tabIdx) {
        if (tabIdx < 0) return;
        int n = errors.size();
        String title = (n == 0) ? "Errores" : ("Errores (" + n + ")");
        jTabbedPane2.setTitleAt(tabIdx, title);
    }

    /**
     * Mueve el caret del editor a la línea indicada (1-based). En
     * {@link JTextPane} la API es vía el root element del documento
     * (no hay {@code getLineStartOffset} como en JTextArea).
     */
    private void jumpToLine(int line) {
        Element root = editorArea.getDocument().getDefaultRootElement();
        int idx = Math.min(Math.max(0, line - 1), root.getElementCount() - 1);
        if (idx < 0) return;
        Element el = root.getElement(idx);
        editorArea.setCaretPosition(el.getStartOffset());
        editorArea.requestFocusInWindow();
    }

    // ==================== Debug: panels y models ====================

    /** Modelo: slots locales del frame actual mostrados como i32 crudos. */
    private static final class LocalsTableModel extends AbstractTableModel {
        private int[] values = new int[0];                       // crudo (fallback)
        private java.util.List<BpvmClient.NamedLocal> named =     // H6.a.1
                java.util.Collections.emptyList();
        private boolean hasNames() { return !named.isEmpty(); }

        @Override public int getRowCount() { return hasNames() ? named.size() : values.length; }
        @Override public int getColumnCount() { return hasNames() ? 4 : 3; }
        @Override public String getColumnName(int c) {
            return hasNames()
                ? new String[]{"Nombre", "Tipo", "Valor", "Offset (bp+)"}[c]
                : new String[]{"Slot", "Offset (bp+)", "Valor (i32)"}[c];
        }
        @Override public boolean isCellEditable(int r, int c) { return false; }
        @Override public Class<?> getColumnClass(int c) { return Object.class; }
        @Override public Object getValueAt(int r, int c) {
            if (hasNames()) {
                BpvmClient.NamedLocal nl = named.get(r);
                switch (c) {
                    case 0: return nl.name;
                    case 1: return nl.type;
                    case 2: return nl.display;   // H6.a.2: ya renderizado por tipo en la VM
                    case 3: return nl.offset;
                    default: return null;
                }
            }
            switch (c) {
                case 0: return r;
                case 1: return r * 4;
                case 2: return values[r];
                default: return null;
            }
        }
        /** H6.a.1: si `named` no está vacío, se muestra por nombre; si no, cae
         *  al array crudo `vs` (módulos sin .dbg v3). */
        void update(int[] vs, java.util.List<BpvmClient.NamedLocal> nm) {
            this.values = (vs != null) ? vs : new int[0];
            this.named  = (nm != null) ? nm : java.util.Collections.emptyList();
            fireTableDataChanged();
        }
    }

    /**
     * Modelo: properties públicas de todos los módulos cargados (N11).
     * El DebugContext invoca a `ModuleManager.snapshotAllProperties` que
     * lee directamente el data block — no ejecuta bytecode, evitando
     * side-effects y posibles deadlocks de sync property.
     */
    private static final class ModulePropsTableModel extends AbstractTableModel {
        private java.util.List<edu.bpgenvm.vm.ModuleManager.PropertyView> rows =
                java.util.Collections.emptyList();
        @Override public int getRowCount() { return rows.size(); }
        @Override public int getColumnCount() { return 4; }
        @Override public String getColumnName(int c) {
            return new String[]{"Módulo", "Property", "Tipo", "Valor"}[c];
        }
        @Override public boolean isCellEditable(int r, int c) { return false; }
        @Override public Class<?> getColumnClass(int c) { return String.class; }
        @Override public Object getValueAt(int r, int c) {
            edu.bpgenvm.vm.ModuleManager.PropertyView pv = rows.get(r);
            switch (c) {
                case 0: return pv.module;
                case 1: return pv.name;
                case 2: return pv.type;
                case 3: return pv.display;
                default: return "";
            }
        }
        void update(java.util.List<edu.bpgenvm.vm.ModuleManager.PropertyView> vs) {
            this.rows = vs;
            fireTableDataChanged();
        }
    }

    /** Modelo: call stack reconstruido desde DebugContext.stackFrames(). */
    private final class StackTableModel extends AbstractTableModel {
        private List<int[]> frames = java.util.Collections.emptyList();
        private List<String> labels = java.util.Collections.emptyList();
        @Override public int getRowCount() { return frames.size(); }
        @Override public int getColumnCount() { return 3; }
        @Override public String getColumnName(int c) {
            return new String[]{"Frame", "PC", "Función / fuente"}[c];
        }
        @Override public boolean isCellEditable(int r, int c) { return false; }
        @Override public Object getValueAt(int r, int c) {
            int[] frame = frames.get(r);
            switch (c) {
                case 0: return r;
                case 1: return frame[0];
                case 2: return labels.get(r);
                default: return null;
            }
        }
        void update(List<int[]> fs, List<String> ls) {
            this.frames = fs;
            this.labels = ls;
            fireTableDataChanged();
        }
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JMenu jMenu1;
    private javax.swing.JMenu jMenu2;
    private javax.swing.JMenu jMenu3;
    private javax.swing.JMenuBar jMenuBar1;
    private javax.swing.JMenuItem jMenuItem1;
    private javax.swing.JMenuItem jMenuItem2;
    private javax.swing.JMenuItem jMenuItem3;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JPanel jPanel2;
    private javax.swing.JPanel jPanel3;
    private javax.swing.JScrollPane jScrollPane1;
    private javax.swing.JPopupMenu.Separator jSeparator1;
    private javax.swing.JSplitPane jSplitPane1;
    private javax.swing.JSplitPane jSplitPane2;
    private javax.swing.JSplitPane jSplitPane3;
    private javax.swing.JTabbedPane jTabbedPane1;
    private javax.swing.JTabbedPane jTabbedPane2;
    private javax.swing.JTable jTable1;
    // End of variables declaration//GEN-END:variables
}
