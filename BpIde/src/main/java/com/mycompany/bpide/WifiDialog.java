// ============================================================
// WifiDialog.java — V8/H3 (6-nov): la WiFi de la placa, desde el IDE.
//
// Escribe /sys/wifi.json por el cable que haya: es la preconfiguración que se decidió el 19-oct, y evita tener que escribirlo
// a mano. La placa lo lee al encender la WiFi, así que para que entre hay que reiniciarla.
//
// La clave va en claro en la placa: cualquier programa, o quien tenga el cable, puede leerla (Eduardo: «la seguridad, de momento
// no»). Aquí se enseña con asteriscos, nada más.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Window;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.function.Consumer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.SwingWorker;

public final class WifiDialog extends JDialog {

    private static final String FICHERO = "/sys/wifi.json";

    private final Backend backend;
    private final Consumer<String> status;

    private final JTextField ssid = new JTextField(22);
    private final JPasswordField clave = new JPasswordField(22);
    private final JTextField pais = new JTextField("ES", 3);
    private final JCheckBox arranca = new JCheckBox("Encenderla al arrancar la placa");
    private final JSpinner apagado = new JSpinner(new SpinnerNumberModel(600, 0, 86400, 60));
    private final JButton guardar = new JButton("Guardar en la placa");

    public WifiDialog(Window padre, Backend backend, Consumer<String> status) {
        super(padre, "WiFi de la placa", ModalityType.APPLICATION_MODAL);
        this.backend = backend;
        this.status = status;

        JPanel campos = new JPanel(new GridBagLayout());
        campos.setBorder(BorderFactory.createEmptyBorder(10, 10, 4, 10));
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(3, 3, 3, 3);
        c.anchor = GridBagConstraints.WEST;
        fila(campos, c, 0, "Red (SSID):", ssid);
        fila(campos, c, 1, "Clave:", clave);
        fila(campos, c, 2, "País (dos letras):", pais);
        c.gridx = 1; c.gridy = 3;
        campos.add(arranca, c);
        JPanel segundos = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0));
        segundos.add(apagado);
        segundos.add(new JLabel("  s (0: no se apaga sola)"));
        fila(campos, c, 4, "Apagarla sin actividad:", segundos);
        c.gridx = 0; c.gridy = 5; c.gridwidth = 2;
        campos.add(new JLabel("<html><small>La clave se guarda en claro en la placa. Para que entre, hay que reiniciarla.</small></html>"), c);

        JPanel botones = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton cancelar = new JButton("Cancelar");
        botones.add(guardar);
        botones.add(cancelar);
        guardar.addActionListener(e -> guarda());
        cancelar.addActionListener(e -> dispose());

        add(campos, BorderLayout.CENTER);
        add(botones, BorderLayout.SOUTH);
        pack();
        setLocationRelativeTo(padre);
        lee();
    }

    private static void fila(JPanel p, GridBagConstraints c, int y, String nombre, java.awt.Component campo) {
        c.gridy = y;
        c.gridx = 0;
        p.add(new JLabel(nombre), c);
        c.gridx = 1;
        p.add(campo, c);
    }

    /** Lo que ya tenga la placa. Sin fichero, los campos se quedan como están. */
    private void lee() {
        guardar.setEnabled(false);
        new SwingWorker<Map<String, Object>, Void>() {
            @Override protected Map<String, Object> doInBackground() {
                try {
                    return Json.parseFlatObject(new String(backend.get(FICHERO), StandardCharsets.UTF_8));
                } catch (IOException | RuntimeException sinFichero) {
                    return null;
                }
            }
            @Override protected void done() {
                guardar.setEnabled(true);
                try {
                    Map<String, Object> m = get();
                    if (m == null) return;
                    ssid.setText(Json.getString(m, "ssid", ""));
                    clave.setText(Json.getString(m, "password", ""));
                    pais.setText(Json.getString(m, "country", pais.getText()));
                    arranca.setSelected(Boolean.TRUE.equals(m.get("autoStart")));
                    apagado.setValue((int) Json.getLong(m, "autoOff", 600));
                } catch (Exception ignorado) { /* sin nada que enseñar */ }
            }
        }.execute();
    }

    private void guarda() {
        final String red = ssid.getText();
        final String letras = pais.getText().trim().toUpperCase();
        final String laClave = new String(clave.getPassword());
        if (red.isEmpty() || red.getBytes(StandardCharsets.UTF_8).length > 32) { aviso("La red son de 1 a 32 bytes."); return; }
        if (laClave.getBytes(StandardCharsets.UTF_8).length > 64) { aviso("La clave son 64 bytes como mucho."); return; }
        if (!letras.isEmpty() && letras.length() != 2) { aviso("El país son dos letras, como ES."); return; }
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("ssid", red);
        m.put("password", laClave);
        if (!letras.isEmpty()) m.put("country", letras);
        m.put("autoStart", arranca.isSelected());
        m.put("autoOff", ((Number) apagado.getValue()).longValue());
        final byte[] datos = Json.write(m).getBytes(StandardCharsets.UTF_8);
        guardar.setEnabled(false);
        new SwingWorker<Void, Void>() {
            @Override protected Void doInBackground() throws IOException {
                backend.put(FICHERO, datos);
                return null;
            }
            @Override protected void done() {
                guardar.setEnabled(true);
                try {
                    get();
                    status.accept("WiFi: " + FICHERO + " guardado; reinicia la placa para que entre");
                    dispose();
                } catch (Exception e) {
                    Throwable causa = e.getCause() != null ? e.getCause() : e;
                    aviso("No se ha podido guardar: " + causa.getMessage());
                }
            }
        }.execute();
    }

    private void aviso(String texto) {
        JOptionPane.showMessageDialog(this, texto, "WiFi de la placa", JOptionPane.WARNING_MESSAGE);
    }
}
