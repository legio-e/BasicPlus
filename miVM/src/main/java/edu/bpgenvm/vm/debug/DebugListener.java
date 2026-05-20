// ============================================================
// DebugListener.java
// Receptor de eventos del debugger. Reemplaza al antiguo
// DebugSession.Listener (que tenía métodos separados onPaused/
// onResumed con la firma acoplada al DebugContext live).
//
// Razón del cambio: cuando la VM sea un proceso/dispositivo
// separado, los eventos son lo único que cruza el canal. Pasar
// un solo tipo polimórfico mantiene la API constante entre los
// modos in-process y remoto: el callback es el mismo, lo que
// cambia es el origen del DebugEvent.
//
// Las queries de memoria/locals/stack se hacen aparte —
// hoy via DebugContext, mañana via un cliente RPC.
// ============================================================
package edu.bpgenvm.vm.debug;

public interface DebugListener {
    /**
     * Llamado por la VM (o el adaptador local) cada vez que aparece un
     * evento. Puede ser invocado desde el thread worker BP: si necesitas
     * tocar la UI, reenvía a tu EDT/equivalente con invokeLater.
     */
    void onEvent(DebugEvent e);
}
