// ============================================================
// PromptDialog.java
// N20 — IDE-side renderer del `IO.prompt(spec)` builtin de BP.
//
// El programa BP envía un JSON spec con el formulario; este diálogo lo
// muestra al usuario, captura los valores y los devuelve serializados
// como JSON al programa.
//
// Formato del `spec` (todos los campos opcionales salvo `fields`):
//   {
//     "title": "Datos del usuario",
//     "message": "Por favor rellena:",
//     "fields": [
//       { "name": "nombre", "label": "Nombre:",   "type": "text",    "default": "" },
//       { "name": "edad",   "label": "Edad:",     "type": "integer", "default": 0 },
//       { "name": "ok",     "label": "Acepto",    "type": "boolean", "default": false },
//       { "name": "color",  "label": "Color:",    "type": "select",
//         "options": ["rojo","verde","azul"], "default": "rojo" }
//     ],
//     "customHtml": "<html><body>...</body></html>"   // opcional; ver más abajo
//   }
//
// Si está presente `customHtml`, el diálogo lo renderiza con JEditorPane
// (HTML 3.2, sin JS). Útil para layouts ricos; los `<input name="X">`
// dentro del HTML se extraen al pulsar OK. Si no, se construyen widgets
// Swing nativos según `fields`.
//
// La respuesta es un JSON map { name: value, ... } con los tipos
// preservados:
//   - text   → string
//   - integer → long
//   - boolean → bool
//   - select  → string (la opción elegida)
//
// Cancel devuelve `{}` (la VM tiene `Map.has` para distinguir cancel
// de respuesta válida sin campos).
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class PromptDialog {
    private PromptDialog() {}

    /** Muestra el diálogo modal y devuelve el JSON con los valores.
     *  Debe llamarse desde el EDT. */
    public static String showAndCollect(Frame owner, String specJson) {
        Map<String, Object> spec;
        try {
            spec = Json.parseFlatObject(specJson);
        } catch (Throwable t) {
            JOptionPane.showMessageDialog(owner,
                    "spec inválido en prompt(): " + t.getMessage(),
                    "prompt()", JOptionPane.ERROR_MESSAGE);
            return "{}";
        }
        String title   = Json.getString(spec, "title", "Datos");
        String message = Json.getString(spec, "message", "");
        String html    = Json.getString(spec, "customHtml", null);

        JDialog dlg = new JDialog(owner, title, true);
        dlg.setLayout(new BorderLayout(6, 6));

        // Cuerpo
        JPanel body = new JPanel();
        body.setLayout(new BoxLayout(body, BoxLayout.Y_AXIS));
        body.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));
        if (!message.isEmpty()) {
            JLabel msg = new JLabel(message);
            msg.setAlignmentX(Component.LEFT_ALIGNMENT);
            body.add(msg);
            body.add(Box.createRigidArea(new Dimension(0, 6)));
        }

        // Campos: o widgets nativos, o HTML custom.
        List<FieldWidget> widgets = new ArrayList<>();
        if (html != null && !html.isEmpty()) {
            // HTML custom: JEditorPane render. Limitación: extraemos
            // valores de los <input name="X"> via HTMLDocument.
            JEditorPane pane = new JEditorPane("text/html", html);
            pane.setEditable(true);   // editable para que los inputs acepten texto
            pane.setAlignmentX(Component.LEFT_ALIGNMENT);
            JScrollPane scroll = new JScrollPane(pane);
            scroll.setPreferredSize(new Dimension(420, 240));
            scroll.setAlignmentX(Component.LEFT_ALIGNMENT);
            body.add(scroll);
            widgets.add(new HtmlPaneWidget(pane));
        } else {
            List<Object> fields = Json.getList(spec, "fields");
            if (fields != null) {
                for (Object o : fields) {
                    if (!(o instanceof Map)) continue;
                    @SuppressWarnings("unchecked")
                    Map<String, Object> f = (Map<String, Object>) o;
                    FieldWidget w = buildFieldWidget(f);
                    if (w == null) continue;
                    widgets.add(w);
                    JPanel row = new JPanel(new BorderLayout(6, 0));
                    row.setAlignmentX(Component.LEFT_ALIGNMENT);
                    row.add(new JLabel(w.label() + " "), BorderLayout.WEST);
                    row.add(w.component(), BorderLayout.CENTER);
                    body.add(row);
                    body.add(Box.createRigidArea(new Dimension(0, 4)));
                }
            }
        }
        dlg.add(body, BorderLayout.CENTER);

        // Botones
        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton ok = new JButton("OK");
        JButton cancel = new JButton("Cancel");
        buttons.add(ok);
        buttons.add(cancel);
        dlg.add(buttons, BorderLayout.SOUTH);

        // ENTER en cualquier campo = OK.
        dlg.getRootPane().setDefaultButton(ok);

        final String[] result = { "{}" };
        ok.addActionListener(e -> {
            StringBuilder sb = new StringBuilder();
            sb.append('{');
            boolean first = true;
            for (FieldWidget w : widgets) {
                Map<String, Object> values = w.collect();
                for (Map.Entry<String, Object> kv : values.entrySet()) {
                    if (!first) sb.append(',');
                    first = false;
                    sb.append('"').append(Json.escape(kv.getKey())).append("\":");
                    appendJsonValue(sb, kv.getValue());
                }
            }
            sb.append('}');
            result[0] = sb.toString();
            dlg.dispose();
        });
        cancel.addActionListener(e -> {
            result[0] = "{}";
            dlg.dispose();
        });

        dlg.pack();
        dlg.setLocationRelativeTo(owner);
        dlg.setVisible(true);    // bloquea hasta dispose
        return result[0];
    }

    private static void appendJsonValue(StringBuilder sb, Object v) {
        if (v == null) sb.append("null");
        else if (v instanceof Boolean) sb.append(((Boolean) v) ? "true" : "false");
        else if (v instanceof Number)  sb.append(v.toString());
        else                            sb.append('"').append(Json.escape(v.toString())).append('"');
    }

    // ============================================================
    // FieldWidget — abstracción de un campo del form
    // ============================================================
    private interface FieldWidget {
        String label();
        java.awt.Component component();
        /** Devuelve un map name → value (típicamente 1 entry; HtmlPane
         *  devuelve N entries, una por cada <input> del HTML). */
        Map<String, Object> collect();
    }

    private static FieldWidget buildFieldWidget(Map<String, Object> f) {
        String name  = Json.getString(f, "name", "");
        String label = Json.getString(f, "label", name);
        String type  = Json.getString(f, "type", "text");
        if (name.isEmpty()) return null;
        switch (type) {
            case "integer": {
                long def = Json.getLong(f, "default", 0L);
                JSpinner sp = new JSpinner(new SpinnerNumberModel((int) def,
                        Integer.MIN_VALUE, Integer.MAX_VALUE, 1));
                return new IntegerFieldWidget(name, label, sp);
            }
            case "boolean": {
                boolean def = Json.getBool(f, "default", false);
                JCheckBox cb = new JCheckBox("", def);
                return new BooleanFieldWidget(name, label, cb);
            }
            case "select": {
                List<Object> opts = Json.getList(f, "options");
                java.util.Vector<String> v = new java.util.Vector<>();
                if (opts != null) for (Object o : opts) v.add(String.valueOf(o));
                JComboBox<String> cb = new JComboBox<>(v);
                String def = Json.getString(f, "default", null);
                if (def != null) cb.setSelectedItem(def);
                return new SelectFieldWidget(name, label, cb);
            }
            case "text":
            default: {
                String def = Json.getString(f, "default", "");
                JTextField tf = new JTextField(def, 20);
                return new TextFieldWidget(name, label, tf);
            }
        }
    }

    private static final class TextFieldWidget implements FieldWidget {
        final String name, label; final JTextField tf;
        TextFieldWidget(String n, String l, JTextField c) { name=n; label=l; tf=c; }
        public String label() { return label; }
        public java.awt.Component component() { return tf; }
        public Map<String, Object> collect() {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put(name, tf.getText());
            return m;
        }
    }
    private static final class IntegerFieldWidget implements FieldWidget {
        final String name, label; final JSpinner sp;
        IntegerFieldWidget(String n, String l, JSpinner c) { name=n; label=l; sp=c; }
        public String label() { return label; }
        public java.awt.Component component() { return sp; }
        public Map<String, Object> collect() {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put(name, ((Number) sp.getValue()).longValue());
            return m;
        }
    }
    private static final class BooleanFieldWidget implements FieldWidget {
        final String name, label; final JCheckBox cb;
        BooleanFieldWidget(String n, String l, JCheckBox c) { name=n; label=l; cb=c; }
        public String label() { return label; }
        public java.awt.Component component() { return cb; }
        public Map<String, Object> collect() {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put(name, cb.isSelected());
            return m;
        }
    }
    private static final class SelectFieldWidget implements FieldWidget {
        final String name, label; final JComboBox<String> cb;
        SelectFieldWidget(String n, String l, JComboBox<String> c) { name=n; label=l; cb=c; }
        public String label() { return label; }
        public java.awt.Component component() { return cb; }
        public Map<String, Object> collect() {
            Map<String, Object> m = new LinkedHashMap<>();
            Object v = cb.getSelectedItem();
            m.put(name, v == null ? "" : v.toString());
            return m;
        }
    }
    /** HTML custom: extrae los <input name="X" value="Y"> y los devuelve
     *  como string en el JSON resultado. */
    private static final class HtmlPaneWidget implements FieldWidget {
        final JEditorPane pane;
        HtmlPaneWidget(JEditorPane p) { this.pane = p; }
        public String label() { return ""; }
        public java.awt.Component component() { return pane; }
        public Map<String, Object> collect() {
            Map<String, Object> out = new LinkedHashMap<>();
            javax.swing.text.Document doc = pane.getDocument();
            if (!(doc instanceof javax.swing.text.html.HTMLDocument)) return out;
            javax.swing.text.html.HTMLDocument hdoc = (javax.swing.text.html.HTMLDocument) doc;
            // Recorrer elementos buscando inputs.
            javax.swing.text.ElementIterator it = new javax.swing.text.ElementIterator(hdoc);
            javax.swing.text.Element e;
            while ((e = it.next()) != null) {
                javax.swing.text.AttributeSet attrs = e.getAttributes();
                Object modelAttr = attrs.getAttribute(javax.swing.text.StyleConstants.ModelAttribute);
                Object nameAttr = attrs.getAttribute(javax.swing.text.html.HTML.Attribute.NAME);
                if (nameAttr == null) continue;
                String name = nameAttr.toString();
                // Texto del input típicamente vive en el PlainDocument del modelo.
                if (modelAttr instanceof javax.swing.text.PlainDocument) {
                    try {
                        javax.swing.text.PlainDocument pd = (javax.swing.text.PlainDocument) modelAttr;
                        out.put(name, pd.getText(0, pd.getLength()));
                    } catch (Throwable t) { /* ignore */ }
                } else if (modelAttr instanceof javax.swing.JToggleButton.ToggleButtonModel) {
                    out.put(name, ((javax.swing.JToggleButton.ToggleButtonModel) modelAttr).isSelected());
                }
            }
            return out;
        }
    }
}
