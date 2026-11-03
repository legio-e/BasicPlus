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

    /** H5.c — slot de vtable resuelto para cada `obj::metodo`. Lo calcula el
     *  semántico (que sabe la clase del receptor) y lo consume el emisor, que
     *  sólo tiene que escribirlo: así el slot se decide en UN sitio. */
    public final Map<Ast.IExpr, Integer> methodRefSlots = new IdentityHashMap<>();

    /** #389 — las llamadas que NO son llamadas: `Cosa(o)` / `string(o)`, o sea
     *  la conversión de REFERENCIA escrita con el nombre del tipo (la misma
     *  forma que `byte(someInt)` de L10 o `integer(d)` de H1.3b, extendida a
     *  referencias).
     *
     *  La decide el semántico —que es quien sabe si la clase declara un
     *  constructor de un argumento, en cuyo caso gana el constructor— y el
     *  emisor se limita a consultar este conjunto. Misma disciplina que
     *  `methodRefSlots`: la regla vive en UN sitio y no puede divergir.
     *
     *  En bytecode no emite NADA: un handle es un handle, y lo único que cambia
     *  es el tipo estático con el que el compilador mira el valor a partir de
     *  ahí. Por eso los `.mod` salen idénticos. */
    public final java.util.Set<Ast.IExpr> refCasts =
            java.util.Collections.newSetFromMap(new IdentityHashMap<Ast.IExpr, Boolean>());

    /** V8 H7.3 (#756) — los métodos y properties que una clase declara e implementan un miembro de una interfaz de su
     *  cabecera (los que llevan `@implement`): el emisor les pone el bit «implementa» en su fila de 4.5, y el cargador
     *  dice E12 si la interfaz deja de tenerlo. Lo decide el semántico (checkInterfaces), en UN sitio. */
    public final java.util.Set<Symbol> implementanInterfaz =
            java.util.Collections.newSetFromMap(new IdentityHashMap<Symbol, Boolean>());

    /** V8/H7.1 (#747) — los `iif(c, a, b)` del módulo: el ternario, que el emisor despliega con saltos (sólo se
     *  calcula la rama que toca). Lo decide el semántico, en UN sitio, como `refCasts`. */
    public final java.util.Set<Ast.IExpr> ternarios =
            java.util.Collections.newSetFromMap(new IdentityHashMap<Ast.IExpr, Boolean>());

    /** V8/H7.2 (#749) — los `for x in lista` sobre una List (no un array): el emisor los recorre con length() y get(i),
     *  y x es un Object. */
    public final java.util.Set<Ast.ForStmt> forDeListas =
            java.util.Collections.newSetFromMap(new IdentityHashMap<Ast.ForStmt, Boolean>());

    /** V8/H7.1 (#747) — de cada asignación compuesta (`x op= e`), el tipo en que se opera: el de `x op e`, como en C#.
     *  Si no es el de `x`, el resultado se recorta al de `x` al guardarlo (`byte b; b += 1` opera en integer). */
    public final Map<Ast.AssignStmt, BpType> compuestos = new IdentityHashMap<>();

    /** #746 — ¿se puede calcular dos veces sin que se note? Una variable, un literal, `this`, el campo de algo así, el
     *  elemento de un array así, o una operación entre ellos. Una llamada no, ni una propiedad (su getter es código).
     *  Lo usan los dos emisores para el destino de un compuesto: si no es puro, se calcula una vez. */
    public boolean esPuro(Ast.IExpr e) {
        if (e instanceof Ast.ParenExpr) return esPuro(((Ast.ParenExpr) e).inner);
        if (e instanceof Ast.IntLitExpr || e instanceof Ast.LongLitExpr || e instanceof Ast.FloatLitExpr
                || e instanceof Ast.DoubleLitExpr || e instanceof Ast.StringLitExpr || e instanceof Ast.BoolLitExpr
                || e instanceof Ast.NullLitExpr || e instanceof Ast.ThisExpr) return true;
        if (e instanceof Ast.IdentifierExpr) {
            Symbol s = exprSymbols.get(e);
            return s instanceof Symbol.VarSymbol || s instanceof Symbol.ParamSymbol || s instanceof Symbol.ConstSymbol;
        }
        if (e instanceof Ast.MemberAccessExpr)
            return exprSymbols.get(e) instanceof Symbol.VarSymbol && esPuro(((Ast.MemberAccessExpr) e).target);
        if (e instanceof Ast.IndexExpr) return esPuro(((Ast.IndexExpr) e).target) && esPuro(((Ast.IndexExpr) e).index);
        if (e instanceof Ast.UnaryExpr) return esPuro(((Ast.UnaryExpr) e).operand);
        if (e instanceof Ast.BinaryExpr) return esPuro(((Ast.BinaryExpr) e).left) && esPuro(((Ast.BinaryExpr) e).right);
        return false;
    }

    /** #325 — cada `Thread(obj::metodo(args))` VÁLIDO del módulo, en orden de
     *  análisis. El emisor sintetiza una subclase de Thread por entrada, y las
     *  clases hay que emitirlas antes que el código que las construye. Se
     *  recoge aquí en vez de rastrear el AST otra vez porque el semántico ya
     *  pasa por todas las expresiones: un recorrido menos que mantener. */
    public final List<Ast.BoundCallExpr> boundCalls = new ArrayList<>();

    /** H5.c — `kind` resuelto para cada `raise`: el ORDINAL del evento en la
     *  jerarquía (los de la base primero, luego los propios en orden de
     *  declaración). Único dentro de la clase, que es todo lo que hace falta:
     *  el handler discrimina por (sender, kind) y el sender ya dice la clase. */
    public final Map<Ast.Node, Integer> eventKinds = new IdentityHashMap<>();

    /** V8 #771 — `ev == null` y `ev != null` con un evento, dentro de su clase: «¿tiene suscriptor?». El emisor lee el
     *  campo del receptor del evento y lo compara con null. El evento, en exprSymbols (el lado que no es el null). */
    public final java.util.Set<Ast.BinaryExpr> eventosNulos =
            java.util.Collections.newSetFromMap(new IdentityHashMap<Ast.BinaryExpr, Boolean>());

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
