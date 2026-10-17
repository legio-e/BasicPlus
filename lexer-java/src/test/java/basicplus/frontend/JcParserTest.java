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
            "      print(a, \" \", b, c, \"\\n\");\n"));
        assertNull(ComparaAst.primeraDiferenciaJc(a, b));
        // …y en el modo de BP, las mismas cosas sí cuentan.
        assertNotNull(ComparaAst.primeraDiferencia(a, b));
    }

    @Test
    @DisplayName("el ejemplo de V8_IDEAS hace la ida y vuelta: BP → JC → el mismo árbol")
    void idaYVueltaDelEjemplo() throws IOException {
        Path doc = Paths.get("../docs/V8_IDEAS.md");
        assumeTrue(Files.isRegularFile(doc), "hace falta ../docs/V8_IDEAS.md");
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
            "      if (i > 0) i = 1; else if (i < 0) i = 2; else i = 3;\n"));
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
        // El import Core implícito, al final, como el parser de BP.
        assertTrue(m.imports.get(m.imports.size() - 1).implicito);
    }

    @Test
    @DisplayName("lo que la primera fase deja fuera, o JC no tiene, es un error con su motivo")
    void erroresConNombre() {
        error(jcMain("      int x = 1;\n      x *= 2;\n"), "compuestos");
        error(jcMain("      print();\n"), "vacío");
        error(jcMain("      for (int i = 0; i < 3; i++) print(i);\n"), "<=");
        error(jcMain("      switch (1) { }\n"), "segunda fase");
        error("module M\n{\n   class K { }\n}\n", "segunda fase");
        error(jcMain("      int y = int(2.5);\n"), "(int) x");
        error(jcMain("      ++y;\n"), "detrás");
    }

    private static void error(String jc, String motivo) {
        JcLexer.Error e = assertThrows(JcLexer.Error.class, () -> JcParser.leer(jc));
        assertTrue(e.getMessage().contains(motivo), "se esperaba un motivo con «" + motivo + "»: " + e.getMessage());
    }
}
