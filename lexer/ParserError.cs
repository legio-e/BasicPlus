// ============================================================
// ParserError.cs
// Diagnóstico (error) producido durante el análisis sintáctico.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class ParserError
{
    public string Message { get; }
    public int Line { get; }
    public int Column { get; }

    public ParserError(string message, int line, int column)
    {
        Message = message;
        Line = line;
        Column = column;
    }

    public override string ToString() => $"[{Line}:{Column}] error sintáctico: {Message}";
}
