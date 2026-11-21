package com.mycompany.bpide;

/**
 * Diagnóstico inmutable producido por el frontend o la VM. Se almacena en
 * la {@link ObservableList} de errores que la pestaña "Errores" muestra.
 * Cada instancia corresponde a una línea matcheada por el regex
 * {@code [LINE:COL] (error|aviso) <categoría>: <mensaje>}.
 */
public final class CompileError {
    public final String  file;        // basename del .bp ("Json.bp")
    public final int     line;        // 1-based
    public final int     column;      // 1-based
    public final String  kind;        // "error" o "aviso"
    public final String  category;    // "sintáctico" / "semántico" / ...
    public final String  message;     // texto sin prefijo

    public CompileError(String file, int line, int column,
                        String kind, String category, String message) {
        this.file     = file;
        this.line     = line;
        this.column   = column;
        this.kind     = kind;
        this.category = category;
        this.message  = message;
    }

    @Override public String toString() {
        return file + ":" + line + ":" + column + " " + kind + " " + category + ": " + message;
    }
}
