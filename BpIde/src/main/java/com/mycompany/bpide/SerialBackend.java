// ============================================================
// SerialBackend.java
// Backend BPVM v1 sobre USB CDC contra el firmware Pico.
// Hermano TCP: BpvmBackend (mismo wire, distinto transporte).
//
// La parte común (list/get/put/del/run/reset, framing, chunk→línea,
// auto-CONTINUE PausedEvent) vive en AbstractBpvmBackend. Aquí solo
// queda:
//   - openTransport: connectSerial(port, baud).
//   - postConnect: syncTime al RTC del Pico — best-effort.
//   - mem(): vía INFO (fsTotalBytes/fsUsedBytes) + LIST para count.
//   - save(): SAVE real (persiste FS RAM a flash).
//   - log(): LOG_DUMP del log persistente.
//   - bootsel(): extensión Pico-only (reboot al USB MSC bootloader).
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;

import java.io.IOException;
import java.util.List;
import java.util.Map;

public final class SerialBackend extends AbstractBpvmBackend {

    /** Baud nominal — USB CDC ignora el valor (no necesita baud rate);
     *  lo dejamos a 115200 por inercia y para herramientas externas. */
    private static final int DEFAULT_BAUD = 115200;

    @Override public String displayName() { return "Pico (serial v1)"; }

    @Override protected void openTransport(String endpoint, BpvmClient c) throws IOException {
        c.connectSerial(endpoint, DEFAULT_BAUD);
    }

    @Override protected String helloLabel(String endpoint) {
        return "bpvm-pico wire-v1 @ " + endpoint;
    }

    @Override protected void postConnect(BpvmClient c) throws IOException {
        // Sincronizar RTC del Pico con wall clock del PC. Best-effort:
        // si el firmware no soporta TIME (versión vieja) log y seguir.
        try { c.syncTime(System.currentTimeMillis() / 1000L, TIMEOUT_MS); }
        catch (IOException ignored) { /* anecdotal */ }
    }

    @Override public String mem() throws IOException {
        require();
        Map<String, Object> info = client.getInfo(TIMEOUT_MS);
        long total = Json.getLong(info, "fsTotalBytes", 0);
        long used  = Json.getLong(info, "fsUsedBytes", 0);
        long free  = total - used;
        // fileCount lo sacamos del LIST (barato — entries ya van en RAM
        // del firmware). Si crece el coste, se puede añadir a INFO.
        int fileCount;
        try { fileCount = client.listFiles("", TIMEOUT_MS).size(); }
        catch (IOException ioe) { fileCount = -1; }
        return "total=" + total + " used=" + used + " free=" + free
                + " count=" + fileCount;
    }

    @Override public void save() throws IOException {
        require();
        client.save(TIMEOUT_MS);
    }

    @Override public String log() throws IOException {
        require();
        return client.logDump(TIMEOUT_MS);
    }

    /** Pico-only: reboot al bootloader BOOTSEL (USB MSC para reflashear).
     *  El backend genérico no lo expone — quien la llama hace cast a
     *  SerialBackend. */
    public void bootsel() throws IOException {
        require();
        BpvmClient c = this.client;
        this.client = null;
        c.bootsel();
    }
}
