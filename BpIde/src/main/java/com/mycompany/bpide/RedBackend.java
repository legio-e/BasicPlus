// ============================================================
// RedBackend.java
// V8/H3 (6-nov): la placa por la red. Es la misma placa que por el puerto serie —el mismo wire, y la hora, el log, la memoria y
// el INFO, que no da el de la VM Java—, sólo que por TCP. Por eso hereda de SerialBackend y lo único que cambia es el transporte.
//
// El endpoint es la IP o el nombre de la placa, con su puerto o sin él (3333, el de la placa).
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;

public final class RedBackend extends SerialBackend {

    @Override public String displayName() { return "Placa (red)"; }

    @Override protected void openTransport(String endpoint, BpvmClient c) throws IOException {
        String host = endpoint;
        int puerto = BuscarPlacas.PUERTO;
        int dos = endpoint.lastIndexOf(':');
        if (dos >= 0) {
            host = endpoint.substring(0, dos);
            try { puerto = Integer.parseInt(endpoint.substring(dos + 1).trim()); }
            catch (NumberFormatException nfe) { throw new IOException("puerto inválido en " + endpoint); }
        }
        try {
            c.connectRemote(host.trim(), puerto);
        } catch (IOException e) {
            throw new IOException("no se pudo conectar con la placa en " + host + ":" + puerto
                    + ": ¿tiene la WiFi encendida y está en esta red?", e);
        }
    }

    /** #803: por la red nadie avisa si la otra punta desaparece. */
    @Override protected void postConnect(BpvmClient c) throws IOException {
        super.postConnect(c);
        c.vigilarConexion();
    }

    @Override protected String helloLabel(String endpoint) {
        return "bpvm wire-v1 por la red @ " + endpoint;
    }
}
