package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeSet;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * QUÉ construcciones del lenguaje pasan por el AOT — medido, no recordado.
 *
 * <h3>Por qué existe</h3>
 *
 * El emisor rechaza con mensajes genéricos ({@code statement no soportado},
 * {@code expression no soportada}), así que la lista de lo que NO va sólo existe
 * si alguien la escribe a mano — y una lista escrita a mano se pudre sola. Pasó:
 * {@code AOT_LIMITES.md} nombraba <b>cinco</b> límites cuando el censo del
 * 21-ago-2026 encontró <b>veinticuatro</b>.
 *
 * <p>Este test le da la vuelta: recorre los nodos del AST con un fragmento BP
 * mínimo por cada uno, mide si el emisor lo acepta, y compara contra la foto
 * esperada. Si alguien añade soporte —o lo rompe— <b>el test lo dice</b>, y la
 * lista se actualiza a propósito en vez de quedarse rancia.
 *
 * <h3>Cómo se lee un fallo</h3>
 *
 * El mensaje trae las dos listas y su diferencia. Si has añadido soporte a un
 * nodo, mueve su nombre en {@link #SOPORTADOS} y listo — eso es el registro de
 * que la cobertura creció. Si <b>no</b> lo has tocado y aparece una diferencia,
 * es una regresión.
 */
class AotCoberturaTest {

    /** Fragmento BP mínimo que produce cada nodo, dentro de una `native`. */
    private static final Map<String, String> CASOS = new LinkedHashMap<>();
    static {
        // --- statements ---
        CASOS.put("AssignStmt",    "var t: integer := 1\n      t := 2\n      return t");
        CASOS.put("IfStmt",        "if a > 0 then\n        return 1\n      endif\n      return 0");
        CASOS.put("WhileStmt",     "var t: integer := 0\n      while t < 3 do\n        t += 1\n      endwh\n      return t");
        CASOS.put("ReturnStmt",    "return a");
        CASOS.put("VarDecl",       "var t: integer := 7\n      return t");
        CASOS.put("ForStmt",       "var t: integer := 0\n      for i := 1 to 3 do\n        t += i\n      next i\n      return t");
        CASOS.put("DoLoopStmt",    "var t: integer := 0\n      do\n        t += 1\n      loop t < 3\n      return t");
        CASOS.put("PrintStmt",     "print \"hola\"\n      return a");
        CASOS.put("SwitchStmt",    "switch a\n        case 1\n          return 10\n      endsw\n      return 0");
        CASOS.put("BreakStmt",     "while true do\n        break\n      endwh\n      return a");
        CASOS.put("ContinueStmt",  "var t: integer := 0\n      while t < 3 do\n        t += 1\n        continue\n      endwh\n      return t");
        CASOS.put("TryStmt",       "try\n        return 1\n      catch e: Exception\n        return 2\n      endtry");
        // `throw "vaya"` NO es BP: #248 solo deja lanzar instancias de Exception.
        // El fragmento anterior media eso, no el AOT.
        CASOS.put("ThrowStmt",     "throw RuntimeError(\"vaya\")\n      return a");

        // --- expresiones ---
        CASOS.put("IntLitExpr",    "return 42");
        CASOS.put("LongLitExpr",   "var l: long := 42L\n      return a");
        CASOS.put("FloatLitExpr",  "var f: float := 1.5\n      return a");
        CASOS.put("DoubleLitExpr", "var d: double := 1.5d\n      return a");
        CASOS.put("BoolLitExpr",   "var b: boolean := true\n      return a");
        CASOS.put("StringLitExpr", "var s: string := \"x\"\n      return a");
        // `string` no admite null; una REFERENCIA sí. La clase la pone el modulo del arnes.
        CASOS.put("NullLitExpr",   "var z: Caja := null\n      return a");
        CASOS.put("BinaryExpr",    "return a + 1");
        CASOS.put("UnaryExpr",     "return -a");
        CASOS.put("ParenExpr",     "return (a + 1)");
        CASOS.put("IdentifierExpr","return a");
        // `v` ya es un PARAMETRO de f: la variable duplicada tapaba la medida.
        CASOS.put("ArrayLitExpr",  "var w: integer[] := [1, 2, 3]\n      return w[0]");
        CASOS.put("IndexExpr",     "return v[0]");
        // El destructuring ASIGNA: las dos variables tienen que existir antes.
        CASOS.put("TupleExpr",     "var x: integer\n      var y: integer\n      { x, y } := dosCosas()\n      return x + y");
        // InstanceOfExpr se quedó FUERA a propósito: el fragmento que tenía era
        // `return a`, que no produce ese nodo — un caso que dice medir una cosa y
        // mide otra es peor que no tenerlo, porque cuenta como cobertura.
    }

    /**
     * La foto ESPERADA — se rellena midiendo, no de memoria. Cambiarla es el
     * gesto que registra que la cobertura del AOT se movió.
     */
    private static final Set<String> SOPORTADOS = new TreeSet<>(java.util.Arrays.asList(
        // Medido el 24-ago-2026: 24 de 27. ThrowStmt entra SIN tocar el emisor
        // — ya estaba soportado desde #186/#213 y lo tapaba un fragmento mal
        // escrito (`throw "vaya"`, que #248 no permite). Los otros cuatro casos
        // estaban igual de mal; al arreglarlos la foto pasó de "23 medidos" a
        // "27 medidos de verdad".
        "AssignStmt", "BinaryExpr", "BoolLitExpr", "BreakStmt", "ContinueStmt",
        "DoLoopStmt", "DoubleLitExpr", "FloatLitExpr", "ForStmt", "IdentifierExpr",
        "IfStmt", "IndexExpr", "IntLitExpr", "LongLitExpr", "NullLitExpr",
        "ParenExpr", "PrintStmt", "ReturnStmt", "StringLitExpr", "SwitchStmt",
        "ThrowStmt", "UnaryExpr", "VarDecl", "WhileStmt"
    ));

    /** El motivo del ultimo rechazo, para que el informe no culpe al nodo
     *  equivocado: un fragmento mal escrito se rechaza igual que uno no
     *  soportado, y sin el motivo no se distinguen. */
    private static String ultimoMotivo = "";

    /** ¿El emisor traga este cuerpo dentro de una `native`? */
    private static boolean pasa(String cuerpo) {
        String src =
              "module CobTest\n"
            // `import Core` explicito: sin el no hay Exception ni RuntimeError,
            // y los casos de excepciones median "no compila" en vez de medir el AOT.
            + "  import Core\n"
            // Una clase, para poder escribir una referencia (el caso de `null`).
            + "  class Caja\n"
            + "    public property n: integer\n"
            + "  end Caja\n"
            // Una funcion auxiliar que devuelve dos valores, para el caso de la
            // tupla; y `v: integer[]` como parametro, para el del indice. Asi
            // ninguno de los dos depende de poder CREAR el array dentro de la
            // native, que es otra pregunta distinta.
            + "  function dosCosas(): (integer, integer)\n"
            + "    return (1, 2)\n"
            + "  end dosCosas\n"
            + "  native function f(a: integer, v: integer[]): integer\n"
            + "      " + cuerpo + "\n"
            + "  end f\n"
            + "  function Main()\n"
            + "    print f(1, newIntArray(3))\n"
            + "  end Main\n"
            + "end CobTest\n";
        try {
            Parser p = new Parser(new Lexer(src).tokenize());
            Ast.ModuleNode m = p.parseModule();
            if (!p.getErrors().isEmpty() || m == null) {
                ultimoMotivo = "NO PARSEA (el fragmento del test esta mal): " + p.getErrors();
                return false;
            }
            // Los imports se resuelven con el MISMO codigo que el compilador
            // (Main.loadImportsForAnalyzer contra la stdlib de este checkout).
            // Reimplementarlo aqui seria un doble, y un doble mas amable que el
            // original convierte el censo en adorno.
            SemanticAnalyzer sa = new SemanticAnalyzer();
            Main.Ctx ctx = new Main.Ctx();
            ctx.verbose = false;
            ctx.outDir  = java.nio.file.Paths.get("target");
            // El cwd del test es lexer-java/, donde no hay BpVM.cfg: hay que
            // decirle desde donde caminar hacia arriba para encontrarlo (y con
            // el, el stdlibDir donde vive Core.mod).
            ctx.autodiscoverFromSource(java.nio.file.Paths.get("pom.xml").toAbsolutePath());
            Main.loadImportsForAnalyzer(m, java.nio.file.Paths.get("target", "CobTest.bp"),
                                        ctx, sa, 0);
            SemanticInfo info = sa.analyze(m);
            // Un fragmento que no pasa el analisis semantico NO mide el AOT:
            // mide que esta mal escrito. Sin este filtro se cuenta como "no
            // soportado" y la foto miente hacia abajo — pasó siete veces
            // (23 y 24-ago-2026), y `throw \"vaya\"` fue la ultima: #248 solo
            // deja lanzar Exception, asi que el caso no llegaba a preguntarle
            // nada al emisor.
            StringBuilder errs = new StringBuilder();
            for (SemanticDiagnostic d : info.diagnostics)
                if (d.kind == SemanticDiagnostic.Kind.ERROR)
                    errs.append(errs.length() == 0 ? "" : " | ").append(d.message);
            if (errs.length() > 0) {
                ultimoMotivo = "NO COMPILA (el fragmento del test esta mal): " + errs;
                return false;
            }
            AotCEmitter em = new AotCEmitter(m.name);
            em.setSemanticInfo(info);
            return !em.emitModule(m).isEmpty();
        } catch (Throwable t) {
            ultimoMotivo = String.valueOf(t.getMessage());
            return false;
        }
    }

    @Test
    @DisplayName("la cobertura del AOT es la que dice la foto — ni más ni menos")
    void coberturaMedida() {
        Set<String> ok = new TreeSet<>();
        Set<String> no = new TreeSet<>();
        Map<String, String> motivos = new LinkedHashMap<>();
        for (Map.Entry<String, String> e : CASOS.entrySet()) {
            if (pasa(e.getValue())) ok.add(e.getKey());
            else { no.add(e.getKey()); motivos.put(e.getKey(), ultimoMotivo); }
        }
        // El informe va SIEMPRE, pase o falle: es la lista que sustituye a la
        // escrita a mano, y sirve para decidir qué ampliar a continuación.
        System.out.println("-- cobertura AOT: " + ok.size() + " de " + CASOS.size() + " --");
        System.out.println("   PASAN:    " + ok);
        System.out.println("   RECHAZAN: " + no);
        for (Map.Entry<String, String> m : motivos.entrySet()) {
            String r = String.valueOf(m.getValue());
            System.out.println("     - " + m.getKey() + ": " + (r.length() > 90 ? r.substring(0, 90) : r));
        }
        assertEquals(SOPORTADOS, ok,
                "la cobertura del AOT cambió. Si has AÑADIDO soporte, mueve el nodo a"
                + " SOPORTADOS (eso es el registro). Si no has tocado el emisor, es una"
                + " regresión.\n   miden: " + ok + "\n   esperados: " + SOPORTADOS);
    }
}
