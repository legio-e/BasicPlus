package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #386 — el argumento de {@code Main} sale de su VALOR POR DEFECTO, y sólo vale
 * cadena vacía si no declara ninguno.
 *
 * <h3>Qué se está protegiendo</h3>
 *
 * El arranque horneaba {@code ""} mirando únicamente que {@code Main} tuviera un
 * parámetro. O sea que esto:
 *
 * <pre>
 *   function Main(arg: string := "/sd/medidas.db")
 *     if arg != "" then ...
 * </pre>
 *
 * compilaba SIN UNA PALABRA y {@code arg} llegaba vacío: la rama era código
 * muerto y nadie avisaba. Aceptar un valor y tirarlo es peor que no aceptarlo —
 * el LEEME de una entrega llegó a prometer «ejecutar pasándole /sd/medidas.db»,
 * que era imposible.
 *
 * <h3>Por qué basta con el valor por defecto</h3>
 *
 * Idea de Eduardo: el lenguaje YA tiene dónde escribir ese valor (H8.1), así que
 * no hace falta mecanismo nuevo. Pasar el argumento EN TIEMPO DE EJECUCIÓN —wire
 * + IDE + los 3 firmwares— sigue pendiente aparte; esto no lo sustituye, lo
 * desbloquea: hoy ya se puede parametrizar desde el fuente.
 *
 * <h3>La prueba es una A/B, y el control es la mitad del valor</h3>
 *
 * La marca NO aparece en el cuerpo del programa: si está en el .mod es porque el
 * arranque la puso. Y el caso sin default es el CONTROL — sin él, un .mod que
 * internara la cadena por cualquier otro motivo daría verde igual.
 */
class ArgumentoDeMainTest {

    /** Una marca que no puede salir de ningún otro sitio del compilador. */
    private static final String MARCA = "/marca-386/no-esta-en-el-cuerpo.db";

    /** El mismo módulo con y sin valor por defecto: lo ÚNICO que cambia es eso. */
    private static String modulo(String parametro) {
        return "module ArgMain\n"
             + "  function Main(" + parametro + ")\n"
             + "    print arg\n"
             + "  end Main\n"
             + "end ArgMain\n";
    }

    private static Set<String> compilarYLeer(Path tmp, String parametro) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("ArgMain.bp"),
                modulo(parametro).getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "ArgMain";
        assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        return ModSimbolos.conjunto(out.resolve("ArgMain.mod"));
    }

    @Test
    @DisplayName("con valor por defecto, el arranque se lo pasa a Main")
    void elDefaultLlegaAMain(@TempDir Path tmp) throws Exception {
        Set<String> cadenas = compilarYLeer(tmp, "arg: string := \"" + MARCA + "\"");
        assertTrue(cadenas.contains(MARCA),
                "la marca no está en el .mod, así que el arranque sigue horneando"
                + " la cadena vacía y el valor por defecto se pierde en silencio");
    }

    @Test
    @DisplayName("sin valor por defecto no cambia nada: sigue siendo cadena vacía")
    void sinDefaultSigueVacio(@TempDir Path tmp) throws Exception {
        // El CONTROL de la prueba de arriba. Mismo módulo, misma marca ausente:
        // si apareciera igualmente, el verde de arriba no probaría nada.
        Set<String> cadenas = compilarYLeer(tmp, "arg: string");
        assertFalse(cadenas.contains(MARCA),
                "sin valor por defecto la marca no puede estar en ningún sitio");
    }
}
