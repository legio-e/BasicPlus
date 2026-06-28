/*
 * GuiBackend.java — backend gráfico de la VM-Java para el módulo Gui (V3).
 *
 * Implementa los builtins __gui* con Swing. Cada widget BP es un `handle` (int,
 * = lvglId) que mapea a un nodo aquí. El contrato Gui.* es idéntico al de la
 * VM-C (que lo cumple con LVGL); aquí lo cumple Swing. La PARIDAD se verifica
 * por dumpTree() — el modelo del árbol de widgets, NO los píxeles (Java2D y LVGL
 * rasterizan distinto). Por eso el dump muestra la geometría AUTORADA (align+offset
 * o posición explícita) y el tamaño autorado (-1 = auto), nunca los píxeles
 * computados (que dependen del tamaño preferido de cada backend).
 *
 * H6: Object→Component→widgets. Component aporta geometría (x/y/width/height),
 * scroll (opt-in), refresh y eventos onClick/onChange (upcall).
 */
package edu.bpgenvm.gui;

import javax.swing.*;
import java.awt.*;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class GuiBackend {

    // Anclas: deben coincidir con Gui.Align.* en bpstdlib/Gui.bp.
    public static final int TOP_LEFT = 0, TOP_MID = 1, TOP_RIGHT = 2,
                            LEFT_MID = 3, CENTER = 4, RIGHT_MID = 5,
                            BOTTOM_LEFT = 6, BOTTOM_MID = 7, BOTTOM_RIGHT = 8;

    // Tipo de evento del upcall: click → onClick, change → onChange.
    public static final int KIND_CLICK = 0, KIND_CHANGE = 1;

    /** Un widget: su componente Swing + metadatos de layout para el dump. */
    private static final class Node {
        final int handle;
        final String type;          // "screen" | "panel" | "label" | "button"
        final JComponent comp;
        final int parent;           // handle del padre (0 = raíz)
        final List<Integer> children = new ArrayList<>();
        int w = -1, h = -1;         // -1 = tamaño preferido (auto)
        int x = 0, y = 0;           // posición explícita (cuando posSet)
        boolean posSet = false;     // true → x,y mandan; false → align manda
        int align = TOP_LEFT, dx = 0, dy = 0;
        int scroll = 0;             // ScrollDir: 0=NONE 1=HOR 2=VER 3=BOTH (default NONE)
        boolean hasValue = false;   // true en value-widgets (checkbox, switch, slider, bar, ...)
        boolean suppressEvents = false;  // setChecked/setValue programático: no emitir onChange (paridad LVGL)
        int value = 0;              // valor actual (checkbox/switch: 0/1; slider/bar: entero)
        int rmin = 0, rmax = 100;   // rango de value-widgets enteros (clamp); default LVGL/Swing 0..100
        int trows = 0, tcols = 0;   // table: dimensiones de la rejilla
        String[] cells = null;      // table: celdas row-major (trows*tcols)
        int imgAsset = 0;           // imageview: id del asset Image asignado (0 = ninguno)
        int renderedVersion = 0;    // imageview: versión del asset ya renderizada (version-stamp)
        int reloads = 0;            // imageview: nº de recargas reales (prueba la optimización)
        int fontSize = 0;           // tamaño de fuente en px (0 = por defecto); el dump lo refleja
        boolean readOnly = false;   // textarea: solo lectura (sin cursor, no editable)
        String text = "";
        Node(int handle, String type, JComponent comp, int parent) {
            this.handle = handle; this.type = type; this.comp = comp; this.parent = parent;
        }
    }

    private JFrame frame;
    private int screenHandle = 0;
    private int screenW = 480, screenH = 320;     // tamaño por defecto (configurable)
    private final Map<Integer, Node> nodes = new HashMap<>();
    private int nextHandle = 1;

    /** Un asset de imagen (bitmap) cargado de archivo. NO es un Node: vive en su
     *  propio registro y puede compartirse entre varias ImageView. En host el
     *  modelo solo necesita ruta + dimensiones (del header PNG); el render real
     *  (ImageIO) es opcional/aparte. */
    private static final class ImageAsset {
        String path = "";
        int w = 0, h = 0;
        boolean loaded = false;
        int version = 0;   // sube en cada loadFile: el ImageView recarga si cambió
    }
    private final Map<Integer, ImageAsset> images = new HashMap<>();
    private int nextImageId = 1;

    // Upcall de eventos. handle del widget → objptr del objeto BP dueño (lo
    // registra __guiBindClick). `events` es el "bit de IRQ pendiente": el EDT de
    // Swing mete ahí {objptr, kind} (o {EVENT_CLOSE,0} al cerrar) y el worker BP
    // lo saca en el lazo de __guiRun. Único punto entre hilos; el EDT nunca
    // ejecuta bytecode BP.
    public static final int EVENT_CLOSE = -1;
    private final java.util.concurrent.BlockingQueue<int[]> events =
            new java.util.concurrent.LinkedBlockingQueue<>();

    // ---- Creación ----

    /** Pantalla raíz (lazy). Devuelve su handle. */
    public int screenActive() {
        if (screenHandle != 0) return screenHandle;
        JPanel root = new JPanel(null);
        root.setBackground(Color.BLACK);
        int h = nextHandle++;
        Node n = new Node(h, "screen", root, 0);
        n.w = screenW; n.h = screenH;
        nodes.put(h, n);
        screenHandle = h;
        return h;
    }

    /** ¿es `parent` un contenedor vivo? (paridad con bpvm_gui_parent_alive de la VM-C). */
    public boolean parentAlive(int parent) { return parent > 0 && nodes.get(parent) != null; }

    public int createObj(int parent)    { return create("panel",  new JPanel(null), parent); }
    public int createLabel(int parent)  { return create("label",  new JLabel(),     parent); }
    public int createButton(int parent) { return create("button", new JButton(),    parent); }
    public int createCheckbox(int parent) {
        int h = create("checkbox", new JCheckBox(), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createSwitch(int parent) {
        int h = create("toggle", new JToggleButton(), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createSlider(int parent) {
        int h = create("slider", new JSlider(0, 100, 0), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createBar(int parent) {
        int h = create("bar", new JProgressBar(0, 100), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createSpinbox(int parent) {
        int h = create("spinbox", new JSpinner(new SpinnerNumberModel(0, 0, 100, 1)), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createLed(int parent) {
        // Indicador (solo salida): un JLabel sirve de placeholder; el modelo (value)
        // es la verdad para el dump. No se enlaza evento.
        int h = create("led", new JLabel(), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public int createDropdown(int parent) {
        int h = create("dropdown", new JComboBox<String>(), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;   // value = índice seleccionado
        return h;
    }
    public int createTextarea(int parent) {
        // Texto editable: el contenido (n.text) es la verdad; NO es value-widget (sin val=).
        return create("textarea", new JTextArea(), parent);
    }
    public int createList(int parent) {
        int h = create("list", new JList<String>(new DefaultListModel<String>()), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;   // value = índice seleccionado
        return h;
    }
    public int createKeyboard(int parent) {
        // Teclado en pantalla: placeholder en miVM (el cableado a un textarea es render-only).
        return create("keyboard", new JPanel(null), parent);
    }
    public void keyboardSetTextarea(int handle, int taHandle) { /* render-only (LVGL); no-op en el modelo */ }
    public int createMsgbox(int parent) {
        // Aviso (no modal): placeholder JPanel en miVM; el modelo (text=mensaje,
        // value=botón pulsado) es la verdad. Los botones reales se pintan en LVGL.
        int h = create("msgbox", new JPanel(null), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;
        return h;
    }
    public void setButtons(int handle, String labels) { /* botones = render LVGL; no-op en el modelo */ }
    public int createTabview(int parent) {
        int h = create("tabview", new JTabbedPane(), parent);
        Node n = nodes.get(h); if (n != null) n.hasValue = true;   // value = pestaña activa
        return h;
    }
    public int tabviewAddTab(int handle, String name) {
        // La página (JPanel) se añade al JTabbedPane vía create(); el nombre va en el
        // nodo (dump) y, si se puede, como título de la pestaña. Al añadir la 1ª
        // pestaña, JTabbedPane auto-selecciona el índice 0 y dispara su ChangeListener;
        // eso NO es un onChange del usuario (VM-C no lo emite) → lo silenciamos.
        Node tv = nodes.get(handle);
        boolean prevSuppress = (tv != null) && tv.suppressEvents;
        if (tv != null) tv.suppressEvents = true;
        int h;
        try {
            h = create("tabpage", new JPanel(null), handle);
        } finally {
            if (tv != null) tv.suppressEvents = prevSuppress;
        }
        Node n = nodes.get(h); if (n != null) n.text = name;
        if (tv != null && tv.comp instanceof JTabbedPane) {
            JTabbedPane tp = (JTabbedPane) tv.comp;
            if (tp.getTabCount() > 0) tp.setTitleAt(tp.getTabCount() - 1, name);
        }
        return h;
    }
    public int createTable(int parent) {
        // Placeholder JPanel en miVM; las celdas (n.cells) son la verdad del dump.
        return create("table", new JPanel(null), parent);
    }
    public void tableSetGrid(int handle, int rows, int cols) {
        Node n = nodes.get(handle); if (n == null) return;
        if (rows < 0) rows = 0;
        if (cols < 0) cols = 0;
        String[] nc = new String[rows * cols];
        for (int i = 0; i < nc.length; i++) nc[i] = "";
        if (n.cells != null) {   // conserva las celdas que sigan en rango
            for (int r = 0; r < rows && r < n.trows; r++)
                for (int c = 0; c < cols && c < n.tcols; c++)
                    nc[r * cols + c] = n.cells[r * n.tcols + c];
        }
        n.trows = rows; n.tcols = cols; n.cells = nc;
    }
    public void tableSetCell(int handle, int row, int col, String text) {
        Node n = nodes.get(handle); if (n == null || n.cells == null) return;
        if (row < 0 || row >= n.trows || col < 0 || col >= n.tcols) return;
        n.cells[row * n.tcols + col] = (text != null) ? text : "";
    }
    public String tableGetCell(int handle, int row, int col) {
        Node n = nodes.get(handle); if (n == null || n.cells == null) return "";
        if (row < 0 || row >= n.trows || col < 0 || col >= n.tcols) return "";
        String s = n.cells[row * n.tcols + col];
        return s != null ? s : "";
    }
    // ---- Image: asset (bitmap) separado del control que lo muestra. ----
    public int imageNew() {
        int id = nextImageId++;
        images.put(id, new ImageAsset());
        return id;
    }
    // Carga un PNG: saca width/height del header IHDR (sin decoder). Devuelve 1
    // si la cabecera PNG es válida, 0 si no (no encontrado / no es PNG).
    public int imageLoadFile(int id, String path) {
        ImageAsset a = images.get(id); if (a == null) return 0;
        a.path = (path != null) ? path : "";
        a.w = 0; a.h = 0; a.loaded = false;
        try (InputStream in = new FileInputStream(a.path)) {
            byte[] b = new byte[24];
            int n = 0;
            while (n < 24) { int r = in.read(b, n, 24 - n); if (r < 0) break; n += r; }
            if (n >= 24
                && (b[0] & 0xFF) == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G'
                && (b[4] & 0xFF) == 0x0D && (b[5] & 0xFF) == 0x0A
                && (b[6] & 0xFF) == 0x1A && (b[7] & 0xFF) == 0x0A) {
                a.w = ((b[16] & 0xFF) << 24) | ((b[17] & 0xFF) << 16) | ((b[18] & 0xFF) << 8) | (b[19] & 0xFF);
                a.h = ((b[20] & 0xFF) << 24) | ((b[21] & 0xFF) << 16) | ((b[22] & 0xFF) << 8) | (b[23] & 0xFF);
                a.loaded = true;
            }
        } catch (Exception e) { /* no encontrado / ilegible → loaded=false */ }
        a.version++;   // (re)carga: el asset cambió → los ImageView lo recargarán
        return a.loaded ? 1 : 0;
    }
    public int imageWidth(int id)  { ImageAsset a = images.get(id); return (a == null) ? 0 : a.w; }
    public int imageHeight(int id) { ImageAsset a = images.get(id); return (a == null) ? 0 : a.h; }
    public int createImageView(int parent) {
        // Placeholder JLabel en miVM; el asset asignado (n.imgAsset) es la verdad del dump.
        return create("imageview", new JLabel(), parent);
    }
    public void imageViewSetImage(int viewHandle, int imgId) {
        Node n = nodes.get(viewHandle); if (n == null) return;
        if (n.imgAsset != imgId) {   // cambia de imagen → forzar recarga en el próximo refresh
            n.imgAsset = imgId;
            n.renderedVersion = 0;
        }
    }
    // refresh: recarga solo si el asset cambió desde la última vez (version-stamp).
    // Si no cambió, no hace nada (no re-decodifica). reloads cuenta las recargas
    // reales — es el observable que prueba la optimización en el dump.
    public void imageViewRefresh(int viewHandle) {
        Node n = nodes.get(viewHandle); if (n == null) return;
        ImageAsset a = images.get(n.imgAsset);
        int cur = (a != null) ? a.version : 0;
        if (cur != n.renderedVersion) {
            n.renderedVersion = cur;   // (en device: re-decode + lv_image_set_src)
            n.reloads++;
        }
    }
    // ---- Fuente: tamaño de texto por componente (catálogo). El modelo guarda el
    //      px pedido (lo refleja el dump); el render deriva la fuente Swing. ----
    public void setFontSize(int handle, int px) {
        Node n = nodes.get(handle); if (n == null) return;
        n.fontSize = px;
        if (n.comp != null && px > 0) {
            java.awt.Font f = n.comp.getFont();
            if (f != null) n.comp.setFont(f.deriveFont((float) px));
        }
    }
    public int getFontSize(int handle) { Node n = nodes.get(handle); return (n == null) ? 0 : n.fontSize; }
    // ---- Textarea read-only: sin cursor, no editable. ----
    public void setReadonly(int handle, int ro) {
        Node n = nodes.get(handle); if (n == null) return;
        n.readOnly = (ro != 0);
        if (n.comp instanceof javax.swing.text.JTextComponent) {
            ((javax.swing.text.JTextComponent) n.comp).setEditable(!n.readOnly);
        }
    }
    public int getReadonly(int handle) { Node n = nodes.get(handle); return (n != null && n.readOnly) ? 1 : 0; }

    private int create(String type, JComponent comp, int parent) {
        int h = nextHandle++;
        Node n = new Node(h, type, comp, parent);
        nodes.put(h, n);
        Node p = nodes.get(parent);
        if (p != null) {
            p.children.add(h);
            if (p.comp instanceof Container) ((Container) p.comp).add(comp);
        }
        relayout(n);
        return h;
    }

    // ---- Configuración ----

    public void setText(int handle, String s) {
        Node n = nodes.get(handle); if (n == null) return;
        n.text = s;
        if (n.comp instanceof JLabel)  ((JLabel)  n.comp).setText(s);
        else if (n.comp instanceof AbstractButton) ((AbstractButton) n.comp).setText(s); // JButton + JCheckBox
        else if (n.comp instanceof javax.swing.text.JTextComponent) {
            // textarea: setText programático dispara el DocumentListener → suprimir.
            n.suppressEvents = true;
            try { ((javax.swing.text.JTextComponent) n.comp).setText(s); }
            finally { n.suppressEvents = false; }
        }
        relayout(n);
    }
    // dropdown: opciones \n-separadas (modelo del JComboBox). getText = contenido (textarea) u opciones.
    public void setOptions(int handle, String opts) {
        Node n = nodes.get(handle); if (n == null) return;
        n.text = (opts != null) ? opts : "";
        if (n.comp instanceof JComboBox) {
            @SuppressWarnings("unchecked")
            JComboBox<String> cb = (JComboBox<String>) n.comp;
            n.suppressEvents = true;
            try {
                cb.removeAllItems();
                if (!n.text.isEmpty()) for (String o : n.text.split("\n", -1)) cb.addItem(o);
            } finally { n.suppressEvents = false; }
        } else if (n.comp instanceof JList) {
            @SuppressWarnings("unchecked")
            JList<String> jl = (JList<String>) n.comp;
            DefaultListModel<String> m = new DefaultListModel<>();
            if (!n.text.isEmpty()) for (String o : n.text.split("\n", -1)) m.addElement(o);
            n.suppressEvents = true;
            try { jl.setModel(m); } finally { n.suppressEvents = false; }
        }
    }
    public String getText(int handle) {
        Node n = nodes.get(handle); return n != null ? n.text : "";
    }
    // Geometría — backend = la verdad. x/y explícitos (posSet) o vía align.
    public void setWidth(int handle, int w)  { Node n = nodes.get(handle); if (n != null) { n.w = w; relayout(n); } }
    public void setHeight(int handle, int h) { Node n = nodes.get(handle); if (n != null) { n.h = h; relayout(n); } }
    public int  getWidth(int handle)  { Node n = nodes.get(handle); return (n == null) ? 0 : (n.w >= 0 ? n.w : n.comp.getPreferredSize().width); }
    public int  getHeight(int handle) { Node n = nodes.get(handle); return (n == null) ? 0 : (n.h >= 0 ? n.h : n.comp.getPreferredSize().height); }
    public void setX(int handle, int x) { Node n = nodes.get(handle); if (n != null) { n.x = x; n.posSet = true; relayout(n); } }
    public void setY(int handle, int y) { Node n = nodes.get(handle); if (n != null) { n.y = y; n.posSet = true; relayout(n); } }
    public int  getX(int handle) { Node n = nodes.get(handle); return (n == null) ? 0 : (n.posSet ? n.x : n.comp.getX()); }
    public int  getY(int handle) { Node n = nodes.get(handle); return (n == null) ? 0 : (n.posSet ? n.y : n.comp.getY()); }
    public void align(int handle, int a, int dx, int dy) {
        Node n = nodes.get(handle); if (n == null) return;
        n.align = a; n.dx = dx; n.dy = dy; n.posSet = false; relayout(n);
    }
    public void setScrollDir(int handle, int dir) {
        // Modelo/dump ya reflejan el scroll; el render Swing (envolver en
        // JScrollPane cuando dir != NONE) es pendiente — no afecta a la paridad.
        Node n = nodes.get(handle); if (n != null) n.scroll = dir;
    }
    public int  getScrollDir(int handle) { Node n = nodes.get(handle); return (n == null) ? 0 : n.scroll; }
    public void setBgColor(int handle, int rgb) {
        Node n = nodes.get(handle); if (n == null) return;
        n.comp.setOpaque(true);
        n.comp.setBackground(new Color(rgb & 0xFFFFFF));
    }
    public void setTextColor(int handle, int rgb) {
        Node n = nodes.get(handle); if (n != null) n.comp.setForeground(new Color(rgb & 0xFFFFFF));
    }
    // Carga de fuente en runtime (loadFont) — paridad con la VM-C. Java2D/Swing no
    // puede parsear el .bin binfont de LVGL, así que asigna el id (1-based, MISMO
    // contador que la VM-C → ids idénticos) y renderiza con la fuente por defecto.
    private int loadedFontCount = 0;
    public int loadFont(String path) {
        // path ignorado en Swing; solo cuenta para el id (parity). No afecta al dump.
        return ++loadedFontCount;
    }
    public void setFont(int handle, int fontId) {
        // fontId = id devuelto por loadFont (1-based). En Swing no tenemos la .bin →
        // no-op (mantiene la fuente por defecto). NO afecta al dumpTree (paridad con
        // la VM-C, donde setFont aplica al lv_obj pero no al modelo).
    }
    public void refresh(int handle) {
        // Invalidar → repintar (espejo de lv_obj_invalidate). Solo render.
        Node n = nodes.get(handle); if (n != null) n.comp.repaint();
    }
    // Value-widgets (checkbox por ahora): el estado vive en n.value (modelo = verdad).
    public void setChecked(int handle, boolean v) {
        Node n = nodes.get(handle); if (n == null) return;
        n.value = v ? 1 : 0;
        // setSelected dispara el ItemListener síncronamente; lo silenciamos para
        // que el cambio PROGRAMÁTICO no emita onChange (LVGL tampoco lo hace —
        // solo la interacción del usuario). El modelo (n.value) ya está fijado.
        n.suppressEvents = true;
        try {
            if (n.comp instanceof AbstractButton) ((AbstractButton) n.comp).setSelected(v);
        } finally {
            n.suppressEvents = false;
        }
    }
    public boolean getChecked(int handle) {
        Node n = nodes.get(handle); return n != null && n.value != 0;
    }
    // Value-widgets enteros (slider/bar): n.value clampado a [rmin,rmax] (igual en
    // las 2 VMs → el dump coincide). El set programático no emite onChange.
    public void setValue(int handle, int v) {
        Node n = nodes.get(handle); if (n == null) return;
        int cv = v < n.rmin ? n.rmin : (v > n.rmax ? n.rmax : v);
        n.value = cv;
        n.suppressEvents = true;
        try {
            if (n.comp instanceof JSlider)           ((JSlider) n.comp).setValue(cv);
            else if (n.comp instanceof JProgressBar) ((JProgressBar) n.comp).setValue(cv);
            else if (n.comp instanceof JSpinner)     ((JSpinner) n.comp).setValue(Integer.valueOf(cv));
            else if (n.comp instanceof JComboBox) {  // dropdown: value = índice seleccionado
                JComboBox<?> cb = (JComboBox<?>) n.comp;
                if (cv >= 0 && cv < cb.getItemCount()) cb.setSelectedIndex(cv);
            }
            else if (n.comp instanceof JList) {       // list: value = índice seleccionado
                JList<?> jl = (JList<?>) n.comp;
                if (cv >= 0 && cv < jl.getModel().getSize()) jl.setSelectedIndex(cv);
            }
            else if (n.comp instanceof JTabbedPane) { // tabview: value = pestaña activa
                JTabbedPane tp = (JTabbedPane) n.comp;
                if (cv >= 0 && cv < tp.getTabCount()) tp.setSelectedIndex(cv);
            }
            // led (JLabel) y demás: el modelo (n.value) es la verdad; sin widget que tocar.
        } finally {
            n.suppressEvents = false;
        }
    }
    public int getValue(int handle) {
        Node n = nodes.get(handle); return n != null ? n.value : 0;
    }
    public void setRange(int handle, int mn, int mx) {
        Node n = nodes.get(handle); if (n == null) return;
        n.rmin = mn; n.rmax = mx;
        if (n.value < mn) n.value = mn; else if (n.value > mx) n.value = mx;   // re-clampa
        n.suppressEvents = true;
        try {
            if (n.comp instanceof JSlider) {
                JSlider s = (JSlider) n.comp; s.setMinimum(mn); s.setMaximum(mx); s.setValue(n.value);
            } else if (n.comp instanceof JProgressBar) {
                JProgressBar b = (JProgressBar) n.comp; b.setMinimum(mn); b.setMaximum(mx); b.setValue(n.value);
            } else if (n.comp instanceof JSpinner) {
                ((JSpinner) n.comp).setModel(new SpinnerNumberModel(n.value, mn, mx, 1));
            }
        } finally {
            n.suppressEvents = false;
        }
    }
    public void clean(int handle) {
        Node n = nodes.get(handle); if (n == null) return;
        if (n.comp instanceof Container) ((Container) n.comp).removeAll();
        n.children.clear();
    }
    public void delete(int handle) {
        Node n = nodes.get(handle); if (n == null) return;
        if (n.comp.getParent() != null) n.comp.getParent().remove(n.comp);
        nodes.remove(handle);
    }
    public void screenLoad(int handle) { /* una sola pantalla por ahora */ }

    // ---- Layout absoluto (posición explícita x,y · o ancla relativa al padre) ----
    private void relayout(Node n) {
        if (n.parent == 0) return;                 // la screen ocupa todo
        Node p = nodes.get(n.parent); if (p == null) return;
        int w  = (n.w >= 0) ? n.w : n.comp.getPreferredSize().width;
        int h  = (n.h >= 0) ? n.h : n.comp.getPreferredSize().height;
        if (n.posSet) {                            // posición explícita (x,y)
            n.comp.setBounds(n.x, n.y, w, h);
            return;
        }
        int pw = (p.w >= 0) ? p.w : Math.max(p.comp.getWidth(),  screenW);
        int ph = (p.h >= 0) ? p.h : Math.max(p.comp.getHeight(), screenH);
        int x = 0, y = 0;
        switch (n.align) {
            case TOP_LEFT:     x = 0;          y = 0;          break;
            case TOP_MID:      x = (pw-w)/2;   y = 0;          break;
            case TOP_RIGHT:    x = pw-w;       y = 0;          break;
            case LEFT_MID:     x = 0;          y = (ph-h)/2;   break;
            case CENTER:       x = (pw-w)/2;   y = (ph-h)/2;   break;
            case RIGHT_MID:    x = pw-w;       y = (ph-h)/2;   break;
            case BOTTOM_LEFT:  x = 0;          y = ph-h;       break;
            case BOTTOM_MID:   x = (pw-w)/2;   y = ph-h;       break;
            case BOTTOM_RIGHT: x = pw-w;       y = ph-h;       break;
            default: break;
        }
        n.comp.setBounds(x + n.dx, y + n.dy, w, h);
    }

    // ---- Eventos (upcall): bind del objeto BP + cola EDT→worker ----

    /** Engancha el listener de Swing al widget `handle`. El listener SOLO encola
     *  {objptr, kind} (no ejecuta BP; eso lo hace el worker en el lazo de
     *  __guiRun). Value-widgets → KIND_CHANGE (toggle booleano o arrastre);
     *  el resto → KIND_CLICK. El set PROGRAMÁTICO no emite (n.suppressEvents). */
    public void bindClick(int handle, int objptr) {
        Node n = nodes.get(handle);
        if (n == null) return;
        if (n.hasValue && n.comp instanceof AbstractButton) {
            // Toggle booleano (checkbox, switch): el cambio es CHANGE.
            AbstractButton b = (AbstractButton) n.comp;
            b.addItemListener(e -> {
                n.value = b.isSelected() ? 1 : 0;
                if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
            });
        } else if (n.hasValue && n.comp instanceof JSlider) {
            // Slider: el arrastre es CHANGE. n.value = valor (ya clampado por el rango).
            JSlider s = (JSlider) n.comp;
            s.addChangeListener(e -> {
                n.value = s.getValue();
                if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
            });
        } else if (n.hasValue && n.comp instanceof JSpinner) {
            // Spinbox: el cambio de valor es CHANGE.
            JSpinner sp = (JSpinner) n.comp;
            sp.addChangeListener(e -> {
                n.value = ((Integer) sp.getValue()).intValue();
                if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
            });
        } else if (n.comp instanceof JComboBox) {
            // Dropdown: la selección es CHANGE; n.value = índice elegido.
            @SuppressWarnings("unchecked")
            JComboBox<String> cb = (JComboBox<String>) n.comp;
            cb.addActionListener(e -> {
                n.value = cb.getSelectedIndex();
                if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
            });
        } else if (n.comp instanceof javax.swing.text.JTextComponent) {
            // Textarea: la edición es CHANGE; n.text = contenido.
            javax.swing.text.JTextComponent jtc = (javax.swing.text.JTextComponent) n.comp;
            jtc.getDocument().addDocumentListener(new javax.swing.event.DocumentListener() {
                private void changed() {
                    n.text = jtc.getText();
                    if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
                }
                public void insertUpdate(javax.swing.event.DocumentEvent e)  { changed(); }
                public void removeUpdate(javax.swing.event.DocumentEvent e)  { changed(); }
                public void changedUpdate(javax.swing.event.DocumentEvent e) { changed(); }
            });
        } else if (n.comp instanceof JList) {
            // List: la selección de un ítem es CHANGE; n.value = índice.
            JList<?> jl = (JList<?>) n.comp;
            jl.addListSelectionListener(e -> {
                if (!e.getValueIsAdjusting()) {
                    n.value = jl.getSelectedIndex();
                    if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
                }
            });
        } else if (n.comp instanceof JTabbedPane) {
            // Tabview: el cambio de pestaña activa es CHANGE; n.value = índice activo.
            JTabbedPane tp = (JTabbedPane) n.comp;
            tp.addChangeListener(e -> {
                n.value = tp.getSelectedIndex();
                if (!n.suppressEvents) events.offer(new int[]{objptr, KIND_CHANGE});
            });
        } else if (n.comp instanceof JButton) {
            ((JButton) n.comp).addActionListener(e -> events.offer(new int[]{objptr, KIND_CLICK}));
        } else {
            n.comp.addMouseListener(new java.awt.event.MouseAdapter() {
                @Override public void mouseClicked(java.awt.event.MouseEvent e) { events.offer(new int[]{objptr, KIND_CLICK}); }
            });
        }
    }

    /** Inyecta un clic sintetico sobre el objeto BP `objptr` (diagnostico /
     *  pruebas; verificable headless). */
    public void injectClick(int objptr) { events.offer(new int[]{objptr, KIND_CLICK}); }
    /** Inyecta un cambio de valor sintético (diagnóstico headless). */
    public void injectChange(int objptr) { events.offer(new int[]{objptr, KIND_CHANGE}); }

    // ---- Lazo de eventos: la ventana se muestra aquí; el BOMBEO lo hace la VM
    //      (que es quien sabe ejecutar BP) sacando eventos con takeEvent(). ----

    /** Muestra la ventana (NO bloquea). En headless encola EVENT_CLOSE para que
     *  el lazo de __guiRun drene los clics inyectados y termine sin bloquear. */
    public void start() {
        if (screenHandle == 0) screenActive();
        final Node root = nodes.get(screenHandle);
        if (java.awt.GraphicsEnvironment.isHeadless()) {
            events.offer(new int[]{EVENT_CLOSE, 0});
            return;
        }
        try {
            SwingUtilities.invokeAndWait(() -> {
                frame = new JFrame("BasicPlus GUI");
                frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
                frame.getContentPane().setPreferredSize(new Dimension(screenW, screenH));
                frame.getContentPane().add(root.comp);
                root.comp.setBounds(0, 0, screenW, screenH);
                for (Node n : nodes.values()) relayout(n);
                frame.pack();
                frame.setLocationRelativeTo(null);
                frame.addWindowListener(new java.awt.event.WindowAdapter() {
                    @Override public void windowClosed(java.awt.event.WindowEvent e) {
                        events.offer(new int[]{EVENT_CLOSE, 0});
                    }
                });
                frame.setVisible(true);
            });
        } catch (Exception e) {
            events.offer(new int[]{EVENT_CLOSE, 0});   // no se pudo crear ventana → no bloquear
        }
    }

    /** Saca el siguiente evento del lazo: {objptr, kind} del widget, o
     *  {EVENT_CLOSE, 0}. Bloquea hasta que haya uno (lo alimenta el EDT, o
     *  injectClick). */
    public int[] takeEvent() {
        try { return events.take(); }
        catch (InterruptedException ie) { Thread.currentThread().interrupt(); return new int[]{EVENT_CLOSE, 0}; }
    }

    // ---- Volcado del árbol (paridad de comportamiento; NO píxeles) ----
    public String dumpTree() {
        StringBuilder sb = new StringBuilder();
        if (screenHandle != 0) dump(sb, screenHandle, 0);
        return sb.toString();
    }
    private void dump(StringBuilder sb, int handle, int depth) {
        Node n = nodes.get(handle); if (n == null) return;
        for (int i = 0; i < depth; i++) sb.append("  ");
        sb.append(n.type);
        if (!n.text.isEmpty()) sb.append(" \"").append(n.text).append("\"");
        sb.append(" [").append(n.w).append("x").append(n.h);
        if (n.posSet) sb.append(" pos=").append(n.x).append(",").append(n.y);
        else sb.append(" align=").append(n.align).append(" +").append(n.dx).append(",").append(n.dy);
        if (n.scroll != 0) sb.append(" scroll=").append(n.scroll);
        if (n.hasValue) sb.append(" val=").append(n.value);
        if (n.cells != null) {   // table: dimensiones + celdas row-major (|-sep)
            sb.append(" grid=").append(n.trows).append("x").append(n.tcols).append(" \"");
            for (int i = 0; i < n.cells.length; i++) {
                if (i > 0) sb.append("|");
                sb.append(n.cells[i] != null ? n.cells[i] : "");
            }
            sb.append("\"");
        }
        if (n.type.equals("imageview")) {   // imageview: asset asignado (ruta + dims) + recargas
            if (n.imgAsset != 0) {
                ImageAsset a = images.get(n.imgAsset);
                String p = (a != null) ? a.path : "";
                int iw = (a != null) ? a.w : 0, ih = (a != null) ? a.h : 0;
                sb.append(" img=\"").append(p).append("\" ").append(iw).append("x").append(ih);
            } else {
                sb.append(" img=<none>");
            }
            sb.append(" reloads=").append(n.reloads);
        }
        if (n.fontSize != 0) sb.append(" font=").append(n.fontSize);
        if (n.readOnly) sb.append(" ro");
        sb.append("]\n");
        for (int c : n.children) dump(sb, c, depth + 1);
    }
}
