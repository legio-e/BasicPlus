// ============================================================
// VmClientSmoke.java
// Smoke test ejecutable del VmClient: lanza la VM como subproceso,
// ejecuta un .mod, y vuelca a stdout todo lo que sale por el canal
// (print del programa + eventos del debugger).
//
// Para correrlo:
//   mvn -pl BpIde exec:java -Dexec.mainClass=com.mycompany.bpide.VmClientSmoke \
//                           -Dexec.args="C:/.../MoveTest.mod"
//
// O directamente con java -cp con el classpath ya construido por Maven.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.DebugEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

public final class VmClientSmoke {
    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("uso: VmClientSmoke <fichero.mod o .bpproject>");
            System.exit(2);
        }
        String target = args[0];

        try (VmClient vm = new VmClient()) {
            vm.setDiagSink(s -> System.err.println("DIAG  " + s));
            vm.setOutputSink(s -> System.out.print("OUT   " + s.replace("\n", "\\n\n")));
            vm.setEventListener(new edu.bpgenvm.vm.debug.DebugListener() {
                @Override public void onEvent(DebugEvent ev) {
                    System.out.println("EVENT " + ev);
                    if (ev instanceof PausedEvent) {
                        // Auto-continue: el primer paused es por STEP_INTO en línea 1.
                        new Thread(() -> {
                            try { Thread.sleep(80); } catch (InterruptedException ignored) {}
                            vm.sendCommand(StepCommand.CONTINUE);
                        }, "smoke-continue").start();
                    } else if (ev instanceof ResumedEvent) {
                        // nada
                    }
                }
            });

            vm.start(target, true);   // --wait-client
            // Sin --exited todavía: damos margen y luego cerramos.
            Thread.sleep(3000);
        }
        System.out.println("DONE");
    }
}
