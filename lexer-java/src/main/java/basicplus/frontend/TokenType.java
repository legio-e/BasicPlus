// ============================================================
// TokenType.java
// Enumeración de todos los tipos de token de BASICPLUS.
// ============================================================
package basicplus.frontend;

public enum TokenType {

    // ---- Literales ----
    INTEGER_LIT,
    LONG_LIT,        // H1.2 (V2): literal long i64 con sufijo L (p.ej. 5L)
    FLOAT_LIT,
    DOUBLE_LIT,      // H1.3 (V2): literal double f64 con sufijo d (p.ej. 1.5d)
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

    // ---- Modificador para funciones AOT (H3 #162). Marca la función
    //      como candidata a tener una versión C-nativa. El bytecode
    //      .mod se emite igual (el flag NO afecta la compilación BP);
    //      solo se propaga al .bpi y al emisor AOT cuando éste exista. ----
    NATIVE,

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

    // ---- H1.2 (V2): long = entero de 64 bits (i64) ----
    LONG,

    // ---- H1.3 (V2): double = coma flotante de 64 bits (f64) ----
    DOUBLE,

    // ---- Palabras clave: tipos enteros estrechos (L10) ----
    // `byte` = uint8, `int8` = signed 8-bit, `word` = uint16, `int16` =
    // signed 16-bit, `short` es alias gramatical de `int16`. Promocionan
    // a INTEGER al cargar; el store desde INTEGER requiere cast explícito
    // del estilo `byte(x)` / `int16(x)` / `word(x)` / `short(x)`.
    BYTE, INT8, WORD, INT16, SHORT,

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
