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
 * V8/H8, tema 8 (7-nov) — `==` entre una clase y una antecesora suya: la identidad, como en Java (lo que vale, en
 * samples/IgualdadRefs.bp, del corpus). Aquí, lo que sigue sin valer: dos clases sin parentesco.
 */
class IgualdadRefsTest {

    private static Object[] compila(Path tmp, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("M.bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = "M";
        b.version = 7;
        File std = new File("../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try { System.setOut(ps); System.setErr(ps); ok = Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8) };
    }

    @Test @DisplayName("un Perro y un Gato no se comparan")
    void sinParentesco(@TempDir Path tmp) throws IOException {
        String fuente = "module M\n  import Core\n  class Animal\n    public function Animal()\n    end Animal\n  end Animal\n"
                + "  class Perro extends Animal\n    public function Perro()\n      super()\n    end Perro\n  end Perro\n"
                + "  class Gato extends Animal\n    public function Gato()\n      super()\n    end Gato\n  end Gato\n"
                + "  function Main()\n    var p: Perro := Perro()\n    var g: Gato := Gato()\n    print p == g\n  end Main\nend M\n";
        Object[] r = compila(tmp, fuente);
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("incompatible: 'Perro' vs 'Gato'"), (String) r[1]);
    }
}
