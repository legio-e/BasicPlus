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
 *  │ JList con ficheros del dispositivo     │
 *  │   - Hello.mod (1911 bytes)             │
 *  │   - BenchCpu.mod (1870 bytes)          │
 *  ├────────────────────────────────────────┤
 *  │ Status: COMxx  |  N ficheros, free=YYY │
 *  └────────────────────────────────────────┘
 *
 * Threading: todas las operaciones del puerto serie se ejecutan en un
 * SwingWorker para no bloquear el EDT. La UI muestra "trabajando..." en
 * la status bar y reactiva los botones cuando termina.
 */
package com.mycompany.bpide;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.List;
import java.util.function.Consumer;

public final class PicoExplorer extends JPanel {

    private final PicoClient client = new PicoClient();

    /** Sink al que mandar el output de RUN. Lo enchufa FrmMain a la consola. */
    private Consumer<String> outputSink;

    /* --- UI components ----------------------------------------- */
    private final JComboBox<String> portCombo = new JComboBox<>();
    private final JButton btnConnect = new JButton("Connect");
    private final JButton btnRefresh = new JButton("Refresh");
    private final JButton btnUpload  = new JButton("Upload…");
    private final JButton btnRun     = new JButton("Run");
    private final JButton btnGet     = new JButton("Download…");
    private final JButton btnDelete  = new JButton("Delete");
    private final JButton btnSave    = new JButton("Save");
    private final JButton btnLog     = new JButton("Log");
    private final JButton btnReset   = new JButton("Reset");

    private final DefaultListModel<PicoClient.RemoteFile> listModel =
            new DefaultListModel<>();
    private final JList<PicoClient.RemoteFile> fileList = new JList<>(listModel);
    private final JLabel status = new JLabel("Disconnected");

    public PicoExplorer() {
        super(new BorderLayout());

        // Toolbar 2 filas: fila 1 = puerto + connect; fila 2 = acciones.
        JPanel toolbar = new JPanel(new GridLayout(2, 1));
        JPanel row1 = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        row1.add(new JLabel("Port:"));
        portCombo.setPreferredSize(new Dimension(120, 22));
        row1.add(portCombo);
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

        toolbar.add(row1);
        toolbar.add(row2);
        add(toolbar, BorderLayout.NORTH);

        // Lista de ficheros remotos.
        fileList.setCellRenderer(new DefaultListCellRenderer() {
            @Override
            public Component getListCellRendererComponent(JList<?> list,
                    Object value, int index, boolean isSelected, boolean cellHasFocus) {
                JLabel l = (JLabel) super.getListCellRendererComponent(
                        list, value, index, isSelected, cellHasFocus);
                if (value instanceof PicoClient.RemoteFile) {
                    PicoClient.RemoteFile f = (PicoClient.RemoteFile) value;
                    l.setText(f.name + "  (" + f.size + " bytes)");
                    l.setIcon(UIManager.getIcon("FileView.fileIcon"));
                }
                return l;
            }
        });
        fileList.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) onRun();
            }
        });
        add(new JScrollPane(fileList), BorderLayout.CENTER);

        // Status bar.
        status.setBorder(BorderFactory.createEmptyBorder(2, 6, 2, 6));
        add(status, BorderLayout.SOUTH);

        // Refresca puertos al hacer click en el combo (mejor que solo al inicio).
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

    /** ¿Hay conexión activa al dispositivo? Lo consulta FrmMain antes de
     *  ofrecer "Run on Pico". */
    public boolean isConnected() { return client.isConnected(); }

    /**
     * Pipeline "Run on Pico" desde el IDE: sube un .mod local al
     * dispositivo (sobreescribiendo si ya existía con ese nombre) y lo
     * ejecuta inmediatamente. El output stream va al sink configurado.
     * No es bloqueante; corre en SwingWorker.
     *
     * Requiere que el cliente ya esté conectado (Connect manual desde
     * el panel).
     */
    public void uploadAndRun(File modFile) {
        uploadAndRun(modFile, java.util.Collections.emptyList());
    }

    /** Variante deps-aware: sube primero los .mod de la lista (drivers
     *  de dispositivo) y luego el principal, y ejecuta el principal. */
    public void uploadAndRun(File modFile, java.util.List<File> depMods) {
        if (!client.isConnected()) {
            if (outputSink != null) outputSink.accept(
                    "[Pico] no conectado — pulsa Connect primero");
            return;
        }
        if (modFile == null || !modFile.isFile()) {
            if (outputSink != null) outputSink.accept(
                    "[Pico] fichero no existe: " + modFile);
            return;
        }
        final String name = modFile.getName();
        final java.util.List<File> deps = (depMods != null)
                ? depMods : java.util.Collections.emptyList();
        status.setText("Compile&Run on Pico: uploading "
                + (deps.size() + 1) + " file(s)...");
        if (outputSink != null) {
            outputSink.accept("[Pico] subiendo " + name + " (" + modFile.length() + " bytes)"
                    + (deps.isEmpty() ? "" : " + " + deps.size() + " dep(s)"));
        }
        runAsync(() -> {
            // 0) Sondea LS para saber qué hay ya en la FS del Pico.
            //    PUT tras RUN tiene un bug en el firmware (USB CDC se
            //    atasca tras la primera sesión I/O larga). Workaround:
            //    si el .mod ya está allí con tamaño igual, saltamos
            //    el PUT y hacemos solo RUN — eso sí funciona.
            java.util.Map<String, Long> remote = new java.util.HashMap<>();
            try {
                java.util.List<PicoClient.RemoteFile> ls = client.ls();
                for (PicoClient.RemoteFile rf : ls) {
                    remote.put(rf.name, rf.size);
                }
            } catch (java.io.IOException lsErr) {
                // Si LS falla, seguimos sin caché — el PUT decidirá.
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Pico] LS falló, subiré todo: " + lsErr.getMessage()));
                }
            }

            // 1) Subir deps primero (drivers), solo los que no estén ya o tengan tamaño distinto.
            for (File dep : deps) {
                Long sz = remote.get(dep.getName());
                if (sz != null && sz == dep.length()) {
                    if (outputSink != null) {
                        SwingUtilities.invokeLater(() -> outputSink.accept(
                                "[Pico] " + dep.getName() + " ya en FS (" + dep.length()
                                + " bytes), salto PUT"));
                    }
                    continue;
                }
                byte[] depData = Files.readAllBytes(dep.toPath());
                client.put(dep.getName(), depData);
            }
            // 2) Subir el módulo principal solo si difiere.
            Long mainSz = remote.get(name);
            if (mainSz != null && mainSz == modFile.length()) {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(
                            "[Pico] " + name + " ya en FS (" + modFile.length()
                            + " bytes), salto PUT"));
                }
            } else {
                byte[] data = Files.readAllBytes(modFile.toPath());
                client.put(name, data);
            }
            // 3) Ejecutar el principal — sus deps ya están en el FS de la Pico.
            return client.run(name, line -> {
                if (outputSink != null) {
                    SwingUtilities.invokeLater(() -> outputSink.accept(line));
                }
            });
        }, statusStr -> {
            if (outputSink != null) {
                outputSink.accept("[Pico] VM finished: " + statusStr);
            }
            status.setText("Done: " + statusStr);
            onRefresh();
        });
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
        portCombo.setEnabled(!connected);
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
        if (client.isConnected()) {
            client.close();
            listModel.clear();
            setConnectedUI(false);
            status.setText("Disconnected");
            return;
        }
        String port = (String) portCombo.getSelectedItem();
        if (port == null || port.isEmpty()) {
            status.setText("No port selected");
            return;
        }
        status.setText("Connecting to " + port + "...");
        runAsync(() -> {
            client.connect(port, 115200);
            String hello = client.hello();
            return hello;
        }, hello -> {
            setConnectedUI(true);
            status.setText(port + " — " + hello);
            onRefresh();
        });
    }

    private void onRefresh() {
        if (!client.isConnected()) return;
        runAsync(() -> {
            List<PicoClient.RemoteFile> fs = client.ls();
            String mem = client.mem();
            return new Object[]{fs, mem};
        }, result -> {
            @SuppressWarnings("unchecked")
            List<PicoClient.RemoteFile> fs = (List<PicoClient.RemoteFile>) ((Object[]) result)[0];
            String mem = (String) ((Object[]) result)[1];
            listModel.clear();
            for (PicoClient.RemoteFile f : fs) listModel.addElement(f);
            status.setText(fs.size() + " files  |  " + mem);
        });
    }

    private void onUpload() {
        if (!client.isConnected()) return;
        JFileChooser fc = new JFileChooser();
        fc.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter(
                ".mod files", "mod"));
        if (fc.showOpenDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File f = fc.getSelectedFile();
        if (f == null) return;
        status.setText("Uploading " + f.getName() + "...");
        runAsync(() -> {
            byte[] data = Files.readAllBytes(f.toPath());
            client.put(f.getName(), data);
            return data.length;
        }, n -> {
            status.setText("Uploaded " + f.getName() + " (" + n + " bytes)");
            onRefresh();
        });
    }

    private void onRun() {
        if (!client.isConnected()) return;
        PicoClient.RemoteFile sel = fileList.getSelectedValue();
        if (sel == null) {
            status.setText("Select a file to run");
            return;
        }
        status.setText("Running " + sel.name + "...");
        if (outputSink != null) outputSink.accept("--- RUN " + sel.name + " on Pico ---");
        runAsync(() -> {
            return client.run(sel.name, line -> {
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
        if (!client.isConnected()) return;
        PicoClient.RemoteFile sel = fileList.getSelectedValue();
        if (sel == null) return;
        JFileChooser fc = new JFileChooser();
        fc.setSelectedFile(new File(sel.name));
        if (fc.showSaveDialog(this) != JFileChooser.APPROVE_OPTION) return;
        File out = fc.getSelectedFile();
        runAsync(() -> {
            byte[] data = client.get(sel.name);
            Files.write(out.toPath(), data);
            return data.length;
        }, n -> status.setText("Downloaded " + sel.name + " → "
                + out.getName() + " (" + n + " bytes)"));
    }

    private void onDelete() {
        if (!client.isConnected()) return;
        PicoClient.RemoteFile sel = fileList.getSelectedValue();
        if (sel == null) return;
        int rc = JOptionPane.showConfirmDialog(this,
                "Borrar " + sel.name + " del dispositivo?", "Confirmar",
                JOptionPane.YES_NO_OPTION);
        if (rc != JOptionPane.YES_OPTION) return;
        runAsync(() -> { client.del(sel.name); return null; },
                v -> { status.setText("Deleted " + sel.name); onRefresh(); });
    }

    private void onSave() {
        if (!client.isConnected()) return;
        status.setText("Saving FS to flash...");
        runAsync(() -> { client.save(); return null; },
                v -> status.setText("FS saved to flash"));
    }

    private void onLog() {
        if (!client.isConnected()) return;
        runAsync(client::log, txt -> {
            JTextArea area = new JTextArea(txt);
            area.setEditable(false);
            area.setFont(new Font("Consolas", Font.PLAIN, 11));
            JScrollPane sp = new JScrollPane(area);
            sp.setPreferredSize(new Dimension(600, 400));
            JOptionPane.showMessageDialog(this, sp,
                    "Persistent log — Pico", JOptionPane.PLAIN_MESSAGE);
        });
    }

    private void onReset() {
        if (!client.isConnected()) return;
        int rc = JOptionPane.showConfirmDialog(this,
                "Reiniciar el dispositivo? El puerto se desconectará.",
                "Confirmar reset", JOptionPane.YES_NO_OPTION);
        if (rc != JOptionPane.YES_OPTION) return;
        runAsync(() -> { client.reset(); return null; },
                v -> {
                    client.close();
                    listModel.clear();
                    setConnectedUI(false);
                    status.setText("Reset sent, port closed");
                });
    }

    /* ============================================================
     * Helpers para ejecutar ops en background y resolver en EDT.
     * ============================================================ */

    private interface IOAction<T> { T run() throws IOException; }

    private <T> void runAsync(IOAction<T> task, Consumer<T> onSuccess) {
        // Deshabilita botones mientras corre — evita doble-click race.
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
        boolean connected = client.isConnected();
        btnConnect.setEnabled(true);   // siempre disponible
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
