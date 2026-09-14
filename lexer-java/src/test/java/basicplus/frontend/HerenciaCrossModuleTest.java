package basicplus.frontend;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.io.TempDir;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * #492 (V7/H2) — LA MATRIZ: qué miembros de una clase base cruzan la frontera de un
 * módulo hacia una clase derivada, y cuáles no.
 *
 * <p>Nació de una pregunta de Eduardo el 14-sep-2026 —*«lo que hay que verificar es si
 * es el único caso o hay más»*— cuando `OwnerList extends Core.List` no veía `items`.
 * Se contestó midiendo, no leyendo: una base con un miembro de cada clase y un hijo que
 * usa uno cada vez, dos veces — en el MISMO módulo (el control) y en OTRO módulo
 * (`Der extends Basemod.Base`). Lo que el control acepta y la frontera rechaza es un
 * agujero de la interfaz del `.mod`; lo que el control rechaza y la frontera acepta es
 * un agujero al revés; lo que fallan los dos no es de módulos, es del compilador.
 *
 * <p>Es un test de CARACTERIZACIÓN: afirma lo que el compilador hace HOY, agujeros
 * incluidos, para que ningún cambio los mueva sin que se note. Cuando B (`#492`: la
 * interfaz exporta campos, `final`, estáticos y los constructores sobrecargados) cierre
 * un agujero, este test se pone rojo en ese caso — y se le cambia el valor esperado en
 * la tabla de abajo, que es el registro de lo que queda. Los casos con la marca
 * {@code B} son los que B tiene que voltear.
 *
 * <p>Los controles negativos son parte de la prueba: lo `private` NO cruza ni en el
 * mismo módulo (`#508`), y `final` NO se sobreescribe en el mismo módulo. Sin ellos, un
 * «cruza todo» sería indistinguible de «no comprueba nada».
 */
class HerenciaCrossModuleTest {

    /** La base: un miembro de cada clase. Se compila UNA vez por caso, tal cual. */
    private static final String BASE_MIEMBROS =
        "  public class Base\n"
      + "    var n: integer := 1\n"
      + "    private var p: integer := 2\n"
      + "    var otro: Base\n"
      + "    public const Base.MAX: integer := 3\n"
      + "    public property Base.total: integer := 0\n"
      + "    public property edad: integer := 0\n"
      + "    property tam: integer := 0\n"
      + "    private property oculta: integer := 0\n"
      + "    event onX(v: integer)\n"
      + "    public function Base()\n"
      + "      this.n := 1\n"
      + "    end Base\n"
      + "    public function Base(v: integer)\n"
      + "      this.n := v\n"
      + "    end Base\n"
      + "    function prot(): integer\n"
      + "      return this.n\n"
      + "    end prot\n"
      + "    public function pub(): integer\n"
      + "      return this.n + 1\n"
      + "    end pub\n"
      + "    private function priv(): integer\n"
      + "      return 0\n"
      + "    end priv\n"
      + "    public function f(a: integer): integer\n"
      + "      return a\n"
      + "    end f\n"
      + "    public function f(a: string): integer\n"
      + "      return 1\n"
      + "    end f\n"
      + "    public final function fin(): integer\n"
      + "      return 9\n"
      + "    end fin\n"
      + "    public function Base.cuantos(): integer\n"
      + "      return 7\n"
      + "    end Base.cuantos\n"
      + "    public function eq(o: Base): boolean\n"
      + "      return o.n == this.n\n"
      + "    end eq\n"
      + "    public function arr(a: integer[]): integer\n"
      + "      return len(a)\n"
      + "    end arr\n"
      + "    public function crea(): Base\n"
      + "      return this\n"       // `return Base()` peta el emisor: #507
      + "    end crea\n"
      + "  end Base\n";

    private static final String ENUM =
        "  public enum Color\n"
      + "    ROJO\n"
      + "    VERDE\n"
      + "  end Color\n";

    /** Un caso: qué usa el hijo, qué añade a su clase, cómo llama a `super`, y lo esperado. */
    static final class Caso {
        final String clave, uso, extras, ctor;
        final boolean control;   // compila con las dos clases en el MISMO módulo
        final boolean cross;     // compila con `Der extends Basemod.Base` en OTRO módulo — HOY
        final String nota;       // «B» si B debe voltear el valor de cross; «bug #NNN» si es del compilador
        Caso(String clave, String uso, String extras, String ctor, boolean control, boolean cross, String nota) {
            this.clave = clave; this.uso = uso; this.extras = extras; this.ctor = ctor;
            this.control = control; this.cross = cross; this.nota = nota;
        }
        @Override public String toString() { return clave + (nota.isEmpty() ? "" : " [" + nota + "]"); }
    }

    private static final String SUPER_PUB =
        "    public function pub(): integer\n      return super.pub() + 1\n    end pub\n";
    private static final String FIN_OVERRIDE =
        "    public function fin(): integer\n      return 1\n    end fin\n";
    private static final String NAT_PROPIO =
        "    var m: integer := 3\n    public native function nat(): integer\n      return this.m * 2\n    end nat\n";
    private static final String NAT_HEREDADO =
        "    public native function nat(): integer\n      return this.n * 2\n    end nat\n";
    private static final String NAT_PROP =
        "    public native function nat(): integer\n      return this.edad * 2\n    end nat\n";

    /** `{MOD}` se sustituye por `Basemod.` en el caso cross y por nada en el control. */
    static Stream<Caso> casos() {
        return Stream.of(
            //        clave              uso                                              extras        ctor       control cross  nota
            new Caso("campo_prot",      "return this.n",                                 "",           "super()", true,  true,  "B: cruza desde bpi 8 (14-sep)"),
            new Caso("campo_priv",      "return this.p",                                 "",           "super()", false, false, "control negativo (#508)"),
            new Caso("campo_prot_ref",  "this.otro := this\n      return 1",             "",           "super()", true,  true,  "B: cruza desde bpi 8"),
            new Caso("metodo_prot",     "return this.prot()",                            "",           "super()", true,  true,  ""),
            new Caso("metodo_pub",      "return this.pub()",                             "",           "super()", true,  true,  ""),
            new Caso("metodo_priv",     "return this.priv()",                            "",           "super()", false, false, "control negativo"),
            new Caso("prop_pub",        "this.edad := 4\n      return this.edad",        "",           "super()", true,  true,  ""),
            new Caso("prop_prot",       "this.tam := 5\n      return this.tam",          "",           "super()", true,  true,  ""),
            new Caso("prop_priv",       "return this.oculta",                            "",           "super()", false, false, "control negativo (#508); cross: B desde bpi 8 (`prop … private` viaja)"),
            new Caso("sobrecarga",      "return this.f(1) + this.f(\"a\")",              "",           "super()", true,  true,  ""),
            new Caso("ctor_sobrecarga", "return this.n",                                 "",           "super(5)", false, false, "bug #509: super sólo ve la 1ª sobrecarga"),
            new Caso("ctor_default",    "return 0",                                      "",           "super()", true,  true,  ""),
            new Caso("super_metodo",    "return this.pub()",                             SUPER_PUB,    "super()", true,  true,  ""),
            new Caso("const_estatica",  "return {MOD}Base.MAX",                          "",           "super()", true,  true,  "control: #510 cerrado el 14-sep (el ctor tapaba a la clase base)"),
            new Caso("metodo_estatico", "return {MOD}Base.cuantos()",                    "",           "super()", true,  true,  "#510 · B: cruza desde bpi 8 (`staticmethod` + alias `__cls_s_`)"),
            new Caso("prop_estatica",   "{MOD}Base.total := 1\n      return {MOD}Base.total", "",     "super()", true,  true,  "#510 · B: cruza desde bpi 8 (`staticprop` + alias `__cls_sg_/__cls_ss_`)"),
            new Caso("evento",          "raise onX(5)\n      return 0",                  "",           "super()", true,  true,  "B: cruza desde bpi 8 (el raise heredado va por la ranura de la interfaz)"),
            new Caso("firma_propia",    "if this.eq(this) then\n        return 1\n      endif\n      return 0", "", "super()", true, true, ""),
            new Caso("firma_array",     "var a: integer[] := [1, 2]\n      return this.arr(a)", "",   "super()", true,  true,  ""),
            new Caso("devuelve_base",   "var b: {MOD}Base := this.crea()\n      return b.pub()", "",  "super()", true,  true,  ""),
            new Caso("final_override",  "return this.fin()",                             FIN_OVERRIDE, "super()", false, false, "control negativo; cross: B desde bpi 8 (`final` viaja)"),
            new Caso("nat_campo_propio", "return this.nat()",                            NAT_PROPIO,   "super()", true,  true,  ""),
            new Caso("nat_campo_heredado", "return this.nat()",                          NAT_HEREDADO, "super()", true,  true,  "B: cruza desde bpi 8 (el semántico; el AOT sigue sin leer campos: por getter)"),
            new Caso("nat_prop_heredada", "return this.nat()",                           NAT_PROP,     "super()", true,  true,  ""),
            new Caso("enum_modulo",     "var c: {MOD}Color := {MOD}Color.VERDE\n      return 0", "",  "super()", true,  true,  "")
        );
    }

    private static String derClase(Caso c, String mod) {
        return "  public class Der extends " + mod + "Base\n"
             + "    public function Der()\n      " + c.ctor + "\n    end Der\n"
             + c.extras
             + "    public function usa(): integer\n      " + c.uso.replace("{MOD}", mod) + "\n    end usa\n"
             + "  end Der\n";
    }

    private static final String MAIN =
        "  public function main()\n    var d: Der := Der()\n    print d.usa()\n  end main\n";

    private static boolean compila(Path tmp, String main, List<String[]> fuentes) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes)
            Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = main;
        File std = new File("../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(),
                "hace falta ../bpstdlib/Core.mod (compila la stdlib antes de este test)");
        b.dependencies.add(std.getAbsolutePath());
        try {
            return Main.buildProject(b, "mivm", false);
        } catch (RuntimeException petada) {
            // El emisor no debería reventar nunca (#506/#507: un error del emisor tiene que
            // ser un diagnóstico con línea). Mientras lo haga, aquí cuenta como «no compila»
            // y el caso queda registrado en la tabla con su nota; cuando deje de petar y
            // además cruce, el caso se pondrá rojo y se le cambia el valor esperado.
            System.err.println("  [matriz] el compilador PETÓ: " + petada);
            return false;
        }
    }

    @ParameterizedTest(name = "control (mismo módulo): {0}")
    @MethodSource("casos")
    @DisplayName("el control: las dos clases en el MISMO módulo")
    void control(Caso c, @TempDir Path tmp) throws IOException {
        String same = "module Same\n  import Core\n" + ENUM + BASE_MIEMBROS + derClase(c, "") + MAIN + "end Same\n";
        List<String[]> fuentes = new ArrayList<>();
        fuentes.add(new String[]{"Same", same});
        assertEquals(c.control, compila(tmp, "Same", fuentes),
                "control «" + c.clave + "»: " + (c.control ? "debe compilar" : "NO debe compilar") + " en el mismo módulo" + (c.nota.isEmpty() ? "" : " (" + c.nota + ")"));
    }

    @ParameterizedTest(name = "frontera (otro módulo): {0}")
    @MethodSource("casos")
    @DisplayName("la frontera: Der extends Basemod.Base en OTRO módulo — lo que cruza HOY")
    void frontera(Caso c, @TempDir Path tmp) throws IOException {
        String base = "module Basemod\n  import Core\n" + ENUM + BASE_MIEMBROS + "end Basemod\n";
        String der  = "module Dermod\n  import Core\n  import Basemod\n" + derClase(c, "Basemod.") + MAIN + "end Dermod\n";
        List<String[]> fuentes = new ArrayList<>();
        fuentes.add(new String[]{"Basemod", base});
        fuentes.add(new String[]{"Dermod", der});
        assertEquals(c.cross, compila(tmp, "Dermod", fuentes),
                "frontera «" + c.clave + "»: HOY " + (c.cross ? "cruza" : "NO cruza") + (c.nota.isEmpty() ? "" : " (" + c.nota + ")")
                + ". Si esto acaba de cambiar por un arreglo, actualiza la tabla del test: es el registro de lo que queda.");
    }
}
