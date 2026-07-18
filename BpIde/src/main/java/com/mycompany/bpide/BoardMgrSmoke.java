// ============================================================
// BoardMgrSmoke.java
// Smoke del cliente de gestión de placa (H9): conecta el BpvmClient del IDE a un
// boardsim ya en marcha (tools/boardsim.c) y ejerce STATE/ENV_*/PART_* de punta a
// punta — verifica que las clases de reply del cliente casan con lo que emite el
// sim (el mismo wire que hará FrmBoard). No necesita placa.
//
// Correr:  arranca el sim en un puerto, luego:
//   mvn -q exec:java -Dexec.mainClass=com.mycompany.bpide.BoardMgrSmoke \
//                    -Dexec.args="5107"
// Sale 0 si todo pasa, 1 si algún assert falla.
// ============================================================
package com.mycompany.bpide;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class BoardMgrSmoke {
    static int fails = 0;
    static void check(boolean cond, String msg) {
        System.out.println((cond ? "  ok  : " : "  FAIL: ") + msg);
        if (!cond) fails++;
    }

    public static void main(String[] args) throws Exception {
        int port = args.length > 0 ? Integer.parseInt(args[0]) : 5107;
        long T = 5000;
        System.out.println("=== BoardMgr smoke (cliente IDE ↔ boardsim :" + port + ") ===");
        try (BpvmClient c = new BpvmClient()) {
            c.connectRemote("127.0.0.1", port);

            BpvmClient.BoardState st = c.boardState(T);
            check(st.state >= 0 && st.name != null, "STATE → estado " + st.state + " (" + st.name + ")");

            // asegurar identidad + particiones (idempotente si ya estaban)
            c.envSet("board", "Pico2", T);
            c.envSet("flashSizeBytes", "4194304", T);
            List<BpvmClient.EnvVar> env = c.envList(T);
            boolean sawBoard = env.stream().anyMatch(e -> e.key.equals("board") && e.value.equals("Pico2"));
            check(sawBoard, "ENV_SET+ENV_LS → board=Pico2 visible (" + env.size() + " vars)");
            check(c.envGet("flashSizeBytes", T).equals("4194304"), "ENV_GET flashSizeBytes");

            BpvmClient.PartTable def = c.partDefaults(T);
            check(def.parts.size() == 2, "PART_DEFAULTS → 2 particiones sugeridas");

            Map<String, Long> sizes = new LinkedHashMap<>();
            sizes.put("fs", 0x100000L);
            sizes.put("packs", 0x100000L);
            c.partApply(sizes, T);
            BpvmClient.PartTable lay = c.partLayout(T);
            check(!lay.missing && lay.parts.size() == 2, "PART_APPLY + PART_LS → layout completo");
            BpvmClient.Partition fs = lay.parts.get(0);
            check(fs.name.equals("fs") && fs.offset == 0x100000L, "offset de fs DERIVADO = base");

            // validación fallida → IOException con el detalle, layout intacto
            boolean threw = false;
            try {
                Map<String, Long> bad = new LinkedHashMap<>();
                bad.put("fs", 0x300000L); bad.put("packs", 0x300000L);
                c.partApply(bad, T);
            } catch (java.io.IOException e) { threw = true; }
            check(threw, "PART_APPLY inválido → IOException (validación en el peer)");
            check(c.partLayout(T).parts.get(0).size == 0x100000L, "layout bueno sobrevive al apply inválido");

            System.out.println(fails == 0 ? "[status=OK]" : "[status=FAIL: " + fails + "]");
        }
        System.exit(fails == 0 ? 0 : 1);
    }
}
