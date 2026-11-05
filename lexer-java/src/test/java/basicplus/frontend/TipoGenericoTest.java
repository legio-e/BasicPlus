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
 * V8/H8 (5-nov) — los tipos genéricos en el fuente. `#792`: el `>` que cierra un tipo al final de una línea no es un
 * operador colgante. Que salen bien en las dos VMs, Genericos, del corpus; aquí, lo que el compilador deja y lo que no.
 */
class TipoGenericoTest {

    /** Como en EmisorConErroresTest: un proyecto con la stdlib de ../bpstdlib; {ok, lo que dijo el compilador}. */
    private static Object[] compila(Path tmp, String main, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + ".bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
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

    private static void compilaBien(Path tmp, String fuente) throws IOException {
        Object[] r = compila(tmp, "T", fuente);
        assertTrue((Boolean) r[0], (String) r[1]);
    }

    @Test @DisplayName("#792: el retorno de una función, al final de la línea")
    void retorno(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  function f(): List<Object>\n    return List()\n  end f\n"
                + "  function Main()\n    print f().length()\n  end Main\nend T\n");
    }

    @Test @DisplayName("#792: una property, un campo y una var sin valor")
    void declaraciones(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  property p: List<Object>\n  class C\n    var c: List<Object>\n"
                + "    public property q: List<Object>\n    public function C()\n      this.c := List()\n"
                + "    end C\n  end C\n  function Main()\n    var v: List<Object>\n    v := List()\n"
                + "    print v.length()\n  end Main\nend T\n");
    }

    @Test @DisplayName("#792: con un comentario detrás y con líneas en blanco")
    void comentarioYBlancos(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  function f(): List<Object>   // el retorno\n\n\n"
                + "    return List()\n  end f\n  function Main()\n    print f().length()\n  end Main\nend T\n");
    }

    @Test @DisplayName("#792: dentro de unos paréntesis no hay salto que devolver")
    void entreParentesis(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  function f(a: List<Object>\n      ): integer\n"
                + "    return a.length()\n  end f\n  function Main()\n    print f(List())\n  end Main\nend T\n");
    }

    @Test @DisplayName("#792: un `>` que acaba una línea de verdad sigue siendo un operador colgante")
    void operadorColgante(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  function Main()\n    var b: boolean := 3 >\n      2\n"
                + "    print b\n  end Main\nend T\n");
    }
}
