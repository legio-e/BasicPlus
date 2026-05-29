// ============================================================
// Backend.java
// Abstracción del transporte que PicoExplorer usa para operar contra
// un servidor BPVM (firmware Pico via USB CDC, o VM-Java daemon/remoto
// via TCP). Ambos hablan el mismo wire v1.
//
// Implementaciones:
//   - SerialBackend → wire v1 sobre USB CDC al firmware Pico.
//   - BpvmBackend   → wire v1 sobre TCP a la VM Java (local o remoto).
//
// La parte común (list/get/put/del/run/reset, framing, chunk→línea,
// auto-CONTINUE PausedEvent) vive en AbstractBpvmBackend. Las dos
// concretas solo aportan qué `BpvmClient.connect*` invocar y un par de
// operaciones plataforma-específicas (mem/save/log) donde el wire v1
// admite divergencia legítima.
//
// Histórico (#137): antes de la migración a wire v1, el SerialBackend
// usaba PicoClient (cliente texto-protocolo legacy). #150/#151 lo
// reemplazaron por BpvmClient.connectSerial(), dejando PicoClient
// como código muerto que #137 retiró. La enumeración de puertos
// vive ahora en SerialPorts.
// ============================================================
package com.mycompany.bpide;

import java.io.IOException;
import java.util.List;
import java.util.function.Consumer;

public interface Backend extends AutoCloseable {

    /** Entrada de un directorio devuelta por list(). Neutral entre
     *  backends — todos hablan wire v1 sobre BpvmClient.RemoteFile, que
     *  ya trae `isDirectory`; aquí lo normalizamos al campo `isDir`. */
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

    /** Borra el log persistente (RAM + flash) en el dispositivo. Tras
     *  esto, log() devuelve cadena vacía hasta que el firmware vuelva a
     *  loggear algo y haga flush. UnsupportedOperationException si no
     *  aplica al backend (e.g. VM Java). */
    default void clearLog() throws IOException {
        throw new UnsupportedOperationException("clearLog no soportado");
    }

    /** Reboot/reset del backend. */
    void reset() throws IOException;
}
