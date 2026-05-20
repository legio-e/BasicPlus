// ============================================================
// Token.cs
// Representa un token producido por el lexer.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class Token
{
    public TokenType Type { get; }

    /// <summary>
    /// Texto exacto del fuente que dio lugar a este token
    /// (sin desescapar). Para NEWLINE es "\n" o "\r\n".
    /// </summary>
    public string Lexeme { get; }

    /// <summary>
    /// Valor semántico del token cuando aplica:
    ///   - IntegerLit  -> long
    ///   - FloatLit    -> double
    ///   - StringLit   -> string ya desescapado
    ///   - Identifier  -> string (el propio nombre)
    /// Para el resto es null.
    /// </summary>
    public object? Value { get; }

    public int Line { get; }
    public int Column { get; }

    public Token(TokenType type, string lexeme, object? value, int line, int column)
    {
        Type = type;
        Lexeme = lexeme;
        Value = value;
        Line = line;
        Column = column;
    }

    public override string ToString()
    {
        // Representación legible para el demo:
        //   line:col   TYPE             lexema   (valor)
        string lexemeShown = Type == TokenType.Newline
            ? (Lexeme == "\n" ? "\\n" : (Lexeme == "\r\n" ? "\\r\\n" : "\\r"))
            : Lexeme;

        string valuePart = Value switch
        {
            null => string.Empty,
            string s when Type == TokenType.StringLit => $"  =>  \"{Escape(s)}\"",
            string s => $"  =>  {s}",
            _ => $"  =>  {Value}"
        };

        return $"{Line,4}:{Column,-3}  {Type,-12}  '{lexemeShown}'{valuePart}";
    }

    private static string Escape(string s)
    {
        return s.Replace("\\", "\\\\")
                .Replace("\"", "\\\"")
                .Replace("\n", "\\n")
                .Replace("\t", "\\t")
                .Replace("\r", "\\r");
    }
}
