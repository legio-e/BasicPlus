/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo de clases, herencia, dispatch dinámico (vtable) y GC preciso por
 * field bitmap.
 *
 *   class Animal { peso (int); nombre (ref);
 *                  hablar()  → print(this.peso) }
 *
 *   class Perro extends Animal {
 *       dueno (ref);
 *       hablar() → print(this.peso * 10)   // override
 *   }
 *
 * main()
 *   - Crea a1 = Animal(100, "Felix")
 *   - Crea p1 = Perro(15, "Rex", "Felix")
 *   - Crea p2 = Perro(30, "Toby", "Rex")
 *   - Test 1 (polimorfismo): arr = [a1, p1, p2]; for i: arr[i].hablar()
 *       Esperado: 100, 150, 300 — el slot 0 de la vtable se resuelve a
 *       Animal.hablar para a1 y a Perro.hablar para p1/p2 según el
 *       class_ptr del objeto.
 *   - Test 2 (GC preciso): crea un nombre dinámico "OOPS" en heap,
 *       lo asigna a un Animal nuevo (a1.nombre), pone a 0 todas las
 *       demás refs, lanza GC manual y lee a1.nombre. Si el field bitmap
 *       de Animal marca `nombre` como ref, el GC traza ese campo desde
 *       a1 y "OOPS" sobrevive.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainOOP {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("OOPDemo");

            // ---- Strings constantes ----
            w.addConstantString("titulo",     "=== Demo clases + herencia + vtable ===");
            w.addConstantString("tag_poly",   "[polimorfismo] hablar() sobre [Animal, Perro, Perro]:");
            w.addConstantString("tag_gc",     "[GC preciso] tras descartar p1/p2/arr/tmpName, leemos a1.nombre:");
            w.addConstantString("name_felix", "Felix");
            w.addConstantString("name_rex",   "Rex");
            w.addConstantString("name_toby",  "Toby");

            // ---- class Animal { peso (int), nombre (ref); hablar() } ----
            w.addClass("Animal", null);
            w.declareField("peso",   false);   // bit 0 del bitmap = 0
            w.declareField("nombre", true);    // bit 1 del bitmap = 1 (GC traza este slot)
            w.addMethod("hablar");
                w.emitGetParam("this");
                w.emitGetField("Animal", "peso");
                w.emit(OpCode.PRINT);
                push(w, 0);                    // dummy return value
                w.emitRet();
            w.endClass();

            // ---- class Perro extends Animal { dueno (ref); hablar() override } ----
            w.addClass("Perro", "Animal");
            w.declareField("dueno", true);     // bit 2 del bitmap = 1
            w.addMethod("hablar");             // override, mismo slot (0) que Animal.hablar
                w.emitGetParam("this");
                w.emitGetField("Perro", "peso");
                push(w, 10);
                w.emit(OpCode.MUL);
                w.emit(OpCode.PRINT);
                push(w, 0);
                w.emitRet();
            w.endClass();

            // ---- main ----
            w.addFunction("main", true);
            w.declareLocal("a1");
            w.declareLocal("p1");
            w.declareLocal("p2");
            w.declareLocal("arr");
            w.declareLocal("i");
            w.declareLocal("tmpName");
            w.declareLocal("ret");             // dump de valores de retorno descartados

            int LBL_TOP = w.newLabel();
            int LBL_END = w.newLabel();

            w.emitLeaGlobal("titulo"); w.emit(OpCode.PRINT_STRING);

            // a1 = Animal(peso=100, nombre="Felix")
            w.emitNewObject("Animal"); w.emitSetLocal("a1");
            w.emitGetLocal("a1"); push(w, 100);              w.emitSetField("Animal", "peso");
            w.emitGetLocal("a1"); w.emitLeaGlobal("name_felix"); w.emitSetField("Animal", "nombre");

            // p1 = Perro(peso=15, nombre="Rex", dueno="Felix")
            w.emitNewObject("Perro"); w.emitSetLocal("p1");
            w.emitGetLocal("p1"); push(w, 15);                w.emitSetField("Perro", "peso");
            w.emitGetLocal("p1"); w.emitLeaGlobal("name_rex");   w.emitSetField("Perro", "nombre");
            w.emitGetLocal("p1"); w.emitLeaGlobal("name_felix"); w.emitSetField("Perro", "dueno");

            // p2 = Perro(peso=30, nombre="Toby", dueno="Rex")
            w.emitNewObject("Perro"); w.emitSetLocal("p2");
            w.emitGetLocal("p2"); push(w, 30);                w.emitSetField("Perro", "peso");
            w.emitGetLocal("p2"); w.emitLeaGlobal("name_toby"); w.emitSetField("Perro", "nombre");
            w.emitGetLocal("p2"); w.emitLeaGlobal("name_rex");  w.emitSetField("Perro", "dueno");

            // arr = new int[3]; arr[0]=a1; arr[1]=p1; arr[2]=p2
            push(w, 3); w.emit(OpCode.NEWARRAY); w.emitSetLocal("arr");
            w.emitGetLocal("arr"); push(w, 0); w.emitGetLocal("a1"); w.emit(OpCode.ASTORE);
            w.emitGetLocal("arr"); push(w, 1); w.emitGetLocal("p1"); w.emit(OpCode.ASTORE);
            w.emitGetLocal("arr"); push(w, 2); w.emitGetLocal("p2"); w.emit(OpCode.ASTORE);

            // for (i = 0; i < 3; i++) arr[i].hablar()
            w.emitLeaGlobal("tag_poly"); w.emit(OpCode.PRINT_STRING);
            push(w, 0); w.emitSetLocal("i");
            w.declareLabel(LBL_TOP);
                w.emitGetLocal("i"); push(w, 3); w.emit(OpCode.LT);
                w.emitJumpIfFalse(LBL_END);

                w.emitGetLocal("arr"); w.emitGetLocal("i"); w.emit(OpCode.ALOAD);
                w.emitInvokeVirtual("Animal", "hablar", 0);   // dispatch dinámico
                w.emitSetLocal("ret");                        // descarta el return value (dummy 0)

                w.emitGetLocal("i"); push(w, 1); w.emit(OpCode.ADD); w.emitSetLocal("i");
                w.emitJump(LBL_TOP);
            w.declareLabel(LBL_END);

            // ====== Test GC preciso: a1.nombre debe sobrevivir vía el bitmap ======
            w.emitLeaGlobal("tag_gc"); w.emit(OpCode.PRINT_STRING);

            // tmpName = string UTF-8 dinámico en heap "OOPS" (H2: string = byte[]
            // TYPE_ARRAY_I8). Antes usaba NEWARRAY/ASTORE (i32 codepoints), que
            // PRINT_STRING ahora lee como UTF-8 y truncaba a "O".
            push(w, 4); w.emit(OpCode.NEWARRAY_I8); w.emitSetLocal("tmpName");
            w.emitGetLocal("tmpName"); push(w, 0); push(w, 'O'); w.emit(OpCode.ASTORE_I8);
            w.emitGetLocal("tmpName"); push(w, 1); push(w, 'O'); w.emit(OpCode.ASTORE_I8);
            w.emitGetLocal("tmpName"); push(w, 2); push(w, 'P'); w.emit(OpCode.ASTORE_I8);
            w.emitGetLocal("tmpName"); push(w, 3); push(w, 'S'); w.emit(OpCode.ASTORE_I8);

            // a1 = nuevo Animal(peso=99, nombre=tmpName)  -- reasignamos a1 para descartar el viejo
            w.emitNewObject("Animal"); w.emitSetLocal("a1");
            w.emitGetLocal("a1"); push(w, 99);          w.emitSetField("Animal", "peso");
            w.emitGetLocal("a1"); w.emitGetLocal("tmpName"); w.emitSetField("Animal", "nombre");

            // Cortar TODAS las refs al heap excepto a1 (que mantiene "OOPS" vía a1.nombre)
            push(w, 0); w.emitSetLocal("p1");
            push(w, 0); w.emitSetLocal("p2");
            push(w, 0); w.emitSetLocal("arr");
            push(w, 0); w.emitSetLocal("tmpName");

            // GC manual: el sweep recicla p1/p2/arr y los antiguos objetos huérfanos.
            // a1 sigue alive (en local); su nombre "OOPS" sólo sobrevive si el bitmap traza el campo.
            w.emit(OpCode.GC_COLLECT);

            // Verifica que a1.nombre apunta a un array que aún contiene "OOPS"
            w.emitGetLocal("a1"); w.emitGetField("Animal", "nombre"); w.emit(OpCode.PRINT_STRING);

            w.emit(OpCode.HALT);

            w.writeToFile("OOPDemo.mod");
            System.out.println("[Compilador] 'OOPDemo.mod' generado.");

            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("OOPDemo.mod", "OOPDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado:");
            System.out.println("  === Demo clases + herencia + vtable ===");
            System.out.println("  [polimorfismo] hablar() sobre [Animal, Perro, Perro]:");
            System.out.println("  VM [PRINT]: 100   (Animal.hablar)");
            System.out.println("  VM [PRINT]: 150   (Perro.hablar override, 15*10)");
            System.out.println("  VM [PRINT]: 300   (Perro.hablar override, 30*10)");
            System.out.println("  [GC preciso] ...");
            System.out.println("  VM [GC]: heap=...");
            System.out.println("  OOPS              (a1.nombre vivo gracias al bitmap de campos-ref)");
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
