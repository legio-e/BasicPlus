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
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #546 (V7) — qué admite un switch como sujeto (Eduardo, 24-sep): un entero (integer, long y los estrechos),
 * un boolean, un enum o un string; un real o una referencia, error de compilación con nombre. Y un `break`
 * dentro de un case sale del SWITCH: sin bucle alrededor también compila (antes el emisor fallaba sin
 * línea). Lo que el switch HACE con cada sujeto se prueba ejecutando, en las dos VMs:
 * {@code samples/SwitchTipos.bp} en compat.sh (que pone en rojo una línea FALLO) y, dentro de una native,
 * {@code make test-aotswitch}.
 */
class SwitchSujetoTest {

    /** Los errores de un Main con este cuerpo, como «línea: mensaje». */
    private static List<String> errores(String cuerpo) {
        String src = "module T\n  function Main()\n" + cuerpo + "  end Main\nend T\n";
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "el fuente del test no parsea: " + p.getErrors() + "\n" + src);
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        List<String> r = new ArrayList<>();
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) r.add(d.line + ": " + d.message);
        return r;
    }

    @Test @DisplayName("string, long, integer (con un case long), boolean y un entero estrecho: valen")
    void validos() {
        String[] casos = {
            "    var s: string := \"a\"\n    switch s\n      case \"a\", \"b\"\n        print 1\n    endsw\n",
            "    var l: long := 5L\n    switch l\n      case 5000000000L\n        print 1\n      case 7\n        print 2\n    endsw\n",
            "    var i: integer := 1\n    switch i\n      case 1, 2\n        print 1\n      case 5000000000L\n        print 2\n    endsw\n",
            "    var b: boolean := true\n    switch b\n      case true\n        print 1\n    endsw\n",
            "    var w: byte := 3\n    switch w\n      case 3\n        print 1\n    endsw\n",
        };
        for (String c : casos) {
            List<String> e = errores(c);
            assertTrue(e.isEmpty(), "no debía dar error: " + e + "\n" + c);
        }
    }

    @Test @DisplayName("un real como sujeto, o un case real contra un sujeto entero: error con nombre")
    void real() {
        List<String> e = errores("    var d: double := 2.5d\n    switch d\n      case 2.5d\n        print 1\n    endsw\n");
        assertEquals(1, e.size(), e.toString());
        assertTrue(e.get(0).startsWith("4: switch sobre 'double': un real no se compara por igualdad exacta"), e.get(0));

        e = errores("    var f: float := 1.5\n    switch f\n      case 1.5\n        print 1\n    endsw\n");
        assertEquals(1, e.size(), e.toString());
        assertTrue(e.get(0).startsWith("4: switch sobre 'float'"), e.get(0));

        e = errores("    var i: integer := 2\n    switch i\n      case 2.5\n        print 1\n    endsw\n");
        assertEquals(1, e.size(), e.toString());
        assertTrue(e.get(0).startsWith("5: valor de 'case' de tipo 'float': un real no se compara por igualdad exacta"), e.get(0));
    }

    @Test @DisplayName("una referencia como sujeto: error con nombre (para comparar referencias, if)")
    void referencia() {
        List<String> e = errores("    var a: integer[] := null\n    switch a\n      case null\n        print 1\n    endsw\n");
        assertEquals(1, e.size(), e.toString());
        assertTrue(e.get(0).startsWith("4: switch sobre 'integer[]'") && e.get(0).endsWith("para comparar referencias, usa if"), e.get(0));
    }

    @Test @DisplayName("un break dentro de un case, sin bucle alrededor, compila (antes el emisor fallaba sin línea)")
    void breakSinBucle(@TempDir Path tmp) throws IOException {
        String f = "module T\n  function Main()\n    var k: integer := 2\n    switch k\n      case 2\n        print 2\n"
                 + "        break\n    endsw\n    print 3\n  end Main\nend T\n";
        String salida = compilaConCore(tmp, "T", f);
        assertFalse(salida.contains("errores del emisor"), salida);
        assertTrue(Files.isRegularFile(tmp.resolve("out").resolve("T.mod")), salida);
    }

    /** El build de verdad (el emisor también), con ../../bpstdlib de dependencia; devuelve la salida. */
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
}
