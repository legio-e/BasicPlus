package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTimeoutPreemptively;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #384 — una palabra reservada NUNCA puede ser un nombre, y el compilador tiene
 * que DECIRLO en vez de colgarse o de quejarse de otra cosa.
 *
 * <h3>Por qué recorre la tabla entera y no prueba dos casos</h3>
 *
 * El bug se descubrió con `next`, y el mismo día mordió `step`. Arreglar y
 * probar esas dos habría dado un verde sobre DOS de setenta y tres, que es
 * justo el error de censar por el nombre en vez de por la primitiva. La lista
 * sale de {@link Lexer#palabrasReservadas()}, o sea del mismo mapa que usa el
 * lexer: una palabra nueva entra en la prueba el día que se añade, sola.
 *
 * <p>El censo del 13-ago sobre las 73 dio: 0 cuelgues, 70 con error que nombra
 * la palabra, 2 contextuales que compilan (correcto) y 1 —`event`— con un
 * mensaje que señalaba al paréntesis sin nombrarla. Este test fija ese
 * resultado para que no se pueda desandar.
 *
 * <h3>El timeout NO es una precaución de estilo</h3>
 *
 * El fallo original era un CUELGUE, no un error equivocado: el compilador no
 * terminaba nunca y en el IDE parecía congelado. Un test sin límite de tiempo
 * reproduciría el bug colgando la suite entera, que es la forma más cara de
 * enterarse. Por eso cada caso corre con tope y `assertTimeoutPreemptively`,
 * que mata el hilo — la variante sin `Preemptively` esperaría a que terminase,
 * y ahí está precisamente el problema.
 */
class PalabrasReservadasTest {

    /** Segundos por palabra. Una compilación de juguete tarda milisegundos; con
     *  dos hay margen de sobra para una máquina cargada sin que un cuelgue de
     *  verdad se disfrace de lentitud. */
    private static final long TOPE_SEGUNDOS = 2;

    /**
     * Las CONTEXTUALES: están en la tabla del lexer, pero sí valen como nombre.
     *
     * <p>Escrita a mano y no deducida, a propósito. Que una palabra sea
     * contextual es una DECISIÓN (L4, tarea #56), no una propiedad que se pueda
     * mirar en el mapa. Si mañana alguien hace contextual una tercera, este test
     * se pondrá rojo — y eso es lo que se quiere: que la decisión pase por aquí
     * y quede escrita, en vez de colarse porque el test la dedujo sola.
     */
    private static final Set<String> CONTEXTUALES =
            new HashSet<>(Arrays.asList("get", "set"));

    static Stream<String> todasLasPalabras() {
        List<String> palabras = new ArrayList<>(Lexer.palabrasReservadas());
        palabras.sort(String::compareTo);   // orden estable entre máquinas
        return palabras.stream();
    }

    /** Un módulo de juguete que usa `palabra` como nombre de función. */
    private static String moduloCon(String palabra) {
        return "module KwTest\n"
             + "  public function " + palabra + "(): boolean\n"
             + "    return true\n"
             + "  end " + palabra + "\n"
             + "end KwTest\n";
    }

    /** Compila hasta el parser y devuelve sus errores. No emite: lo que se está
     *  probando ocurre antes, y así el test no depende del backend. */
    private static List<ParserError> erroresDeParseo(String fuente) {
        Lexer lexer = new Lexer(fuente);
        List<Token> tokens = lexer.tokenize();
        Parser parser = new Parser(tokens);
        parser.parseModule();
        return parser.getErrors();
    }

    @ParameterizedTest(name = "«{0}» como nombre de función")
    @MethodSource("todasLasPalabras")
    @DisplayName("ninguna palabra reservada cuelga el compilador, y el error la nombra")
    void ningunaPalabraCuelgaYElErrorLaNombra(String palabra) {
        // El tope cubre TODO el caso: si el parser entra en bucle, este assert
        // falla en 2 s en vez de dejar la suite colgada.
        assertTimeoutPreemptively(Duration.ofSeconds(TOPE_SEGUNDOS), () -> {
            List<ParserError> errores = erroresDeParseo(moduloCon(palabra));

            if (CONTEXTUALES.contains(palabra)) {
                assertTrue(errores.isEmpty(),
                        "«" + palabra + "» es contextual (L4): debe valer como nombre"
                        + " de función y no dio ningún error. Errores: " + errores);
                return;
            }

            assertFalse(errores.isEmpty(),
                    "«" + palabra + "» es reservada y se ha aceptado como nombre"
                    + " de función — el compilador debería rechazarla");

            // Y que el mensaje la NOMBRE. Un error correcto que no dice cuál es
            // la palabra deja al usuario buscando: es lo que pasaba con `event`,
            // que señalaba al '(' dos tokens más allá.
            boolean laNombra = errores.stream()
                    .anyMatch(e -> e.message.contains(palabra));
            assertTrue(laNombra,
                    "el error de «" + palabra + "» no la nombra, así que no dice"
                    + " qué hay que cambiar. Mensajes: " + errores);
        });
    }

    @Test
    @DisplayName("el test cubre la tabla ENTERA, no una copia que se quedó corta")
    void laListaSaleDelLexer() {
        Set<String> palabras = Lexer.palabrasReservadas();
        // Guardián barato contra el fallo silencioso de que el accesor devuelva
        // vacío (refactor del mapa, orden de inicialización estática...): sin
        // esto, cero casos ejecutados se vería como VERDE.
        assertTrue(palabras.size() > 50,
                "sólo " + palabras.size() + " palabras reservadas; se esperaban"
                + " decenas. ¿El accesor devuelve el mapa de verdad?");
        assertTrue(palabras.containsAll(CONTEXTUALES),
                "las contextuales deben seguir en la tabla del lexer: si salieran"
                + " de ella, este test dejaría de comprobar lo que cree");
    }
}
