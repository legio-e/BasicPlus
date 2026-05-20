// ============================================================
// SemanticInfo.cs
// Información que el analizador semántico añade ENCIMA del AST,
// sin mutar los nodos. Se accede consultando los diccionarios
// con la propia referencia del nodo.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class SemanticInfo
{
    /// <summary>Tipo resuelto de cada expresión.</summary>
    public Dictionary<IExpr, BpType> ExprTypes { get; }
        = new(ReferenceEqualityComparer.Instance);

    /// <summary>Símbolo al que resuelve un identificador / acceso a miembro.</summary>
    public Dictionary<IExpr, Symbol> ExprSymbols { get; }
        = new(ReferenceEqualityComparer.Instance);

    /// <summary>Símbolo declarado por un nodo de declaración.</summary>
    public Dictionary<AstNode, Symbol> DeclSymbols { get; }
        = new(ReferenceEqualityComparer.Instance);

    /// <summary>Símbolo de la clase si la expresión es 'this'/'super' o un acceso miembro instancia.</summary>
    public ModuleSymbol? Module { get; set; }

    public List<SemanticDiagnostic> Diagnostics { get; } = new();

    public bool HasErrors => Diagnostics.Any(d => d.Kind == DiagnosticKind.Error);
}
