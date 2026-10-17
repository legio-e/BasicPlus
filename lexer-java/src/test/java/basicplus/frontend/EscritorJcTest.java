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
    @DisplayName("los arrays de tamaño fijo, como en C: el tamaño detrás del nombre; el del montón, int[]")
    void arraysFijos() throws IOException {
        String jc = jc("Fijos", conMain("Fijos",
            "    var nums: integer[10]\n" +
            "    var b: byte[4] := [1, 2, 250]\n" +
            "    var x, y: integer[3]\n" +
            "    var din: integer[] := newIntArray(2)\n" +
            "    print len(nums); len(b); len(x); len(y); len(din)\n"));
        assertTrue(jc.contains("int nums[10];"), jc);
        assertTrue(jc.contains("byte b[4] = [1, 2, 250];"), jc);
        assertTrue(jc.contains("int x[3], y[3];"), jc);
        assertTrue(jc.contains("int[] din = newIntArray(2);"), jc);
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
    @DisplayName("lo que JC no tiene lo dice, con su motivo")
    void loQueNoEscribe() throws IOException {
        noSoportado("Bucle", conMain("Bucle", "    do\n      break\n    loop\n"), "sin condición");
        noSoportado("Pa", conMain("Pa", "    var s: integer := 2\n    for i := 1 to 9 step s do print i\n"),
                    "paso no es un número");
        noSoportado("Re", conMain("Re", "    var int: integer := 1\n    print int\n"), "palabra reservada");
        noSoportado("Ba", conMain("Ba", "    var base: integer := 1\n    print base\n"), "«base»");
    }

    @Test
    @DisplayName("las clases, el switch y parallel: la forma de C# y la ida y vuelta (puntos 34 a 36)")
    void clasesSwitchYParallel() throws IOException {
        String bp =
            "module Clases\n" +
            "  public class Animal\n" +
            "    var nombre: string\n" +
            "    private var edad: integer\n" +
            "    var Animal.cuantos: integer\n" +
            "    public const Animal.MAX: integer := 10\n" +
            "    event cambia(a: Animal)\n" +
            "    public property Edad: integer\n      get\n        return this.edad\n      endget\n" +
            "      set(v)\n        this.edad := v\n      endset\n    endprop\n" +
            "    public property Nombre: string\n      get\n        return this.nombre\n      endget\n" +
            "      set(value)\n        this.nombre := value\n      endset\n    endprop\n" +
            "    public function Animal(n: string)\n      this.nombre := n\n      this.edad := 0\n" +
            "      Animal.cuantos += 1\n    end Animal\n" +
            "    public function Animal.crear(): Animal\n      return Animal(\"anon\")\n    end Animal.crear\n" +
            "    public function hablar(): string\n      return \"...\"\n    end hablar\n" +
            "    public function crece()\n      this.edad += 1\n      raise cambia(this)\n    end crece\n" +
            "    public function ~Animal()\n      Animal.cuantos -= 1\n    end ~Animal\n" +
            "  end Animal\n" +
            "  public class Perro extends Animal\n" +
            "    public function Perro(n: string)\n      super(n)\n    end Perro\n" +
            "    public function Perro()\n      Perro(\"rex\")\n    end Perro\n" +
            "    public function hablar(): string\n      return \"guau \" + super.hablar()\n    end hablar\n" +
            "    public function event alCambiar(a: Animal)\n      print \"cambia\"\n    end alCambiar\n" +
            "  end Perro\n" +
            "  function main()\n" +
            "    var owner p: Perro := Perro(\"toby\")\n" +
            "    p.cambia := p::alCambiar\n" +
            "    p.crece()\n" +
            "    switch p.Edad\n      case 1, 2\n        print \"joven\"\n      default\n        print \"otro\"\n    endsw\n" +
            "    parallel\n      case t1 := Thread(4096)\n        print \"hilo\"\n      default\n        print \"main\"\n" +
            "    endpar\n" +
            "  end main\n" +
            "end Clases\n";
        String jc = jc("Clases", bp);
        assertTrue(jc.contains("public class Perro : Animal"), jc);
        assertTrue(jc.contains("      protected string nombre;\n"), jc);     // lo que BP no marca, protegido
        assertTrue(jc.contains("      protected static int cuantos;\n"), jc);
        assertTrue(jc.contains("      public const int MAX = 10;\n"), jc);   // en C# la constante ya es de la clase
        assertTrue(jc.contains("      event cambia(Animal a);\n"), jc);
        assertTrue(jc.contains("         set(v)\n"), jc);                    // el set con nombre, como en BP
        assertTrue(jc.contains("         set\n"), jc);                       // y el de C#, cuando se llama value
        assertTrue(jc.contains("public function Perro(string n) : base(n)"), jc);
        assertTrue(jc.contains("public function Perro() : this(\"rex\")"), jc);
        assertTrue(jc.contains("return \"guau \" + base.hablar();"), jc);
        assertTrue(jc.contains("public static function Animal crear()"), jc);
        assertTrue(jc.contains("public function ~Animal()"), jc);
        assertTrue(jc.contains("p.cambia = p::alCambiar;"), jc);
        assertTrue(jc.contains("         case 1, 2:\n"), jc);
        assertTrue(jc.contains("         case t1 = new Thread(4096):\n"), jc);
        Path f = tmp.resolve("Clases.bp");
        assertEquals("EQUIVALENTE", EscritorJc.comprobarJcFichero(f));
    }
}
