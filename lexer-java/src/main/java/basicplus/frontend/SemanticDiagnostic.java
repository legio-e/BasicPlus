// ============================================================
// SemanticDiagnostic.java
// Mensaje (error o aviso) emitido por el analizador semántico.
// ============================================================
package basicplus.frontend;

public final class SemanticDiagnostic {

    public enum Kind { ERROR, WARNING }

    public final Kind kind;
    public final String message;
    public final int line;
    public final int column;

    public SemanticDiagnostic(Kind kind, String message, int line, int column) {
        this.kind = kind;
        this.message = message;
        this.line = line;
        this.column = column;
    }

    @Override
    public String toString() {
        String label = (kind == Kind.ERROR) ? "error semántico" : "aviso semántico";
        return "[" + line + ":" + column + "] " + label + ": " + message;
    }
}
