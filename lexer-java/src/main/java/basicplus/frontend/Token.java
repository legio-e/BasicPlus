// ============================================================
// Token.java
// Representa un token producido por el lexer (Java 8).
// ============================================================
package basicplus.frontend;

/**
 * Token léxico.
 *
 *   type    : tipo de token (palabra clave, literal, operador...)
 *   lexeme  : texto exacto del fuente que dio lugar a este token
 *             (sin desescapar). Para NEWLINE es "\n" o "\r\n".
 *   value   : valor semántico:
 *               - INTEGER_LIT  -> Long
 *               - FLOAT_LIT    -> Double
 *               - STRING_LIT   -> String ya desescapado
 *               - IDENTIFIER   -> String (el propio nombre)
 *               - resto        -> null
 *   line    : línea (1-based) donde empieza el token
 *   column  : columna (1-based) donde empieza el token
 */
public final class Token {

    public final TokenType type;
    public final String lexeme;
    public final Object value;
    public final int line;
    public final int column;

    public Token(TokenType type, String lexeme, Object value, int line, int column) {
        this.type = type;
        this.lexeme = lexeme;
        this.value = value;
        this.line = line;
        this.column = column;
    }

    // Getters al estilo "record" para no perder simetría con el resto del código.
    public TokenType type()   { return type; }
    public String    lexeme() { return lexeme; }
    public Object    value()  { return value; }
    public int       line()   { return line; }
    public int       column() { return column; }

    @Override
    public String toString() {
        String shownLex;
        if (type == TokenType.NEWLINE) {
            if      ("\n".equals(lexeme))   shownLex = "\\n";
            else if ("\r\n".equals(lexeme)) shownLex = "\\r\\n";
            else if ("\r".equals(lexeme))   shownLex = "\\r";
            else                             shownLex = lexeme;
        } else {
            shownLex = lexeme;
        }

        String valuePart;
        if (value == null) {
            valuePart = "";
        } else if (type == TokenType.STRING_LIT) {
            valuePart = "  =>  \"" + escape(value.toString()) + "\"";
        } else if (value instanceof String) {
            valuePart = "  =>  " + (String) value;
        } else {
            valuePart = "  =>  " + value;
        }

        return String.format("%4d:%-3d  %-12s  '%s'%s", line, column, type, shownLex, valuePart);
    }

    private static String escape(String s) {
        return s.replace("\\", "\\\\")
                .replace("\"", "\\\"")
                .replace("\n", "\\n")
                .replace("\t", "\\t")
                .replace("\r", "\\r");
    }
}
