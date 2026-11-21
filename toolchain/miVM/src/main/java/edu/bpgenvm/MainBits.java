/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo de los opcodes bitwise y de las conversiones con check de rango.
 *
 * Bitwise i32:
 *   BAND, BOR, BXOR, BNOT
 *   SHL  (shift left)
 *   SHR_S (shift right aritmético, preserva signo)
 *   SHR_U (shift right lógico, rellena con ceros)
 *
 * Conversiones con check:
 *   I32_TO_I8 / I32_TO_U8 / I32_TO_I16 / I32_TO_U16
 *   No-op si el valor cabe en el rango; RuntimeException si no.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainBits {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("BitsDemo");
            w.addFunction("main", true);

            // === Bitwise ===

            // 0xF0 & 0x0F = 0
            push(w, 0xF0); push(w, 0x0F); w.emit(OpCode.BAND);  w.emit(OpCode.PRINT);
            // 0xF0 | 0x0F = 0xFF = 255
            push(w, 0xF0); push(w, 0x0F); w.emit(OpCode.BOR);   w.emit(OpCode.PRINT);
            // 0xFF ^ 0x55 = 0xAA = 170
            push(w, 0xFF); push(w, 0x55); w.emit(OpCode.BXOR);  w.emit(OpCode.PRINT);
            // ~15 = -16  (los bits se invierten todos)
            push(w, 15); w.emit(OpCode.BNOT); w.emit(OpCode.PRINT);

            // === Shifts ===

            // 1 << 4 = 16
            push(w, 1); push(w, 4); w.emit(OpCode.SHL); w.emit(OpCode.PRINT);
            // -16 >> 2 = -4   (sign-preserving)
            push(w, -16); push(w, 2); w.emit(OpCode.SHR_S); w.emit(OpCode.PRINT);
            // -16 >>> 2 = 0x3FFFFFFC = 1073741820   (zero-fill)
            push(w, -16); push(w, 2); w.emit(OpCode.SHR_U); w.emit(OpCode.PRINT);

            // Shift por más de 31: el contador se enmascara con & 31.
            // 1 << 32 == 1 << (32 & 31) == 1 << 0 == 1
            push(w, 1); push(w, 32); w.emit(OpCode.SHL); w.emit(OpCode.PRINT);

            // === Idioma común: extraer bytes de un int ===
            // Tomamos 0xDEADBEEF y aislamos los 4 bytes.
            // byte0 (alto): (n >>> 24) & 0xFF
            push(w, 0xDEADBEEF); push(w, 24); w.emit(OpCode.SHR_U);
            push(w, 0xFF); w.emit(OpCode.BAND); w.emit(OpCode.PRINT);   // 0xDE = 222

            push(w, 0xDEADBEEF); push(w, 16); w.emit(OpCode.SHR_U);
            push(w, 0xFF); w.emit(OpCode.BAND); w.emit(OpCode.PRINT);   // 0xAD = 173

            push(w, 0xDEADBEEF); push(w, 8); w.emit(OpCode.SHR_U);
            push(w, 0xFF); w.emit(OpCode.BAND); w.emit(OpCode.PRINT);   // 0xBE = 190

            push(w, 0xDEADBEEF); push(w, 0xFF); w.emit(OpCode.BAND);
            w.emit(OpCode.PRINT);                                       // 0xEF = 239

            // === Conversiones con check (caso válido) ===
            push(w, 100);   w.emit(OpCode.I32_TO_I8);  w.emit(OpCode.PRINT);  // 100
            push(w, -100);  w.emit(OpCode.I32_TO_I8);  w.emit(OpCode.PRINT);  // -100
            push(w, 200);   w.emit(OpCode.I32_TO_U8);  w.emit(OpCode.PRINT);  // 200
            push(w, 30000); w.emit(OpCode.I32_TO_I16); w.emit(OpCode.PRINT);  // 30000
            push(w, 60000); w.emit(OpCode.I32_TO_U16); w.emit(OpCode.PRINT);  // 60000

            // === Combinación: convertir y guardar en byte global ===
            // Si quisiéramos guardar 500 como i8, el check fallaría.
            // Aquí guardamos 100 (válido) en un byte global.
            // (Reescribir el módulo sería incómodo, lo dejamos como demo aritmética.)

            w.emit(OpCode.HALT);
            w.writeToFile("BitsDemo.mod");
            System.out.println("[Compilador] 'BitsDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("BitsDemo.mod", "BitsDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado (en orden):");
            System.out.println("  Bitwise:");
            System.out.println("    0           (0xF0 & 0x0F)");
            System.out.println("    255         (0xF0 | 0x0F)");
            System.out.println("    170         (0xFF ^ 0x55)");
            System.out.println("    -16         (~15)");
            System.out.println("  Shifts:");
            System.out.println("    16          (1 << 4)");
            System.out.println("    -4          (-16 SHR_S 2)");
            System.out.println("    1073741820  (-16 SHR_U 2)");
            System.out.println("    1           (1 << 32 → 1 << 0 por máscara &31)");
            System.out.println("  Extracción de bytes de 0xDEADBEEF:");
            System.out.println("    222, 173, 190, 239  (0xDE, 0xAD, 0xBE, 0xEF)");
            System.out.println("  Conversiones (todos los rangos válidos):");
            System.out.println("    100, -100, 200, 30000, 60000");
            System.out.println();
            System.out.println("Para probar el check de rango, sustituye p.ej. 100 por 200 en I32_TO_I8");
            System.out.println("y verás un RuntimeException 'valor fuera de rango'.");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }

    /** Atajo: emite PUSH + int. */
    private static void push(ModWriter w, int v) throws IOException {
        w.emit(OpCode.PUSH);
        w.emitInt(v);
    }
}
