// ============================================================
// TrazaDeLaVmLocal.java
// V8 #788 (13-nov) — CON LA VM LOCAL, DÓNDE FALLÓ EL PROGRAMA, con sus líneas, como con la placa: el stackTrace del
// EXITED («IdeFallaLib+43 IdeFalla+56»), traducido por TrazaError, a la consola: «en IdeFallaLib.bp:7», «llamado desde
// IdeFalla.bp:7». Antes sólo salía la línea en bruto de la VM («[vm] === traza: … ===»): la traducción estaba sólo en el
// camino de la placa (AbstractBpvmBackend.run).
//
// UNA vez por Run: con un error, la VM-Java manda dos EXITED (la excepción plegada y el fin; DebugServer), y los dos
// traen la traza. Lo usa FrmMain.runOnVmRemote; lo prueba TrazaVmLocalSmoke.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.ExitedEvent;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;

final class TrazaDeLaVmLocal {
    private final Consumer<String> consola;
    private final AtomicBoolean dicha = new AtomicBoolean();

    /** {@code consola} recibe cada línea, sin el salto. */
    TrazaDeLaVmLocal(Consumer<String> consola) { this.consola = consola; }

    /** Con cada EXITED: la primera traza que llegue, a la consola; las demás, no. */
    void alAcabar(ExitedEvent e) {
        if (e == null || e.stackTrace == null || e.stackTrace.trim().isEmpty()) return;
        if (!dicha.compareAndSet(false, true)) return;
        for (String l : TrazaError.lineas(TrazaError.traduce(e.stackTrace))) consola.accept(l);
    }
}
