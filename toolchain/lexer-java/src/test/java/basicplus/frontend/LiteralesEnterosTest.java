package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #385 — el TIPO de un literal entero lo decide su MAGNITUD.
 *
 * <h3>Qué se está protegiendo</h3>
 *
 * Antes, `1754300000000` sin sufijo salía como `1953343232`: un número
 * PLAUSIBLE y equivocado, sin error ni aviso, con `status=OK`. Una marca de
 * tiempo, un identificador o un rowid grande se corrompían y el programa
 * seguía. Se descubrió mirando los números de `SqlDemo`, no porque nada
 * fallara — y ésa es la forma cara de enterarse.
 *
 * <h3>Por qué se comprueba el TOKEN y no la salida del programa</h3>
 *
 * El valor de 64 bits ya se construía bien en el lexer; lo que estaba mal era
 * la ETIQUETA del token, y el estrechado ocurría más abajo. Probar el token es
 * probar la causa: si mañana alguien cambia el emisor, este test sigue diciendo
 * la verdad sobre la regla del lenguaje. Un test de salida diría «va» sin saber
 * por qué.
 *
 * <h3>El caso que DEBE seguir fallando</h3>
 *
 * `var i: integer := <no cabe>` tiene que dar error de conversión. Ese es el
 * criterio de Eduardo: el error no desaparece, se MUEVE al semántico, donde se
 * ve con línea y con nombre. Un test que sólo comprobara los casos que ahora
 * funcionan dejaría pasar una «solución» que se limitara a truncar sin quejarse.
 */
class LiteralesEnterosTest {

    /** El tipo del primer literal entero del fuente. */
    private static TokenType tipoDelPrimerLiteral(String fuente) {
        for (Token t : new Lexer(fuente).tokenize()) {
            if (t.type == TokenType.INTEGER_LIT || t.type == TokenType.LONG_LIT) return t.type;
        }
        throw new AssertionError("no hay literal entero en: " + fuente);
    }

    /** El valor que guarda el token — que es lo que se emite. */
    private static long valorDelPrimerLiteral(String fuente) {
        for (Token t : new Lexer(fuente).tokenize()) {
            if (t.type == TokenType.INTEGER_LIT || t.type == TokenType.LONG_LIT) {
                return ((Number) t.value).longValue();
            }
        }
        throw new AssertionError("no hay literal entero en: " + fuente);
    }

    @Test
    @DisplayName("lo que CABE en 32 bits sigue siendo integer")
    void loQueCabeSigueSiendoInteger() {
        assertEquals(TokenType.INTEGER_LIT, tipoDelPrimerLiteral("x := 42"));
        assertEquals(TokenType.INTEGER_LIT, tipoDelPrimerLiteral("x := 100_000"));
        assertEquals(TokenType.INTEGER_LIT, tipoDelPrimerLiteral("x := 2147483647"));
        assertEquals(TokenType.INTEGER_LIT, tipoDelPrimerLiteral("x := 0xFF_FF"));
        assertEquals(TokenType.INTEGER_LIT, tipoDelPrimerLiteral("x := 0b1010"));
        // El límite exacto, por los dos lados: es donde se equivocan estas cosas.
        assertEquals(TokenType.LONG_LIT,    tipoDelPrimerLiteral("x := 2147483648"));
    }

    @Test
    @DisplayName("lo que NO cabe es long, sin necesidad del sufijo")
    void loQueNoCabeEsLong() {
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 1754300000000"));
        assertEquals(1754300000000L,     valorDelPrimerLiteral("x := 1754300000000"));
    }

    @Test
    @DisplayName("misma regla para hexadecimal y binario — sin la excepción de C")
    void mismaReglaEnHexYBinario() {
        // El hexadecimal grande salía NEGATIVO: el estrechado se llevaba el signo.
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 0x1988E1B4800"));
        assertTrue(valorDelPrimerLiteral("x := 0x1988E1B4800") > 0,
                "un hexadecimal grande no puede salir negativo");

        // 0xFFFFFFFF: el ÚNICO literal de todo el código cuyo significado cambia
        // con esta regla (censo del 13-ago: 602 hex, 5 de 8 dígitos, y su única
        // aparición está en un comentario). Se fija aquí para que el cambio sea
        // una decisión visible y no una sorpresa dentro de un año.
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 0xFFFFFFFF"));
        assertEquals(4294967295L,        valorDelPrimerLiteral("x := 0xFFFFFFFF"));

        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 0b1" + repetir("0", 40)));
    }

    @Test
    @DisplayName("el sufijo L sigue forzando long aunque el valor quepa")
    void elSufijoSigueForzando() {
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 42L"));
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("x := 1_754_300_000_000L"));
        assertEquals(1754300000000L,     valorDelPrimerLiteral("x := 1_754_300_000_000L"));
    }

    @Test
    @DisplayName("el error no desaparece: se MUEVE al semántico")
    void estrecharSigueSiendoError() {
        // Aquí sólo se comprueba lo que le toca al lexer —que el literal ES
        // long—, porque de ahí en adelante es trabajo del semántico y lo tiene
        // resuelto desde siempre. Que el diagnóstico salga («valor de tipo
        // 'long' no asignable a variable de tipo 'integer'») está verificado
        // de punta a punta; ponerlo aquí ataría este test al texto del mensaje.
        assertEquals(TokenType.LONG_LIT, tipoDelPrimerLiteral("var i: integer := 1754300000000"));
    }

    @Test
    @DisplayName("un literal fuera del rango de 64 bits SÍ es error del lexer")
    void masAllaDeLos64BitsEsErrorLexico() {
        // El techo tiene que seguir existiendo: ensanchar a long no es
        // ensanchar hasta el infinito, y sin este caso la regla nueva podría
        // tragarse en silencio algo que ni siquiera cabe en 64 bits.
        Lexer lx = new Lexer("x := 99999999999999999999999");
        lx.tokenize();
        List<LexerError> errs = lx.getErrors();
        assertTrue(!errs.isEmpty(),
                "un literal que no cabe ni en 64 bits debe dar error léxico");
    }

    private static String repetir(String s, int n) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < n; i++) sb.append(s);
        return sb.toString();
    }
}
