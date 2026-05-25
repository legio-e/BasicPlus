// ============================================================
// Backend.java
// Abstracción del transporte que PicoExplorer usa para operar contra
// un servidor BPVM (firmware Pico via serial, o VM-Java daemon via
// TCP wire v1).
//
// El motivo de la abstracción: hoy PicoExplorer está pegado a
// PicoClient (USB CDC line-based). Ahora que la VM-Java habla v1
// completo (PRs 1-6), queremos que el panel pueda apuntar también al
// daemon Java local — útil para iterar UI sin tocar el firmware ni
// requerir HW.
//
// Cuando el firmware Pico se migre a v1, SerialBackend se reescribirá
// para hablar v1 sobre USB CDC y BpvmBackend cubrirá tanto local como
// remoto. El interface aquí ya quedará listo.
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.List;
import java.util.function.Consumer;

public interface Backend extends AutoCloseable {

    /** Entrada de un directorio devuelta por list(). Neutral entre
     *  backends (PicoClient.RemoteFile no tiene isDir; BpvmClient sí).*/
    final class Entry {
        public final String name;
        public final long size;
        public final boolean isDir;
        public Entry(String name, long size, boolean isDir) {
            this.name = name;
            this.size = size;
            this.isDir = isDir;
        }
        @Override public String toString() {
            return (isDir ? "[D] " : "") + name + " (" + size + " bytes)";
        }
    }

    /** Nombre legible para mostrar en la UI ("Pico (serial)", "VM Java"). */
    String displayName();

    /** Conecta usando una cadena de endpoint cuyo formato depende del
     *  backend:
     *    - SerialBackend: "COMxx" (nombre de puerto serie).
     *    - BpvmBackend:   "host:port" (TCP).
     *  Devuelve un string de identificación del servidor (para status). */
    String connect(String endpoint) throws IOException;

    boolean isConnected();
    @Override void close();

    // ---- Operaciones de FS ----

    List<Entry> list() throws IOException;
    byte[] get(String path) throws IOException;
    void   put(String path, byte[] data) throws IOException;
    void   del(String path) throws IOException;

    // ---- Ejecución ----

    /** Arranca el programa en `path`. Cada línea de output del programa
     *  se entrega al `lineSink` a medida que llega. Bloquea hasta que
     *  el programa termina; devuelve un string de status ("OK",
     *  "ERROR …", etc.) para mostrar al usuario. */
    String run(String path, Consumer<String> lineSink) throws IOException;

    // ---- Info / housekeeping (algunos backends devuelven UNSUPPORTED) ----

    /** Stats de memoria/FS en formato libre, una línea, para status bar. */
    String mem() throws IOException;

    /** Persiste el FS RAM a flash. UnsupportedOperationException si no
     *  aplica (e.g. VM Java siempre persiste). */
    void save() throws IOException;

    /** Texto del log persistente del firmware. UnsupportedOperationException
     *  si no aplica. */
    String log() throws IOException;

    /** Reboot/reset del backend. */
    void reset() throws IOException;
}
