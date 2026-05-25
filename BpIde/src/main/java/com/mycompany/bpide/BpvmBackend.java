// ============================================================
// BpvmBackend.java
// Backend BPVM v1 sobre TCP a un daemon VM Java ya corriendo en
// host:port. Hermano serie: SerialBackend (mismo wire, distinto
// transporte).
//
// El daemon lo lanza el usuario manualmente, por ejemplo:
//   java -jar bpgenvm.jar --listen 7332 --workdir /tmp/work --wait-client
//
// No spawneamos subproceso desde aquí porque el caso de uso natural
// del panel es: "tengo un daemon corriendo, quiero operar contra él".
// El subproceso lo maneja el flow doRun/doDebug.
//
// La parte común (list/get/put/del/run/reset, framing, chunk→línea,
// auto-CONTINUE PausedEvent) vive en AbstractBpvmBackend. Aquí solo:
//   - openTransport: connectRemote(host, port).
//   - mem(): sintetizado de listFiles (la VM Java no expone DF aún).
//   - save(): no-op (VM Java siempre persiste al workdir host).
//   - log(): UNSUPPORTED (no hay log persistente en VM Java).
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.List;

public final class BpvmBackend extends AbstractBpvmBackend {

    @Override public String displayName() { return "VM Java (TCP v1)"; }

    @Override protected void openTransport(String endpoint, BpvmClient c) throws IOException {
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
        c.connectRemote(host, port);
    }

    @Override protected String helloLabel(String endpoint) {
        return "bpvm-java v1 @ " + endpoint;
    }

    @Override public String mem() throws IOException {
        // VM Java no expone DF en wire v1 (la stdlib es la del workdir
        // host, sin límites). Sintetizamos sumando tamaños de listFiles
        // — mismo formato que SerialBackend para que la UI muestre algo
        // uniforme en la status bar.
        require();
        List<BpvmClient.RemoteFile> files = client.listFiles("", TIMEOUT_MS);
        long usedBytes = 0;
        for (BpvmClient.RemoteFile f : files) usedBytes += f.size;
        return "files=" + files.size() + " usedBytes=" + usedBytes;
    }

    @Override public void save() throws IOException {
        // VM Java siempre persiste (escribe directamente al workdir host).
        // Devolvemos OK silencioso para que el botón Save no haga ruido.
    }

    @Override public String log() throws IOException {
        throw new IOException("LOG_DUMP no soportado en VM Java (solo Pico)");
    }
}
