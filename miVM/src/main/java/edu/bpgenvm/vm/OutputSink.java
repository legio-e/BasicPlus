// ============================================================
// OutputSink.java
// Destino del output de los opcodes PRINT_* del programa BP.
//
// Existe para desacoplar la VM del `System.out` directo, de modo que
// el IDE (cuando arranque la VM como subproceso) pueda inyectar un
// sink que serialice cada chunk al canal IDE↔VM y lo muestre en una
// ventana del propio IDE.
//
// Convención:
//   - PRINT_NONL / FPRINT_NONL / PRINT_STR_NONL → writeText(...)
//     (sin salto de línea final).
//   - PRINT_CHAR                                → writeChar(...).
//   - PRINT_NL                                  → newline().
//   - PRINT_STRING (opcode legacy con \n)       → writeText + newline.
//
// Las implementaciones DEBEN ser thread-safe: varios workers de la VM
// pueden empujar caracteres al mismo sink concurrentemente.
//
// Las funciones diagnósticas de la VM (GC log, startup banner,
// opcode trace) siguen usando System.out — no son output del programa
// BP. Esto se documenta y se respeta.
// ============================================================
package edu.bpgenvm.vm;

public interface OutputSink {
    /** Escribe `s` sin añadir salto de línea. */
    void writeText(String s);

    /** Escribe un único carácter (de PRINT_CHAR). */
    void writeChar(char c);

    /** Termina la línea actual. */
    void newline();

    /** Solicita que cualquier buffer pendiente se vuelque. La VM lo
     *  llama al fin de la ejecución y en puntos sensibles del debugger. */
    void flush();
}
