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
 * V8/H7.4 (7-nov) — los parámetros `var`. Que funcionan, igual en las dos VMs, lo dice samples/VarParam.bp, del corpus;
 * que JC los escribe y los lee, la ida y vuelta del corpus. Aquí, lo que no vale: cada regla, con su mensaje.
 */
class VarParamTest {

    /** Como en ArrayNuevoTest: un proyecto con la stdlib de ../../bpstdlib; {ok, lo que dijo el compilador}. */
    private static Object[] compila(Path tmp, String main, String extension, String fuente, int version) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + extension), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = version;
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

    /** Un módulo BP con una clase C (x: integer, s: string, un método m y una property owner) y f(var n: integer). */
    private static String bp(String declaraciones, String... cuerpo) {
        return "module M\n  import Core\n"
                + "  public class C\n    public property x: integer\n    public property s: string\n"
                + "    public property owner o: C := null\n"
                + "    public function m(): integer\n      return 1\n    end m\n  end C\n"
                + "  function f(var n: integer)\n    n += 1\n  end f\n"
                + "  function g(n: integer)\n    print n\n  end g\n"
                + declaraciones
                + "  function Main()\n    var c: C := C()\n" + String.join("\n", cuerpo) + "\n  end Main\nend M\n";
    }

    private static void falla(Path tmp, String fuente, String trozo) throws IOException {
        Object[] r = compila(tmp, "M", ".bp", fuente, 7);
        assertFalse((Boolean) r[0], "tenía que fallar con «" + trozo + "»:\n" + r[1]);
        assertTrue(((String) r[1]).contains(trozo), "esperaba «" + trozo + "» y dijo:\n" + r[1]);
    }

    @Test @DisplayName("BP: lo que vale compila")
    void vale(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "M", ".bp", bp("  function h(var n: integer)\n    f(n)\n  end h\n",
                "    f(c::x)", "    h(c::x)", "    g(c.x)"), 7);
        assertTrue((Boolean) r[0], (String) r[1]);
    }

    @Test @DisplayName("BP: un valor a un parámetro var")
    void unValor(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("", "    f(c.x)"), "es un parámetro var: se le pasa 'objeto::property' (con '::', no con '.'");
    }

    @Test @DisplayName("BP: una variable a un parámetro var")
    void unaVariable(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("", "    var k: integer := 1", "    f(k)"), "es un parámetro var: se le pasa 'objeto::property'");
    }

    @Test @DisplayName("BP: x::p a un parámetro que no es var")
    void aUnoNormal(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("", "    g(c::x)"), "sólo se pasa a un parámetro var, y 'n' no lo es");
    }

    @Test @DisplayName("BP: otro tipo")
    void otroTipo(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("", "    f(c::s)"), "tienen que ser del mismo tipo");
    }

    @Test @DisplayName("BP: un método no es una property")
    void unMetodo(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("", "    f(c::m)"), "es un método: a un parámetro var se le pasa una property");
    }

    @Test @DisplayName("BP: una property owner")
    void owner(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("  function k(var n: C)\n    n := null\n  end k\n", "    k(c::o)"), "es una property owner");
    }

    @Test @DisplayName("BP: valor por defecto")
    void porDefecto(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("  function h(var n: integer := 1)\n    n := 2\n  end h\n", "    h(c::x)"),
              "no lleva valor por defecto");
    }

    @Test @DisplayName("BP: sobrecargas que sólo difieren en var")
    void soloVar(@TempDir Path tmp) throws IOException {
        falla(tmp, bp("  function h(var n: integer)\n    n := 2\n  end h\n  function h(n: integer)\n    print n\n  end h\n",
                "    h(c::x)"), "que sólo difiere en 'var'");
    }

    @Test @DisplayName("BP: un evento no lleva var")
    void evento(@TempDir Path tmp) throws IOException {
        String fuente = "module M\n  import Core\n  public class B\n    event onX(var n: integer)\n"
                + "    public function B()\n    end B\n  end B\n  function Main()\n    var b: B := B()\n  end Main\nend M\n";
        falla(tmp, fuente, "un evento no lleva parámetros var");
    }

    @Test @DisplayName("BP: un constructor no lleva var")
    void constructor(@TempDir Path tmp) throws IOException {
        String fuente = "module M\n  import Core\n  public class B\n    public property x: integer\n"
                + "    public function B(var n: integer)\n    end B\n  end B\n  function Main()\n  end Main\nend M\n";
        falla(tmp, fuente, "un constructor no lleva parámetros var");
    }

    @Test @DisplayName("MOD8: no")
    void mod8(@TempDir Path tmp) throws IOException {
        int antes = Main.FORMATO;
        Object[] r;
        try { Main.FORMATO = 8; r = compila(tmp, "M", ".bp", bp("", "    f(c::x)"), 7); }
        finally { Main.FORMATO = antes; }
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("son del enlace tardío (MOD9)"), (String) r[1]);
    }

    @Test @DisplayName("JC: var int n, y x::p")
    void enJc(@TempDir Path tmp) throws IOException {
        String jc = "module M\n{\n   import Core;\n   public class C\n   {\n      public int x { get; set; }\n   }\n"
                + "   function void f(var int n)\n   {\n      n += 1;\n   }\n"
                + "   public function void main()\n   {\n      C c = new C();\n      f(c::x);\n      print(c.x);\n   }\n}\n";
        Object[] r = compila(tmp, "M", ".jc", jc, 7);
        assertTrue((Boolean) r[0], (String) r[1]);
        Object[] mal = compila(tmp.resolve("mal"), "M", ".jc", jc.replace("f(c::x);", "f(c.x);"), 7);
        assertFalse((Boolean) mal[0], (String) mal[1]);
        assertTrue(((String) mal[1]).contains("es un parámetro var"), (String) mal[1]);
    }
}
