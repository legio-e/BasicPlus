// ============================================================
// Ast.java
// Nodos del AST de BASICPLUS (port a Java 8 del Ast.cs en C#).
//
// Organización:
//   - Una clase contenedor 'Ast' (no instanciable).
//   - Clase abstracta 'Node' con line/column.
//   - Interfaces marcador: ITopLevelDecl, IStmt, IExpr.
//   - Clases concretas como nested static classes.
//
// Java 8 no tiene records: cada nodo es una clase final con
// campos public final inicializados por constructor.
// ============================================================
package basicplus.frontend;

import java.util.List;

public final class Ast {

    private Ast() {}

    // ============================================================
    // RAÍZ
    // ============================================================
    public static abstract class Node {
        public final int line;
        public final int column;
        protected Node(int line, int column) {
            this.line = line;
            this.column = column;
        }
    }

    // ---- Marcadores ----
    public interface ITopLevelDecl { }
    public interface IStmt         { }
    public interface IExpr         { }

    // ---- Enumeraciones auxiliares ----
    public enum AssignOpKind { ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN }
    public enum PrintSep     { NONE, COMMA, SEMICOLON }

    // ============================================================
    // MÓDULO
    // ============================================================
    public static final class ModuleNode extends Node {
        public final String library;              // "" si no se declara library
        public final String name;
        public final boolean isInterface;          // `module interface X ...`
        /**
         * Nombre cualificado del contrato padre.
         *   - Si !isInterface  → `module X implements <implementsName>`.
         *   - Si isInterface   → `module interface X extends <implementsName>` (interface inheritance).
         * Null en ambas variantes si no se declara nada.
         */
        public final String implementsName;
        public final List<ImportNode> imports;
        public final List<ITopLevelDecl> defs;
        public ModuleNode(String library, String name, List<ImportNode> imports, List<ITopLevelDecl> defs, int line, int column) {
            this(library, name, false, null, imports, defs, line, column);
        }
        public ModuleNode(String library, String name, boolean isInterface, String implementsName,
                          List<ImportNode> imports, List<ITopLevelDecl> defs, int line, int column) {
            super(line, column);
            this.library = library;
            this.name = name;
            this.isInterface = isInterface;
            this.implementsName = implementsName;
            this.imports = imports;
            this.defs = defs;
        }
    }

    public static final class ImportNode extends Node {
        public final List<String> path;           // segmentos: último = nombre de la interfaz (o módulo, si no hay binding)
        public final String fromPath;             // null si no hay 'from "..."'
        public final String boundImpl;            // nombre simple del módulo concreto en `import Iface:Impl`; null si import directo
        public ImportNode(List<String> path, String fromPath, int line, int column) {
            this(path, fromPath, null, line, column);
        }
        public ImportNode(List<String> path, String fromPath, String boundImpl, int line, int column) {
            super(line, column);
            this.path = path;
            this.fromPath = fromPath;
            this.boundImpl = boundImpl;
        }
    }

    // ============================================================
    // TIPOS
    // ============================================================
    public static abstract class TypeRef extends Node {
        protected TypeRef(int line, int column) { super(line, column); }
    }

    public static final class SimpleTypeRef extends TypeRef {
        public final String name;   // 'integer' | 'float' | ... | nombre de clase/enum
        public SimpleTypeRef(String name, int line, int column) {
            super(line, column);
            this.name = name;
        }
    }

    public static final class ArrayTypeRef extends TypeRef {
        public final TypeRef element;
        public final IExpr size;    // null si no se especifica tamaño
        public ArrayTypeRef(TypeRef element, IExpr size, int line, int column) {
            super(line, column);
            this.element = element;
            this.size = size;
        }
    }

    // ============================================================
    // HELPERS COMPARTIDOS
    // ============================================================
    public static final class DeclName extends Node {
        public final String classQualifier;  // null para no estático
        public final String name;
        public DeclName(String classQualifier, String name, int line, int column) {
            super(line, column);
            this.classQualifier = classQualifier;
            this.name = name;
        }
        public boolean isStatic() { return classQualifier != null; }
    }

    public static final class Param extends Node {
        public final String name;
        public final TypeRef type;
        public Param(String name, TypeRef type, int line, int column) {
            super(line, column);
            this.name = name;
            this.type = type;
        }
    }

    public static final class EnumValue extends Node {
        public final String name;
        public final Long explicitValue;   // null si va por defecto
        public EnumValue(String name, Long explicitValue, int line, int column) {
            super(line, column);
            this.name = name;
            this.explicitValue = explicitValue;
        }
    }

    public static final class GetterDef extends Node {
        public final List<IStmt> body;
        public GetterDef(List<IStmt> body, int line, int column) {
            super(line, column);
            this.body = body;
        }
    }

    public static final class SetterDef extends Node {
        public final String paramName;     // el set requiere parámetro explícito
        public final List<IStmt> body;
        public SetterDef(String paramName, List<IStmt> body, int line, int column) {
            super(line, column);
            this.paramName = paramName;
            this.body = body;
        }
    }

    // ============================================================
    // DECLARACIONES (top-level y miembros)
    // ============================================================
    public static final class ConstDecl extends Node implements ITopLevelDecl, IStmt {
        public final boolean isPublic;
        public final DeclName name;
        public final TypeRef type;     // null si se infiere
        public final IExpr value;
        public ConstDecl(boolean isPublic, DeclName name, TypeRef type, IExpr value, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.type = type;
            this.value = value;
        }
    }

    public static final class VarDecl extends Node implements ITopLevelDecl, IStmt {
        public final boolean isPublic;
        public final boolean isOwner;        // `var owner x: T := ...`
        public final List<DeclName> names;   // soporta 'var x, y, z: integer'
        public final TypeRef type;
        public final IExpr init;             // null si no hay inicializador
        public VarDecl(boolean isPublic, boolean isOwner, List<DeclName> names, TypeRef type, IExpr init, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isOwner = isOwner;
            this.names = names;
            this.type = type;
            this.init = init;
        }
    }

    public static final class FuncDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final boolean isFinal;
        /** Función `intrinsic`: sólo signature; el compilador inlinea opcodes en el call site. */
        public final boolean isIntrinsic;
        /** H3 #162 — función `native`: candidata a tener versión AOT C.
         *  El bytecode .mod se emite igual; el flag se propaga al .bpi
         *  y al emisor AOT (#157). El runtime ignora el flag — quien
         *  decide ejecutar AOT vs bytecode es el AOT registry tras link. */
        public final boolean isNative;
        public final DeclName name;
        public final List<Param> params;
        public final TypeRef returnType;     // null si void
        public final List<IStmt> body;
        public FuncDef(boolean isPublic, boolean isFinal, DeclName name, List<Param> params,
                       TypeRef returnType, List<IStmt> body, int line, int column) {
            this(isPublic, isFinal, false, false, name, params, returnType, body, line, column);
        }
        public FuncDef(boolean isPublic, boolean isFinal, boolean isIntrinsic, DeclName name,
                       List<Param> params, TypeRef returnType, List<IStmt> body, int line, int column) {
            this(isPublic, isFinal, isIntrinsic, false, name, params, returnType, body, line, column);
        }
        public FuncDef(boolean isPublic, boolean isFinal, boolean isIntrinsic, boolean isNative,
                       DeclName name, List<Param> params, TypeRef returnType, List<IStmt> body,
                       int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isIntrinsic = isIntrinsic;
            this.isNative = isNative;
            this.name = name;
            this.params = params;
            this.returnType = returnType;
            this.body = body;
        }
    }

    public static final class PropertyDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final boolean isFinal;
        public final boolean isOwner;        // `public property owner foo: T := ...`
        public final boolean isSync;         // `public sync property foo: T` — getter/setter envuelven con un Mutex compartido por instancia.
        public final DeclName name;
        public final TypeRef type;
        public final IExpr init;             // null si no hay inicializador
        public final GetterDef getter;       // null en forma corta o si se omite
        public final SetterDef setter;       // null en forma corta o si se omite
        public final boolean isShortForm;    // true => get/set implícitos
        public PropertyDef(boolean isPublic, boolean isFinal, boolean isOwner, boolean isSync, DeclName name, TypeRef type,
                           IExpr init, GetterDef getter, SetterDef setter, boolean isShortForm,
                           int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isOwner = isOwner;
            this.isSync = isSync;
            this.name = name;
            this.type = type;
            this.init = init;
            this.getter = getter;
            this.setter = setter;
            this.isShortForm = isShortForm;
        }
    }

    public static final class ClassDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final String name;
        public final String baseClass;        // null si no hay 'extends'
        public final List<ITopLevelDecl> members;
        public ClassDef(boolean isPublic, String name, String baseClass, List<ITopLevelDecl> members,
                        int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.baseClass = baseClass;
            this.members = members;
        }
    }

    public static final class EnumDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final String name;
        public final List<EnumValue> values;
        public EnumDef(boolean isPublic, String name, List<EnumValue> values, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.values = values;
        }
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    public static final class AssignStmt extends Node implements IStmt {
        public final IExpr target;     // primary
        public final AssignOpKind op;
        public final IExpr value;
        public AssignStmt(IExpr target, AssignOpKind op, IExpr value, int line, int column) {
            super(line, column);
            this.target = target;
            this.op = op;
            this.value = value;
        }
    }

    public static final class IfClause extends Node {
        public final IExpr condition;
        public final List<IStmt> body;
        public IfClause(IExpr condition, List<IStmt> body, int line, int column) {
            super(line, column);
            this.condition = condition;
            this.body = body;
        }
    }

    public static final class IfStmt extends Node implements IStmt {
        public final IfClause then_;
        public final List<IfClause> elseIfs;
        public final List<IStmt> else_;       // null si no hay 'else'
        public final boolean isSingleLine;
        public IfStmt(IfClause then_, List<IfClause> elseIfs, List<IStmt> else_, boolean isSingleLine,
                      int line, int column) {
            super(line, column);
            this.then_ = then_;
            this.elseIfs = elseIfs;
            this.else_ = else_;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class CaseClause extends Node {
        public final List<IExpr> values;
        public final List<IStmt> body;
        public CaseClause(List<IExpr> values, List<IStmt> body, int line, int column) {
            super(line, column);
            this.values = values;
            this.body = body;
        }
    }

    public static final class SwitchStmt extends Node implements IStmt {
        public final IExpr subject;
        public final List<CaseClause> cases;
        public final List<IStmt> defaultBody;   // null si no hay 'default'
        public SwitchStmt(IExpr subject, List<CaseClause> cases, List<IStmt> defaultBody,
                          int line, int column) {
            super(line, column);
            this.subject = subject;
            this.cases = cases;
            this.defaultBody = defaultBody;
        }
    }

    /**
     * Rama de un statement {@code parallel}: {@code case <var> := Thread(<stackSize>)}
     * seguido de un cuerpo. Cada rama se desazucara en una subclase anónima de
     * Thread cuyo run() ES el {@code body}.
     */
    public static final class ParallelBranch extends Node {
        public final String varName;        // ej. "t1"
        public final IExpr  stackSizeExpr;  // expresión int para super(stackSize)
        public final List<IStmt> body;      // cuerpo del run()
        /** Nombre de la clase sintetizada; lo fija el semántico. */
        public String synthesizedClassName;
        public ParallelBranch(String varName, IExpr stackSizeExpr, List<IStmt> body, int line, int column) {
            super(line, column);
            this.varName = varName;
            this.stackSizeExpr = stackSizeExpr;
            this.body = body;
        }
    }

    /**
     * Statement {@code parallel ... endpar}: arranca N threads, ejecuta el bloque
     * default (si existe) en el thread llamador, y joinea todos al salir. Es
     * azúcar sintáctico: cada rama → subclase anónima de Thread.
     */
    public static final class ParallelStmt extends Node implements IStmt {
        public final List<ParallelBranch> branches;
        public final List<IStmt> defaultBody;   // null si no hay 'default'
        public ParallelStmt(List<ParallelBranch> branches, List<IStmt> defaultBody, int line, int column) {
            super(line, column);
            this.branches = branches;
            this.defaultBody = defaultBody;
        }
    }

    public static final class WhileStmt extends Node implements IStmt {
        public final IExpr condition;
        public final List<IStmt> body;
        public final boolean isSingleLine;
        public WhileStmt(IExpr condition, List<IStmt> body, boolean isSingleLine, int line, int column) {
            super(line, column);
            this.condition = condition;
            this.body = body;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class DoLoopStmt extends Node implements IStmt {
        public final List<IStmt> body;
        public final IExpr condition;       // null => bucle infinito
        public DoLoopStmt(List<IStmt> body, IExpr condition, int line, int column) {
            super(line, column);
            this.body = body;
            this.condition = condition;
        }
    }

    public static abstract class ForRange extends Node {
        protected ForRange(int line, int column) { super(line, column); }
    }

    public static final class ForNumericRange extends ForRange {
        public final IExpr from;
        public final IExpr to;
        public final IExpr step;            // null si no hay 'step'
        public ForNumericRange(IExpr from, IExpr to, IExpr step, int line, int column) {
            super(line, column);
            this.from = from;
            this.to = to;
            this.step = step;
        }
    }

    public static final class ForInRange extends ForRange {
        public final IExpr iterable;
        public ForInRange(IExpr iterable, int line, int column) {
            super(line, column);
            this.iterable = iterable;
        }
    }

    public static final class ForStmt extends Node implements IStmt {
        public final String iteratorName;
        public final ForRange range;
        public final List<IStmt> body;
        public final boolean isSingleLine;
        public ForStmt(String iteratorName, ForRange range, List<IStmt> body, boolean isSingleLine,
                       int line, int column) {
            super(line, column);
            this.iteratorName = iteratorName;
            this.range = range;
            this.body = body;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class CatchClause extends Node {
        public final String varName;        // null si 'catch' a secas
        public final String exceptionType;  // null si 'catch err' (sin tipo)
        public final List<IStmt> body;
        public CatchClause(String varName, String exceptionType, List<IStmt> body, int line, int column) {
            super(line, column);
            this.varName = varName;
            this.exceptionType = exceptionType;
            this.body = body;
        }
    }

    public static final class TryStmt extends Node implements IStmt {
        public final List<IStmt> body;
        public final List<CatchClause> catches;
        public final List<IStmt> finallyBody;   // null si no hay 'finally'
        public TryStmt(List<IStmt> body, List<CatchClause> catches, List<IStmt> finallyBody,
                       int line, int column) {
            super(line, column);
            this.body = body;
            this.catches = catches;
            this.finallyBody = finallyBody;
        }
    }

    public static final class ReturnStmt extends Node implements IStmt {
        public final IExpr value;       // null si 'return' a secas
        public ReturnStmt(IExpr value, int line, int column) {
            super(line, column);
            this.value = value;
        }
    }

    public static final class ThrowStmt extends Node implements IStmt {
        public final IExpr value;
        public ThrowStmt(IExpr value, int line, int column) {
            super(line, column);
            this.value = value;
        }
    }

    public static final class PrintItem {
        public final PrintSep leadingSep;   // NONE solo en el primer item
        public final IExpr expr;            // null permitido tras separador
        public PrintItem(PrintSep leadingSep, IExpr expr) {
            this.leadingSep = leadingSep;
            this.expr = expr;
        }
    }

    public static final class PrintStmt extends Node implements IStmt {
        public final List<PrintItem> items;
        public PrintStmt(List<PrintItem> items, int line, int column) {
            super(line, column);
            this.items = items;
        }
    }

    public static final class BreakStmt extends Node implements IStmt {
        public BreakStmt(int line, int column) { super(line, column); }
    }

    public static final class ContinueStmt extends Node implements IStmt {
        public ContinueStmt(int line, int column) { super(line, column); }
    }

    public static final class ExprStmt extends Node implements IStmt {
        public final IExpr expr;
        public ExprStmt(IExpr expr, int line, int column) {
            super(line, column);
            this.expr = expr;
        }
    }

    // ============================================================
    // EXPRESIONES
    // ============================================================
    public static final class IntLitExpr extends Node implements IExpr {
        public final long value;
        public IntLitExpr(long value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class FloatLitExpr extends Node implements IExpr {
        public final double value;
        public FloatLitExpr(double value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class StringLitExpr extends Node implements IExpr {
        public final String value;
        public StringLitExpr(String value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class BoolLitExpr extends Node implements IExpr {
        public final boolean value;
        public BoolLitExpr(boolean value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class NullLitExpr extends Node implements IExpr {
        public NullLitExpr(int line, int column) { super(line, column); }
    }

    public static final class ThisExpr extends Node implements IExpr {
        public ThisExpr(int line, int column) { super(line, column); }
    }

    public static final class SuperExpr extends Node implements IExpr {     // para super.foo
        public SuperExpr(int line, int column) { super(line, column); }
    }

    public static final class FieldExpr extends Node implements IExpr {     // contextual
        public FieldExpr(int line, int column) { super(line, column); }
    }

    public static final class SuperCallExpr extends Node implements IExpr {
        public final List<IExpr> args;
        public SuperCallExpr(List<IExpr> args, int line, int column) {
            super(line, column);
            this.args = args;
        }
    }

    public static final class IdentifierExpr extends Node implements IExpr {
        public final String name;
        public IdentifierExpr(String name, int line, int column) {
            super(line, column);
            this.name = name;
        }
    }

    public static final class ArrayLitExpr extends Node implements IExpr {
        public final List<IExpr> elements;
        public ArrayLitExpr(List<IExpr> elements, int line, int column) {
            super(line, column);
            this.elements = elements;
        }
    }

    public static final class ParenExpr extends Node implements IExpr {
        public final IExpr inner;
        public ParenExpr(IExpr inner, int line, int column) {
            super(line, column);
            this.inner = inner;
        }
    }

    public static final class MemberAccessExpr extends Node implements IExpr {
        public final IExpr target;
        public final String member;
        public MemberAccessExpr(IExpr target, String member, int line, int column) {
            super(line, column);
            this.target = target;
            this.member = member;
        }
    }

    public static final class IndexExpr extends Node implements IExpr {
        public final IExpr target;
        public final IExpr index;
        public IndexExpr(IExpr target, IExpr index, int line, int column) {
            super(line, column);
            this.target = target;
            this.index = index;
        }
    }

    public static final class CallExpr extends Node implements IExpr {
        public final IExpr callee;
        public final List<IExpr> args;
        public CallExpr(IExpr callee, List<IExpr> args, int line, int column) {
            super(line, column);
            this.callee = callee;
            this.args = args;
        }
    }

    public static final class UnaryExpr extends Node implements IExpr {
        public final String op;       // "-" o "not"
        public final IExpr operand;
        public UnaryExpr(String op, IExpr operand, int line, int column) {
            super(line, column);
            this.op = op;
            this.operand = operand;
        }
    }

    public static final class BinaryExpr extends Node implements IExpr {
        public final String op;       // "+", "-", "*", "/", "mod", ..., "and", "or", "==", ...
        public final IExpr left;
        public final IExpr right;
        public BinaryExpr(String op, IExpr left, IExpr right, int line, int column) {
            super(line, column);
            this.op = op;
            this.left = left;
            this.right = right;
        }
    }

    /** Operador 'instanceof': la parte derecha NO es una expresión sino el
     *  nombre de una clase (resuelto en análisis semántico). Resultado boolean. */
    public static final class InstanceOfExpr extends Node implements IExpr {
        public final IExpr target;
        public final String typeName;
        public InstanceOfExpr(IExpr target, String typeName, int line, int column) {
            super(line, column);
            this.target = target;
            this.typeName = typeName;
        }
    }
}
