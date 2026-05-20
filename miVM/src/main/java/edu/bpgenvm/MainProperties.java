/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm;

/**
 * Demo de properties con tres scopes:
 *   - Módulo (public): backing global + getter/setter como funciones públicas
 *     (entran en el exports table; otro módulo podría importarlas con CALL_EXT).
 *   - Instancia (public/private): backing field + getter/setter virtuales en
 *     la vtable. La visibilidad solo se enforza en compile-time desde ModWriter.
 *   - Estática (public): backing global cualificado "Clase.prop" + getter/setter
 *     como funciones (no virtuales, sin `this`). Pueden ir al exports table.
 *
 * Verifica también que el check de privacidad ataja un emit que toca una
 * property privada de Punto desde un main que no es método de Punto.
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import java.io.IOException;

public class MainProperties {

    public static void main(String[] args) {
        try {
            ModWriter w = new ModWriter();
            w.addModulo("PropDemo");

            // ---- Strings ----
            w.addConstantString("titulo",     "=== Demo properties: módulo / instancia / estática ===");
            w.addConstantString("tag_mod",    "module.contador  =");
            w.addConstantString("tag_p1x",    "p1.x             =");
            w.addConstantString("tag_p1y",    "p1.y             =");
            w.addConstantString("tag_p2x",    "p2.x             =");
            w.addConstantString("tag_static", "Punto.instancias =");
            w.addConstantString("tag_priv",   "p1.revelar()     =");

            // ---- Module property "contador" pública ----
            w.declareModuleProperty("contador", false, true);

            // ---- Clase Punto con properties de instancia ----
            w.addClass("Punto", null);
            w.declareInstanceProperty("x",       false, true);     // public
            w.declareInstanceProperty("y",       false, true);     // public
            w.declareInstanceProperty("secreto", false, false);    // private

            // Método público que lee la property privada (legítimo: dentro de la clase).
            w.addMethod("revelar");
                w.emitGetParam("this");
                w.emitGetInstanceProperty("Punto", "secreto");
                w.emitRet();
            w.endClass();

            // ---- Static property "Punto.instancias" pública ----
            w.declareStaticProperty("Punto", "instancias", false, true);

            // ---- main ----
            w.addFunction("main", true);
            w.declareLocal("p1");
            w.declareLocal("p2");
            w.declareLocal("ret");

            w.emitLeaGlobal("titulo"); w.emit(OpCode.PRINT_STRING);

            // module.contador = 7
            push(w, 7); w.emitSetModuleProperty("contador"); w.emitSetLocal("ret");
            w.emitLeaGlobal("tag_mod"); w.emit(OpCode.PRINT_STRING);
            w.emitGetModuleProperty("contador"); w.emit(OpCode.PRINT);

            // p1 = new Punto; p1.x = 3; p1.y = 4
            w.emitNewObject("Punto"); w.emitSetLocal("p1");
            w.emitGetLocal("p1"); push(w, 3); w.emitSetInstanceProperty("Punto", "x"); w.emitSetLocal("ret");
            w.emitGetLocal("p1"); push(w, 4); w.emitSetInstanceProperty("Punto", "y"); w.emitSetLocal("ret");

            // Punto.instancias += 1
            w.emitGetStaticProperty("Punto", "instancias");
            push(w, 1); w.emit(OpCode.ADD);
            w.emitSetStaticProperty("Punto", "instancias"); w.emitSetLocal("ret");

            w.emitLeaGlobal("tag_p1x"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("p1"); w.emitGetInstanceProperty("Punto", "x"); w.emit(OpCode.PRINT);
            w.emitLeaGlobal("tag_p1y"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("p1"); w.emitGetInstanceProperty("Punto", "y"); w.emit(OpCode.PRINT);

            // p2 = new Punto; p2.x = 10
            w.emitNewObject("Punto"); w.emitSetLocal("p2");
            w.emitGetLocal("p2"); push(w, 10); w.emitSetInstanceProperty("Punto", "x"); w.emitSetLocal("ret");
            w.emitGetStaticProperty("Punto", "instancias");
            push(w, 1); w.emit(OpCode.ADD);
            w.emitSetStaticProperty("Punto", "instancias"); w.emitSetLocal("ret");

            w.emitLeaGlobal("tag_p2x"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("p2"); w.emitGetInstanceProperty("Punto", "x"); w.emit(OpCode.PRINT);

            // Punto.instancias
            w.emitLeaGlobal("tag_static"); w.emit(OpCode.PRINT_STRING);
            w.emitGetStaticProperty("Punto", "instancias"); w.emit(OpCode.PRINT);

            // p1.revelar() — método público que internamente lee secreto (private permitido dentro de la clase)
            w.emitLeaGlobal("tag_priv"); w.emit(OpCode.PRINT_STRING);
            w.emitGetLocal("p1");
            w.emitInvokeVirtual("Punto", "revelar", 0);
            w.emit(OpCode.PRINT);

            w.emit(OpCode.HALT);

            w.writeToFile("PropDemo.mod");
            System.out.println("[Compilador] 'PropDemo.mod' generado.");

            // === Test compile-time: acceder a una property privada desde main fuera de la clase ===
            System.out.println();
            System.out.println("--- Test de privacidad (compile-time) ---");
            try {
                ModWriter w2 = new ModWriter();
                w2.addModulo("BadDemo");
                w2.addClass("Punto", null);
                w2.declareInstanceProperty("secreto", false, false);
                w2.endClass();
                w2.addFunction("main", true);
                w2.declareLocal("p");
                w2.emitNewObject("Punto"); w2.emitSetLocal("p");
                w2.emitGetLocal("p");
                w2.emitGetInstanceProperty("Punto", "secreto"); // <-- debe lanzar
                System.out.println("[!] FALLO: no se atajó el acceso privado");
            } catch (RuntimeException e) {
                System.out.println("[OK] " + e.getMessage());
            }

            // === Ejecución ===
            System.out.println();
            VirtualMachine vm = new VirtualMachine();
            ModuleManager loader = new ModuleManager(vm);
            vm.setModuleManager(loader);
            loader.executeRootModule("PropDemo.mod", "PropDemo");
            vm.run();

            System.out.println();
            System.out.println("Esperado:");
            System.out.println("  module.contador  = 7");
            System.out.println("  p1.x             = 3");
            System.out.println("  p1.y             = 4");
            System.out.println("  p2.x             = 10");
            System.out.println("  Punto.instancias = 2");
            System.out.println("  p1.revelar()     = 0   (secreto sin inicializar)");
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
