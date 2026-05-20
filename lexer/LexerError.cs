// ============================================================
// LexerError.cs
// Diagnóstico (error o aviso) producido durante la tokenización.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class LexerError
{
    public string Message { get; }
    public int Line { get; }
    public int Column { get; }

    public LexerError(string message, int line, int column)
    {
        Message = message;
        Line = line;
        Column = column;
    }

    public override string ToString() => $"[{Line}:{Column}] error léxico: {Message}";
}
