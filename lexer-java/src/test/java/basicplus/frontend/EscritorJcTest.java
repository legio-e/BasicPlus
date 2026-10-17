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

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

/**
 * V8 #698 (JC, paso 2) — EscritorJc: AST → JC, desde el árbol analizado.
 *
 * <h3>Qué se está protegiendo</h3>
 *
 * La primera prueba es la del ejemplo de V8_IDEAS §E2 («La primera fase, en un ejemplo»). Su JC se escribió A MANO,
 * con lo que decidió Eduardo, ANTES que el escritor: la prueba lee del documento el BP y el JC, y exige que el
 * escritor dé ese JC, carácter a carácter. Si cambia el ejemplo o el escritor, salta aquí. Las demás miran en pequeño
 * lo que el ejemplo no enseña: la precedencia de C# donde la de BP no casa, crear, llamar y convertir, las formas del
 * `for` y del `if`, el `print`, y lo que la primera fase deja fuera, que tiene que decirlo con su motivo.
 *
 * <p>El escritor necesita el análisis, y el análisis, la stdlib (Core): como en las pruebas del AOT,
 * {@link Main#STDLIB_DIR} apunta a la de este checkout.
 */
class EscritorJcTest {

    @TempDir Path tmp;

    @BeforeEach
    void stdlib() throws IOException { Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath(); }

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private EscritorJc.Resultado traduce(String modulo, String bp) throws IOException {
        Path f = tmp.resolve(modulo + ".bp");
        Files.write(f, bp.getBytes(StandardCharsets.UTF_8));
        return EscritorJc.traducir(f);
    }

    private String jc(String modulo, String bp) throws IOException {
        EscritorJc.Resultado r = traduce(modulo, bp);
        assertEquals("ESCRITO", r.clase, r.detalle);
        return r.jc;
    }

    /** Un módulo con una función `main` de ese cuerpo, en BP. */
    private static String conMain(String modulo, String cuerpo) {
        return "module " + modulo + "\n  function main()\n" + cuerpo + "  end main\nend " + modulo + "\n";
    }

    private void noSoportado(String modulo, String bp, String motivo) throws IOException {
        EscritorJc.Resultado r = traduce(modulo, bp);
        assertEquals("NO SOPORTADO", r.clase, r.clase + ": " + r.detalle + "\n" + r.jc);
        assertTrue(r.motivo.contains(motivo), "se esperaba un motivo con «" + motivo + "»: " + r.motivo);
    }

    /** El bloque de código que empieza en la marca, a partir de {@code desde}. */
    private static String bloque(String doc, int desde, String marca) {
        int ini = doc.indexOf(marca, desde);
        assertTrue(ini >= 0, "no está en V8_IDEAS: " + marca);
        ini += 4;   // el ``` y su salto
        return doc.substring(ini, doc.indexOf("```", ini));
    }

    @Test
    @DisplayName("el ejemplo de V8_IDEAS: el JC escrito a mano es el que sale")
    void ejemploDelDocumento() throws IOException {
        Path doc = Paths.get("../docs/V8_IDEAS.md");
        assumeTrue(Files.isRegularFile(doc), "hace falta ../docs/V8_IDEAS.md");
        String d = new String(Files.readAllBytes(doc), StandardCharsets.UTF_8).replace("\r\n", "\n");
        int i = d.indexOf("### La primera fase, en un ejemplo");
        assertTrue(i >= 0, "el ejemplo ya no está en V8_IDEAS");
        String bp = bloque(d, i, "```\nmodule Ejemplo\n  import");
        String esperado = bloque(d, i, "```\nmodule Ejemplo\n{");
        assertEquals(esperado, jc("Ejemplo", bp));
    }

    @Test
    @DisplayName("la precedencia de C#: los paréntesis que pide, y sólo ésos")
    void precedencia() throws IOException {
        String jc = jc("Prec", conMain("Prec",
            "    var a: integer := 1\n    var b: integer := 2\n    var c: integer := 3\n" +
            "    var x: integer := a shl 1 + 2\n" +
            "    var y: boolean := a & b == c\n" +
            "    var z: boolean := not a == b\n" +
            "    var w: integer := a | b + c\n" +
            "    var v: integer := a + b * c\n" +
            "    var p: float := -2.0 ^ 2\n" +
            "    var q: integer := 2 ^ 3 ^ 2\n" +
            "    var r: integer := (a + b) * c\n" +
            "    print x; y; z; w; v; p; q; r\n"));
        assertTrue(jc.contains("int x = (a << 1) + 2;"), jc);    // en BP, shl va con *: (a shl 1) + 2
        assertTrue(jc.contains("bool y = (a & b) == c;"), jc);   // y & también
        assertTrue(jc.contains("bool z = !(a == b);"), jc);      // el not de BP va por debajo de la comparación
        assertTrue(jc.contains("int w = (a | b) + c;"), jc);     // | va con +
        assertTrue(jc.contains("int v = a + b * c;"), jc);       // igual en los dos: ninguno
        assertTrue(jc.contains("float p = -2.0f ** 2;"), jc);    // la potencia, más fuerte que el menos
        assertTrue(jc.contains("int q = 2 ** 3 ** 2;"), jc);     // y asociativa por la derecha
        assertTrue(jc.contains("int r = (a + b) * c;"), jc);     // los del programador, tal cual
    }

    @Test
    @DisplayName("crear, llamar y convertir: new, la llamada y (T) x, según el análisis")
    void crearLlamarConvertir() throws IOException {
        String jc = jc("Conv",
            "module Conv\n" +
            "  function doble(x: integer): integer\n    return x * 2\n  end doble\n" +
            "  function main()\n" +
            "    var d: double := 2.5d\n" +
            "    var i: integer := integer(d)\n" +
            "    var b: byte := byte(i + 1)\n" +
            "    var o: Object := List()\n" +
            "    var l: List := List(o)\n" +
            "    var n: integer := doble(i)\n" +
            "    print i; b; n; l.length()\n" +
            "  end main\n" +
            "end Conv\n");
        assertTrue(jc.contains("int i = (int) d;"), jc);
        assertTrue(jc.contains("byte b = (byte) (i + 1);"), jc);
        assertTrue(jc.contains("Object o = new List();"), jc);    // construye
        assertTrue(jc.contains("List l = (List) o;"), jc);        // convierte: el mismo `List(…)` en BP
        assertTrue(jc.contains("int n = doble(i);"), jc);         // llama
    }

    @Test
    @DisplayName("el for de BP con la ropa de C: las cuatro formas, y el límite con su precedencia")
    void formasDelFor() throws IOException {
        String jc = jc("Fo", conMain("Fo",
            "    for i := 1 to 3 do print i\n" +
            "    for j := 1 to 9 step 2 do print j\n" +
            "    for k := 3 to 1 step -1 do print k\n" +
            "    for m := 9 to 1 step -2 do print m\n" +
            "    for n := 0 to 7 shr 1 do print n\n"));
        assertTrue(jc.contains("for (int i = 1; i <= 3; i++) print(i, \"\\n\");"), jc);
        assertTrue(jc.contains("for (int j = 1; j <= 9; j += 2) print(j, \"\\n\");"), jc);
        assertTrue(jc.contains("for (int k = 3; k >= 1; k--) print(k, \"\\n\");"), jc);
        assertTrue(jc.contains("for (int m = 9; m >= 1; m -= 2) print(m, \"\\n\");"), jc);
        assertTrue(jc.contains("for (int n = 0; n <= 7 >> 1; n++) print(n, \"\\n\");"), jc);
    }

    @Test
    @DisplayName("el if: el elseif de BP es el else if, y un if dentro del else va entre llaves")
    void elseIfYElse() throws IOException {
        // El `print 4` tras el if de una línea no sobra: si éste fuera lo último antes del `endif`, BP no lo leería
        // (#701, el parser toma ese `endif` por suyo).
        String jc = jc("Si", conMain("Si",
            "    var a: integer := 1\n" +
            "    if a > 0 then\n      print 1\n    elseif a < 0 then\n      print 2\n" +
            "    else\n      if a == 0 then print 3\n      print 4\n    endif\n"));
        String esperado =
            "      if (a > 0)\n      {\n         print(1, \"\\n\");\n      }\n" +
            "      else if (a < 0)\n      {\n         print(2, \"\\n\");\n      }\n" +
            "      else\n      {\n         if (a == 0) print(3, \"\\n\");\n         print(4, \"\\n\");\n      }\n";
        assertTrue(jc.contains(esperado), jc);
    }

    @Test
    @DisplayName("el print: la coma de BP es un \" \", y el salto que BP pone solo, un \"\\n\"")
    void elPrint() throws IOException {
        String jc = jc("Pr", conMain("Pr", "    var a: integer := 1\n    print a, a; \"x\"\n    print\n"));
        assertTrue(jc.contains("print(a, \" \", a, \"x\", \"\\n\");"), jc);
        assertTrue(jc.contains("print(\"\\n\");"), jc);
    }

    @Test
    @DisplayName("lo que la primera fase deja fuera lo dice, con su motivo")
    void loQueNoEscribe() throws IOException {
        noSoportado("C1", "module C1\n  class K\n    var x: integer\n  end K\n" +
                          "  function main()\n    print 1\n  end main\nend C1\n", "una clase");
        noSoportado("Sw", conMain("Sw", "    var x: integer := 1\n    switch x\n      case 1\n        print 1\n    endsw\n"),
                    "switch");
        noSoportado("Bucle", conMain("Bucle", "    do\n      break\n    loop\n"), "sin condición");
        noSoportado("Pa", conMain("Pa", "    var s: integer := 2\n    for i := 1 to 9 step s do print i\n"),
                    "paso no es un número");
        noSoportado("Re", conMain("Re", "    var int: integer := 1\n    print int\n"), "palabra reservada");
    }
}
