// ============================================================
// ParserError.java
// Diagnóstico (error) producido durante el análisis sintáctico.
// ============================================================
package basicplus.frontend;

public final class ParserError {
    public final String message;
    public final int line;
    public final int column;

    public ParserError(String message, int line, int column) {
        this.message = message;
        this.line = line;
        this.column = column;
    }

    @Override
    public String toString() {
        return "[" + line + ":" + column + "] error sintáctico: " + message;
    }
}
