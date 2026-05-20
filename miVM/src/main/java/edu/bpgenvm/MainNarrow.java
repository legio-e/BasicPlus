/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo de tipos enteros estrechos (i8/u8/i16/u16).
 *
 * Cubre:
 *   - Constante i8 cargada como signed o unsigned (-1 vs 255).
 *   - Constante i16 cargada como signed o unsigned (-1 vs 65535).
 *   - Array de bytes en data block: lectura ALOAD_I8 vs ALOAD_U8 del mismo byte.
 *   - Array de shorts en data block con valores conocidos.
 *   - NEWARRAY_I8 (heap): se asignan menos bytes que un i32 array equivalente.
 *   - SET/GET globales narrow.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainNarrow {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("NarrowDemo");

            // Data block (en orden inverso al almacenamiento en memoria).
            w.addConstantInt8 ("byteMinus1",  -1);            // 0xFF: pegado a CS (offset -1)
            w.addConstantInt16("shortMinus1", -1);            // 0xFFFF: a continuación (offset -3)
            w.addConstantArrayInt8("bytes", new byte[]{ 10, -5, (byte) 200, 0, 127 });
            w.addConstantArrayInt16("shorts", new short[]{ 1000, -32000, 32767 });
            // Un global byte normal para SET/GET.
            w.addConstantInt8("counterB", 0);

            w.addFunction("main", true);
            w.declareLocal("i");

            // 1) Mismo byte (0xFF = -1 / 255) leído con signo y sin signo.
            w.emitGetGlobalI8("byteMinus1");
            w.emit(OpCode.PRINT);                       // -1
            w.emitGetGlobalU8("byteMinus1");
            w.emit(OpCode.PRINT);                       // 255

            // 2) Mismo short (0xFFFF) leído con signo y sin signo.
            w.emitGetGlobalI16("shortMinus1");
            w.emit(OpCode.PRINT);                       // -1
            w.emitGetGlobalU16("shortMinus1");
            w.emit(OpCode.PRINT);                       // 65535

            // 3) Array de bytes: leer todos como i8 (signed). 200 → -56.
            w.emitLeaGlobal("bytes");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                       // 5

            int LBL_TOP = w.newLabel();
            int LBL_END = w.newLabel();
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emitSetLocal("i");
            w.declareLabel(LBL_TOP);
            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(5);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(LBL_END);

            w.emitLeaGlobal("bytes");
            w.emitGetLocal("i");
            w.emit(OpCode.ALOAD_I8);
            w.emit(OpCode.PRINT);                       // 10, -5, -56, 0, 127

            w.emitGetLocal("i");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal("i");
            w.emitJump(LBL_TOP);
            w.declareLabel(LBL_END);

            // 4) Mismo array como u8: 200 vuelve a 200 (zero-ext).
            w.emitLeaGlobal("bytes");
            w.emit(OpCode.PUSH); w.emitInt(2);
            w.emit(OpCode.ALOAD_U8);
            w.emit(OpCode.PRINT);                       // 200

            // 5) Array de shorts.
            w.emitLeaGlobal("shorts");
            w.emit(OpCode.ALEN);
            w.emit(OpCode.PRINT);                       // 3
            w.emitLeaGlobal("shorts");
            w.emit(OpCode.PUSH); w.emitInt(0);
            w.emit(OpCode.ALOAD_I16);
            w.emit(OpCode.PRINT);                       // 1000
            w.emitLeaGlobal("shorts");
            w.emit(OpCode.PUSH); w.emitInt(1);
            w.emit(OpCode.ALOAD_I16);
            w.emit(OpCode.PRINT);                       // -32000
            w.emitLeaGlobal("shorts");
            w.emit(OpCode.PUSH); w.emitInt(2);
            w.emit(OpCode.ALOAD_I16);
            w.emit(OpCode.PRINT);                       // 32767

            // 6) SET/GET sobre el global byte counterB: escribimos 42, lo leemos.
            w.emit(OpCode.PUSH); w.emitInt(42);
            w.emitSetGlobalI8("counterB");
            w.emitGetGlobalI8("counterB");
            w.emit(OpCode.PRINT);                       // 42

            // SET con un valor > 127: se trunca al byte bajo, luego como i8 sale negativo.
            w.emit(OpCode.PUSH); w.emitInt(0x1FF);  // 511, byte bajo = 0xFF
            w.emitSetGlobalI8("counterB");
            w.emitGetGlobalI8("counterB");
            w.emit(OpCode.PRINT);                       // -1
            w.emitGetGlobalU8("counterB");
            w.emit(OpCode.PRINT);                       // 255

            // 7) NEWARRAY_I8 en heap, 100 bytes (vs 400 que ocuparía un i32 array).
            w.emit(OpCode.PUSH); w.emitInt(100);
            w.emit(OpCode.NEWARRAY_I8);
            w.emit(OpCode.ALEN);                        // tope = length = 100
            w.emit(OpCode.PRINT);                       // 100

            w.emit(OpCode.HALT);
            w.writeToFile("NarrowDemo.mod");
            System.out.println("[Compilador] 'NarrowDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("NarrowDemo.mod", "NarrowDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado (en orden):");
            System.out.println("  -1, 255             (byteMinus1 i8/u8)");
            System.out.println("  -1, 65535           (shortMinus1 i16/u16)");
            System.out.println("  5                   (length de bytes)");
            System.out.println("  10, -5, -56, 0, 127 (bytes[] ALOAD_I8)");
            System.out.println("  200                 (bytes[2] ALOAD_U8)");
            System.out.println("  3                   (length de shorts)");
            System.out.println("  1000, -32000, 32767 (shorts[] ALOAD_I16)");
            System.out.println("  42                  (counterB tras set)");
            System.out.println("  -1, 255             (counterB tras set 0x1FF, i8 y u8)");
            System.out.println("  100                 (length del heap byte array)");
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
