package basicplus.frontend;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.OutputSink;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

/**
 * #494 (V7, 27-sep) — el HEAP DESCARRILADO es UNA EXCEPCIÓN (decisión de Eduardo: ni avisar y seguir, que era
 * barrer con el set truncado y llevarse objetos vivos, ni parar la VM). El espejo en miVM de
 * {@code bpgenvm-c/test/test_heapdescarrilado.c}, con el MISMO programa ({@code bpgenvm-c/samples/HeapRoto.bp}):
 * un gancho de depuración estropea en la primera línea la cabecera del primer bloque del heap
 * ({@link VirtualMachine#estropeaHeapParaPrueba}); el GC siguiente descarrila, no barre, y la reserva que lo pidió
 * lanza la excepción prefabricada, que el programa atrapa e imprime. El control, sin estropear: «sin excepcion».
 */
class HeapDescarriladoTest {

    /** Corre HeapRoto (compilado en `out`) y devuelve lo que imprime. */
    private static String corre(Path out, boolean estropear) throws Exception {
        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        mm.setStdlibDir("../bpstdlib");          // el CWD de la prueba es lexer-java/
        vm.setModuleManager(mm);
        StringBuilder salida = new StringBuilder();
        vm.setProgramOut(new OutputSink() {
            @Override public void writeText(String s) { salida.append(s); }
            @Override public void writeChar(char c) { salida.append(c); }
            @Override public void newline() { salida.append('\n'); }
            @Override public void flush() { }
        });
        boolean[] hecho = {false};
        vm.setDebugHook(ctx -> {
            if (estropear && !hecho[0]) {
                hecho[0] = true;
                vm.estropeaHeapParaPrueba();
            }
        });
        mm.executeRootModule(out.resolve("HeapRoto.mod").toString(), "HeapRoto");
        vm.run();
        assertEquals(estropear, hecho[0], "el gancho tenía que estropear el heap (o no)");
        return salida.toString();
    }

    @Test
    @Timeout(120)
    @DisplayName("el heap descarrilado es una excepción atrapable, con el mismo texto que la VM-C")
    void heapDescarrilado(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.copy(Paths.get("../bpgenvm-c/samples/HeapRoto.bp"), src.resolve("HeapRoto.bp"));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "HeapRoto";
        // RuntimeError es de Core: el compilador necesita su interfaz (el CWD de la prueba, lexer-java/, no
        // tiene BpVM.cfg). La stdlib del formato actual, como AotImportsTest; y se deja como estaba.
        String antes = Main.STDLIB_DIR;
        try {
            Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
            assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        } finally {
            Main.STDLIB_DIR = antes;
        }

        String control = corre(out, false);
        assertTrue(control.contains("sin excepcion"), "el control, con el heap sano, no lanza: " + control);
        assertTrue(control.contains("fin"), control);

        String roto = corre(out, true);
        assertTrue(roto.contains("cazada: " + VirtualMachine.MSG_HEAP_DESCARRILADO),
                "el GC descarrilado tenía que lanzar la prefabricada, y el programa atraparla: " + roto);
        assertTrue(roto.contains("fin"), "tras atraparla, el programa sigue: " + roto);
        // #598 (28-sep): y lo vivo de antes, intacto —un GC descarrilado no marca ni barre—.
        assertTrue(roto.contains("lo de antes: guardado 42"), "lo vivo tenía que sobrevivir al GC descarrilado: " + roto);
        assertTrue(control.contains("lo de antes: guardado 42"), control);
        assertEquals("heap descarrilado (fallo interno de la VM)", VirtualMachine.MSG_HEAP_DESCARRILADO,
                "el MISMO texto que la VM-C (BPVM_MSG_HEAP_DESCARRILADO)");
    }
}
