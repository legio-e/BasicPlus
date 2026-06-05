// ============================================================
// DeviceWireOracleSmoke.java
// H6.b.3.b — ORÁCULO de paridad Java↔C del debugger de DEVICE.
//
//   Cliente REAL del IDE (BpvmClient, connectRemote)
//        ↔ VM-C device-role (bpvm_dbgserver, wire-v1)
//        ↔ resolución de símbolos en el HOST (.dbg + DeviceFrameResolver).
//
// Prueba TODO el camino de "Debug on device" sin hardware (cascada
// Java→C→Pico): el device sólo da pc/bp/cs CRUDOS; el host aplica el .dbg
// para obtener locales POR NOMBRE Y TIPO, igual que la VM-Java pero del lado
// del host. Es la regla de oro de H6 ejercida end-to-end sobre la VM-C.
//
// Guion: connect → PAUSE → RUN → BP_HIT(captura cs) → SET_BP(pc=relPc+cs)
//        → CONTINUE → BP_HIT en la línea objetivo → resolveDeviceFrame →
//        asevera función/línea/locales por nombre y tipo → CONTINUE → EXITED.
//
// Uso (vía exec:java, como DebugSessionSmoke):
//   mvn -pl BpIde exec:java \
//     -Dexec.mainClass=com.mycompany.bpide.DeviceWireOracleSmoke \
//     -Dexec.args="<dbgserver-exe> <mod> <dbg> <line>"
// Sale 0 si OK, 1 si hay fallos, 2 si argumentos inválidos.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.DbgFile;
import edu.bpgenvm.vm.debug.DeviceFrameResolver;
import edu.bpgenvm.vm.debug.ExitedEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.StepCommand;

import java.io.File;
import java.net.ServerSocket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

public final class DeviceWireOracleSmoke {

    public static void main(String[] args) throws Exception {
        if (args.length < 4) {
            System.err.println("uso: DeviceWireOracleSmoke <dbgserver-exe> <mod> <dbg> <line>");
            System.exit(2);
        }
        String exe = args[0], mod = args[1], dbgPath = args[2];
        int line = Integer.parseInt(args[3]);

        File exeF = new File(exe);
        if (!exeF.isFile()) {
            System.err.println("(skip: falta el server C " + exe + " — `make test-listen`)");
            System.exit(0);   // skip limpio, no fallo: el binario nativo es opcional
        }

        DbgFile dbg = DbgFile.load(dbgPath);
        if (dbg == null) { System.err.println("FAIL: no se pudo cargar .dbg: " + dbgPath); System.exit(1); }

        // relPc de la línea objetivo (primer relPc cuya línea-origen == line);
        // es la clave de lineMap = una frontera de instrucción válida para bp.
        int relPc = -1;
        for (int p = 0; p < 300_000; p++) { if (dbg.lineForRelPc(p) == line) { relPc = p; break; } }
        if (relPc < 0) { System.err.println("FAIL: no hay relPc para la línea " + line); System.exit(1); }

        int port = freePort();
        File devNull = File.createTempFile("dbgserver-out", ".log");
        devNull.deleteOnExit();
        Process proc = new ProcessBuilder(exe, Integer.toString(port), new File(mod).getAbsolutePath())
                .redirectOutput(devNull)
                .redirectError(devNull)
                .start();

        BlockingQueue<PausedEvent> paused = new LinkedBlockingQueue<>();
        AtomicBoolean exited = new AtomicBoolean(false);
        List<String> fails = new ArrayList<>();

        try (BpvmClient client = new BpvmClient()) {
            client.setDiagSink(s -> {});      // silencio
            client.setOutputSink(s -> {});    // el OUTPUT del programa no nos interesa aquí
            client.setEventListener(ev -> {
                if (ev instanceof PausedEvent) paused.offer((PausedEvent) ev);
                else if (ev instanceof ExitedEvent) exited.set(true);
            });

            // Conexión con reintento (el server tarda unos ms en abrir el socket).
            boolean connected = false;
            for (int i = 0; i < 100; i++) {
                try { client.connectRemote("127.0.0.1", port); connected = true; break; }
                catch (Exception e) { Thread.sleep(50); }
            }
            if (!connected) {
                System.err.println("FAIL: no se pudo conectar al server C en :" + port);
                proc.destroyForcibly(); System.exit(1);
            }

            // PAUSE antes de RUN → rompe en el 1er opcode → capturamos cs.
            client.requestPause();
            client.runModule(mod);
            PausedEvent first = paused.poll(5, TimeUnit.SECONDS);
            if (first == null) fails.add("no llegó el 1er BP_HIT (PAUSE+RUN)");
            int cs = (first != null) ? first.cs : 0;

            // Breakpoint por PC en la línea objetivo (pc = relPc + cs), CONTINUE.
            PausedEvent at = null;
            if (first != null) {
                int absPc = relPc + cs;
                int bpId = client.setBreakpointPc(absPc, 3000);
                if (bpId <= 0) fails.add("SET_BP(pc=" + absPc + ") bpId<=0: " + bpId);
                client.sendCommand(StepCommand.CONTINUE);
                at = paused.poll(5, TimeUnit.SECONDS);
                if (at == null) fails.add("no llegó el BP_HIT en línea " + line);
            }

            if (at != null) {
                DeviceFrameResolver.Frame f = client.resolveDeviceFrame(dbg, at, 3000);
                System.out.println("FRAME function=" + f.function + " line=" + f.line
                        + " (absPc=" + at.absPc + " cs=" + at.cs + " bp=" + at.bp + ")");
                if (!"suma".equals(f.function)) fails.add("función != suma: " + f.function);
                if (f.line != line) fails.add("línea resuelta " + f.line + " != " + line);

                Map<String, DeviceFrameResolver.Local> by = new HashMap<>();
                for (DeviceFrameResolver.Local l : f.locals) {
                    by.put(l.name, l);
                    System.out.println("  " + l.name + ": " + l.type + " = " + l.display
                            + "  (bp+" + l.offset + ")");
                }
                for (String w : new String[]{"n", "base", "total", "big", "ratio", "msg", "ok", "i"})
                    if (!by.containsKey(w)) fails.add("falta local '" + w + "'");
                // El oráculo: mismos NOMBRES Y TIPOS que la VM-Java resolvió en el smoke.
                checkType(fails, by, "n", "integer");
                checkType(fails, by, "base", "integer");
                checkType(fails, by, "total", "integer");
                checkType(fails, by, "big", "long");
                checkType(fails, by, "ratio", "double");
                checkType(fails, by, "msg", "string");
                checkType(fails, by, "ok", "boolean");
                checkType(fails, by, "i", "integer");
                // params en offset negativo; locales en offset >= 0.
                if (by.containsKey("n") && by.get("n").offset >= 0) fails.add("param n con offset >= 0");
                if (by.containsKey("total") && by.get("total").offset < 0) fails.add("local total con offset < 0");
            }

            // Drena hasta EXITED (robusto aunque la función se llame varias veces).
            client.sendCommand(StepCommand.CONTINUE);
            long deadline = System.currentTimeMillis() + 5000;
            while (!exited.get() && System.currentTimeMillis() < deadline) {
                PausedEvent more = paused.poll(200, TimeUnit.MILLISECONDS);
                if (more != null) client.sendCommand(StepCommand.CONTINUE);
            }
        } finally {
            try { proc.waitFor(2, TimeUnit.SECONDS); } catch (Exception ignored) {}
            proc.destroyForcibly();
        }

        if (!fails.isEmpty()) {
            System.err.println("FALLOS:");
            fails.forEach(s -> System.err.println("  - " + s));
            System.exit(1);
        }
        System.out.println("OK: H6.b.3.b oráculo Java↔C — locales de DEVICE por nombre/tipo vía .dbg host");
    }

    private static void checkType(List<String> fails, Map<String, DeviceFrameResolver.Local> by,
                                  String name, String type) {
        DeviceFrameResolver.Local l = by.get(name);
        if (l != null && !type.equals(l.type)) fails.add(name + " tipo '" + l.type + "' != '" + type + "'");
    }

    private static int freePort() throws Exception {
        try (ServerSocket s = new ServerSocket(0)) { return s.getLocalPort(); }
    }
}
