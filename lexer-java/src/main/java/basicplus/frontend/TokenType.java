// ============================================================
// TokenType.java
// Enumeración de todos los tipos de token de BASICPLUS.
// ============================================================
package basicplus.frontend;

public enum TokenType {

    // ---- Literales ----
    INTEGER_LIT,
    FLOAT_LIT,
    STRING_LIT,

    // ---- Identificador ----
    IDENTIFIER,

    // ---- Palabras clave: estructura del programa ----
    MODULE, END, IMPORT, LIBRARY, FROM, INTERFACE, IMPLEMENTS,

    // ---- Palabras clave: declaraciones ----
    CONST, VAR, FUNCTION, CLASS, EXTENDS, ENUM,
    PROPERTY, GET, SET, ENDPROP, ENDGET, ENDSET,

    // ---- Palabras clave: visibilidad ----
    PUBLIC,

    // ---- Modificador de ownership en var ----
    OWNER,

    // ---- Palabras clave: modificadores de herencia ----
    FINAL,

    // ---- Modificador de concurrencia para properties ----
    SYNC,

    // ---- Modificador para funciones de stdlib que se compilan inline
    //      (el frontend reemplaza la llamada por opcodes en lugar de CALL_EXT). ----
    INTRINSIC,

    // ---- Palabras clave: instancia / clase base ----
    THIS, SUPER,

    // ---- Palabras clave: control de flujo ----
    IF, THEN, ELSEIF, ELSE, ENDIF,
    SWITCH, CASE, DEFAULT, ENDSW,
    WHILE, DO, ENDWH, LOOP,
    FOR, TO, STEP, NEXT, IN,
    BREAK, CONTINUE, RETURN,
    PARALLEL, ENDPAR,

    // ---- Palabras clave: manejo de errores ----
    TRY, CATCH, FINALLY, ENDTRY, THROW,

    // ---- Palabras clave: E/S ----
    PRINT,

    // ---- Palabras clave: tipos primitivos ----
    INTEGER, FLOAT, STRING, BOOLEAN,

    // ---- Palabras clave: literales especiales ----
    TRUE, FALSE, NULL,

    // ---- Palabras clave: operadores con forma de palabra ----
    AND, OR, NOT, XOR, MOD, SHL, SHR, INSTANCEOF,

    // ---- Palabra reservada contextual ----
    // 'field' solo es significativa dentro de un get/set.
    // El lexer la emite siempre como FIELD; el análisis semántico
    // decide si es válida en el contexto.
    FIELD,

    // ---- Operadores y signos de puntuación ----
    ASSIGN,         // :=
    PLUS_ASSIGN,    // +=
    MINUS_ASSIGN,   // -=
    EQ,             // ==
    NEQ,            // !=
    LT,             // <
    GT,             // >
    LE,             // <=
    GE,             // >=
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    BAR,            // |
    AMP,            // &
    LPAREN,         // (
    RPAREN,         // )
    LBRACKET,       // [
    RBRACKET,       // ]
    COMMA,          // ,
    SEMICOLON,      // ;
    COLON,          // :
    DOT,            // .

    // ---- Significativos ----
    NEWLINE,        // \n o \r\n  --  terminadores de sentencia

    // ---- Marcador de fin ----
    EOF
}
