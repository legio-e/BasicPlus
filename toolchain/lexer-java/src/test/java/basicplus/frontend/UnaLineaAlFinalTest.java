package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #701 (24-oct) — un `if`, `while` o `for` de UNA LÍNEA como última sentencia de un bloque de su misma clase. Para
 * dar un error claro a quien escribe `if E then STMT endif`, el parser miraba si detrás venía el terminador; pero la
 * sentencia ya se había comido su salto de línea, y lo que veía era el de la línea siguiente, el del bloque de fuera:
 * se lo llevaba, y el programa no compilaba. Aquí, sólo el parser: las tres formas compilan, y el terminador en SU
 * línea sigue dando el error con su nombre. Lo que hacen al ejecutarse lo mira el corpus
 * ({@code bpgenvm-c/samples/UnaLineaAlFinal.bp}); el error, también {@code samples/errores/UnaLineaConFin.bp}.
 */
class UnaLineaAlFinalTest {

    /** Hasta el parser, y sus errores (como en PalabrasReservadasTest): lo que se prueba ocurre antes de emitir. */
    private static List<ParserError> erroresDeParseo(String fuente) {
        Lexer lexer = new Lexer(fuente);
        List<Token> tokens = lexer.tokenize();
        Parser parser = new Parser(tokens);
        parser.parseModule();
        return parser.getErrors();
    }

    private static String modulo(String cuerpo) {
        return "module M\n  function Main()\n    var a: integer := 0\n" + cuerpo + "  end Main\nend M\n";
    }

    private static void compila(String cuerpo) {
        List<ParserError> e = erroresDeParseo(modulo(cuerpo));
        assertTrue(e.isEmpty(), () -> "tenía que compilar, y dice: " + e);
    }

    private static void diceQue(String cuerpo, String trozo) {
        List<ParserError> e = erroresDeParseo(modulo(cuerpo));
        assertTrue(e.stream().anyMatch(x -> x.message.contains(trozo)), () -> "tenía que decir «" + trozo + "», y dice: " + e);
    }

    @Test
    @DisplayName("un if de una línea, el último de un if: compila")
    void ifEnIf() {
        compila("    if a == 0 then\n      a := 1\n      if a == 1 then a := 2\n    endif\n");
    }

    @Test
    @DisplayName("un while de una línea, el último de un while: compila")
    void whileEnWhile() {
        compila("    while a < 6 do\n      a += 1\n      while a < 3 do a += 1\n    endwh\n");
    }

    @Test
    @DisplayName("un for de una línea, el último de un for: compila, con el next de fuera con nombre y sin él")
    void forEnFor() {
        compila("    for i := 1 to 2 do\n      for j := 1 to 2 do a += j\n    next\n");
        compila("    for i := 1 to 2 do\n      for j := 1 to 2 do a += j\n    next i\n");
    }

    @Test
    @DisplayName("dos if de una línea, uno dentro de otro, los últimos de un if: compila")
    void dosEnIf() {
        compila("    if a == 0 then\n      if a == 0 then if a == 0 then a := 3\n    endif\n");
    }

    @Test
    @DisplayName("el terminador en SU línea sigue siendo el error de mezclar las dos formas, con su nombre")
    void terminadorEnSuLinea() {
        diceQue("    if a == 0 then a := 1 endif\n", "no lleva 'endif'");
        diceQue("    while a < 3 do a += 1 endwh\n", "no lleva 'endwh'");
        diceQue("    for i := 1 to 2 do a += i next\n", "no lleva 'next'");
    }
}
