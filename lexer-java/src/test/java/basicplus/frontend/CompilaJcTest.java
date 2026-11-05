package basicplus.frontend;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.OutputSink;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.regex.Pattern;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8 #698 (18-oct) — el compilador lee un `.jc`: el árbol sale de JcParser, y lo demás es lo de un `.bp` (el análisis,
 * el emisor, el `.dbg`, que apunta al `.jc` con sus líneas). Un proyecto encuentra su main en `<main>.jc` si no hay
 * `<main>.bp`; con los dos, no adivina: lo dice. Un error de JC sale como los de BP, léxico o sintáctico, con su línea
 * y su columna, y el build falla.
 */
class CompilaJcTest {

    private static final String HOLA = String.join("\n",
        "module HolaJc",
        "{",
        "   public class Contador",
        "   {",
        "      protected int n;",
        "      public function Contador(int inicio)",
        "      {",
        "         this.n = inicio;",
        "      }",
        "      public function int siguiente()",
        "      {",
        "         this.n += 1;",
        "         return this.n;",
        "      }",
        "   }",
        "",
        "   public function void main()",
        "   {",
        "      Contador c = new Contador(10);",
        "      for (int i = 1; i <= 3; i++)",
        "      {",
        "         println(\"vuelta \", i, \": \", c.siguiente());",   // la línea 22
        "      }",
        "      println(\"fin\");",                                    // la 24
        "   }",
        "}", "");

    /** Escribe los fuentes (nombre, texto, nombre, texto…) en `tmp/src` y construye el proyecto con su main.
     *  Devuelve {ok, lo que dijo el compilador, el directorio de salida}. */
    private static Object[] compila(Path tmp, String main, String... ficheros) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (int i = 0; i < ficheros.length; i += 2)
            Files.write(src.resolve(ficheros[i]), ficheros[i + 1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = main;
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        String antes = Main.STDLIB_DIR;
        boolean ok;
        try {
            Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
            System.setOut(ps);
            System.setErr(ps);
            ok = Main.buildProject(b, "mivm", false);
        } finally {
            System.setOut(out0);
            System.setErr(err0);
            Main.STDLIB_DIR = antes;
        }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8), out };
    }

    /** Corre `main` (compilado en `out`) en la miVM y devuelve lo que imprime el programa, sin más. */
    private static String corre(Path out, String main) throws Exception {
        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        mm.setStdlibDir("../bpstdlib");          // el CWD de la prueba es lexer-java/
        vm.setModuleManager(mm);
        StringBuilder salida = new StringBuilder();
        vm.setProgramOut(new OutputSink() {
            @Override public void writeText(String s) { salida.append(s); }
            @Override public void writeChar(char c) { salida.append(c); }
            @Override public void newline() { salida.append('\n'); }
            @Override public void writeLine(String s) { salida.append(s).append('\n'); }   // #600
            @Override public void flush() { }
        });
        mm.executeRootModule(out.resolve(main + ".mod").toString(), main);
        vm.run();
        return salida.toString();
    }

    @Test
    @Timeout(120)
    @DisplayName("un proyecto con el main en JC compila y corre, y su .dbg apunta al .jc con sus líneas")
    void compilaYCorre(@TempDir Path tmp) throws Exception {
        Object[] r = compila(tmp, "HolaJc", "HolaJc.jc", HOLA);
        assertTrue((Boolean) r[0], (String) r[1]);
        Path out = (Path) r[2];
        assertEquals("vuelta 1: 11\nvuelta 2: 12\nvuelta 3: 13\nfin\n", corre(out, "HolaJc"));
        String dbg = new String(Files.readAllBytes(out.resolve("HolaJc.dbg")), StandardCharsets.UTF_8);
        assertTrue(Pattern.compile("(?m)^source .*HolaJc\\.jc$").matcher(dbg).find(), dbg);
        for (int linea : new int[]{8, 12, 13, 19, 22, 24})
            assertTrue(Pattern.compile("(?m)^\\d+ " + linea + "$").matcher(dbg).find(),
                    "el .dbg tenía que llevar la línea " + linea + " del .jc:\n" + dbg);
    }

    @Test
    @DisplayName("un error de JC sale como los de BP, léxico o sintáctico, con su línea y su columna, y el build falla")
    void errores(@TempDir Path tmp) throws Exception {
        Object[] lex = compila(tmp.resolve("lexico"), "Malo", "Malo.jc",
                "module Malo\n{\n   string s = \"sin cerrar;\n}\n");
        assertFalse((Boolean) lex[0], (String) lex[1]);
        assertTrue(((String) lex[1]).contains("[3:15] error léxico: literal de cadena sin cerrar"), (String) lex[1]);

        Object[] sin = compila(tmp.resolve("sintactico"), "Malo", "Malo.jc",
                "module Malo\n{\n   public function void main()\n   {\n      int x = ;\n   }\n}\n");
        assertFalse((Boolean) sin[0], (String) sin[1]);
        assertTrue(((String) sin[1]).contains("[5:15] error sintáctico: "), (String) sin[1]);
    }

    /** Dos clases (una hija con su `: base` y su `: this`), una con un constructor de un entero, y una función; en
     *  `main`, lo que ponga cada caso. */
    private static String marcas(String cuerpo) {
        return String.join("\n",
            "module Marcas",
            "{",
            "   public class Animal",
            "   {",
            "      protected string nombre;",
            "      public function Animal(string n)",
            "      {",
            "         this.nombre = n;",
            "      }",
            "      public function string quien()",
            "      {",
            "         return this.nombre;",
            "      }",
            "   }",
            "   public class Perro : Animal",
            "   {",
            "      public function Perro(string n) : base(n)",
            "      {",
            "      }",
            "      public function Perro() : this(\"rex\")",
            "      {",
            "      }",
            "   }",
            "   public class Caja",
            "   {",
            "      protected int v;",
            "      public function Caja(int v)",
            "      {",
            "         this.v = v;",
            "      }",
            "   }",
            "   public function int doble(int x)",
            "   {",
            "      return x * 2;",
            "   }",
            "   public function void main()",
            "   {",
            "      " + cuerpo,
            "   }",
            "}", "");
    }

    @Test
    @Timeout(120)
    @DisplayName("el `new` y el `(T) x` de JC, bien puestos: compila y corre (el análisis no ve nada que decir)")
    void marcasQueCasan(@TempDir Path tmp) throws Exception {
        Object[] r = compila(tmp, "Marcas", "Marcas.jc", marcas(
            "Animal a = new Perro(\"toby\"); Perro p = (Perro) a; Perro q = new Perro();"
            + " println(p.quien(), \" \", q.quien(), \" \", doble((int) 2.5));"));
        assertTrue((Boolean) r[0], (String) r[1]);
        assertEquals("toby rex 4\n", corre((Path) r[2], "Marcas"));
    }

    @Test
    @DisplayName("el `new` y el `(T) x` de JC tienen que casar con lo que es cada llamada; si no, el análisis lo dice")
    void marcasQueNoCasan(@TempDir Path tmp) throws Exception {
        String[][] casos = {
            {"Perro p = Perro(\"x\");", "'Perro(…)' crea un objeto: en JC se escribe 'new Perro(…)'"},
            {"int x = new doble(2);", "'new' crea un objeto de una clase, y 'doble' no es una clase"},
            {"int x = integer(2.5);", "'integer(x)' es una conversión: en JC se escribe '(int) x'"},
            {"Animal a = new Perro(\"toby\"); Perro p = new Perro(a);",
                "'new Perro(x)' es una conversión, no crea un objeto: en JC se escribe '(Perro) x'"},
            {"Caja c = (Caja) 5;", "'(Caja) x' no es una conversión: un 'int' no se convierte en 'Caja'"},
            {"int x = (doble) 5;", "'(doble) x' no es una conversión: 'doble' no es un tipo"},
        };
        for (int i = 0; i < casos.length; i++) {
            Object[] r = compila(tmp.resolve("caso" + i), "Marcas", "Marcas.jc", marcas(casos[i][0]));
            assertFalse((Boolean) r[0], casos[i][0] + "\n" + r[1]);
            assertTrue(((String) r[1]).contains(casos[i][1]), casos[i][0] + "\n" + r[1]);
        }
        // #511: en BP, `Gato("michi")` como primera sentencia de un constructor de Gato es delegar; en JC, eso se
        // escribe `: this("michi")`, y un `new` ahí quería otro objeto: se dice.
        String gato = String.join("\n",
            "module Deleg", "{",
            "   public class Gato", "   {",
            "      protected string n;",
            "      public function Gato(string n)", "      {", "         this.n = n;", "      }",
            "      public function Gato()", "      {", "         new Gato(\"michi\");", "      }",
            "   }",
            "   public function void main()", "   {", "   }",
            "}", "");
        Object[] r = compila(tmp.resolve("delega"), "Deleg", "Deleg.jc", gato);
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("'new Gato(…)' como primera sentencia de su constructor delega en otro"
                + " constructor (#511)"), (String) r[1]);
    }

    /** #797 — una clase cuyo constructor acepta cualquier objeto, una hija suya y otra que no tiene que ver. */
    private static String cajas(String cuerpo) {
        return String.join("\n",
            "module Cajas", "{",
            "   public class Caja", "   {",
            "      protected Object dentro;",
            "      public function Caja(Object x)", "      {", "         this.dentro = x;", "      }",
            "      public function Object saca()", "      {", "         return this.dentro;", "      }",
            "   }",
            "   public class Hija : Caja", "   {",
            "      public function Hija(Object x) : base(x)", "      {", "      }",
            "   }",
            "   public class Otra", "   {", "   }",
            "   public function void main()", "   {",
            "      " + cuerpo,
            "   }",
            "}", "");
    }

    @Test
    @Timeout(120)
    @DisplayName("`(T) x` es una conversión aunque el constructor acepte x, porque lo que construye lleva `new` (#797)")
    void conversionConUnConstructorQueLaAcepta(@TempDir Path tmp) throws Exception {
        Object[] r = compila(tmp, "Cajas", "Cajas.jc", cajas(String.join(" ",
            "Object o = new Caja(\"dentro\");",
            "Caja c = (Caja) o;",                                  // baja: el mismo objeto, sin envolverlo
            "Caja d = new Caja(o);",                               // construye: otro, con el primero dentro
            "println((string) c.saca(), \" \", (string) ((Caja) d.saca()).saca());",
            "Object h = new Hija(\"hija\");",
            "Caja k = (Caja) h;",                                  // desde Object, a un padre
            "Hija j = (Hija) k;",                                  // desde un antepasado
            "Caja m = (Caja) j;",                                  // lo que ya es: a un padre
            "Hija n = (Hija) j;",                                  // lo que ya es: a sí mismo
            "println((string) m.saca(), \" \", (string) n.saca(), \" \", m == (Caja) j, \" \", n == j);")));
        assertTrue((Boolean) r[0], (String) r[1]);
        assertEquals("dentro dentro\nhija hija true true\n", corre((Path) r[2], "Cajas"));
    }

    @Test
    @DisplayName("lo que no es de la familia no se convierte, con constructor o sin él (#797)")
    void conversionFueraDeLaFamilia(@TempDir Path tmp) throws Exception {
        Object[] r = compila(tmp, "Cajas", "Cajas.jc", cajas("Otra o = new Otra(); Caja c = (Caja) o;"));
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("'(Caja) x' no es una conversión: un 'Otra' no se convierte en 'Caja'"), (String) r[1]);
    }

    @Test
    @DisplayName("con el main en .bp y en .jc, el proyecto no adivina cuál: lo dice, y no compila")
    void dosMains(@TempDir Path tmp) throws Exception {
        String bp = "module HolaJc\n  function main()\n    print \"hola\"\n  end main\nend HolaJc\n";
        Object[] r = compila(tmp, "HolaJc", "HolaJc.bp", bp, "HolaJc.jc", HOLA);
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("el main está dos veces, HolaJc.bp y HolaJc.jc"), (String) r[1]);
    }
}
