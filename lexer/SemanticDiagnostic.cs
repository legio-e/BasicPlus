// ============================================================
// SemanticDiagnostic.cs
// Mensaje (error o aviso) emitido por el analizador semántico.
// ============================================================

namespace BasicPlus.Lexer;

public enum DiagnosticKind { Error, Warning }

public sealed class SemanticDiagnostic
{
    public DiagnosticKind Kind { get; }
    public string Message { get; }
    public int Line { get; }
    public int Column { get; }

    public SemanticDiagnostic(DiagnosticKind kind, string message, int line, int column)
    {
        Kind = kind; Message = message; Line = line; Column = column;
    }

    public override string ToString()
    {
        var label = Kind == DiagnosticKind.Error ? "error semántico" : "aviso semántico";
        return $"[{Line}:{Column}] {label}: {Message}";
    }
}
