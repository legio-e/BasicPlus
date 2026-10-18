package basicplus.frontend;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.OutputSink;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8 #700 (18-oct) — un `print` que acaba en `;` no salta de línea. La paridad de {@code compat.sh} no lo guarda:
 * si el compilador volviera a poner el salto, las dos VMs darían lo mismo y el arnés seguiría en verde. Aquí se mira
 * la salida EXACTA, con los saltos, contra la lista de la cabecera del sample ({@code PrintSinSalto.bp}: lo que
 * tiene que salir vive en un sitio). Y lo que el arnés no ve, porque el aviso de fin de la VM se le pega detrás y su
 * filtro tira la línea: lo que queda al acabar el programa sale tal cual, sin el salto.
 */
class PrintSinSaltoTest {

    private static final Path SAMPLE = Paths.get("../bpgenvm-c/samples/PrintSinSalto.bp");   // CWD: lexer-java/

    /** Compila `fuente` (módulo `main`) y lo corre en la miVM; devuelve lo que el programa imprime, sin más. */
    private static String compilaYCorre(Path tmp, String main, String fuente) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + ".bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = main;
        String antes = Main.STDLIB_DIR;
        try {
            Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
            assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        } finally {
            Main.STDLIB_DIR = antes;
        }
        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        mm.setStdlibDir("../bpstdlib");
        vm.setModuleManager(mm);
        StringBuilder salida = new StringBuilder();
        vm.setProgramOut(new OutputSink() {
            @Override public void writeText(String s) { salida.append(s); }
            @Override public void writeChar(char c) { salida.append(c); }
            @Override public void newline() { salida.append('\n'); }
            @Override public void writeLine(String s) { salida.append(s).append('\n'); }   // #600
            @Override public void flush() { }
        });
        mm.executeRootModule(out.resolve(main + ".mod").toString(), main);
        vm.run();
        return salida.toString();
    }

    /** Las líneas de la cabecera del sample entre «Lo que tiene que salir:» y el `module`, cada una con su salto. */
    private static String esperadoDeLaCabecera(List<String> lineas) {
        StringBuilder sb = new StringBuilder();
        boolean dentro = false;
        for (String l : lineas) {
            if (l.startsWith("module ")) break;
            if (dentro) {
                assertTrue(l.startsWith("//   "), "una línea de lo esperado sin su sangría: " + l);
                sb.append(l.substring(5).trim()).append('\n');
            } else if (l.contains("Lo que tiene que salir:")) {
                dentro = true;
            }
        }
        assertTrue(dentro && sb.length() > 0, "la cabecera del sample tiene que decir lo que sale");
        return sb.toString();
    }

    @Test
    @Timeout(120)
    @DisplayName("PrintSinSalto: las piezas de varios `print` acabados en `;` salen en una línea, con un hilo y una native")
    void elSample(@TempDir Path tmp) throws Exception {
        List<String> lineas = Files.readAllLines(SAMPLE, StandardCharsets.UTF_8);
        String salida = compilaYCorre(tmp, "PrintSinSalto", String.join("\n", lineas) + "\n");
        assertEquals(esperadoDeLaCabecera(lineas), salida);
    }

    @Test
    @Timeout(120)
    @DisplayName("lo que queda al acabar el programa sale tal cual, sin el salto")
    void alAcabarElPrograma(@TempDir Path tmp) throws Exception {
        String fuente = String.join("\n",
            "module SinSaltoAlFinal",
            "  function main()",
            "    print \"uno\"",
            "    print \"dos\";",
            "  end main",
            "end SinSaltoAlFinal", "");
        assertEquals("uno\ndos", compilaYCorre(tmp, "SinSaltoAlFinal", fuente));
    }
}
