// ============================================================
// OutputSink.java
// Destino del output de los opcodes PRINT_* del programa BP.
//
// Existe para desacoplar la VM del `System.out` directo, de modo que
// el IDE (cuando arranque la VM como subproceso) pueda inyectar un
// sink que serialice cada chunk al canal IDE↔VM y lo muestre en una
// ventana del propio IDE.
//
// Convención (V7/H7 #600, 28-sep): la VM guarda las piezas de un
// `print` (PRINT_NONL, FPRINT_NONL, PRINT_STR_NONL, PRINT_CHAR…) en la
// línea de SU hilo, y la línea entera llega aquí de una vez:
//   - PRINT_NL, y los que llevan salto (PRINT_STRING, LPRINT, DPRINT)
//                                               → writeLine(la línea).
//   - lo que un hilo deja a medias al acabar     → writeText(...).
// writeChar/newline siguen para quien escriba a mano.
//
// Las implementaciones DEBEN ser thread-safe: varios workers de la VM
// pueden empujar al mismo sink concurrentemente. Y writeLine ENTERA: con
// el mismo cerrojo que writeText, para que nada se cuele en medio de la
// línea (Eduardo: «si haces print "Resultado=", r que se cuele algo en
// medio no es correcto»).
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

    /** V7/H7 #600 — `s` y su salto de línea, de UNA vez: nada de otro hilo entre los dos. */
    void writeLine(String s);

    /** Solicita que cualquier buffer pendiente se vuelque. La VM lo
     *  llama al fin de la ejecución y en puntos sensibles del debugger. */
    void flush();
}
