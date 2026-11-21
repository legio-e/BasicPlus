/*
 * DeviceFileEditor.java — H12 / #231
 *
 * Visor-editor de un fichero que vive en el dispositivo (Pico/STM32/ESP32
 * por serie, o la VM-Java por TCP). Se abre desde PicoExplorer al hacer
 * doble clic en un fichero del árbol, con el botón "Edit", o desde la
 * consola (`edit <fich>`).
 *
 * Comportamiento:
 *   - Lee el contenido con Backend.get(path) (lo hace el llamante y nos lo
 *     pasa ya cargado, para no bloquear el EDT en el constructor).
 *   - Si el contenido parece TEXTO (config .json, .txt, logs, .bp, …) se
 *     muestra editable y se reescribe con Backend.put(path, bytes).
 *   - Si parece BINARIO (.mod y demás) se muestra como volcado hexadecimal
 *     de SOLO LECTURA — para inspeccionar, no para romper.
 *
 * Threading: guardar/recargar van en SwingWorker (I/O de puerto serie);
 * el EDT no se bloquea. Control de cambios: avisa antes de cerrar/recargar
 * si hay ediciones sin guardar.
 *
 * Es deliberadamente independiente del editor de código de FrmMain (que
 * está atado a ficheros locales, breakpoints y compilación): un fichero
 * del device es otra cosa, y mezclarlos arriesgaría esa maquinaria.
 */
package com.mycompany.bpide;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.nio.charset.StandardCharsets;

final class DeviceFileEditor extends JDialog {

    private final Backend backend;
    private final String path;
    private final Runnable onSaved;     // refresca el árbol del explorador (el tamaño cambia)
    private final boolean editable;     // false = binario → hex, solo lectura

    private final JTextArea area = new JTextArea();
    private final JLabel statusLbl = new JLabel(" ");
    private final JButton btnSave = new JButton("Guardar");
    private final JButton btnReload = new JButton("Recargar");
    private final JButton btnClose = new JButton("Cerrar");

    private boolean dirty = false;
    private boolean suppress = false;   // ignora eventos del Document al poblar/recargar

    DeviceFileEditor(Window owner, Backend backend, String path,
                     byte[] content, Runnable onSaved) {
        super(owner, "Device: " + path, ModalityType.MODELESS);
        this.backend = backend;
        this.path = path;
        this.onSaved = onSaved;
        this.editable = looksLikeText(content);

        area.setFont(new Font("Consolas", Font.PLAIN, 12));
        area.setEditable(editable);
        area.setLineWrap(false);
        suppress = true;
        area.setText(editable
                ? new String(content, StandardCharsets.UTF_8)
                : hexDump(content));
        area.setCaretPosition(0);
        suppress = false;
        area.getDocument().addDocumentListener(new DocumentListener() {
            @Override public void insertUpdate(DocumentEvent e) { markDirty(); }
            @Override public void removeUpdate(DocumentEvent e) { markDirty(); }
            @Override public void changedUpdate(DocumentEvent e) { markDirty(); }
        });

        // Barra superior con las acciones.
        JPanel bar = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        bar.add(btnSave);
        bar.add(btnReload);
        bar.add(btnClose);
        if (!editable) {
            JLabel ro = new JLabel("  binario — solo lectura (volcado hex)");
            ro.setForeground(Color.GRAY);
            bar.add(ro);
        }
        btnSave.setEnabled(false);          // se habilita al primer cambio

        statusLbl.setBorder(BorderFactory.createEmptyBorder(2, 6, 2, 6));
        statusLbl.setText(content.length + " bytes" + (editable ? "" : " (binario)"));

        setLayout(new BorderLayout());
        add(bar, BorderLayout.NORTH);
        add(new JScrollPane(area), BorderLayout.CENTER);
        add(statusLbl, BorderLayout.SOUTH);

        btnSave.addActionListener(e -> doSave());
        btnReload.addActionListener(e -> doReload());
        btnClose.addActionListener(e -> tryClose());

        setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
        addWindowListener(new WindowAdapter() {
            @Override public void windowClosing(WindowEvent e) { tryClose(); }
        });

        setSize(700, 540);
        setLocationRelativeTo(owner);
    }

    /* ---- estado de edición ------------------------------------ */

    private void markDirty() {
        if (suppress || !editable || dirty) return;
        dirty = true;
        setTitle("Device: " + path + " *");
        btnSave.setEnabled(true);
    }

    private void clearDirty() {
        dirty = false;
        setTitle("Device: " + path);
        btnSave.setEnabled(false);
    }

    /* ---- acciones --------------------------------------------- */

    private void doSave() {
        if (!editable) return;
        final byte[] data = area.getText().getBytes(StandardCharsets.UTF_8);
        btnSave.setEnabled(false);
        statusLbl.setText("Guardando…");
        new SwingWorker<Void, Void>() {
            @Override protected Void doInBackground() throws Exception {
                backend.put(path, data);
                return null;
            }
            @Override protected void done() {
                try {
                    get();
                    clearDirty();
                    statusLbl.setText("Guardado: " + data.length + " bytes en " + path);
                    if (onSaved != null) onSaved.run();
                } catch (Exception ex) {
                    Throwable c = (ex.getCause() != null) ? ex.getCause() : ex;
                    statusLbl.setText("ERROR al guardar: " + c.getMessage());
                    btnSave.setEnabled(true);
                }
            }
        }.execute();
    }

    private void doReload() {
        if (dirty) {
            int rc = JOptionPane.showConfirmDialog(this,
                    "Hay cambios sin guardar. ¿Descartarlos y recargar desde el device?",
                    "Recargar", JOptionPane.YES_NO_OPTION);
            if (rc != JOptionPane.YES_OPTION) return;
        }
        statusLbl.setText("Recargando…");
        new SwingWorker<byte[], Void>() {
            @Override protected byte[] doInBackground() throws Exception {
                return backend.get(path);
            }
            @Override protected void done() {
                try {
                    byte[] data = get();
                    suppress = true;
                    area.setText(editable
                            ? new String(data, StandardCharsets.UTF_8)
                            : hexDump(data));
                    area.setCaretPosition(0);
                    suppress = false;
                    clearDirty();
                    statusLbl.setText(data.length + " bytes (recargado)");
                } catch (Exception ex) {
                    Throwable c = (ex.getCause() != null) ? ex.getCause() : ex;
                    statusLbl.setText("ERROR al recargar: " + c.getMessage());
                }
            }
        }.execute();
    }

    private void tryClose() {
        if (dirty) {
            int rc = JOptionPane.showConfirmDialog(this,
                    "Hay cambios sin guardar en " + path + ". ¿Cerrar sin guardar?",
                    "Cerrar", JOptionPane.YES_NO_OPTION);
            if (rc != JOptionPane.YES_OPTION) return;
        }
        dispose();
    }

    /* ---- helpers de contenido --------------------------------- */

    /** Heurística texto-vs-binario: sin NUL y con <1% de caracteres de
     *  control "raros" (se permiten \t \n \r). Los bytes altos (UTF-8
     *  multibyte) cuentan como texto. */
    static boolean looksLikeText(byte[] b) {
        if (b.length == 0) return true;
        long ctrl = 0;
        for (byte x : b) {
            int v = x & 0xFF;
            if (v == 0) return false;                       // NUL → binario
            if (v < 0x20 && v != '\t' && v != '\n' && v != '\r') ctrl++;
        }
        return ctrl * 100L <= b.length;
    }

    /** Volcado hex clásico: offset, 16 bytes en hex, columna ASCII. */
    static String hexDump(byte[] b) {
        StringBuilder sb = new StringBuilder(b.length * 4 + 64);
        for (int i = 0; i < b.length; i += 16) {
            sb.append(String.format("%08X  ", i));
            StringBuilder ascii = new StringBuilder(16);
            for (int j = 0; j < 16; j++) {
                if (i + j < b.length) {
                    int v = b[i + j] & 0xFF;
                    sb.append(String.format("%02X ", v));
                    ascii.append((v >= 0x20 && v < 0x7F) ? (char) v : '.');
                } else {
                    sb.append("   ");
                }
                if (j == 7) sb.append(' ');                 // separador de medio
            }
            sb.append(" |").append(ascii).append("|\n");
        }
        return sb.toString();
    }
}
