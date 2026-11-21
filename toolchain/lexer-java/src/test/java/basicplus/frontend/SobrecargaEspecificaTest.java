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
 * V8 #772 (3-nov) — de las sobrecargas que encajan, la MÁS ESPECÍFICA, como en Java (Eduardo: «como en Java»; y su
 * ejemplo, el de los catch: de lo más específico a lo más genérico, y «es el compilador el que tiene que ordenar»). La
 * más específica es la que se puede pasar a todas las demás. Si ninguna lo es, la llamada es ambigua, y el error nombra
 * las que compiten. Que eligen lo mismo en las dos VMs lo prueba SobrecargaEspecifica, del corpus.
 */
class SobrecargaEspecificaTest {

    private static final String CLASES = String.join("\n",
        "  class Animal", "    public function Animal()", "    end Animal", "  end Animal",
        "  class Perro extends Animal", "    public function Perro()", "      super()", "    end Perro", "  end Perro", "");

    private static String modulo(String nombre, String cuerpo) {
        return "module " + nombre + "\n  import Core\n" + CLASES + cuerpo + "end " + nombre + "\n";
    }

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

    @Test @DisplayName("la más específica gana: un Perro va a la de Animal, null a la de string, un byte a la de integer")
    void laMasEspecifica(@TempDir Path tmp) throws IOException {
        String cuerpo = String.join("\n",
            "  function mete(o: Object): string", "    return \"O\"", "  end mete",
            "  function mete(a: Animal): string", "    return \"A\"", "  end mete",
            "  function foo(s: string): string", "    return \"s\"", "  end foo",
            "  function foo(o: Object): string", "    return \"o\"", "  end foo",
            "  function g(x: integer): string", "    return \"i\"", "  end g",
            "  function g(x: long): string", "    return \"l\"", "  end g",
            "  function Main()", "    var p: Perro := Perro()", "    var b: byte := 5",
            "    print mete(p) + foo(null) + g(b)", "  end Main", "");
        Object[] r = compila(tmp, "Especifica", modulo("Especifica", cuerpo));
        String t = (String) r[1];
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + t);
        assertFalse(t.contains("llamada ambigua"), t);
    }

    @Test @DisplayName("ninguna más específica: ambigua, con las que compiten y cómo elegir")
    void ambigua(@TempDir Path tmp) throws IOException {
        String cuerpo = String.join("\n",
            "  function f(a: Perro, b: Object): string", "    return \"1\"", "  end f",
            "  function f(a: Object, b: Perro): string", "    return \"2\"", "  end f",
            "  function Main()", "    var p: Perro := Perro()", "    print f(p, p)", "  end Main", "");
        Object[] r = compila(tmp, "Ambigua", modulo("Ambigua", cuerpo));
        String t = (String) r[1];
        assertFalse((Boolean) r[0], "tiene que fallar:\n" + t);
        assertTrue(t.contains("llamada ambigua a 'f' con (Perro, Perro): encajan f(Perro, Object) y f(Object, Perro), y"
                + " ninguna es más específica que las otras; para elegir una, pasa el argumento en una variable del tipo"
                + " de su parámetro"), t);
        assertFalse(t.contains("usa un cast"), "un cast hacia arriba no se puede escribir:\n" + t);
    }

    @Test @DisplayName("el caso raro, como en Java: h(Perro, Object) y h(Animal, Animal) con (Perro, Perro) es ambigua")
    void elCasoRaro(@TempDir Path tmp) throws IOException {
        String cuerpo = String.join("\n",
            "  function h(a: Perro, b: Object): string", "    return \"1\"", "  end h",
            "  function h(a: Animal, b: Animal): string", "    return \"2\"", "  end h",
            "  function Main()", "    var p: Perro := Perro()", "    print h(p, p)", "  end Main", "");
        Object[] r = compila(tmp, "Rara", modulo("Rara", cuerpo));
        String t = (String) r[1];
        assertFalse((Boolean) r[0], "hasta el 3-nov ganaba la primera por puntos; como en Java, ambigua:\n" + t);
        assertTrue(t.contains("encajan h(Perro, Object) y h(Animal, Animal)"), t);
    }

    @Test @DisplayName("con los mismos tipos, la que rellena menos valores por defecto (14-sep), como antes")
    void losDefectosDesempatan(@TempDir Path tmp) throws IOException {
        String cuerpo = String.join("\n",
            "  function k(a: integer): string", "    return \"1\"", "  end k",
            "  function k(a: integer, b: integer := 0): string", "    return \"2\"", "  end k",
            "  function Main()", "    print k(1) + k(1, 2)", "  end Main", "");
        Object[] r = compila(tmp, "Defectos", modulo("Defectos", cuerpo));
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
    }
}
