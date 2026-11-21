package basicplus.frontend;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

/**
 * V8 #698 (JC, paso 3) — JcLexer y JcParser: de JC, al mismo árbol que el parser de BP.
 *
 * <h3>Qué se está protegiendo</h3>
 *
 * El barrido (`--comprobar-jc`) dice EQUIVALENTE en todo lo que la primera fase escribe: el árbol de BP, escrito en JC
 * y leído con JcParser, es el mismo. Eso sólo vale si la comparación en modo JC sabe ver una diferencia: por eso la
 * primera prueba es la que TIENE que fallar (un `-` por un `+`), y la segunda, la que no tiene que fallar (lo que se
 * acordó que no cuenta: los paréntesis, la mayúscula de `MOD`, el `var` de varios nombres, el tipo de una constante,
 * el `print`). Las demás miran los nodos que el parser fabrica (las marcas del `new` y de la conversión, `v++`, el
 * `print` sin salto) y los errores con nombre de lo que la primera fase deja fuera.
 */
class JcParserTest {

    @TempDir Path tmp;

    @BeforeEach
    void stdlib() throws IOException { Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath(); }

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private static Ast.ModuleNode bp(String fuente) {
        Lexer lx = new Lexer(fuente);
        List<Token> ts = lx.tokenize();
        assertTrue(lx.getErrors().isEmpty(), "errores léxicos: " + lx.getErrors());
        Parser p = new Parser(ts);
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "errores de sintaxis: " + p.getErrors());
        return m;
    }

    private static String bpMain(String cuerpo) { return "module M\n  function main()\n" + cuerpo + "  end main\nend M\n"; }

    private static String jcMain(String cuerpo) {
        return "module M\n{\n   public function void main()\n   {\n" + cuerpo + "   }\n}\n";
    }

    /** El cuerpo de main, en JC, sin el modificador `public` (la `main` de bpMain no es pública). */
    private static String jcMainPrivada(String cuerpo) {
        return "module M\n{\n   function void main()\n   {\n" + cuerpo + "   }\n}\n";
    }

    @Test
    @DisplayName("la comparación de la ida y vuelta VE una diferencia: un - por un +")
    void distingueUnOperador() {
        Ast.ModuleNode a = bp(bpMain("    var a: integer := 1\n    var x: integer := a - 2\n"));
        Ast.ModuleNode b = JcParser.leer(jcMainPrivada("      int a = 1;\n      int x = a + 2;\n"));
        String d = ComparaAst.primeraDiferenciaJc(a, b);
        assertNotNull(d, "un - y un + tienen que salir distintos");
        assertTrue(d.contains(".op"), d);
    }

    @Test
    @DisplayName("lo que se acordó que no cuenta: paréntesis, MOD, var de varios nombres, tipo de una constante, print")
    void loQueNoCuenta() {
        Ast.ModuleNode a = bp(bpMain(
            "    var a, b: integer := 3\n" +
            "    const K := 2\n" +
            "    var c: integer := a MOD b + K\n" +
            "    print a, b; c\n"));
        Ast.ModuleNode b = JcParser.leer(jcMainPrivada(
            "      int a = 3, b = 3;\n" +
            "      const int K = 2;\n" +
            "      int c = ((a % b)) + K;\n" +
            "      println(a, \" \", b, c);\n"));
        assertNull(ComparaAst.primeraDiferenciaJc(a, b));
        // …y en el modo de BP, las mismas cosas sí cuentan.
        assertNotNull(ComparaAst.primeraDiferencia(a, b));
        // #703: un "\n" escrito no es el salto. Imprime los mismos bytes, pero no cierra la línea del hilo.
        Ast.ModuleNode c = JcParser.leer(jcMainPrivada(
            "      int a = 3, b = 3;\n" +
            "      const int K = 2;\n" +
            "      int c = ((a % b)) + K;\n" +
            "      print(a, \" \", b, c, \"\\n\");\n"));
        assertNotNull(ComparaAst.primeraDiferenciaJc(a, c));
    }

    @Test
    @DisplayName("el ejemplo de V8_IDEAS hace la ida y vuelta: BP → JC → el mismo árbol")
    void idaYVueltaDelEjemplo() throws IOException {
        Path doc = Paths.get("../../docs/V8_IDEAS.md");
        assumeTrue(Files.isRegularFile(doc), "hace falta ../../docs/V8_IDEAS.md");
        String d = new String(Files.readAllBytes(doc), StandardCharsets.UTF_8).replace("\r\n", "\n");
        int i = d.indexOf("### La primera fase, en un ejemplo");
        int ini = d.indexOf("```\nmodule Ejemplo\n  import", i) + 4;
        Path f = tmp.resolve("Ejemplo.bp");
        Files.write(f, d.substring(ini, d.indexOf("```", ini)).getBytes(StandardCharsets.UTF_8));
        assertEquals("EQUIVALENTE", EscritorJc.comprobarJcFichero(f));
    }

    @Test
    @DisplayName("los nodos que fabrica el parser: las marcas, v++, el print sin salto, el for y el else if")
    void losNodosDeJc() {
        Ast.ModuleNode m = JcParser.leer(jcMain(
            "      List l = new List();\n" +
            "      int i = (int) 2.5;\n" +
            "      i++;\n" +
            "      print(i);\n" +
            "      for (int k = 9; k >= 1; k -= 2) print(k);\n" +
            "      if (i > 0) i = 1; else if (i < 0) i = 2; else i = 3;\n" +
            "      println(i, \"x\");\n" +
            "      println();\n"));
        Ast.FuncDef f = (Ast.FuncDef) m.defs.get(0);
        Ast.CallExpr nuevo = (Ast.CallExpr) ((Ast.VarDecl) f.body.get(0)).init;
        assertTrue(nuevo.conNew && !nuevo.esConversion);
        Ast.CallExpr conv = (Ast.CallExpr) ((Ast.VarDecl) f.body.get(1)).init;
        assertTrue(conv.esConversion && !conv.conNew);
        assertEquals("integer", ((Ast.IdentifierExpr) conv.callee).name);
        Ast.AssignStmt mas = (Ast.AssignStmt) f.body.get(2);
        assertEquals(Ast.AssignOpKind.PLUS_ASSIGN, mas.op);
        assertEquals(1L, ((Ast.IntLitExpr) mas.value).value);
        Ast.PrintStmt pr = (Ast.PrintStmt) f.body.get(3);
        assertEquals(2, pr.items.size());
        assertNull(pr.items.get(1).expr, "el print de JC no salta: un ';' al final, como en BP (#700)");
        Ast.ForStmt para = (Ast.ForStmt) f.body.get(4);
        Ast.ForNumericRange r = (Ast.ForNumericRange) para.range;
        assertEquals("-", ((Ast.UnaryExpr) r.step).op);
        assertTrue(para.isSingleLine);
        Ast.IfStmt si = (Ast.IfStmt) f.body.get(5);
        assertEquals(1, si.elseIfs.size());
        assertNotNull(si.else_);
        assertTrue(!si.isSingleLine, "con else no es el if de una línea de BP");
        Ast.PrintStmt ln = (Ast.PrintStmt) f.body.get(6);
        assertEquals(2, ln.items.size());
        assertTrue(!ln.sinSalto(), "el println es el print de BP con su salto (#703)");
        assertTrue(((Ast.PrintStmt) f.body.get(7)).items.isEmpty(), "println() es el print vacío de BP: sólo el salto");
        // El import Core implícito, al final, como el parser de BP.
        assertTrue(m.imports.get(m.imports.size() - 1).implicito);
        assertTrue(m.escritoEnJc, "el análisis comprueba el new y el (T) x de un módulo de JC");
    }

    @Test
    @DisplayName("el cuerpo que lanza de una native de pack, el de BP, crea su RuntimeError con new (el análisis lo mira)")
    void nativeDePack() {
        Ast.ModuleNode m = JcParser.leer(
            "module M\n{\n   import v1 from pack \"SQLI\";\n   public native function int sql_open(string camino);\n}\n");
        Ast.FuncDef f = (Ast.FuncDef) m.defs.get(0);
        assertTrue(f.isPackExtern);
        Ast.CallExpr crea = (Ast.CallExpr) ((Ast.ThrowStmt) f.body.get(0)).value;
        assertEquals("RuntimeError", ((Ast.IdentifierExpr) crea.callee).name);
        assertTrue(crea.conNew, "en JC, crear un objeto lleva new: sin la marca, el análisis lo daría por error");
    }

    @Test
    @DisplayName("V8/H7.1 (#747): los compuestos de C# y el ternario, que es el iif(c, a, b) de BP, en los dos parsers")
    void compuestosYTernario() {
        Ast.ModuleNode m = JcParser.leer(jcMain(
            "      int x = 1;\n" +
            "      x *= 2; x /= 2; x %= 2; x &= 3; x |= 4; x ^= 5; x <<= 1; x >>= 1;\n" +
            "      int y = x > 0 ? 1 : x < 0 ? -1 : 0;\n" +
            "      bool b = x > 0 || x < -5 ? true : false;\n"));
        Ast.FuncDef f = (Ast.FuncDef) m.defs.get(0);
        Ast.AssignOpKind[] ops = { Ast.AssignOpKind.MUL_ASSIGN, Ast.AssignOpKind.DIV_ASSIGN, Ast.AssignOpKind.MOD_ASSIGN,
                Ast.AssignOpKind.AND_ASSIGN, Ast.AssignOpKind.OR_ASSIGN, Ast.AssignOpKind.XOR_ASSIGN,
                Ast.AssignOpKind.SHL_ASSIGN, Ast.AssignOpKind.SHR_ASSIGN };
        for (int i = 0; i < ops.length; i++) assertEquals(ops[i], ((Ast.AssignStmt) f.body.get(1 + i)).op);
        // El ternario, asociativo por la derecha: el de la derecha va dentro de la rama del «no».
        Ast.CallExpr t = (Ast.CallExpr) ((Ast.VarDecl) f.body.get(9)).init;
        assertTrue(t.esTernario);
        assertEquals(Ast.TERNARIO, ((Ast.IdentifierExpr) t.callee).name);
        assertTrue(((Ast.CallExpr) t.args.get(2)).esTernario);
        // Por debajo de ||: la condición es todo el `x > 0 || x < -5`.
        Ast.CallExpr t2 = (Ast.CallExpr) ((Ast.VarDecl) f.body.get(10)).init;
        assertEquals("or", ((Ast.BinaryExpr) t2.args.get(0)).op);
        // Y en BP, el mismo árbol: `x mod= 2`, `xor=`, `shl=`, `shr=` pegados, y el iif como una llamada.
        Ast.ModuleNode a = bp(bpMain(
            "    var x: integer := 1\n" +
            "    x *= 2\n    x /= 2\n    x mod= 2\n    x &= 3\n    x |= 4\n    x xor= 5\n    x shl= 1\n    x shr= 1\n" +
            "    var y: integer := iif(x > 0, 1, iif(x < 0, -1, 0))\n" +
            "    var b: boolean := iif(x > 0 or x < -5, true, false)\n"));
        Ast.ModuleNode j = JcParser.leer(jcMainPrivada(
            "      int x = 1;\n" +
            "      x *= 2; x /= 2; x %= 2; x &= 3; x |= 4; x ^= 5; x <<= 1; x >>= 1;\n" +
            "      int y = x > 0 ? 1 : x < 0 ? -1 : 0;\n" +
            "      bool b = x > 0 || x < -5 ? true : false;\n"));
        assertNull(ComparaAst.primeraDiferenciaJc(a, j));
    }

    @Test
    @DisplayName("V8/H7.2 (#749): List<Object> y Collections.Map<int, Object> en los dos parsers; el new de un Map de"
            + " llave entera es el numericMap() de BP, con la marca de cómo se escribió")
    void genericosDeLoQueYaHay() {
        Ast.ModuleNode j = JcParser.leer(
            "module M\n{\n   import Collections;\n   function void main()\n   {\n" +
            "      List<Object> l = new List<Object>();\n" +
            "      Collections.Map<int, Object> n = new Collections.Map<int, Object>();\n" +
            "      Collections.Map<string, Object> m = new Collections.Map<string, Object>();\n" +
            "      foreach (var x in l) println(x);\n" +
            "   }\n}\n");
        Ast.FuncDef f = (Ast.FuncDef) j.defs.get(0);
        Ast.VarDecl vl = (Ast.VarDecl) f.body.get(0);
        assertEquals("Object", ((Ast.SimpleTypeRef) ((Ast.SimpleTypeRef) vl.type).argumentos.get(0)).name);
        Ast.CallExpr nl = (Ast.CallExpr) vl.init;
        assertTrue(nl.conNew && nl.argumentosNew != null && nl.claseNew == null);
        Ast.CallExpr nn = (Ast.CallExpr) ((Ast.VarDecl) f.body.get(1)).init;
        assertEquals("numericMap", ((Ast.MemberAccessExpr) nn.callee).member);
        assertTrue(!nn.conNew && "Collections.Map".equals(nn.claseNew));
        Ast.ModuleNode b = bp(
            "module M\n  import Collections\n  function main()\n" +
            "    var l: List<Object> := List()\n" +
            "    var n: Collections.Map<integer, Object> := Collections.numericMap()\n" +
            "    var m: Collections.Map<string, Object> := Collections.Map()\n" +
            "    for x in l do print x\n" +
            "  end main\nend M\n");
        assertNull(ComparaAst.primeraDiferenciaJc(b, j));
    }

    @Test
    @DisplayName("V8/H8 (#793): `List<List<T>>` cierra con dos `>`, en los dos parsers, y `a >> b` sigue siendo un desplazamiento")
    void genericosAnidados() {
        Ast.ModuleNode j = JcParser.leer(
            "module M\n{\n   function void main()\n   {\n" +
            "      List<List<Object>> l = new List<List<Object>>();\n" +
            "      int a = 8 >> 1;\n" +
            "   }\n}\n");
        Ast.ModuleNode b = bp(
            "module M\n  function main()\n" +
            "    var l: List<List<Object>> := List()\n" +
            "    var a: integer := 8 shr 1\n" +
            "  end main\nend M\n");
        assertNull(ComparaAst.primeraDiferenciaJc(b, j));
        Ast.VarDecl v = (Ast.VarDecl) ((Ast.FuncDef) j.defs.get(0)).body.get(0);
        Ast.SimpleTypeRef dentro = (Ast.SimpleTypeRef) ((Ast.SimpleTypeRef) v.type).argumentos.get(0);
        assertEquals("Object", ((Ast.SimpleTypeRef) dentro.argumentos.get(0)).name);
    }

    @Test
    @DisplayName("lo que JC no tiene, o no deja escribir así, es un error con su motivo")
    void erroresConNombre() {
        error(jcMain("      string s = null;\n      string t = s ?? \"x\";\n"), "??");      // V8/H7.1 (#747)
        error(jcMain("      string s = null;\n      int n = s?.Length;\n"), "?.");
        error(jcMain("      int y = true ? 1;\n"), "ternario");
        error(jcMain("      print();\n"), "vacío");
        error(jcMain("      for (int i = 0; i < 3; i++) print(i);\n"), "<=");
        error(jcMain("      int y = int(2.5);\n"), "(int) x");
        error(jcMain("      ++y;\n"), "detrás");
        error("module M\n{\n   static int x;\n}\n", "clase");                            // static, sólo en una clase
        error("module M\n{\n   class K\n   {\n      protected int x;\n      public function void f()\n"
              + "      {\n         super.f();\n      }\n   }\n}\n", "base");                  // en JC es base
        error("module M\n{\n   class K\n   {\n      public function void f() : base()\n      {\n      }\n   }\n}\n",
              "constructor");                                                            // : base sólo en un constructor
    }

    private static void error(String jc, String motivo) {
        JcLexer.Error e = assertThrows(JcLexer.Error.class, () -> JcParser.leer(jc));
        assertTrue(e.getMessage().contains(motivo), "se esperaba un motivo con «" + motivo + "»: " + e.getMessage());
    }
}
