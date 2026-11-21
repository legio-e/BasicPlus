package basicplus.frontend;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/**
 * #542 (V7/H5, 25-sep) — el AOT resuelve los imports COMO EL COMPILADOR, y uno que no carga es un
 * ERROR con su nombre. Hasta hoy avisaba por stdout y seguía: sin la interfaz de {@code Core},
 * {@code NatNew} salía con {@code getX}/{@code getY} por ranuras fijas y {@code areaDe(6,7)} daba 0
 * con status=OK. Eduardo: «hoy en día Core se importa en todos los módulos; si no está, es un error
 * de carga de módulo».
 *
 * <p>Los fuentes van a un temporal SIN {@code BpVM.cfg} por encima, y el cwd del test
 * ({@code lexer-java/}) tampoco lo tiene: la stdlib sólo llega si se dice ({@code --stdlibDir}, que
 * es {@link Main#STDLIB_DIR}).
 */
class AotImportsTest {

    private static final String NATIVA = String.join("\n",
        "module X",
        "  import Core",
        "  native function doble(n: integer): integer",
        "    return n * 2",
        "  end doble",
        "end X", "");

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private static Path escribe(Path dir, String modulo, String fuente) throws Exception {
        Path bp = dir.resolve(modulo + ".bp");
        Files.write(bp, fuente.getBytes(StandardCharsets.UTF_8));
        return bp;
    }

    @Test @DisplayName("#542: sin la stdlib falta Core — error con su nombre, y ningún C traducido a ciegas")
    void sinStdlibFaltaCore(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = null;
        Path bp = escribe(tmp, "X", NATIVA);
        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp, tmp.resolve("out"), true));
        assertTrue(ex.getMessage().contains("'Core'"), ex.getMessage());
        assertFalse(Files.exists(tmp.resolve("out").resolve("aot_X.c")), "no debe emitir C a ciegas");
    }

    @Test @DisplayName("#542: un import que no existe — error con SU nombre aunque Core cargue")
    void importQueNoExiste(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "X", NATIVA.replace("  import Core", "  import Core\n  import Fantasma"));
        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp, tmp.resolve("out"), true));
        assertTrue(ex.getMessage().contains("'Fantasma'"), ex.getMessage());
        assertFalse(ex.getMessage().contains("'Core'"), "Core sí carga con --stdlibDir: " + ex.getMessage());
    }

    @Test @DisplayName("#542: el control — con la stdlib dicha (--stdlibDir) traduce")
    void conStdlibTraduce(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "X", NATIVA);
        AotMain.AotResult r = AotMain.emitAotC(bp, tmp.resolve("out"), true);
        assertNotNull(r.cFile, "X tiene una native: debe emitir C");
        assertTrue(Files.exists(r.cFile));
    }

    /** El Run de un proyecto en el IDE: compila a {@code out/} (y borra los .bpi) y el AOT escribe su C en
     *  {@code target/}. Con la primera versión de #542, un {@code import Y} de la PROPIA app «faltaba» —el
     *  AOT buscaba en target/— y el módulo entero se quedaba sin AOT; antes de #542 sólo avisaba. */
    @Test @DisplayName("#542: un import de la propia app — el AOT lo busca donde el build dejó los .mod")
    void importDeLaApp(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = tmp.resolve("out"), work = tmp.resolve("target");
        escribe(src, "Y", String.join("\n",
            "module Y", "  public function tres(): integer", "    return 3", "  end tres", "end Y", ""));
        Path bp = escribe(src, "X", NATIVA.replace("  import Core", "  import Core\n  import Y")
            .replace("end X", "  function Main()\n    print doble(Y.tres())\n  end Main\nend X"));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = "X";
        assertTrue(Main.buildProject(b, "mivm", /*pruneBpi*/ true), "el proyecto debe compilar");

        // Sin decirle dónde están los .mod, el AOT no los adivina: 'Y' falta, con su nombre.
        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp, work, true));
        assertTrue(ex.getMessage().contains("'Y'"), ex.getMessage());
        // Diciéndoselo (lo que hace AotBuild con el outDir del proyecto), traduce.
        AotMain.AotResult r = AotMain.emitAotC(bp, work, true, out, java.util.Collections.<String>emptyList());
        assertNotNull(r.cFile, "con los .mod de la app a la vista, X debe emitir C");
        assertTrue(r.cFile.startsWith(work), "el C va a target/, no al outDir: " + r.cFile);
    }

    /** #847 (20-nov) — un módulo SIN native no necesita sus imports para el AOT. Hasta hoy los resolvía
     *  igual, y en el Run de {@code SqlDemo} (importa SQLite, que sólo viaja en un pack, y el AOT no mira
     *  los packs) salía «[aot] omitido SqlDemo.bp: … falta el modulo 'SQLite'», con el programa corriendo
     *  bien. Ahora sale como siempre ha salido un módulo sin native: sin C y sin avisos. */
    @Test @DisplayName("#847: un módulo sin native no resuelve sus imports — ni Core ni uno que no existe")
    void sinNativeNoMiraLosImports(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = null;   // ni la stdlib: no le hace falta
        Path bp = escribe(tmp, "X", String.join("\n",
            "module X",
            "  import Core",
            "  import Fantasma",
            "  function Main()",
            "    print 1",
            "  end Main",
            "end X", ""));
        AotMain.AotResult r = AotMain.emitAotC(bp, tmp.resolve("out"), true);
        assertNull(r.cFile, "sin native no hay C");
        assertTrue(r.warnings.isEmpty(), "ni avisos: " + r.warnings);
        assertFalse(Files.exists(tmp.resolve("out").resolve("aot_X.c")));
    }

    /** #847 — el control: la pregunta mira también los métodos de las clases (el aplanado de AotCEmitter).
     *  Un módulo cuya única native es un método sigue resolviendo sus imports, y el que falta sigue siendo
     *  un error con su nombre (#542). */
    @Test @DisplayName("#847: una native que es un método — sigue mirando los imports, y el que falta es un error")
    void nativeEnMetodoSigueMirandoLosImports(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "X", String.join("\n",
            "module X",
            "  import Core",
            "  import Fantasma",
            "  public class Caja",
            "    public property n: integer",
            "    public function Caja(v: integer)",
            "      this.n := v",
            "    end Caja",
            "    public native function doble(): integer",
            "      return this.n * 2",
            "    end doble",
            "  end Caja",
            "end X", ""));
        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp, tmp.resolve("out"), true));
        assertTrue(ex.getMessage().contains("'Fantasma'"), ex.getMessage());
        assertFalse(Files.exists(tmp.resolve("out").resolve("aot_X.c")), "no debe emitir C a ciegas");
    }

    /** Un proyecto que usa una librería en pack ({@code "dependencies": ["../../bpstdlib/sqlite/out"]}): el
     *  compilador la mira, y el AOT también. */
    @Test @DisplayName("#542: un import que sólo vive en una dependencia del proyecto — con ella traduce")
    void importDeUnaDependencia(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path dep = Files.createDirectories(tmp.resolve("dep"));
        Path lib = escribe(Files.createDirectories(tmp.resolve("lib")), "Lib", String.join("\n",
            "module Lib", "  public function uno(): integer", "    return 1", "  end uno", "end Lib", ""));
        assertTrue(Main.compileFile(lib, dep, "mivm"), "Lib debe compilar");
        Path app = Files.createDirectories(tmp.resolve("app"));
        Path bp = escribe(app, "X", NATIVA.replace("  import Core", "  import Core\n  import Lib"));
        Path work = tmp.resolve("target");

        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp, work, true));
        assertTrue(ex.getMessage().contains("'Lib'"), ex.getMessage());
        AotMain.AotResult r = AotMain.emitAotC(bp, work, true, work,
                java.util.Collections.singletonList(dep.toString()));
        assertNotNull(r.cFile, "con la dependencia dicha, X debe emitir C");
    }

    /** V7/H6 G5.b0 (26-sep) — {@code import v1 from pack "SQLI"} NO es un módulo: es la versión de la API
     *  que la placa publica con esa marca, y sus funciones son las {@code native} externas del propio
     *  módulo. Desde #542 el AOT lo contaba como un import que falta y {@code SQLite.bp} y {@code Gui2.bp}
     *  se quedaban sin su {@code .mdn} («falta el modulo 'v1'»): el pack salía sin el puente y en la placa
     *  cada llamada lanzaba «falta el codigo nativo del pack». Se salta POR LA MARCA, no por el nombre:
     *  el control es un {@code import v1} normal, que sigue faltando con su nombre. */
    @Test @DisplayName("G5.b0: la API de un pack nativo (import v1 from pack \"MARC\") no es un import que falte")
    void importDeLaApiNativa(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "X", String.join("\n",
            "module X",
            "  import Core",
            "  import v1 from pack \"MARC\"",
            "  public native function suma(a: integer, b: integer): integer",
            "end X", ""));
        AotMain.AotResult r = AotMain.emitAotC(bp, tmp.resolve("out"), true);
        assertNotNull(r.cFile, "X tiene una native externa: debe emitir C (el puente al pack)");
        assertTrue(Files.exists(r.cFile));

        // El control: un módulo que se llama `v1` y NO viene de un pack nativo sigue faltando.
        Path otro = Files.createDirectories(tmp.resolve("otro"));
        Path bp2 = escribe(otro, "Z", NATIVA.replace("module X", "module Z").replace("end X", "end Z")
                .replace("  import Core", "  import Core\n  import v1"));
        AotMain.ImportsFaltanException ex = assertThrows(AotMain.ImportsFaltanException.class,
                () -> AotMain.emitAotC(bp2, otro.resolve("out"), true));
        assertTrue(ex.getMessage().contains("'v1'"), ex.getMessage());
    }
}
