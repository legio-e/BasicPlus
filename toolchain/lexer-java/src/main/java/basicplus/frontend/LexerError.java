// ============================================================
// LexerError.java
// Diagnóstico (error o aviso) producido durante la tokenización.
// ============================================================
package basicplus.frontend;

public final class LexerError {
    public final String message;
    public final int line;
    public final int column;

    public LexerError(String message, int line, int column) {
        this.message = message;
        this.line = line;
        this.column = column;
    }

    @Override
    public String toString() {
        return "[" + line + ":" + column + "] error léxico: " + message;
    }
}
