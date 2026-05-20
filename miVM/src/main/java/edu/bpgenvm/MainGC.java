/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo del GC mark-and-sweep conservativo.
 *
 * FASE 1 — reuso del hueco (GC manual):
 *   Aloca 3 arrays de 100 ints (a, b, c). Marca sentinelas en a[0] y a[99].
 *   Suelta b escribiendo 0 sobre su local. GC manual confirma que se liberan
 *   ~408 bytes. Aloca d del mismo tamaño y GC manual confirma que d reutiliza
 *   el hueco (heap total no crece).
 *
 * FASE 2 — disparo automático (stress):
 *   Bucle que aloca un array de {@link #STRESS_ARR_SIZE} ints en una local
 *   `trash` {@link #STRESS_ITERS} veces. Sólo la última ref vive; las
 *   anteriores son basura. El producto (STRESS_ITERS × array_size) está
 *   dimensionado para superar varias veces el heap libre, así que cada
 *   ~32 iters el bump-allocator se queda sin sitio y se dispara GC
 *   automáticamente. Esperamos ver varias líneas "VM [GC]: ..." sin que
 *   nadie las pida.
 *
 * FASE 3 — integridad del payload:
 *   Lee a[0] y a[99]. Si los sentinelas siguen ahí, el GC no ha pisado
 *   ningún byte del objeto vivo a lo largo de los ~5-6 GCs anteriores.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainGC {

    private static final int STRESS_ITERS = 200;
    /**
     * Tamaño (en ints, i32) de cada array alocado en el stress de FASE 2.
     * Calibrado para que ~32 allocs llenen los ~256 KiB de heap libre del
     * VirtualMachine actual (VM expandido a 512 KiB total tras Phase 4b).
     * Cada array = 8 (header) + STRESS_ARR_SIZE*4 (payload). Producto
     * STRESS_ITERS × tamaño = ~1.6 MiB → ~6 GCs automáticos.
     */
    private static final int STRESS_ARR_SIZE = 2000;
    private static final int SENTINEL_FIRST = 1010101;
    private static final int SENTINEL_LAST  = 2020202;

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("GCDemo");

            // ---- Strings (data block, leídos en runtime con PRINT_STRING) ----
            w.addConstantString("titulo",            "=== Demo GC mark-and-sweep ===");
            w.addConstantString("tag_baseline",      "FASE 1 [GC manual] baseline (a,b,c vivos):");
            w.addConstantString("tag_after_drop",    "FASE 1 [GC manual] tras soltar b:");
            w.addConstantString("tag_after_realloc", "FASE 1 [GC manual] tras allocar d (reuso del hueco):");
            w.addConstantString("tag_lens",          "FASE 1 lengths de a/c/d y ref de b:");
            w.addConstantString("tag_stress",        "FASE 2 [GC automatico] 200 allocs sobre una local:");
            w.addConstantString("tag_stress_end",    "FASE 2 GC manual final tras vaciar trash:");
            w.addConstantString("tag_payload",       "FASE 3 integridad de a tras todos los GCs:");
            w.addConstantString("tag_a0",            "  a[0]  esperado=1010101, real=");
            w.addConstantString("tag_a99",           "  a[99] esperado=2020202, real=");

            w.addFunction("main", true);
            w.declareLocal("a");
            w.declareLocal("b");
            w.declareLocal("c");
            w.declareLocal("d");
            w.declareLocal("i");
            w.declareLocal("trash");

            int LBL_TOP = w.newLabel();
            int LBL_END = w.newLabel();

            w.emitLeaGlobal("titulo");
            w.emit(OpCode.PRINT_STRING);

            // ============================================================
            // FASE 1: reuso del hueco
            // ============================================================
            push(w, 100); w.emit(OpCode.NEWARRAY); w.emitSetLocal("a");
            push(w, 100); w.emit(OpCode.NEWARRAY); w.emitSetLocal("b");
            push(w, 100); w.emit(OpCode.NEWARRAY); w.emitSetLocal("c");

            // Sentinelas en `a` para test de integridad de fase 3
            w.emitGetLocal("a"); push(w, 0);  push(w, SENTINEL_FIRST); w.emit(OpCode.ASTORE);
            w.emitGetLocal("a"); push(w, 99); push(w, SENTINEL_LAST);  w.emit(OpCode.ASTORE);

            w.emitLeaGlobal("tag_baseline");      w.emit(OpCode.PRINT_STRING);
            w.emit(OpCode.GC_COLLECT);

            push(w, 0); w.emitSetLocal("b");
            w.emitLeaGlobal("tag_after_drop");    w.emit(OpCode.PRINT_STRING);
            w.emit(OpCode.GC_COLLECT);

            push(w, 100); w.emit(OpCode.NEWARRAY); w.emitSetLocal("d");
            w.emitLeaGlobal("tag_after_realloc"); w.emit(OpCode.PRINT_STRING);
            w.emit(OpCode.GC_COLLECT);

            w.emitLeaGlobal("tag_lens"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("a"); w.emit(OpCode.ALEN); w.emit(OpCode.PRINT);  // 100
            w.emitGetLocal("c"); w.emit(OpCode.ALEN); w.emit(OpCode.PRINT);  // 100
            w.emitGetLocal("d"); w.emit(OpCode.ALEN); w.emit(OpCode.PRINT);  // 100
            w.emitGetLocal("b"); w.emit(OpCode.PRINT);                       // 0

            // ============================================================
            // FASE 2: disparo automático del GC
            //
            // for (i = 0; i < STRESS_ITERS; i++) trash = new int[STRESS_ARR_SIZE];
            //
            // Cada array ocupa 8 + STRESS_ARR_SIZE*4 bytes. Con ~256 KiB de
            // heap libre (tras la expansión a 512 KiB del VM), cada ~32
            // iters el bump se satura y dispara GC. Las refs anteriores en
            // `trash` son inalcanzables tras la sobreescritura, así que el
            // sweep recupera prácticamente todo el heap.
            // ============================================================
            w.emitLeaGlobal("tag_stress"); w.emit(OpCode.PRINT_STRING);

            push(w, 0); w.emitSetLocal("i");
            w.declareLabel(LBL_TOP);
                w.emitGetLocal("i"); push(w, STRESS_ITERS); w.emit(OpCode.LT);
                w.emitJumpIfFalse(LBL_END);

                push(w, STRESS_ARR_SIZE); w.emit(OpCode.NEWARRAY); w.emitSetLocal("trash");

                w.emitGetLocal("i"); push(w, 1); w.emit(OpCode.ADD); w.emitSetLocal("i");
                w.emitJump(LBL_TOP);
            w.declareLabel(LBL_END);

            // Suelta trash y dispara un GC manual final: el heap debe quedar
            // alive = 4*408 = 1632 bytes (a, c, d, y... ojo: la última `trash`
            // ya está suelta, así que sólo a, c, d → alive = 1224).
            push(w, 0); w.emitSetLocal("trash");
            w.emitLeaGlobal("tag_stress_end"); w.emit(OpCode.PRINT_STRING);
            w.emit(OpCode.GC_COLLECT);

            // ============================================================
            // FASE 3: integridad del payload tras todos los GCs
            // ============================================================
            w.emitLeaGlobal("tag_payload"); w.emit(OpCode.PRINT_STRING);

            w.emitLeaGlobal("tag_a0"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("a"); push(w, 0);  w.emit(OpCode.ALOAD); w.emit(OpCode.PRINT);

            w.emitLeaGlobal("tag_a99"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("a"); push(w, 99); w.emit(OpCode.ALOAD); w.emit(OpCode.PRINT);

            w.emit(OpCode.HALT);
            w.writeToFile("GCDemo.mod");
            System.out.println("[Compilador] 'GCDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("GCDemo.mod", "GCDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado:");
            System.out.println("  FASE 1:");
            System.out.println("    GC baseline:      alive=1224, libres=0");
            System.out.println("    GC tras soltar b: alive=816,  libres=408");
            System.out.println("    GC tras alloc d:  alive=1224, libres=0 (reuso del hueco)");
            System.out.println("    lengths: 100, 100, 100, 0");
            System.out.println("  FASE 2:");
            System.out.println("    ~5-6 lineas 'VM [GC]: ...' durante el bucle (sin GC_COLLECT manual).");
            System.out.println("    GC final tras soltar trash: alive=1224 (solo a, c, d).");
            System.out.println("  FASE 3:");
            System.out.println("    a[0]  = 1010101");
            System.out.println("    a[99] = 2020202");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private static void push(ModWriter w, int v) throws IOException {
        w.emit(OpCode.PUSH);
        w.emitInt(v);
    }
}
