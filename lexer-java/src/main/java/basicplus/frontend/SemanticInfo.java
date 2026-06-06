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

    /** BUG-2 — ClassSymbol resuelto del tipo de cada cláusula catch (puede ser
     *  cross-module). El emisor lo usa para decidir TRY_BEGIN (local) vs
     *  TRY_BEGIN_EXT (cross-module). Key = CatchClause. */
    public final Map<Ast.CatchClause, Symbol.ClassSymbol> catchClassSymbols = new IdentityHashMap<>();

    /** Módulo procesado (símbolo raíz). */
    public Symbol.ModuleSymbol module;

    /** L2 v3 — layout binario de cada clase emitida (numFields, numMethods,
     *  fieldBitmap, ownerBitmap). Lo populates el MivmEmitter al cerrar
     *  cada clase; lo consume ModuleInterface.extractClass para emitirlo
     *  al .bpi y que importadores con extends cross-module reserven el
     *  layout correcto. Key = ClassSymbol del módulo. */
    public final Map<Symbol.ClassSymbol, ClassBinaryLayout> classLayouts = new IdentityHashMap<>();

    /** Layout binario inmutable de una clase. fieldBitmap/ownerBitmap son
     *  arrays de int donde cada int = 32 slots (mismo formato que el
     *  descriptor binario que escribe ModWriter). */
    public static final class ClassBinaryLayout {
        public final int numFields;
        public final int numMethods;
        public final int[] fieldBitmap;
        public final int[] ownerBitmap;
        public ClassBinaryLayout(int numFields, int numMethods, int[] fieldBitmap, int[] ownerBitmap) {
            this.numFields = numFields;
            this.numMethods = numMethods;
            this.fieldBitmap = fieldBitmap;
            this.ownerBitmap = ownerBitmap;
        }
    }

    public final List<SemanticDiagnostic> diagnostics = new ArrayList<>();

    public boolean hasErrors() {
        for (SemanticDiagnostic d : diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) return true;
        return false;
    }
}
