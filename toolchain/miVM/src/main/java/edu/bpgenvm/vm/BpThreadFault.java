// ============================================================
// BpThreadFault.java
// Excepción que señala que un thread BP individual encontró un
// error de runtime que NO debe tumbar la VM entera — sólo mata
// al thread que la levantó.
//
// Casos típicos: violación de Mutex (re-entrada, unlock no propio),
// y futuros runtime errors que deberían poder ser atrapados con
// `try/catch e: RuntimeError` desde BP. Mientras la VM no soporte
// construir+throwear instancias BP de RuntimeError desde código
// nativo, esta excepción es el puente: el scheduler la atrapa,
// imprime un mensaje BP-style con tid + worker, y deja que el
// resto de threads continúe.
// ============================================================
package edu.bpgenvm.vm;

public class BpThreadFault extends RuntimeException {
    public BpThreadFault(String message) { super(message); }
}
