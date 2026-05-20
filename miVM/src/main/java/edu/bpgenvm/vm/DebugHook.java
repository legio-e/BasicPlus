package edu.bpgenvm.vm;

/**
 * Hook de depuración que el VM invoca antes de despachar cada opcode
 * cuyo PC cae en una línea origen distinta a la anterior. El IDE
 * instala una implementación de este hook vía
 * {@link VirtualMachine#setDebugHook(DebugHook)}; cuando el hook está
 * a null la VM no paga ningún coste de lookup.
 *
 * <p>La implementación típica (en el IDE) decide si pausar el thread BP
 * según breakpoints o modo step. Si pausa, bloquea el thread (vía
 * {@code Object.wait} o una {@code BlockingQueue}) hasta que el usuario
 * dé un comando de continuación; mientras está bloqueado el thread, el
 * EDT del IDE puede inspeccionar el {@link DebugContext} con total
 * seguridad de que la memoria de la VM no cambia.</p>
 */
public interface DebugHook {
    /**
     * Llamado por la VM justo antes del opcode en {@code ctx.absPc}, sólo
     * cuando la línea origen cambia respecto al opcode anterior (o cuando
     * el thread se posa por primera vez en una línea conocida).
     */
    void onLineChange(DebugContext ctx);
}
