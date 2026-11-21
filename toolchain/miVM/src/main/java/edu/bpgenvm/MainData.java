/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo del nuevo data block (constantes + globales) introducido en el
 * formato v2 de los .mod. Genera DataDemo.mod con:
 *
 *   - una constante INCREMENTO = 100  (offset -8 desde CS)
 *   - un global   acumulador = 0      (offset -4 desde CS)
 *
 * main() imprime el acumulador, le suma INCREMENTO dos veces (imprimiendo
 * cada paso) y finalmente imprime la propia constante para comprobar que
 * el acceso de lectura también funciona.
 *
 * Salida esperada: 0  100  200  100
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainData {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("DataDemo");

            // Declarar antes de cualquier función. Ambos viven en el data block,
            // que se inyecta en memoria justo antes del código del módulo.
            w.addConstantInt("INCREMENTO", 100);  // valor fijo en disco
            w.declareGlobal("acumulador");        // 4 bytes a cero en disco

            w.addFunction("main", true);

            // Imprime el valor inicial del acumulador (= 0).
            w.emitGetGlobal("acumulador");
            w.emit(OpCode.PRINT);

            // acumulador = acumulador + INCREMENTO;
            w.emitGetGlobal("acumulador");
            w.emitGetGlobal("INCREMENTO");
            w.emit(OpCode.ADD);
            w.emitSetGlobal("acumulador");

            // Imprime (= 100).
            w.emitGetGlobal("acumulador");
            w.emit(OpCode.PRINT);

            // Otra vuelta.
            w.emitGetGlobal("acumulador");
            w.emitGetGlobal("INCREMENTO");
            w.emit(OpCode.ADD);
            w.emitSetGlobal("acumulador");

            // Imprime (= 200).
            w.emitGetGlobal("acumulador");
            w.emit(OpCode.PRINT);

            // Verifica que la constante también es legible directamente.
            w.emitGetGlobal("INCREMENTO");
            w.emit(OpCode.PRINT);                 // 100

            w.emit(OpCode.HALT);

            w.writeToFile("DataDemo.mod");
            System.out.println("[Compilador] 'DataDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("DataDemo.mod", "DataDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado (en orden): 0  100  200  100");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
