// ============================================================
// StatModuleSmoke.java — #466: «¿tienes este módulo, donde sea?»
//
// El cliente REAL del IDE (BpvmClient.statModule: STAT por NOMBRE) contra el
// micro simulado (bpvm-sim por TCP), que corre el REPL común de las cinco
// placas. Sube un módulo falso a /lib y otro a /app/proj y comprueba que la
// placa contesta DÓNDE lo tiene, con magic y crc, con el orden del RUN
// (el proyecto primero), y que lo que no está es null. Es la mitad Java de
// lo que sim_smoke.py prueba en Python: la misma pregunta, desde el IDE.
//
// Uso:  bpvm-sim --port=5109 --flash-file=<tmp> &
//       java -cp BpIde-5.0.jar com.mycompany.bpide.StatModuleSmoke 5109
// Sale 0 si OK, 1 si hay fallos.
// ============================================================
package com.mycompany.bpide;

public final class StatModuleSmoke {
    private static int fallos = 0;

    private static void check(boolean ok, String msg) {
        System.out.println((ok ? "  ok  : " : "  FAIL: ") + msg);
        if (!ok) fallos++;
    }

    public static void main(String[] args) throws Exception {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 5109;
        System.out.println("=== StatModuleSmoke: el IDE pregunta a la placa (sim :" + port + ") ===");
        try (BpvmClient c = new BpvmClient()) {
            c.setDiagSink(s -> {});
            boolean connected = false;
            for (int i = 0; i < 50 && !connected; i++) {
                try { c.connectRemote("127.0.0.1", port); connected = true; }
                catch (java.io.IOException e) { Thread.sleep(200); }
            }
            if (!connected) {
                System.out.println("FAIL: no conecta con el sim en el puerto " + port);
                System.exit(1);
            }

            byte[] lib  = concat("MOD7".getBytes("US-ASCII"), new byte[32]);
            byte[] proj = concat("MOD6".getBytes("US-ASCII"), filled(32, (byte) 1));
            try { c.mkdir("/lib", 3000); } catch (java.io.IOException e) { /* ya existe */ }
            try { c.mkdir("/app/proj", 3000); } catch (java.io.IOException e) { /* ya existe */ }
            c.uploadFile("/lib/Fake.mod", lib, 5000);
            c.uploadFile("/app/proj/Fake.mod", proj, 5000);

            BpvmClient.ModStat st = c.statModule("Fake.mod", null, 5000);
            check(st != null && "/lib/Fake.mod".equals(st.path) && st.version() == 7 && st.size == 36,
                  "sin base → lo encuentra en /lib, MOD7, 36 B: "
                  + (st == null ? "null" : st.path + " " + st.magic + " " + st.size));
            check(st != null && st.crc >= 0 && st.crc == crc32(lib),
                  "y su crc es el del fichero subido");
            st = c.statModule("Fake.mod", "/app/proj", 5000);
            check(st != null && "/app/proj/Fake.mod".equals(st.path) && st.version() == 6,
                  "con base /app/proj → el del proyecto PRIMERO (el orden del RUN), MOD6");
            check(c.statModule("Nadie.mod", null, 5000) == null, "lo que no está → null");
            check(BpvmClient.ModStat.versionOf("MOD7") == 7 && BpvmClient.ModStat.versionOf("xyz") == 0
                  && BpvmClient.ModStat.versionOf(null) == 0, "versionOf: MOD7 → 7; otra cosa → 0");

            c.deleteFile("/lib/Fake.mod", 3000);
            c.deleteFile("/app/proj/Fake.mod", 3000);
        }
        System.out.println(fallos == 0 ? "[status=OK]" : "[status=FAIL]");
        System.exit(fallos == 0 ? 0 : 1);
    }

    private static byte[] concat(byte[] a, byte[] b) {
        byte[] r = new byte[a.length + b.length];
        System.arraycopy(a, 0, r, 0, a.length);
        System.arraycopy(b, 0, r, a.length, b.length);
        return r;
    }
    private static byte[] filled(int n, byte v) { byte[] r = new byte[n]; java.util.Arrays.fill(r, v); return r; }
    private static long crc32(byte[] d) { java.util.zip.CRC32 c = new java.util.zip.CRC32(); c.update(d); return c.getValue(); }
}
