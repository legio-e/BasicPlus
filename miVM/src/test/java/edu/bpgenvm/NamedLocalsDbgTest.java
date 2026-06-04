package edu.bpgenvm;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.HashSet;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

/**
 * H6.a.1 — verifica que el `.dbg` v3 (sección `vars`) se parsea y que
 * {@link ModuleManager#functionForPc} resuelve la función + sus variables
 * (params + locales) por offset. Es la mitad VM del feature "locales por
 * nombre"; la otra mitad (DebugServer.LOCALS_REPLY.named + IDE) es plumbing
 * directo sobre esto.
 *
 * <p>Carga {@code samples/LocalsDbg.mod} (NO ejecuta bytecode: executeRootModule
 * sólo carga+linka). Si el sample no está compilado, el test se omite — así no
 * rompe CI sin el artefacto, pero verifica localmente tras `--backend=mivm`.</p>
 */
class NamedLocalsDbgTest {

    @Test
    void functionForPcResuelveVarsDeSuma() throws Exception {
        // CWD del test = miVM/ ; el sample vive en ../samples/.
        File mod = new File("../samples/LocalsDbg.mod");
        assumeTrue(mod.isFile(),
                "samples/LocalsDbg.mod no compilado (compila con --backend=mivm); test omitido");

        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        vm.setModuleManager(mm);
        mm.executeRootModule(mod.getPath(), "LocalsDbg");   // carga+linka, NO ejecuta

        // Busca un pc cuya línea-origen sea 10 (`total := total + i`, dentro de
        // suma) escaneando con getLineForPc — misma mecánica que usa el hook.
        int pcLinea10 = -1;
        for (int pc = 0; pc < 300_000; pc++) {
            if (mm.getLineForPc(pc) == 10) { pcLinea10 = pc; break; }
        }
        assertTrue(pcLinea10 >= 0, "no se encontró pc para la línea 10 (¿.dbg sin lines?)");

        ModuleManager.FunctionVars fv = mm.functionForPc(pcLinea10);
        assertNotNull(fv, "functionForPc devolvió null para un pc dentro de suma");
        assertEquals("suma", fv.name, "el pc de la línea 10 debe caer en suma");

        // suma(n, base): params n,base (offset<0) + locales total,big(8B),i (offset>=0).
        Set<String> names = new HashSet<>();
        ModuleManager.LocalVarDescriptor big = null;
        for (ModuleManager.LocalVarDescriptor v : fv.vars) {
            names.add(v.name);
            if (v.name.equals("big")) big = v;
        }
        assertTrue(names.containsAll(java.util.Arrays.asList("n", "base", "total", "big", "i")),
                "faltan vars; vi " + names);

        // 'big' es long → 8 bytes + tipo "long". 'n'/'base' son params (offset<0).
        assertNotNull(big);
        assertEquals(8, big.sizeBytes, "big:long debe ocupar 8 bytes");
        assertEquals("long", big.type, "big debe etiquetarse como long");   // H6.a.2
        for (ModuleManager.LocalVarDescriptor v : fv.vars) {
            if (v.name.equals("n") || v.name.equals("base")) {
                assertTrue(v.offset < 0, v.name + " es param → offset<0, vi " + v.offset);
                assertEquals("integer", v.type, v.name + " debe ser integer");   // H6.a.2
            }
            if (v.name.equals("total") || v.name.equals("i")) {
                assertTrue(v.offset >= 0, v.name + " es local → offset>=0, vi " + v.offset);
                assertEquals("integer", v.type, v.name + " debe ser integer");   // H6.a.2
            }
        }
    }
}
