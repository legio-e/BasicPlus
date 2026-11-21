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
 * V8/H8 (5-nov) — `A[n]` en BP y `new A[n]` en JC: un array de n casillas de A. Que sale igual en las dos VMs, de cada
 * tipo, lo dice ArrayNuevo, del corpus; en JC, el mismo corpus; en una native, AotCoberturaTest y NatSwitch; lo que no
 * vale en BP, samples/errores/ArrayNuevoMal. Aquí, lo que el corpus no puede decir: lo que no vale en JC, y que un nombre
 * con valor gana a un tipo en BP y no tras `new`.
 */
class ArrayNuevoTest {

    /** Como en EmisorConErroresTest: un proyecto con la stdlib de ../../bpstdlib; {ok, lo que dijo el compilador}. */
    private static Object[] compila(Path tmp, String main, String extension, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + extension), fuente.getBytes(StandardCharsets.UTF_8));
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

    private static String jc(String nombre, String... cuerpo) {
        return "module " + nombre + "\n{\n   import Core;\n   public class A\n   {\n   }\n"
                + "   public function void main()\n   {\n" + String.join("\n", cuerpo) + "\n   }\n}\n";
    }

    private static String bp(String nombre, String... cuerpo) {
        return "module " + nombre + "\n  import Core\n  public class A\n    public property x: integer\n  end A\n"
                + "  function Main()\n" + String.join("\n", cuerpo) + "\n  end Main\nend " + nombre + "\n";
    }

    @Test @DisplayName("JC: new con una clase, con los tipos de JC y con el módulo delante")
    void enJc(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Vale", ".jc", jc("Vale",
            "      A[] a = new A[3];", "      int[] b = new int[2 + 1];", "      string[] c = new string[1];",
            "      bool[] d = new bool[1];", "      sbyte[] e = new sbyte[1];", "      ushort[] f = new ushort[1];",
            "      A[] g = new Vale.A[1];", "      println(len(a) + len(b) + len(c) + len(d) + len(e) + len(f) + len(g));"));
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
    }

    @Test @DisplayName("JC: sin new, el error dice cómo se escribe")
    void sinNew(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal", ".jc", jc("Mal", "      A[] a = A[3];", "      println(len(a));"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("'A[n]' crea un array: en JC se escribe 'new A[n]'"), (String) r[1]);
        Object[] r2 = compila(tmp, "Mal1", ".jc", jc("Mal1", "      int[] a = integer[3];", "      println(len(a));"));
        assertFalse((Boolean) r2[0]);
        assertTrue(((String) r2[1]).contains("'integer[n]' crea un array: en JC se escribe 'new int[n]'"), (String) r2[1]);
    }

    @Test @DisplayName("JC: tras new va un tipo, aunque el nombre tenga valor")
    void noEsUnTipo(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal2", ".jc", jc("Mal2", "      int[] v = [1, 2];", "      println(len(new v[3]));"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("tras 'new' va el tipo de los elementos, y 'v' no es un tipo"), (String) r[1]);
        // Un nombre con valor no tapa al tipo: `new` dice que es un tipo
        Object[] r2 = compila(tmp, "Vale2", ".jc", jc("Vale2", "      int[] A = [1, 2];", "      println(len(new A[3]));"));
        assertTrue((Boolean) r2[0], "tiene que compilar:\n" + r2[1]);
    }

    @Test @DisplayName("JC: un array de arrays no se crea con new")
    void deArrays(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal3", ".jc", jc("Mal3", "      println(len(new int[3][2]));"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("un array de arrays no se crea con new"), (String) r[1]);
    }

    @Test @DisplayName("JC: `new int` sin corchetes no es nada")
    void sinCorchetes(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal4", ".jc", jc("Mal4", "      println(new int(3));"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("tras el tipo: new int[n]"), (String) r[1]);
    }

    @Test @DisplayName("BP: un nombre con valor gana y se indexa; sin él, crea")
    void ganaElValor(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Vale3", ".bp", bp("Vale3", "    var A: integer[] := [1, 2]", "    print A[1] + 1"));
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
        Object[] r2 = compila(tmp, "Mal5", ".bp", bp("Mal5", "    print A[1] + 1"));
        assertFalse((Boolean) r2[0], "A[1] es un A[]: no se le suma 1\n" + r2[1]);
    }

    @Test @DisplayName("BP: un array, de un campo o de una global, se sigue indexando")
    void losDeSiempre(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Vale4", ".bp", "module Vale4\n  import Core\n  public class C\n"
            + "    public property v: integer[]\n    public function C()\n      this.v := [4, 5]\n    end C\n"
            + "    public function dos(): integer\n      return this.v[1]\n    end dos\n  end C\n"
            + "  var g: integer[] := [1]\n  function Main()\n    var c: C := C()\n    print c.v[0], g[0], c.dos()\n"
            + "  end Main\nend Vale4\n");
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
    }
}
