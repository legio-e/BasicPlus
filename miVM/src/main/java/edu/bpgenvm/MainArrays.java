/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo de arrays en sus tres sabores y de strings interinos.
 *
 *   1) Constante / array fijo en data block        -> emitLeaGlobal(...)
 *   2) Array fijo en frame local                    -> emitLeaLocal(...)
 *   3) Array dinámico en heap                       -> NEWARRAY
 *
 * Las etiquetas ahora son IDs enteros anónimos dados por newLabel(): el
 * contador se resetea por función. Los saltos hacia atrás se optimizan al
 * opcode más corto que entre (JUMP8 / JUMP16 / JUMP).
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainArrays {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("ArrDemo");

            // === DATA BLOCK ===
            w.addConstantString("titulo", "Demo de arrays:");
            w.addConstantArray("primos", new int[]{2, 3, 5, 7, 11});
            w.addGlobalArray("squares", 5);

            // === main() ===
            w.addFunction("main", true);
            w.declareLocalArray("buffer", 3);
            w.declareLocal("i");
            w.declareLocal("heap");

            // Etiquetas para los 4 bucles. newLabel() asigna IDs anónimos.
            int LBL_FILL_TOP = w.newLabel();
            int LBL_FILL_END = w.newLabel();
            int LBL_PR_TOP   = w.newLabel();
            int LBL_PR_END   = w.newLabel();
            int LBL_HF_TOP   = w.newLabel();
            int LBL_HF_END   = w.newLabel();
            int LBL_HP_TOP   = w.newLabel();
            int LBL_HP_END   = w.newLabel();

            // ---- Imprime el título ----
            w.emitLeaGlobal("titulo");
            w.emit(OpCode.PRINT_STRING);

            // ---- Length y dos elementos del array constante "primos" ----
            w.emitLeaGlobal("primos");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                              // 5
            w.emitLeaGlobal("primos");
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 2
            w.emitLeaGlobal("primos");
            w.emit(OpCode.PUSH); w.emitInt(4);
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 11

            // ---- Llenar "squares" (global) con (i+1)^2 ----
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emitSetLocal("i");
            w.declareLabel(LBL_FILL_TOP);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(5);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(LBL_FILL_END);
            w.emitLeaGlobal("squares");
            w.emitGetLocal("i");
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emit(OpCode.MUL);
            w.emit(OpCode.ASTORE);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal("i");
            w.emitJump(LBL_FILL_TOP);
            w.declareLabel(LBL_FILL_END);

            // ---- Imprime los squares ----
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emitSetLocal("i");
            w.declareLabel(LBL_PR_TOP);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(5);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(LBL_PR_END);
            w.emitLeaGlobal("squares");
            w.emitGetLocal("i");
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 1, 4, 9, 16, 25
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal("i");
            w.emitJump(LBL_PR_TOP);
            w.declareLabel(LBL_PR_END);

            // ---- Buffer local fijo: 100, 200, 300 ----
            w.emitLeaLocal("buffer");
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emit(OpCode.PUSH); w.emitInt(100);
            w.emit(OpCode.ASTORE);
            w.emitLeaLocal("buffer");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.PUSH); w.emitInt(200);
            w.emit(OpCode.ASTORE);
            w.emitLeaLocal("buffer");
            w.emit(OpCode.PUSH); w.emitInt(2);
            w.emit(OpCode.PUSH); w.emitInt(300);
            w.emit(OpCode.ASTORE);

            w.emitLeaLocal("buffer");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                              // 3
            w.emitLeaLocal("buffer");
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 100
            w.emitLeaLocal("buffer");
            w.emit(OpCode.PUSH); w.emitInt(2);
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 300

            // ---- Array dinámico en heap ----
            w.emit(OpCode.PUSH); w.emitInt(4);
            w.emit(OpCode.NEWARRAY);
            w.emitSetLocal("heap");

            // heap[i] = (i+1) * 1000
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emitSetLocal("i");
            w.declareLabel(LBL_HF_TOP);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(4);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(LBL_HF_END);
            w.emitGetLocal("heap");
            w.emitGetLocal("i");
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emit(OpCode.PUSH); w.emitInt(1000);
            w.emit(OpCode.MUL);
            w.emit(OpCode.ASTORE);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal("i");
            w.emitJump(LBL_HF_TOP);
            w.declareLabel(LBL_HF_END);

            // Imprime length + cada elemento del heap
            w.emitGetLocal("heap");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                              // 4

            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emitSetLocal("i");
            w.declareLabel(LBL_HP_TOP);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(4);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(LBL_HP_END);
            w.emitGetLocal("heap");
            w.emitGetLocal("i");
            w.emit(OpCode.ALOAD);
            w.emit(OpCode.PRINT);                              // 1000, 2000, 3000, 4000
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal("i");
            w.emitJump(LBL_HP_TOP);
            w.declareLabel(LBL_HP_END);

            w.emit(OpCode.HALT);
            w.writeToFile("ArrDemo.mod");
            System.out.println("[Compilador] 'ArrDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("ArrDemo.mod", "ArrDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado:");
            System.out.println("  Demo de arrays:");
            System.out.println("  5  2  11");
            System.out.println("  1  4  9  16  25");
            System.out.println("  3  100  300");
            System.out.println("  4  1000  2000  3000  4000");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
