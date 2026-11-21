// ============================================================
// PromptSender.java
// Interface para que la VM emita un promptRequest al cliente IDE
// sin acoplarse directamente al DebugServer (que es del paquete
// `edu.bpgenvm.vm.debug` y la VM lo cablea como dependencia opcional).
//
// La VM llama a `send(requestId, spec)` desde el builtin PROMPT.
// Cuando el cliente responda con promptResponse, el DebugServer
// llama a `vm.deliverPromptResponse(requestId, values)` para
// despertar el thread bloqueado.
// ============================================================
package edu.bpgenvm.vm.debug;

public interface PromptSender {
    /** Envía un mensaje al cliente IDE pidiéndole renderizar un formulario.
     *  @param requestId  ID asignado por la VM, vuelve en la respuesta.
     *  @param spec       contenido del formulario (típicamente JSON-string
     *                    que el IDE parsea; opaco para la VM). */
    void send(long requestId, String spec);
}
