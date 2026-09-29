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

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H7 #612 (29-sep) — un ERROR de Java en un worker (un OutOfMemoryError, un StackOverflowError: un fallo de la VM,
 * no del programa) para la VM, lo dice, y el RUN acaba como «fallo interno» (la salida 3 de la tabla común de #481,
 * como la VM-C). Antes el worker moría con la traza de Java y el RUN acababa como si hubiera terminado: sin imprimir
 * nada y con salida 0; y con varios workers, el hilo de ese worker se quedaba RUNNING para siempre (la prueba de
 * estrés de #593 lo vio con el split de #611). El gancho de depuración corre DENTRO del worker: lanzar desde él un
 * Error es provocarlo donde pasa de verdad, en un hilo que no es el main. Con uno y con dos workers; y el control.
 */
class FalloInternoTest {

    private static final String PROGRAMA =
            "module FalloInt\n"
          + "  class Suma extends Thread\n"
          + "    public property n: integer\n"
          + "    public function Suma()\n"
          + "      super(0)\n"
          + "      this.n := 0\n"
          + "    end Suma\n"
          + "    public function run()\n"
          + "      var i: integer := 0\n"
          + "      while i < 50 do\n"
          + "        this.n := this.n + i\n"
          + "        i := i + 1\n"
          + "      endwh\n"
          + "    end run\n"
          + "  end Suma\n"
          + "\n"
          + "  function main(arg: string)\n"
          + "    var s: Suma := Suma()\n"
          + "    s.start()\n"
          + "    s.join()\n"
          + "    print \"suma: \", s.n\n"
          + "  end main\n"
          + "end FalloInt\n";

    /** Corre FalloInt con `workers` workers; con `provocar`, el gancho lanza un Error en el hilo que no es el main. */
    private static VirtualMachine corre(Path out, int workers, boolean provocar, StringBuilder salida) throws Exception {
        VirtualMachine vm = new VirtualMachine();
        vm.setNumWorkers(workers);
        ModuleManager mm = new ModuleManager(vm);
        mm.setStdlibDir("../bpstdlib");          // el CWD de la prueba es lexer-java/
        vm.setModuleManager(mm);
        vm.setProgramOut(new OutputSink() {
            @Override public void writeText(String s) { salida.append(s); }
            @Override public void writeChar(char c) { salida.append(c); }
            @Override public void newline() { salida.append('\n'); }
            @Override public void writeLine(String s) { salida.append(s).append('\n'); }
            @Override public void flush() { }
        });
        boolean[] hecho = {false};
        vm.setDebugHook(ctx -> {
            if (provocar && ctx.tid != 0 && !hecho[0]) {
                hecho[0] = true;
                throw new AssertionError("provocado por la prueba de #612");
            }
        });
        mm.executeRootModule(out.resolve("FalloInt.mod").toString(), "FalloInt");
        vm.run();
        assertEquals(provocar, hecho[0], "el gancho tenía que provocar el Error (o no)");
        return vm;
    }

    @Test
    @Timeout(120)
    @DisplayName("un Error de Java en un worker para la VM y acaba como fallo interno, con uno y con dos workers")
    void falloInterno(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("FalloInt.bp"), PROGRAMA.getBytes(java.nio.charset.StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "FalloInt";
        String antes = Main.STDLIB_DIR;
        try {
            Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
            assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        } finally {
            Main.STDLIB_DIR = antes;
        }

        for (int workers : new int[] {1, 2}) {
            StringBuilder control = new StringBuilder();
            VirtualMachine sano = corre(out, workers, false, control);
            assertTrue(control.toString().contains("suma:  1225"),   // `print a, b`: el separador es un espacio
                    "el control, con " + workers + " worker(s): " + control);
            assertFalse(sano.isFalloInterno(), "el control no es un fallo interno");
            assertEquals("", sano.getRuntimeError(), "el control acaba bien");

            StringBuilder roto = new StringBuilder();
            VirtualMachine vm = corre(out, workers, true, roto);
            assertTrue(vm.isFalloInterno(), "con " + workers + " worker(s), el Error es un fallo interno de la VM");
            assertTrue(vm.getRuntimeError().startsWith("fallo interno de la VM"),
                    "y lo dice: «" + vm.getRuntimeError() + "»");
            assertFalse(roto.toString().contains("suma:"), "el programa no llega a imprimir: " + roto);
        }
    }
}
