// ============================================================
// TokenType.cs
// Enumeración de todos los tipos de token de BASICPLUS.
// ============================================================

namespace BasicPlus.Lexer;

public enum TokenType
{
    // ---- Literales ----
    IntegerLit,
    FloatLit,
    StringLit,

    // ---- Identificador ----
    Identifier,

    // ---- Palabras clave: estructura del programa ----
    Module,
    End,
    Import,

    // ---- Palabras clave: declaraciones ----
    Const,
    Var,
    Function,
    Class,
    Extends,
    Enum,
    Property,
    Get,
    Set,
    EndProp,
    EndGet,
    EndSet,

    // ---- Palabras clave: visibilidad ----
    Public,

    // ---- Palabras clave: modificadores de herencia ----
    Final,

    // ---- Palabras clave: instancia / clase base ----
    This,
    Super,

    // ---- Palabras clave: control de flujo ----
    If,
    Then,
    ElseIf,
    Else,
    EndIf,
    Switch,
    Case,
    Default,
    EndSw,
    While,
    Do,
    EndWh,
    Loop,
    For,
    To,
    Step,
    Next,
    In,
    Break,
    Continue,
    Return,

    // ---- Palabras clave: manejo de errores ----
    Try,
    Catch,
    Finally,
    EndTry,
    Throw,

    // ---- Palabras clave: E/S ----
    Print,

    // ---- Palabras clave: tipos primitivos ----
    Integer,
    Float,
    String,
    Boolean,

    // ---- Palabras clave: literales especiales ----
    True,
    False,
    Null,

    // ---- Palabras clave: operadores con forma de palabra ----
    And,
    Or,
    Not,
    Xor,
    Mod,
    Shl,
    Shr,

    // ---- Palabra reservada contextual ----
    // 'field' solo es significativa dentro de un get/set;
    // el lexer la emite siempre como Field y el análisis
    // semántico decide si es válida en el contexto.
    Field,

    // ---- Operadores y signos de puntuación ----
    Assign,         // :=
    PlusAssign,     // +=
    MinusAssign,    // -=
    Eq,             // ==
    Neq,            // !=
    Lt,             // <
    Gt,             // >
    Le,             // <=
    Ge,             // >=
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /
    Bar,            // |
    Amp,            // &
    LParen,         // (
    RParen,         // )
    LBracket,       // [
    RBracket,       // ]
    Comma,          // ,
    Semicolon,      // ;
    Colon,          // :
    Dot,            // .

    // ---- Significativos ----
    Newline,        // \n o \r\n  -- terminadores de sentencia

    // ---- Marcador de fin ----
    Eof
}
