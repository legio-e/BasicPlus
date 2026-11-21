package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

import java.time.Duration;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTimeoutPreemptively;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Una palabra inesperada dentro de una clase NO puede colgar el compilador.
 *
 * <h3>Qué pasaba</h3>
 *
 * El bucle que lee los miembros de una clase no tenía el watchdog anti-cuelgue
 * de §7a — el que sí tienen {@code parseBody} y {@code parseModule}. Si el token
 * no empieza un miembro válido, {@code parseDefStmt} no avanzaba y el parser se
 * quedaba dando vueltas PARA SIEMPRE: en el IDE, congelado y sin un mensaje.
 *
 * <p>No es un caso rebuscado. Lo dispara esto, que es lo primero que escribe
 * quien viene de Java:
 *
 * <pre>
 *   public static function e(): integer
 * </pre>
 *
 * <b>BP no tiene {@code static}</b>: un miembro de clase se cualifica con su
 * clase ({@code public function Caja.metodo()}, {@code public const Caja.MAX}).
 * Pero eso hay que DECIRLO, no colgarse.
 *
 * <h3>Por qué recorre varias palabras y no sólo `static`</h3>
 *
 * Porque el fallo no era de `static`: era de CUALQUIER token que no empiece un
 * miembro. Fijar sólo `static` sería censar por el nombre en vez de por la
 * primitiva, que es el error que ya se ha pagado otras veces aquí. `pepito` y
 * `shared` colgaban exactamente igual.
 *
 * <h3>El tope de tiempo NO es una precaución de estilo</h3>
 *
 * El fallo era un CUELGUE. Un test sin límite lo reproduciría colgando la suite
 * entera, que es la forma más cara de enterarse. Por eso
 * {@code assertTimeoutPreemptively}, que mata el hilo — la variante sin
 * {@code Preemptively} esperaría a que terminase, y ahí está el problema.
 */
class CuerpoDeClaseTest {

    private static final long TOPE_SEGUNDOS = 3;

    /** Una clase con `palabra` donde debería empezar un miembro. */
    private static String claseCon(String palabra) {
        return "module CxTest\n"
             + "  public class Caja\n"
             + "    public function Caja()\n"
             + "    end Caja\n"
             + "    public " + palabra + " function e(): integer\n"
             + "      return 3\n"
             + "    end e\n"
             + "  end Caja\n"
             + "end CxTest\n";
    }

    private static List<ParserError> erroresDe(String fuente) {
        Parser p = new Parser(new Lexer(fuente).tokenize());
        p.parseModule();
        return p.getErrors();
    }

    @ParameterizedTest(name = "«{0}» dentro de una clase no cuelga")
    @ValueSource(strings = {"static", "shared", "pepito", "final2", "abstract"})
    @DisplayName("una palabra inesperada en el cuerpo de una clase da ERROR, no un cuelgue")
    void palabraInesperadaNoCuelga(String palabra) {
        assertTimeoutPreemptively(Duration.ofSeconds(TOPE_SEGUNDOS), () -> {
            List<ParserError> errs = erroresDe(claseCon(palabra));
            assertFalse(errs.isEmpty(),
                    "«" + palabra + "» no empieza un miembro de clase: tiene que dar error");
            assertTrue(errs.stream().anyMatch(e -> e.message.contains(palabra)),
                    "el error no nombra «" + palabra + "», así que no dice qué cambiar."
                    + " Mensajes: " + errs);
        });
    }

    @Test
    @DisplayName("con 'static' el error apunta a la forma de BP, no sólo a que está mal")
    void staticApuntaALaFormaDeBp() {
        assertTimeoutPreemptively(Duration.ofSeconds(TOPE_SEGUNDOS), () -> {
            List<ParserError> errs = erroresDe(claseCon("static"));
            // Quien viene de Java necesita saber QUÉ escribir, no sólo que su
            // línea está mal (mismo criterio que #384).
            assertTrue(errs.stream().anyMatch(e -> e.message.contains("Caja.metodo")
                                                || e.message.contains("cualifica")),
                    "el error de 'static' no dice cómo se escribe un miembro de clase"
                    + " en BP. Mensajes: " + errs);
        });
    }

    @Test
    @DisplayName("la forma BUENA de BP sigue compilando sin una queja")
    void laFormaBuenaSigueYendo() {
        String bueno =
              "module CxTest\n"
            + "  public class Caja\n"
            + "    public function Caja()\n"
            + "    end Caja\n"
            + "    public function Caja.estatico(): integer\n"   // método de clase
            + "      return 3\n"
            + "    end Caja.estatico\n"
            + "    public const Caja.MAX: integer := 10\n"        // const de clase
            + "  end Caja\n"
            + "end CxTest\n";
        // El control: sin esto, «no cuelga» se podría conseguir rompiendo la
        // sintaxis buena, y el test iría verde igual.
        assertTrue(erroresDe(bueno).isEmpty(),
                "la forma cualificada es la de BP y debe seguir compilando: "
                + erroresDe(bueno));
    }
}
