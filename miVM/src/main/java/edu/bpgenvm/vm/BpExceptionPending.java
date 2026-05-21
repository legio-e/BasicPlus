// ============================================================
// BpExceptionPending.java
// Señal interna: un builtin nativo (típicamente uno de mutex / IO /
// sandbox path) ha construido una instancia BP de RuntimeError y la
// ha empujado al stack del ThreadContext. El dispatcher CALL_BUILTIN
// del intérprete la captura y ejecuta el unwind igual que haría el
// opcode THROW.
//
// Diferencia clave con BpThreadFault:
//   - BpThreadFault termina el thread BP que la lanzó. Mensaje a stderr.
//     El programa NO puede atraparla.
//   - BpExceptionPending TRANSFIERE control al handler `catch e:
//     RuntimeError` más cercano en la cadena handlerStack del thread.
//     Si no hay handler que la atrape, la VM la convierte en stack-trace
//     "uncaught exception" y termina el thread (mismo efecto que un
//     `throw` BP sin atrapar).
//
// Construcción: el builtin llama a VM.throwBpRuntimeError(tc, msg), que
// se encarga de TODO (alocar string, alocar objeto, push, throw).
// ============================================================
package edu.bpgenvm.vm;

public class BpExceptionPending extends RuntimeException {
    /** Referencia BP al objeto exception ya empujado al stack del tc.
     *  Lo guardamos también aquí para que el catch del dispatcher pueda
     *  acceder al ref incluso si el push al stack falló por algún motivo. */
    public final int objRef;
    public BpExceptionPending(int objRef) {
        super("BpExceptionPending(ref=" + objRef + ")");
        this.objRef = objRef;
    }
}
