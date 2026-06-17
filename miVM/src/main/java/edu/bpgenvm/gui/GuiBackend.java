/*
 * GuiBackend.java — backend gráfico de la VM-Java para el módulo Gui (V3, H3).
 *
 * Implementa los builtins __gui* con Swing. Cada widget BP es un `handle` (int)
 * que mapea a un nodo aquí. El contrato Gui.* es idéntico al de la VM-C (que lo
 * cumple con LVGL); aquí lo cumple Swing. La PARIDAD se verifica por dumpTree()
 * — el modelo del árbol de widgets, NO los píxeles (Java2D y LVGL rasterizan
 * distinto). Layout absoluto (como LVGL): align(ancla, dx, dy) sobre layout null.
 *
 * El upcall (click → onClick BP) es H3.4: aquí se deja el gancho (onClickCb).
 */
package edu.bpgenvm.gui;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class GuiBackend {

    // Anclas: deben coincidir con Gui.Align.* en bpstdlib/Gui.bp.
    public static final int TOP_LEFT = 0, TOP_MID = 1, TOP_RIGHT = 2,
                            LEFT_MID = 3, CENTER = 4, RIGHT_MID = 5,
                            BOTTOM_LEFT = 6, BOTTOM_MID = 7, BOTTOM_RIGHT = 8;

    /** Un widget: su componente Swing + metadatos de layout para el dump. */
    private static final class Node {
        final int handle;
        final String type;          // "screen" | "panel" | "label" | "button"
        final JComponent comp;
        final int parent;           // handle del padre (0 = raíz)
        final List<Integer> children = new ArrayList<>();
        int w = -1, h = -1;         // -1 = tamaño preferido (auto)
        int align = TOP_LEFT, dx = 0, dy = 0;
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

    // H3.4 — upcall de eventos. handleToObj: handle del widget → objptr del
    // objeto BP dueño (lo registra __guiBindClick). `events` es el "bit de IRQ
    // pendiente": el EDT de Swing mete ahí el handle pulsado (o EVENT_CLOSE al
    // cerrar la ventana) y el worker BP lo saca en el lazo de __guiRun. Es el
    // único punto entre hilos; el EDT nunca ejecuta bytecode BP.
    public static final int EVENT_CLOSE = -1;
    private final java.util.concurrent.BlockingQueue<Integer> events =
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

    public int createObj(int parent)    { return create("panel",  new JPanel(null), parent); }
    public int createLabel(int parent)  { return create("label",  new JLabel(),     parent); }
    public int createButton(int parent) { return create("button", new JButton(),    parent); }

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
        else if (n.comp instanceof JButton) ((JButton) n.comp).setText(s);
        relayout(n);
    }
    public void setWidth(int handle, int w)  { Node n = nodes.get(handle); if (n != null) { n.w = w; relayout(n); } }
    public void setHeight(int handle, int h) { Node n = nodes.get(handle); if (n != null) { n.h = h; relayout(n); } }
    public void align(int handle, int a, int dx, int dy) {
        Node n = nodes.get(handle); if (n == null) return;
        n.align = a; n.dx = dx; n.dy = dy; relayout(n);
    }
    public void setBgColor(int handle, int rgb) {
        Node n = nodes.get(handle); if (n == null) return;
        n.comp.setOpaque(true);
        n.comp.setBackground(new Color(rgb & 0xFFFFFF));
    }
    public void setTextColor(int handle, int rgb) {
        Node n = nodes.get(handle); if (n != null) n.comp.setForeground(new Color(rgb & 0xFFFFFF));
    }
    public void setFont(int handle, int fontId) {
        // fontId se interpreta como tamaño en px (Gui.Font llegará en 2ª tanda).
        Node n = nodes.get(handle);
        if (n != null && fontId > 0) n.comp.setFont(n.comp.getFont().deriveFont((float) fontId));
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

    // ---- Layout absoluto (ancla relativa al padre + offset) ----
    private void relayout(Node n) {
        if (n.parent == 0) return;                 // la screen ocupa todo
        Node p = nodes.get(n.parent); if (p == null) return;
        int pw = (p.w >= 0) ? p.w : Math.max(p.comp.getWidth(),  screenW);
        int ph = (p.h >= 0) ? p.h : Math.max(p.comp.getHeight(), screenH);
        int w  = (n.w >= 0) ? n.w : n.comp.getPreferredSize().width;
        int h  = (n.h >= 0) ? n.h : n.comp.getPreferredSize().height;
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

    /** Engancha el listener de clic de Swing al widget `handle`. El listener
     *  SOLO encola el objptr del objeto BP dueño (no ejecuta BP; eso lo hace el
     *  worker en el lazo de __guiRun). La cola lleva objptrs — lo que el dispatch
     *  necesita, ya resuelto en el momento del clic. */
    public void bindClick(int handle, int objptr) {
        Node n = nodes.get(handle);
        if (n == null) return;
        if (n.comp instanceof JButton) {
            ((JButton) n.comp).addActionListener(e -> events.offer(objptr));
        } else {
            n.comp.addMouseListener(new java.awt.event.MouseAdapter() {
                @Override public void mouseClicked(java.awt.event.MouseEvent e) { events.offer(objptr); }
            });
        }
    }

    /** Inyecta un clic sintetico sobre el objeto BP `objptr` (diagnostico /
     *  pruebas; verificable headless). */
    public void injectClick(int objptr) { events.offer(objptr); }

    // ---- Lazo de eventos: la ventana se muestra aquí; el BOMBEO lo hace la VM
    //      (que es quien sabe ejecutar BP) sacando eventos con takeEvent(). ----

    /** Muestra la ventana (NO bloquea). En headless encola EVENT_CLOSE para que
     *  el lazo de __guiRun drene los clics inyectados y termine sin bloquear. */
    public void start() {
        if (screenHandle == 0) screenActive();
        final Node root = nodes.get(screenHandle);
        if (java.awt.GraphicsEnvironment.isHeadless()) {
            events.offer(EVENT_CLOSE);
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
                        events.offer(EVENT_CLOSE);
                    }
                });
                frame.setVisible(true);
            });
        } catch (Exception e) {
            events.offer(EVENT_CLOSE);   // no se pudo crear ventana → no bloquear
        }
    }

    /** Saca el siguiente evento del lazo: objptr del widget pulsado, o
     *  EVENT_CLOSE. Bloquea hasta que haya uno (lo alimenta el EDT, o injectClick). */
    public int takeEvent() {
        try { return events.take(); }
        catch (InterruptedException ie) { Thread.currentThread().interrupt(); return EVENT_CLOSE; }
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
        sb.append(" [").append(n.w).append("x").append(n.h)
          .append(" align=").append(n.align).append(" +").append(n.dx).append(",").append(n.dy).append("]\n");
        for (int c : n.children) dump(sb, c, depth + 1);
    }
}
