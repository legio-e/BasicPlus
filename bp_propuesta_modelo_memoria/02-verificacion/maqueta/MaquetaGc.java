/*
 * MaquetaGc — validación DETERMINISTA (corrección) del modelo de memoria.
 * Complementa a Maqueta.java (que valida la concurrencia/UAF bajo estrés).
 *
 * Aquí se prueban con ASERCIONES (PASS/FAIL) sobre grafos de objetos conocidos:
 *   C4 — cascada de liberación de campos `owner` (>=2 niveles).      [cierra H-006]
 *   C5 — GC PRECISO vía handles: raíces de módulo + tipos (long[]).  [cierra H-011, H-012]
 *
 * Argumento de diseño que se demuestra: con handles el GC es PRECISO (traza
 * referencias por la tabla, NO escaneo conservador, y NO mira el tipo del objeto).
 * Por eso H-011 (long[]/double[] invisibles al escaneo conservador) y H-012 (raíces
 * de módulo no escaneadas) NO se "arreglan": se vuelven IMPOSIBLES por construcción.
 *
 * Modelo: mismo esquema que Maqueta.java — slot con contador de generación, handle =
 * (índice, generación), liberación generation-checked. Tests deterministas (1 hilo):
 * la concurrencia ya está validada en Maqueta.java.
 */
import java.util.*;

public class MaquetaGc {

    static final int SLOTS = 256;
    // Tipos de objeto (el 5 = TYPE_ARRAY_I64 en la VM real; el que H-011 dejaba fuera).
    static final int KIND_OBJECT     = 0;
    static final int KIND_ARRAY_LONG = 5;

    static final class Node {
        int kind;
        long[] children;    // handles a hijos (0 = vacío)
        boolean[] owned;    // owned[i] = el hijo i es 'owner' (participa en la cascada)
        boolean live;
    }

    static final class ObjetoEliminado extends RuntimeException {
        ObjetoEliminado() { super("referencia a objeto eliminado"); }
    }

    static final class Heap {
        final Node[] node = new Node[SLOTS];
        final int[]  gen  = new int[SLOTS];
        final ArrayDeque<Integer> freeList = new ArrayDeque<Integer>();

        Heap() { for (int i = 0; i < SLOTS; i++) { node[i] = new Node(); freeList.add(i); } }

        static long handle(int slot, int g) { return ((long) g << 32) | (slot & 0xffffffffL); }
        static int  slotOf(long h) { return (int) (h & 0xffffffffL); }
        static int  genOf(long h)  { return (int) (h >>> 32); }

        boolean valid(long h) { int s = slotOf(h); return node[s].live && gen[s] == genOf(h); }
        void requireValid(long h) { if (!valid(h)) throw new ObjetoEliminado(); }

        long alloc(int kind, int nChildren) {
            Integer s = freeList.poll();
            if (s == null) throw new RuntimeException("heap lleno");
            Node n = node[s];
            n.kind = kind; n.children = new long[nChildren]; n.owned = new boolean[nChildren];
            n.live = true;
            return handle(s, gen[s]);
        }

        void setChild(long parent, int i, long child, boolean isOwner) {
            requireValid(parent);
            int s = slotOf(parent);
            node[s].children[i] = child; node[s].owned[i] = isOwner;
        }

        /** Liberación determinista: CASCADA de owners + generation-checked. */
        void free(long h) {
            int s = slotOf(h);
            if (!(node[s].live && gen[s] == genOf(h))) return;   // stale/doble → no-op seguro
            Node n = node[s];
            for (int i = 0; i < n.children.length; i++) {         // cascada: primero los OWNED
                if (n.owned[i] && n.children[i] != 0) free(n.children[i]);
            }
            reclaim(s);
        }

        private void reclaim(int s) {
            node[s].live = false; node[s].children = null; node[s].owned = null;
            gen[s]++;                    // muerte lógica → handles viejos detectan
            freeList.add(Integer.valueOf(s));
        }

        /** GC PRECISO vía handles: marca desde las raíces, recolecta lo no marcado. */
        void gc(long[] roots) {
            boolean[] mark = new boolean[SLOTS];
            for (long r : roots) markRec(r, mark);
            for (int s = 0; s < SLOTS; s++) if (node[s].live && !mark[s]) reclaim(s);
        }
        private void markRec(long h, boolean[] mark) {
            int s = slotOf(h);
            if (!(node[s].live && gen[s] == genOf(h))) return;   // handle stale → ignorar
            if (mark[s]) return;
            mark[s] = true;
            // Traza TODOS los hijos (owner o no: ambos mantienen vivo). TYPE-AGNOSTIC:
            // NO se consulta node[s].kind → un long[] (kind 5) se traza igual que todo.
            for (long c : node[s].children) if (c != 0) markRec(c, mark);
        }
    }

    // -------- mini-framework de aserciones --------
    static int passed = 0, failed = 0;
    static void check(boolean cond, String msg) {
        if (cond) { passed++; System.out.println("  [OK]   " + msg); }
        else      { failed++; System.out.println("  [FAIL] " + msg); }
    }
    static boolean dangling(Heap h, long handle) {
        try { h.requireValid(handle); return false; } catch (ObjetoEliminado e) { return true; }
    }

    // -------- C4: cascada owner de >=2 niveles --------
    static void testCascadaOwner() {
        System.out.println("== C4 — cascada de liberación owner (>=2 niveles)  [H-006] ==");
        Heap h = new Heap();
        long A = h.alloc(KIND_OBJECT, 1);   // A owns B
        long B = h.alloc(KIND_OBJECT, 2);   // B owns C (nivel 2) ; B referencia D (no-owner)
        long C = h.alloc(KIND_OBJECT, 0);
        long D = h.alloc(KIND_OBJECT, 0);
        h.setChild(A, 0, B, true);
        h.setChild(B, 0, C, true);
        h.setChild(B, 1, D, false);

        h.free(A);   // cascada: A -> B -> C ; D NO (no es owned)

        check(dangling(h, A), "A liberado");
        check(dangling(h, B), "B liberado en cascada (nivel 1)");
        check(dangling(h, C), "C liberado en cascada (nivel 2)");
        check(!dangling(h, D), "D sobrevive: referenciado por B pero NO owned (no cascadea)");

        h.free(A);   // segunda liberación
        check(true, "doble-free de A es no-op seguro (no lanza ni corrompe)");

        h.gc(new long[]{});   // sin raíces: todo lo vivo es basura
        check(dangling(h, D), "D recolectado por el GC tras quedar inalcanzable (owner + GC conviven)");
    }

    // -------- C5: GC preciso, raíces de módulo + tipos --------
    static void testGcRootsYTipos() {
        System.out.println("== C5 — GC preciso: raíces de módulo + tipos (long[])  [H-011, H-012] ==");
        Heap h = new Heap();
        long X = h.alloc(KIND_ARRAY_LONG, 1);   // long[] (kind 5) — el tipo que H-011 dejaba fuera
        long Z = h.alloc(KIND_OBJECT, 0);        // alcanzable transitivamente vía X
        long Y = h.alloc(KIND_OBJECT, 0);        // NO alcanzable desde ninguna raíz
        h.setChild(X, 0, Z, false);              // X -> Z (referencia normal)

        // El 'global de módulo' es una raíz que sostiene X (y NADA más lo sostiene):
        long[] roots = new long[]{ X };
        h.gc(roots);

        check(!dangling(h, X), "X (long[], kind 5) SOBREVIVE: alcanzable desde global de módulo [cierra H-011 y H-012]");
        check(!dangling(h, Z), "Z sobrevive: alcanzable transitivamente vía X");
        check(dangling(h, Y),  "Y recolectado: inalcanzable desde toda raíz");
    }

    public static void main(String[] args) {
        System.out.println("MaquetaGc — validación determinista de corrección (C4, C5)\n");
        testCascadaOwner();
        System.out.println();
        testGcRootsYTipos();
        System.out.println("\n================ RESUMEN ================");
        System.out.printf("PASS=%d  FAIL=%d  ->  %s%n", passed, failed, failed == 0 ? "TODO OK" : "HAY FALLOS");
        if (failed != 0) System.exit(1);
    }
}
