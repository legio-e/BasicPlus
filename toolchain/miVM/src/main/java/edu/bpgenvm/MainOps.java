/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo independiente que ejercita los opcodes aritméticos, lógicos y de
 * comparación añadidos: SUB, MUL, DIV, MOD, NEG, AND, OR, NOT, GT, GE, LE, NEQ.
 *
 * Genera un único módulo (DemoOps.mod) sin dependencias externas, lo carga
 * con el mismo ModuleManager y lo ejecuta. Cada operación imprime su
 * resultado para que sea trivial comparar con el valor esperado anotado
 * en los comentarios de la generación.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainOps {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("DemoOps");
            w.addFunction("main", true);

            // Helper visual: en este lenguaje "PRINT" siempre saca el tope de la pila.
            //
            // Layout de cada bloque:
            //   PUSH a; PUSH b; OP; PRINT   →  imprime f(a, b)
            //   PUSH a; OP_UNARIO; PRINT     →  imprime f(a)
            //
            // El valor anotado en cada comentario es el resultado esperado.

            // SUB: 10 - 3 = 7
            pushInt(w, 10); pushInt(w, 3);  w.emit(OpCode.SUB);  w.emit(OpCode.PRINT);

            // MUL: 6 * 7 = 42
            pushInt(w, 6);  pushInt(w, 7);  w.emit(OpCode.MUL);  w.emit(OpCode.PRINT);

            // DIV: 20 / 4 = 5
            pushInt(w, 20); pushInt(w, 4);  w.emit(OpCode.DIV);  w.emit(OpCode.PRINT);

            // MOD: 23 % 5 = 3
            pushInt(w, 23); pushInt(w, 5);  w.emit(OpCode.MOD);  w.emit(OpCode.PRINT);

            // NEG: -(42) = -42
            pushInt(w, 42);                 w.emit(OpCode.NEG);  w.emit(OpCode.PRINT);

            // AND: 1 && 1 = 1   ;   1 && 0 = 0
            pushInt(w, 1);  pushInt(w, 1);  w.emit(OpCode.AND);  w.emit(OpCode.PRINT);
            pushInt(w, 1);  pushInt(w, 0);  w.emit(OpCode.AND);  w.emit(OpCode.PRINT);

            // OR: 0 || 1 = 1   ;   0 || 0 = 0
            pushInt(w, 0);  pushInt(w, 1);  w.emit(OpCode.OR);   w.emit(OpCode.PRINT);
            pushInt(w, 0);  pushInt(w, 0);  w.emit(OpCode.OR);   w.emit(OpCode.PRINT);

            // NOT: !0 = 1 ; !5 = 0
            pushInt(w, 0);                  w.emit(OpCode.NOT);  w.emit(OpCode.PRINT);
            pushInt(w, 5);                  w.emit(OpCode.NOT);  w.emit(OpCode.PRINT);

            // GT: 5 > 3 = 1 ; 2 > 3 = 0
            pushInt(w, 5);  pushInt(w, 3);  w.emit(OpCode.GT);   w.emit(OpCode.PRINT);
            pushInt(w, 2);  pushInt(w, 3);  w.emit(OpCode.GT);   w.emit(OpCode.PRINT);

            // GE: 5 >= 5 = 1 ; 4 >= 5 = 0
            pushInt(w, 5);  pushInt(w, 5);  w.emit(OpCode.GE);   w.emit(OpCode.PRINT);
            pushInt(w, 4);  pushInt(w, 5);  w.emit(OpCode.GE);   w.emit(OpCode.PRINT);

            // LE: 3 <= 5 = 1 ; 6 <= 5 = 0
            pushInt(w, 3);  pushInt(w, 5);  w.emit(OpCode.LE);   w.emit(OpCode.PRINT);
            pushInt(w, 6);  pushInt(w, 5);  w.emit(OpCode.LE);   w.emit(OpCode.PRINT);

            // NEQ: 5 != 3 = 1 ; 5 != 5 = 0
            pushInt(w, 5);  pushInt(w, 3);  w.emit(OpCode.NEQ);  w.emit(OpCode.PRINT);
            pushInt(w, 5);  pushInt(w, 5);  w.emit(OpCode.NEQ);  w.emit(OpCode.PRINT);

            w.emit(OpCode.HALT);

            w.writeToFile("DemoOps.mod");
            System.out.println("[Compilador] 'DemoOps.mod' generado.");

            // Ejecutar
            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("DemoOps.mod", "DemoOps");
            vm.run();

            System.out.println();
            System.out.println("Esperado (en orden):");
            System.out.println("  7  42  5  3  -42  1 0  1 0  1 0  1 0  1 0  1 0  1 0");
        } catch (IOException e) {
            System.err.println("Error en demo de opcodes: " + e.getMessage());
            e.printStackTrace();
        }
    }

    /** Atajo: emite PUSH + int de 32 bits. */
    private static void pushInt(ModWriter w, int v) throws IOException {
        w.emit(OpCode.PUSH);
        w.emitInt(v);
    }
}
