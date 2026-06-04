// ============================================================
// DebugSessionSmoke.java
// Valida A1.9: DebugSession con BpvmClient attached.
//   - Pone un breakpoint en una línea conocida ANTES del attach.
//   - attach replica el breakpoint al wire.
//   - addListener registra PausedEvent → llama a las queries
//     (getLocals/getStackFrames/getModuleProperties).
//   - Manda continue cuando pausa.
//
// Uso:
//   mvn -pl BpIde exec:java -Dexec.mainClass=com.mycompany.bpide.DebugSessionSmoke \
//                           -Dexec.args="<ruta.mod> <basename.bp> <linea>"
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DebugEvent;
import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public final class DebugSessionSmoke {
    public static void main(String[] args) throws Exception {
        if (args.length < 3) {
            System.err.println("uso: DebugSessionSmoke <mod> <basename.bp> <linea>");
            System.exit(2);
        }
        String target = args[0];
        String bpFile = args[1];
        int bpLine = Integer.parseInt(args[2]);

        DebugSession debug = new DebugSession();
        // Breakpoint ANTES del attach: debe replicarse al wire al conectar.
        debug.toggleBreakpoint(bpFile, bpLine);

        AtomicInteger pauseCount = new AtomicInteger(0);

        try (BpvmClient client = new BpvmClient()) {
            client.setDiagSink(s -> System.err.println("DIAG  " + s));
            client.setOutputSink(s -> System.out.print("OUT   " + s.replace("\n", "\\n\n")));

            debug.addListener(new edu.bpgenvm.vm.debug.DebugListener() {
                @Override public void onEvent(DebugEvent ev) {
                    if (ev instanceof PausedEvent) {
                        PausedEvent pe = (PausedEvent) ev;
                        int n = pauseCount.incrementAndGet();
                        System.out.println("PAUSE " + n + " line=" + pe.line + " tid=" + pe.tid);
                        // Queries durante pausa.
                        int[] locals = debug.getLocals(2000);
                        List<BpvmClient.NamedLocal> named = debug.getNamedLocals(2000);  // H6.a.1
                        List<int[]> frames = debug.getStackFrames(2000);
                        List<ModuleManager.PropertyView> props =
                                debug.getModuleProperties(2000);
                        System.out.println("  locals=" + locals.length
                                + " named=" + named.size()
                                + " frames=" + frames.size()
                                + " props=" + props.size());
                        for (BpvmClient.NamedLocal nl : named) {
                            System.out.println("    " + nl.name + " = "
                                    + (nl.isArray ? ("array[len=" + nl.value + "]")
                                                  : Long.toString(nl.value))
                                    + "  (bp+" + nl.offset + ", " + nl.size + "B)");
                        }
                        // Continue tras procesar.
                        new Thread(() -> {
                            try { Thread.sleep(50); } catch (InterruptedException ignored) {}
                            debug.sendCommand(StepCommand.CONTINUE);
                        }, "smoke-continue-" + n).start();
                    } else if (ev instanceof ResumedEvent) {
                        // silencio
                    } else if (ev instanceof ExitedEvent) {
                        ExitedEvent xe = (ExitedEvent) ev;
                        System.out.println("EXIT  code=" + xe.exitCode + " " + xe.reason);
                    }
                }
            });

            client.start(target, /*waitClient=*/true);
            debug.attach(client);
            try {
                client.waitForExit();
            } finally {
                debug.detach();
            }
        }
        System.out.println("DONE pauseCount=" + pauseCount.get());
    }
}
