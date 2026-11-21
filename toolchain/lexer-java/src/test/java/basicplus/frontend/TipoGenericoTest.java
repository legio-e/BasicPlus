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
 * operador colgante. `#793`: `C<X>`, una orden al compilador. `#794`: `x[i]` sobre un objeto. Que salen bien en las dos VMs,
 * Genericos, GenericosOrden e IndiceObjeto, del corpus; los errores, GenericosMal e IndiceMal; aquí, lo que el compilador deja
 * y lo que no.
 */
class TipoGenericoTest {

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

    private static void compilaBien(Path tmp, String fuente) throws IOException {
        Object[] r = compila(tmp, "T", fuente);
        assertTrue((Boolean) r[0], (String) r[1]);
    }

    private static String error(Path tmp, String fuente) throws IOException {
        Object[] r = compila(tmp, "T", fuente);
        assertFalse((Boolean) r[0], "tenía que fallar:\n" + r[1]);
        return (String) r[1];
    }

    private static final String CLASES = "  class Animal\n    public function Animal()\n    end Animal\n  end Animal\n"
            + "  class Perro extends Animal\n    public function Perro()\n    end Perro\n  end Perro\n";

    private static String conClases(String cuerpo) {
        return "module T\n  import Core\n" + CLASES + "  function Main()\n" + cuerpo + "  end Main\nend T\n";
    }

    @Test @DisplayName("#793: `List<Perro>` no pasa a `List<Animal>`, ni al revés")
    void sinVarianza(@TempDir Path tmp) throws IOException {
        assertTrue(error(tmp, conClases("    var p: List<Perro> := List()\n    var a: List<Animal> := p\n"))
                .contains("valor de tipo 'List<Perro>' no asignable a variable de tipo 'List<Animal>'"));
    }

    @Test @DisplayName("#793: una List a secas se mezcla con una List<X> en los dos sentidos")
    void aSecas(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, conClases("    var p: List<Perro> := List()\n    var c: List := p\n"
                + "    var q: List<Perro> := c\n    print q.length()\n"));
    }

    @Test @DisplayName("#793: una hija entra donde se guarda su padre, y sale como el padre")
    void hijaEntra(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, conClases("    var a: List<Animal> := List()\n    a.add(Perro())\n"
                + "    var x: Animal := a.get(0)\n    print x == null\n"));
    }

    @Test @DisplayName("#793: add(5) vale en una List<Integer> y en una List<Comparable>, por su caja")
    void cajas(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, conClases("    var a: List<Integer> := List()\n    a.add(5)\n"
                + "    var b: List<Comparable> := List()\n    b.add(5)\n    b.add(2.5d)\n    print a.length()\n"));
    }

    @Test @DisplayName("#793: y no en una List<Long> ni en una de otra cosa")
    void cajaQueNoEs(@TempDir Path tmp) throws IOException {
        assertTrue(error(tmp, conClases("    var a: List<Long> := List()\n    a.add(5)\n"))
                .contains("'List<Long>' guarda 'Long', no 'integer' (add)"));
    }

    @Test @DisplayName("#832: un tipo que no existe entre `<` y `>` da su error, y lo que cuelga de la variable calla")
    void argumentoQueNoExiste(@TempDir Path tmp) throws IOException {
        // antes, `Sensor` pasaba a ser `Object` sin decirlo: «'Object' no tiene miembro de instancia 'valor'», dos veces
        String dijo = error(tmp, conClases("    var l: List<Sensor> := List()\n    l.add(Perro())\n"
                + "    print l.get(0).valor()\n    for s in l do\n      print s.valor()\n    next\n"));
        assertTrue(dijo.contains("tipo 'Sensor' no encontrado"), dijo);
        assertFalse(dijo.contains("no tiene miembro"), dijo);
        assertFalse(dijo.contains("llamada ambigua"), dijo);
    }

    @Test @DisplayName("#793: `Object` en la firma de la raíz no es el elemento: toString y compareTo, como siempre")
    void loDeLaRaiz(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, conClases("    var a: List<Perro> := List()\n    print a.toString() == \"\"\n"));
    }

    @Test @DisplayName("#793: en una native, lo que sale de un C<X> da el mismo error que un cast a mano")
    void enUnaNative() throws Exception {
        assertTrue(aot("var c: Caja := l.get(0)\n      return c.n").contains("lleva un cast"));
        assertEquals("", aot("return l.length()"));
    }

    @Test @DisplayName("#794: en una native, `x[i]` es la llamada, con sus mismos límites; el compuesto, no se traduce")
    void indiceEnUnaNative() throws Exception {
        assertEquals("", aot("return t[1]"));
        assertEquals(aot("t.set(1, 2)\n      return 0"), aot("t[1] := 2\n      return 0"));   // los límites de la llamada
        assertTrue(aot("var c: Caja := l[0]\n      return c.n").contains("lleva un cast"));
        assertTrue(aot("t[1] += 2\n      return 0").contains("no se traduce en una native"));
    }

    @Test @DisplayName("#794: `x[i]` de un objeto no va en un desempaquetado")
    void indiceEnUnDesempaquetado(@TempDir Path tmp) throws IOException {
        assertTrue(error(tmp, "module T\n  import Core\n  class Tab\n    public function Tab()\n    end Tab\n"
                + "    public function get(i: integer): integer\n      return i\n    end get\n"
                + "    public function set(i: integer, v: integer)\n    end set\n  end Tab\n"
                + "  function dos(): (integer, integer)\n    return (1, 2)\n  end dos\n"
                + "  function Main()\n    var t: Tab := Tab()\n    var b: integer := 0\n"
                + "    { t[0], b } := dos()\n    print b\n  end Main\nend T\n")
                .contains("no va en un desempaquetado"));
    }

    @Test @DisplayName("#794: dentro de `getPin(i)`, `this.pin[i]` es el campo: no se llama a sí mismo")
    void elCampoGanaDentro(@TempDir Path tmp) throws IOException {
        compilaBien(tmp, "module T\n  import Core\n  class R\n    var pin: integer[]\n    public function R()\n"
                + "      this.pin := integer[2]\n    end R\n    public function getPin(i: integer): integer\n"
                + "      return this.pin[i]\n    end getPin\n  end R\n  function Main()\n    var r: R := R()\n"
                + "    print r.pin[0]\n  end Main\nend T\n");
    }

    /** Como en AotCoberturaTest: lo que dice el emisor de una native con este cuerpo; "" si la traduce. */
    private static String aot(String cuerpo) throws Exception {
        String src = "module CobTest\n  import Core\n  public class Caja\n    public property n: integer\n"
                + "    public function Caja(v: integer)\n      this.n := v\n    end Caja\n  end Caja\n"
                + "  public class Tab\n    public function Tab()\n    end Tab\n"
                + "    public function get(i: integer): integer\n      return i\n    end get\n"
                + "    public function set(i: integer, v: integer)\n    end set\n  end Tab\n"
                + "  native function f(l: List<Caja>, t: Tab): integer\n      " + cuerpo + "\n  end f\n"
                + "  function Main()\n    print f(List(), Tab())\n  end Main\nend CobTest\n";
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), String.valueOf(p.getErrors()));
        SemanticAnalyzer sa = new SemanticAnalyzer();
        Main.Ctx ctx = new Main.Ctx();
        ctx.verbose = false;
        ctx.outDir = java.nio.file.Paths.get("target");
        ctx.autodiscoverFromSource(java.nio.file.Paths.get("pom.xml").toAbsolutePath());
        Main.loadImportsForAnalyzer(m, java.nio.file.Paths.get("target", "CobTest.bp"), ctx, sa, 0);
        SemanticInfo info = sa.analyze(m);
        for (SemanticDiagnostic d : info.diagnostics)
            assertNotEquals(SemanticDiagnostic.Kind.ERROR, d.kind, d.message);
        AotCEmitter em = new AotCEmitter(m.name);
        em.setSemanticInfo(info);
        try { em.emitModule(m); return ""; }
        catch (RuntimeException e) { return String.valueOf(e.getMessage()); }
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
