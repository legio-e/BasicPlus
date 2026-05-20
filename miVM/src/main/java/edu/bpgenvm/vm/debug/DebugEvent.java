// ============================================================
// DebugEvent.java
// Evento serializable que la VM emite al cliente del debugger (hoy el
// IDE en proceso; en A1.4+ el IDE remoto vía socket).
//
// Diseño:
//  - Inmutable (todos los campos final).
//  - Sólo datos (sin referencias a estado vivo del VM). Para queries
//    de memoria/stack el cliente usa una DebugQueries aparte; en
//    local hoy es DebugContext, en remoto será un cliente RPC.
//  - El tipo de evento se discrimina por la subclase concreta. Cuando
//    serialicemos a JSON, usaremos un campo "type": "paused" / "resumed"
//    / "exited" / "exception".
//  - Los campos están pensados para ir directamente al wire — nombres
//    estables, tipos primitivos o String.
// ============================================================
package edu.bpgenvm.vm.debug;

public abstract class DebugEvent {
    /** Marca temporal (epoch ms). Útil para ordenar eventos en logs y
     *  para que el cliente sepa cuán fresco es lo que recibió. */
    public final long timestampMs;

    protected DebugEvent() {
        this.timestampMs = System.currentTimeMillis();
    }

    /** Identificador estable del tipo de evento. Usado al serializar
     *  a JSON: { "type": "paused", ... }. */
    public abstract String type();
}
