// ============================================================
// Ast.cs
// Definiciones de los nodos del AST de BASICPLUS.
//
// Diseño:
//   - 'AstNode' es la raíz abstracta. Todos los nodos llevan
//     línea y columna del primer token que los originó.
//   - Tres interfaces-marcador clasifican los nodos por dónde
//     son válidos:
//       * ITopLevelDecl : declaraciones a nivel módulo o clase.
//       * IStmt         : sentencias dentro de un cuerpo.
//       * IExpr         : expresiones.
//   - Los records que pueden aparecer en dos sitios (VarDecl y
//     ConstDecl: como declaración o como sentencia) implementan
//     las dos interfaces.
// ============================================================

namespace BasicPlus.Lexer;

// -------------------- Raíz --------------------
public abstract record AstNode(int Line, int Column);

// -------------------- Marcadores --------------------
public interface ITopLevelDecl { }   // miembro de módulo o clase
public interface IStmt { }           // sentencia
public interface IExpr { }           // expresión

// -------------------- Módulo --------------------
public sealed record ModuleNode(
    string Name,
    List<ImportNode> Imports,
    List<ITopLevelDecl> Defs,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record ImportNode(
    List<string> Path,
    int Line, int Column
) : AstNode(Line, Column);

// -------------------- Tipos --------------------
public abstract record TypeRef(int Line, int Column) : AstNode(Line, Column);

public sealed record SimpleTypeRef(
    string Name,                 // 'integer' | 'float' | ... | nombre de clase/enum
    int Line, int Column
) : TypeRef(Line, Column);

public sealed record ArrayTypeRef(
    TypeRef Element,
    IExpr? Size,                 // null si no se especifica tamaño
    int Line, int Column
) : TypeRef(Line, Column);

// -------------------- Helpers compartidos --------------------
public sealed record DeclName(
    string? ClassQualifier,      // null para no estático; "Mi" para 'var Mi.x'
    string Name,
    int Line, int Column
) : AstNode(Line, Column)
{
    public bool IsStatic => ClassQualifier is not null;
}

public sealed record Param(
    string Name,
    TypeRef Type,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record EnumValue(
    string Name,
    long? ExplicitValue,         // null si va por defecto
    int Line, int Column
) : AstNode(Line, Column);

public sealed record GetterDef(
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record SetterDef(
    string ParamName,            // el set requiere parámetro explícito
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column);

// -------------------- Declaraciones (top-level y miembros) --------------------
public sealed record ConstDecl(
    bool IsPublic,
    DeclName Name,
    TypeRef? Type,               // opcional; se infiere si null
    IExpr Value,
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl, IStmt;

public sealed record VarDecl(
    bool IsPublic,
    List<DeclName> Names,        // soporta 'var x, y, z: integer'
    TypeRef Type,
    IExpr? Init,
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl, IStmt;

public sealed record FuncDef(
    bool IsPublic,
    bool IsFinal,
    DeclName Name,
    List<Param> Params,
    TypeRef? ReturnType,
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl;

public sealed record PropertyDef(
    bool IsPublic,
    bool IsFinal,
    DeclName Name,
    TypeRef Type,
    IExpr? Init,
    GetterDef? Getter,           // null en forma corta
    SetterDef? Setter,           // null en forma corta o si se omite
    bool IsShortForm,            // true => get/set implícitos
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl;

public sealed record ClassDef(
    bool IsPublic,
    string Name,
    string? BaseClass,           // 'extends X' opcional
    List<ITopLevelDecl> Members, // const, var, property, function (miembros)
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl;

public sealed record EnumDef(
    bool IsPublic,
    string Name,
    List<EnumValue> Values,
    int Line, int Column
) : AstNode(Line, Column), ITopLevelDecl;

// -------------------- Sentencias --------------------
public enum AssignOpKind { Assign, PlusAssign, MinusAssign }

public sealed record AssignStmt(
    IExpr Target,                // primary
    AssignOpKind Op,
    IExpr Value,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record IfClause(
    IExpr Condition,
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record IfStmt(
    IfClause Then,
    List<IfClause> ElseIfs,
    List<IStmt>? Else,           // null si no hay 'else'
    bool IsSingleLine,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record CaseClause(
    List<IExpr> Values,
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record SwitchStmt(
    IExpr Subject,
    List<CaseClause> Cases,
    List<IStmt>? Default,        // null si no hay 'default'
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record WhileStmt(
    IExpr Condition,
    List<IStmt> Body,
    bool IsSingleLine,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record DoLoopStmt(
    List<IStmt> Body,
    IExpr? Condition,            // null => bucle infinito
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public abstract record ForRange(int Line, int Column) : AstNode(Line, Column);

public sealed record ForNumericRange(
    IExpr From,
    IExpr To,
    IExpr? Step,                 // null si no se especifica 'step'
    int Line, int Column
) : ForRange(Line, Column);

public sealed record ForInRange(
    IExpr Iterable,
    int Line, int Column
) : ForRange(Line, Column);

public sealed record ForStmt(
    string IteratorName,
    ForRange Range,
    List<IStmt> Body,
    bool IsSingleLine,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record CatchClause(
    string? VarName,             // null si 'catch' a secas (sin enlazar)
    string? ExceptionType,       // null si 'catch err' (sin tipo)
    List<IStmt> Body,
    int Line, int Column
) : AstNode(Line, Column);

public sealed record TryStmt(
    List<IStmt> Body,
    List<CatchClause> Catches,
    List<IStmt>? Finally,        // null si no hay 'finally'
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record ReturnStmt(
    IExpr? Value,                // null si 'return' a secas
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record ThrowStmt(
    IExpr Value,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public enum PrintSep { None, Comma, Semicolon }

public sealed record PrintItem(
    PrintSep LeadingSep,         // None solo en el primer item
    IExpr? Expr                  // null permitido tras separador (espacio sin valor)
);

public sealed record PrintStmt(
    List<PrintItem> Items,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

public sealed record BreakStmt(int Line, int Column)    : AstNode(Line, Column), IStmt;
public sealed record ContinueStmt(int Line, int Column) : AstNode(Line, Column), IStmt;

public sealed record ExprStmt(
    IExpr Expr,
    int Line, int Column
) : AstNode(Line, Column), IStmt;

// -------------------- Expresiones --------------------
public sealed record IntLitExpr   (long   Value, int Line, int Column) : AstNode(Line, Column), IExpr;
public sealed record FloatLitExpr (double Value, int Line, int Column) : AstNode(Line, Column), IExpr;
public sealed record StringLitExpr(string Value, int Line, int Column) : AstNode(Line, Column), IExpr;
public sealed record BoolLitExpr  (bool   Value, int Line, int Column) : AstNode(Line, Column), IExpr;
public sealed record NullLitExpr  (int Line, int Column)               : AstNode(Line, Column), IExpr;
public sealed record ThisExpr     (int Line, int Column)               : AstNode(Line, Column), IExpr;
public sealed record SuperExpr    (int Line, int Column)               : AstNode(Line, Column), IExpr;     // para super.foo
public sealed record FieldExpr    (int Line, int Column)               : AstNode(Line, Column), IExpr;     // contextual

public sealed record SuperCallExpr(
    List<IExpr> Args,            // super(args) — solo válido como primera sentencia de constructor
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record IdentifierExpr(
    string Name,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record ArrayLitExpr(
    List<IExpr> Elements,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record ParenExpr(
    IExpr Inner,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record MemberAccessExpr(
    IExpr Target,
    string Member,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record IndexExpr(
    IExpr Target,
    IExpr Index,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record CallExpr(
    IExpr Callee,                // IdentifierExpr o MemberAccessExpr (o algo más exótico)
    List<IExpr> Args,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record UnaryExpr(
    string Op,                   // "-" o "not"
    IExpr Operand,
    int Line, int Column
) : AstNode(Line, Column), IExpr;

public sealed record BinaryExpr(
    string Op,                   // "+", "-", "*", "/", "mod", "shl", "shr",
                                 // "&", "|", "xor", "and", "or",
                                 // "==", "!=", "<", ">", "<=", ">="
    IExpr Left,
    IExpr Right,
    int Line, int Column
) : AstNode(Line, Column), IExpr;
