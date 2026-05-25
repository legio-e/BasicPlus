// ============================================================
// BpvmBackend.java
// Implementación de Backend sobre BpvmClient — habla wire v1 (TCP) a
// un daemon BPVM ya corriendo en host:port. Hoy se usa contra la VM
// Java en local; mañana servirá también para el firmware Pico cuando
// éste se migre a v1.
//
// El daemon lo lanza el usuario manualmente, por ejemplo:
//   java -jar bpgenvm.jar --listen 7332 --workdir /tmp/pico-mock --wait-client
//
// No spawneamos subproceso desde aquí porque el caso de uso natural
// del panel es: "tengo un daemon corriendo en alguna parte, quiero
// operar contra él". El subproceso lo maneja el flow doRun/doDebug.
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Consumer;

public final class BpvmBackend implements Backend {

    /** Timeout para operaciones sincronas request/reply. */
    private static final long TIMEOUT_MS = 10_000L;
    /** Timeout para esperar EXITED tras un RUN. Generoso porque un
     *  programa BP puede tardar. */
    private static final long RUN_TIMEOUT_MS = 60 * 60 * 1000L; // 1h

    private BpvmClient client;

    @Override public String displayName() { return "VM Java (TCP v1)"; }

    @Override public String connect(String endpoint) throws IOException {
        if (endpoint == null || endpoint.isEmpty()) {
            throw new IOException("falta endpoint (ej. localhost:7332)");
        }
        String host;
        int port;
        int colon = endpoint.lastIndexOf(':');
        if (colon < 0) {
            host = "localhost";
            try { port = Integer.parseInt(endpoint); }
            catch (NumberFormatException nfe) {
                throw new IOException("endpoint inválido (esperado 'host:port'): " + endpoint);
            }
        } else {
            host = endpoint.substring(0, colon);
            try { port = Integer.parseInt(endpoint.substring(colon + 1)); }
            catch (NumberFormatException nfe) {
                throw new IOException("puerto inválido en endpoint: " + endpoint);
            }
            if (host.isEmpty()) host = "localhost";
        }
        BpvmClient c = new BpvmClient();
        c.connectRemote(host, port);
        if (!c.isHandshakeDone()) {
            c.close();
            throw new IOException("handshake con " + host + ":" + port + " no completado");
        }
        this.client = c;
        return "bpvm-java v1 @ " + host + ":" + port;
    }

    @Override public boolean isConnected() {
        return client != null && client.isHandshakeDone();
    }

    @Override public void close() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    @Override public List<Entry> list() throws IOException {
        require();
        List<BpvmClient.RemoteFile> raw = client.listFiles("", TIMEOUT_MS);
        List<Entry> out = new ArrayList<>(raw.size());
        for (BpvmClient.RemoteFile f : raw) {
            out.add(new Entry(f.name, f.size, f.isDirectory));
        }
        return out;
    }

    @Override public byte[] get(String path) throws IOException {
        require();
        return client.downloadFile(path, TIMEOUT_MS);
    }

    @Override public void put(String path, byte[] data) throws IOException {
        require();
        client.uploadFile(path, data, TIMEOUT_MS);
    }

    @Override public void del(String path) throws IOException {
        require();
        client.deleteFile(path, TIMEOUT_MS);
    }

    @Override public String run(String path, Consumer<String> lineSink) throws IOException {
        require();
        // Plumbing: cableamos el output sink y un event listener para
        // capturar EXITED. Tras el RUN bloqueamos en el future.
        CompletableFuture<String> done = new CompletableFuture<>();
        client.setOutputSink(line -> {
            if (lineSink != null) lineSink.accept(line);
        });
        client.setEventListener(ev -> {
            if (ev instanceof edu.bpgenvm.vm.debug.ExitedEvent) {
                edu.bpgenvm.vm.debug.ExitedEvent e = (edu.bpgenvm.vm.debug.ExitedEvent) ev;
                done.complete("exit " + e.exitCode + " (" + e.reason + ")");
            } else if (ev instanceof edu.bpgenvm.vm.debug.ExceptionEvent) {
                edu.bpgenvm.vm.debug.ExceptionEvent e = (edu.bpgenvm.vm.debug.ExceptionEvent) ev;
                done.complete("EXCEPTION: " + e.message);
            } else if (ev instanceof edu.bpgenvm.vm.debug.PausedEvent) {
                // El daemon arranca en STEP_INTO → primer hook pausa.
                // En el flow "Run" del Explorer queremos no-debug:
                // auto-continue al primer paused.
                client.sendCommand(edu.bpgenvm.vm.debug.StepCommand.CONTINUE);
            }
        });
        client.runModule(path);
        try {
            return done.get(RUN_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        } catch (TimeoutException te) {
            throw new IOException("timeout esperando EXITED del programa");
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
            throw new IOException("interrupted", ie);
        } catch (java.util.concurrent.ExecutionException ee) {
            throw new IOException("fallo en RUN", ee.getCause());
        } finally {
            client.setOutputSink(null);
            client.setEventListener(null);
        }
    }

    @Override public String mem() throws IOException {
        // v1 no tiene un equivalente directo de PicoClient.mem() ("free=Y").
        // Usamos DF como aproximación: total/used/free/fileCount.
        // PR-7b: DF aún no expuesto en BpvmClient — sintetizamos vía
        // listFiles para no añadir API ahora. Si esto creciera, abrir
        // BpvmClient.df() en su propio PR.
        require();
        List<BpvmClient.RemoteFile> files = client.listFiles("", TIMEOUT_MS);
        long usedBytes = 0;
        for (BpvmClient.RemoteFile f : files) usedBytes += f.size;
        return files.size() + " files, " + usedBytes + " bytes used";
    }

    @Override public void save() throws IOException {
        // VM Java siempre persiste (escribe directamente al workdir host).
        // Devolvemos OK silencioso para que el botón Save no haga ruido.
    }

    @Override public String log() throws IOException {
        throw new IOException("LOG_DUMP no soportado en VM Java (sólo Pico)");
    }

    @Override public void reset() throws IOException {
        // PR-7c — RESET tipado: BpvmClient.reset() manda el request,
        // espera el RESET_REPLY (corto timeout porque el server hace
        // System.exit poco después), y cierra el socket en el finally.
        // Anulamos `this.client` después porque ya está close()d.
        require();
        BpvmClient c = this.client;
        this.client = null;   // evitar usar el cliente cerrado
        c.reset();
    }

    private void require() throws IOException {
        if (!isConnected()) throw new IOException("BpvmBackend no conectado");
    }
}
