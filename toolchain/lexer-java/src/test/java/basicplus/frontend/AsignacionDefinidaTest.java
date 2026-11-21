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
import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #540 (V7/H1) — la asignación definida de las locales: leer una local sin inicializador por un camino en
 * el que no ha recibido valor es un ERROR de compilación (Eduardo, 24-sep). Un caso por regla de camino
 * (`docs/V7_IDEAS.md` §H1.7), y los que NO deben dar error, que son la mitad de la prueba: un análisis que
 * grita donde no toca rompe programas buenos.
 */
class AsignacionDefinidaTest {

    /** Los errores de «sin valor» de un Main con este cuerpo (y lo que se ponga antes en el módulo). */
    private static List<String> errores(String cuerpo, String antes) {
        String src = "module T\n" + antes + "  function Main()\n" + cuerpo + "  end Main\nend T\n";
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "el fuente del test no parsea: " + p.getErrors() + "\n" + src);
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        List<String> r = new ArrayList<>();
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) {
                assertTrue(d.message.contains("se usa sin tener valor"), "otro error, el fuente del test esta mal: " + d.message + "\n" + src);
                r.add(d.line + ":" + d.message.substring(d.message.indexOf('\''), d.message.indexOf('\'', d.message.indexOf('\'') + 1) + 1));
            }
        return r;
    }

    private static List<String> errores(String cuerpo) { return errores(cuerpo, ""); }

    /** Para lo que necesita Core (clases, excepciones): el build de verdad, con ../../bpstdlib de dependencia.
     *  Devuelve la salida del compilador. */
    private static String compilaConCore(Path tmp, String modulo, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(modulo + ".bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = modulo;
        File std = new File("../../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        try { System.setOut(ps); System.setErr(ps); Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new String(buf.toByteArray(), StandardCharsets.UTF_8);
    }

    /** Las de «sin valor» de una salida del compilador, como «linea:'var'» (la línea sale como [l:c]). */
    private static List<String> sinValorEn(String salida) {
        List<String> r = new ArrayList<>();
        for (String l : salida.split("\n")) {
            if (!l.contains("se usa sin tener valor")) continue;
            int a = l.indexOf('['), c = l.indexOf(':', a), q = l.indexOf('\''), q2 = l.indexOf('\'', q + 1);
            r.add(l.substring(a + 1, c) + ":" + l.substring(q, q2 + 1));
        }
        return r;
    }

    private static void sinError(String cuerpo) {
        List<String> e = errores(cuerpo);
        assertTrue(e.isEmpty(), "no debía dar error: " + e + "\n" + cuerpo);
    }

    /** Un error, en esa línea del fuente (Main empieza en la 3), por esa variable. */
    private static void unError(String cuerpo, int linea, String var) {
        List<String> e = errores(cuerpo);
        assertEquals(java.util.Collections.singletonList(linea + ":'" + var + "'"), e, cuerpo);
    }

    @Test @DisplayName("leer antes de asignar es error; asignar y leer, no")
    void secuencia() {
        unError("    var x: integer\n    print x\n", 4, "x");
        sinError("    var x: integer\n    x := 1\n    print x\n");
        sinError("    var x: integer := 3\n    print x\n");
    }

    @Test @DisplayName("un error por variable, aunque se lea más veces")
    void unaVez() {
        unError("    var x: integer\n    print x\n    print x + 1\n", 4, "x");
    }

    @Test @DisplayName("var x, y: sólo la que no tiene valor")
    void variosNombres() {
        unError("    var x, y: integer\n    x := 1\n    print x\n    print y\n", 6, "y");
    }

    @Test @DisplayName("un array fijo se pone a cero al declararlo: no se vigila")
    void arrayFijo() {
        sinError("    var b: byte[4]\n    print b[0]\n");
    }

    @Test @DisplayName("x += 1 y a[0] := 1 LEEN la variable")
    void compuestaYElemento() {
        unError("    var x: integer\n    x += 1\n", 4, "x");
        unError("    var a: integer[]\n    a[0] := 1\n", 4, "a");
    }

    @Test @DisplayName("if: con valor si lo recibe en TODAS las ramas; sin else, la rama vacía cuenta")
    void si() {
        sinError("    var x: integer\n    if 1 < 2 then\n      x := 1\n    elseif 2 < 3 then\n      x := 2\n    else\n      x := 3\n    endif\n    print x\n");
        unError("    var x: integer\n    if 1 < 2 then\n      x := 1\n    endif\n    print x\n", 7, "x");
        unError("    var x: integer\n    if 1 < 2 then\n      x := 1\n    elseif 2 < 3 then\n      print 0\n    else\n      x := 3\n    endif\n    print x\n", 11, "x");
    }

    @Test @DisplayName("una rama que no sigue (return, throw) no cuenta en el cruce")
    void ramaQueNoSigue() {
        sinError("    var x: integer\n    if 1 < 2 then\n      x := 1\n    else\n      return\n    endif\n    print x\n");
    }

    @Test @DisplayName("while y for: el cuerpo puede no ejecutarse")
    void bucles() {
        unError("    var x: integer\n    var i: integer := 0\n    while i < 3 do\n      x := i\n      i := i + 1\n    endwh\n    print x\n", 9, "x");
        unError("    var x: integer\n    for i := 1 to 3 do\n      x := i\n    next i\n    print x\n", 7, "x");
        sinError("    var i: integer := 0\n    while i < 3 do\n      var x: integer\n      x := i\n      print x\n      i := i + 1\n    endwh\n");
    }

    @Test @DisplayName("while true: se sale por sus break")
    void whileTrue() {
        sinError("    var x: integer\n    while true do\n      x := 1\n      break\n    endwh\n    print x\n");
    }

    @Test @DisplayName("do … loop: el cuerpo corre al menos una vez; un break antes de asignar, no")
    void hazMientras() {
        sinError("    var x: integer\n    var k: integer := 3\n    do\n      x := k\n      k := k - 1\n    loop k > 0\n    print x\n");
        unError("    var x: integer\n    var k: integer := 3\n    do\n      if k == 2 then\n        break\n      endif\n      x := k\n      k := k - 1\n    loop k > 0\n    print x\n", 12, "x");
    }

    @Test @DisplayName("declarar en el cuerpo de un bucle lo deja sin valor en cada vuelta")
    void declaraEnBucle() {
        unError("    var i: integer := 0\n    while i < 2 do\n      var x: integer\n      if i == 1 then\n        print x\n      endif\n      x := 5\n      i := i + 1\n    endwh\n", 7, "x");
    }

    @Test @DisplayName("switch: todos los case y el default; sin default, o con un break antes, no")
    void segun() {
        sinError("    var x: integer\n    switch 2\n      case 1\n        x := 1\n      case 2\n        x := 2\n      default\n        x := 3\n    endsw\n    print x\n");
        unError("    var x: integer\n    switch 2\n      case 1\n        x := 1\n      case 2\n        x := 2\n    endsw\n    print x\n", 10, "x");
        unError("    var x: integer\n    switch 2\n      case 1\n        break\n      default\n        x := 3\n    endsw\n    print x\n", 10, "x");
    }

    @Test @DisplayName("try: cada catch parte de ANTES del cuerpo; lo que el finally asigna, se tiene")
    void intento(@TempDir Path tmp) throws IOException {
        String f = "module T\n  function Main()\n"
                 + "    var x: integer\n    try\n      x := 1\n    catch e1: Core.Exception\n      print 0\n    endtry\n    print x\n"
                 + "    var y: integer\n    try\n      y := 1\n    catch e2: Core.Exception\n      y := 2\n    endtry\n    print y\n"
                 + "    var z: integer\n    try\n      print 1\n    finally\n      z := 5\n    endtry\n    print z\n"
                 + "  end Main\nend T\n";
        assertEquals(java.util.Collections.singletonList("9:'x'"), sinValorEn(compilaConCore(tmp, "T", f)));
    }

    @Test @DisplayName("el get y el set de una property también se revisan, y los métodos")
    void property(@TempDir Path tmp) throws IOException {
        String f = "module T\n  public class Caja\n    public property n: integer\n      get\n        var r: integer\n        return r\n      endget\n    endprop\n"
                 + "    public function Caja()\n    end Caja\n"
                 + "    public function m(): integer\n      var q: integer\n      return q\n    end m\n  end Caja\n"
                 + "  function Main()\n    print Caja().n\n  end Main\nend T\n";
        List<String> e = sinValorEn(compilaConCore(tmp, "T", f));
        assertEquals(2, e.size(), e.toString());
        assertTrue(e.contains("6:'r'") && e.contains("13:'q'"), e.toString());
    }
}
