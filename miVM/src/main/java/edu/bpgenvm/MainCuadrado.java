/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo histórico: genera dos módulos (Aritmetica + Cliente) y los enlaza.
 * Cliente.main() llama a Aritmetica.cuadrado(i) (en realidad x+x) en un bucle
 * 1..3 e imprime el resultado.
 *
 * Renombrado desde Main.java cuando se introdujo la CLI de la VM en Main.java.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainCuadrado {
    public static void main(String[] args) {
        try {
            // =================================================================
            // FASE 1: GENERAR MÓDULO MATEMÁTICO INDEPENDIENTE (Aritmetica.mod)
            // =================================================================
            ModWriter writerA = new ModWriter();
            writerA.addModulo("Aritmetica");

            writerA.addFunction("cuadrado", true);
            writerA.declareParam("x");

            writerA.emitGetParam("x");
            writerA.emitGetParam("x");
            writerA.emit(OpCode.ADD);
            writerA.emitRet();

            writerA.writeToFile("Aritmetica.mod");
            writerA = null;
            System.out.println("[Compilador] 'Aritmetica.mod' generado de forma aislada.");

            // =================================================================
            // FASE 2: GENERAR MÓDULO CLIENTE INDEPENDIENTE (Cliente.mod)
            // =================================================================
            ModWriter writerB = new ModWriter();
            writerB.addModulo("Cliente");

            writerB.addFunction("main", true);
            writerB.declareLocal("i");

            int LBL_INICIO = writerB.newLabel();
            int LBL_FIN    = writerB.newLabel();

            writerB.emit(OpCode.PUSH);
            writerB.emitInt(1);
            writerB.emitSetLocal("i");

            writerB.declareLabel(LBL_INICIO);

            writerB.emitGetLocal("i");
            writerB.emit(OpCode.PUSH);
            writerB.emitInt(4);
            writerB.emit(OpCode.LT);

            writerB.emitJumpIfFalse(LBL_FIN);

            writerB.emitGetLocal("i");
            writerB.emitCallExt("Aritmetica.cuadrado");
            writerB.emit(OpCode.PRINT);

            writerB.emitGetLocal("i");
            writerB.emit(OpCode.PUSH);
            writerB.emitInt(1);
            writerB.emit(OpCode.ADD);
            writerB.emitSetLocal("i");

            writerB.emitJump(LBL_INICIO);

            writerB.declareLabel(LBL_FIN);
            writerB.emit(OpCode.HALT);

            writerB.writeToFile("Cliente.mod");
            writerB = null;
            System.out.println("[Compilador] 'Cliente.mod' generado de forma aislada.");

            // =================================================================
            // FASE 3: EJECUCIÓN AUTÓNOMA EN LA MÁQUINA VIRTUAL
            // =================================================================
            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);

            System.out.println("\n[Loader] Analizando dependencias y cargando módulos de forma automática...");
            loader.executeRootModule("Cliente.mod", "Cliente");

            vm.run();

        } catch (IOException e) {
            System.err.println("Error crítico en el ecosistema: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
