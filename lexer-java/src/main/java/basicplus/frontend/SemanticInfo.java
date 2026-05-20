// ============================================================
// SemanticInfo.java
// Información que el analizador semántico añade ENCIMA del AST,
// sin mutar los nodos. Las claves usan IDENTIDAD de referencia
// (IdentityHashMap), no equals(), para evitar colisiones entre
// nodos sintácticamente equivalentes.
// ============================================================
package basicplus.frontend;

import java.util.ArrayList;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;

public final class SemanticInfo {

    /** Tipo resuelto de cada expresión. */
    public final Map<Ast.IExpr, BpType> exprTypes = new IdentityHashMap<>();

    /** Símbolo al que resuelve un identificador / acceso a miembro. */
    public final Map<Ast.IExpr, Symbol> exprSymbols = new IdentityHashMap<>();

    /** Símbolo declarado por un nodo de declaración. */
    public final Map<Ast.Node, Symbol> declSymbols = new IdentityHashMap<>();

    /** Módulo procesado (símbolo raíz). */
    public Symbol.ModuleSymbol module;

    public final List<SemanticDiagnostic> diagnostics = new ArrayList<>();

    public boolean hasErrors() {
        for (SemanticDiagnostic d : diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) return true;
        return false;
    }
}
