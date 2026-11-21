package basicplus.frontend;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8/H8 (4-nov; Eduardo: «el 12 si») — `&`, `|` y `xor` entre dos boolean, como en Java y en C#: calculan los dos lados. Que
 * salen igual en las dos VMs lo prueba BoolBits, del corpus; que pasan por el AOT, AotCoberturaTest. Aquí, lo que el
 * compilador deja y lo que no.
 */
class BoolBitsTest {

    /** Como en EmisorConErroresTest: un proyecto con la stdlib de ../../bpstdlib; {ok, lo que dijo el compilador}. */
    private static Object[] compila(Path tmp, String main, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + ".bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try { System.setOut(ps); System.setErr(ps); ok = Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8) };
    }

    private static String modulo(String nombre, String cuerpo) {
        return "module " + nombre + "\n  import Core\n  function Main()\n" + cuerpo + "  end Main\nend " + nombre + "\n";
    }

    @Test @DisplayName("entre dos boolean: & | xor, sus compuestos, y el resultado es un boolean")
    void entreDosBoolean(@TempDir Path tmp) throws IOException {
        String cuerpo = String.join("\n",
            "    var a: boolean := true & false", "    var b: boolean := a | true", "    var c: boolean := a xor b",
            "    c &= a", "    c |= b", "    c xor= a", "    if a & b then", "      print 1", "    endif", "");
        Object[] r = compila(tmp, "Bits", modulo("Bits", cuerpo));
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
    }

    @Test @DisplayName("un boolean y un entero, no: el mensaje dice lo que vale")
    void unoDeCada(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal", modulo("Mal", "    var a: boolean := true & 1\n"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("'&' requiere integer/long, o dos boolean"), (String) r[1]);
    }

    @Test @DisplayName("un desplazamiento sobre un boolean, no")
    void desplazar(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal2", modulo("Mal2", "    var a: boolean := true shl false\n"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("'shl' requiere integer/long"), (String) r[1]);
    }
}
