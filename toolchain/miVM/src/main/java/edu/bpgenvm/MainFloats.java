/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo del tipo f32. Genera FloatDemo.mod con:
 *
 *   - constantes PI y E en el data block (addConstantFloat)
 *   - un global acumulador inicializado a 0 (= 0.0f)
 *   - un array constante de floats con valores conocidos
 *
 * main() prueba:
 *   - FPRINT de constantes
 *   - FMUL: PI * E almacenado en el global, FPRINT
 *   - FADD y FSUB con literales (emitPushFloat)
 *   - Conversiones I2F y F2I
 *   - Comparación FLT entre floats que produce 0/1 entero
 *   - Lectura de un float dentro de un array constante
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainFloats {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("FloatDemo");

            // Data block: declaramos en el orden que queremos.
            // El primero declarado queda pegado a CS (-4); cada siguiente más abajo.
            w.addConstantFloat("PI", 3.14159f);
            w.addConstantFloat("E",  2.71828f);
            w.declareGlobal("acumulador"); // 0.0f
            w.addConstantArrayFloat("muestras", new float[]{1.5f, 2.5f, 3.5f, 4.5f});

            w.addFunction("main", true);

            // 1) FPRINT de las constantes.
            w.emitGetGlobal("PI");
            w.emit(OpCode.FPRINT);                   // ~3.14159
            w.emitGetGlobal("E");
            w.emit(OpCode.FPRINT);                   // ~2.71828

            // 2) acumulador = PI * E
            w.emitGetGlobal("PI");
            w.emitGetGlobal("E");
            w.emit(OpCode.FMUL);
            w.emitSetGlobal("acumulador");
            w.emitGetGlobal("acumulador");
            w.emit(OpCode.FPRINT);                   // ~8.53973

            // 3) FADD/FSUB con literales: (10.0 + 0.5) - 7.25 = 3.25
            w.emitPushFloat(10.0f);
            w.emitPushFloat(0.5f);
            w.emit(OpCode.FADD);
            w.emitPushFloat(7.25f);
            w.emit(OpCode.FSUB);
            w.emit(OpCode.FPRINT);                   // 3.25

            // 4) Conversiones: 42 (int) -> 42.0 (float), luego 7.7 -> 7 (int)
            w.emit(OpCode.PUSH); w.emitInt(42);
            w.emit(OpCode.PRINT);                    // 42 (int)
            w.emit(OpCode.PUSH); w.emitInt(42);
            w.emit(OpCode.I2F);
            w.emit(OpCode.FPRINT);                   // 42.000

            w.emitPushFloat(7.7f);
            w.emit(OpCode.F2I);
            w.emit(OpCode.PRINT);                    // 7

            // 5) Comparación float: 1.5 < 2.0 ? -> 1 (truncado a int)
            w.emitPushFloat(1.5f);
            w.emitPushFloat(2.0f);
            w.emit(OpCode.FLT);
            w.emit(OpCode.PRINT);                    // 1

            w.emitPushFloat(3.0f);
            w.emitPushFloat(2.0f);
            w.emit(OpCode.FLT);
            w.emit(OpCode.PRINT);                    // 0

            // 6) Array de floats: imprime length y muestras[2]
            w.emitLeaGlobal("muestras");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                    // 4

            w.emitLeaGlobal("muestras");
            w.emit(OpCode.PUSH); w.emitInt(2);
            w.emit(OpCode.ALOAD);                    // empuja bits de 3.5f
            w.emit(OpCode.FPRINT);                   // 3.5

            // 7) División por cero IEEE 754: produce Infinity (sin throw)
            w.emitPushFloat(1.0f);
            w.emitPushFloat(0.0f);
            w.emit(OpCode.FDIV);
            w.emit(OpCode.FPRINT);                   // Infinity

            w.emit(OpCode.HALT);
            w.writeToFile("FloatDemo.mod");
            System.out.println("[Compilador] 'FloatDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("FloatDemo.mod", "FloatDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado (en orden):");
            System.out.println("  FPRINT  ~3.14159");
            System.out.println("  FPRINT  ~2.71828");
            System.out.println("  FPRINT  ~8.53973   (PI * E)");
            System.out.println("  FPRINT  3.25");
            System.out.println("  PRINT   42");
            System.out.println("  FPRINT  42.0");
            System.out.println("  PRINT   7          (de 7.7)");
            System.out.println("  PRINT   1          (1.5 < 2.0)");
            System.out.println("  PRINT   0          (3.0 < 2.0)");
            System.out.println("  PRINT   4          (length de muestras)");
            System.out.println("  FPRINT  3.5        (muestras[2])");
            System.out.println("  FPRINT  Infinity   (1.0 / 0.0 IEEE)");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
