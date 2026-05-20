// ============================================================
// Symbols.cs
// Tabla de símbolos y representación de los nombres declarados
// (módulos, clases, funciones, propiedades, vars, consts, enums,
// parámetros, locales).
//
// Los símbolos son CLASES (no records) porque tienen identidad y
// referencias cíclicas (una clase apunta a su tipo, su tipo apunta
// a la clase).
// ============================================================

namespace BasicPlus.Lexer;

public abstract class Symbol
{
    public string Name { get; }
    public int Line { get; }
    public int Column { get; }
    public AstNode? Decl { get; init; }   // nodo del AST que originó el símbolo

    protected Symbol(string name, int line, int column)
    {
        Name = name; Line = line; Column = column;
    }

    public override string ToString() => $"{GetType().Name} '{Name}'";
}

// ============================================================
// Módulo (raíz)
// ============================================================
public sealed class ModuleSymbol : Symbol
{
    public Scope Members { get; } = new(name: "module");
    public FunctionSymbol? Initializer { get; set; }   // función con el mismo nombre del módulo
    public FunctionSymbol? MainFunction  { get; set; } // 'Main' (relevante si es módulo principal)
    public ModuleSymbol(string name, int line, int col) : base(name, line, col) { }
}

// ============================================================
// Clase
// ============================================================
public sealed class ClassSymbol : Symbol
{
    public bool IsPublic { get; }
    public Scope InstanceMembers { get; } = new(name: "instance");
    public Scope StaticMembers   { get; } = new(name: "static");
    public ClassSymbol? BaseClass { get; set; }     // resuelto en pase 2
    public string? BaseClassName  { get; }          // nombre crudo de 'extends X' (null si no hay)
    public FunctionSymbol? Constructor { get; set; }
    public ClassDef AstNode { get; }

    public ClassSymbol(string name, bool isPublic, string? baseClassName, ClassDef ast, int line, int col)
        : base(name, line, col)
    {
        IsPublic = isPublic;
        BaseClassName = baseClassName;
        AstNode = ast;
        Decl = ast;
    }

    /// <summary>Busca un miembro de instancia subiendo por la cadena de herencia.</summary>
    public Symbol? LookupInstance(string name)
    {
        var c = this;
        while (c is not null)
        {
            if (c.InstanceMembers.TryLookup(name, out var s)) return s;
            c = c.BaseClass;
        }
        return null;
    }

    /// <summary>Busca un miembro estático subiendo por la cadena de herencia.</summary>
    public Symbol? LookupStatic(string name)
    {
        var c = this;
        while (c is not null)
        {
            if (c.StaticMembers.TryLookup(name, out var s)) return s;
            c = c.BaseClass;
        }
        return null;
    }

    /// <summary>true si this hereda (transitivamente) de other.</summary>
    public bool IsSubclassOf(ClassSymbol other)
    {
        var c = BaseClass;
        while (c is not null)
        {
            if (ReferenceEquals(c, other)) return true;
            c = c.BaseClass;
        }
        return false;
    }

    /// <summary>Busca el constructor más cercano subiendo por la jerarquía.</summary>
    public FunctionSymbol? FindConstructor()
    {
        var c = this;
        while (c is not null)
        {
            if (c.Constructor is not null) return c.Constructor;
            c = c.BaseClass;
        }
        return null;
    }
}

// ============================================================
// Funciones (método o función a nivel módulo)
// ============================================================
public sealed class FunctionSymbol : Symbol
{
    public bool IsPublic { get; }
    public bool IsFinal  { get; set; }
    public bool IsStatic { get; }
    public ClassSymbol? OwnerClass { get; }   // null = función a nivel módulo
    public List<ParamSymbol> Params { get; }  = new();
    public BpType? ReturnType { get; set; }   // null = sin retorno (void implícito)
    public bool IsConstructor { get; set; }   // función con el mismo nombre que su clase
    public bool IsModuleInitializer { get; set; }
    public FuncDef AstNode { get; }

    public FunctionSymbol(string name, bool isPublic, bool isFinal, bool isStatic, ClassSymbol? owner, FuncDef ast)
        : base(name, ast.Line, ast.Column)
    {
        IsPublic = isPublic; IsFinal = isFinal; IsStatic = isStatic;
        OwnerClass = owner; AstNode = ast; Decl = ast;
    }

    /// <summary>Compara solo nombre y firma de parámetros (no el tipo de retorno).</summary>
    public bool HasSameSignatureAs(FunctionSymbol other)
    {
        if (Name != other.Name) return false;
        if (Params.Count != other.Params.Count) return false;
        for (int i = 0; i < Params.Count; i++)
            if (Params[i].Type is null || other.Params[i].Type is null
                || !Params[i].Type!.SameAs(other.Params[i].Type!))
                return false;
        return true;
    }
}

public sealed class ParamSymbol : Symbol
{
    public BpType? Type { get; set; }
    public ParamSymbol(string name, int line, int col) : base(name, line, col) { }
}

// ============================================================
// Variables, constantes, propiedades
// ============================================================
public sealed class VarSymbol : Symbol
{
    public bool IsPublic { get; }
    public bool IsStatic { get; }
    public ClassSymbol? OwnerClass { get; }
    public BpType? Type { get; set; }
    public bool IsLocal { get; }              // true si es local de función / parámetro / for var
    public VarSymbol(string name, bool isPublic, bool isStatic, ClassSymbol? owner, bool isLocal, int line, int col)
        : base(name, line, col)
    {
        IsPublic = isPublic; IsStatic = isStatic; OwnerClass = owner; IsLocal = isLocal;
    }
}

public sealed class ConstSymbol : Symbol
{
    public bool IsPublic { get; }
    public bool IsStatic { get; }
    public ClassSymbol? OwnerClass { get; }
    public BpType? Type { get; set; }
    public ConstSymbol(string name, bool isPublic, bool isStatic, ClassSymbol? owner, int line, int col)
        : base(name, line, col)
    {
        IsPublic = isPublic; IsStatic = isStatic; OwnerClass = owner;
    }
}

public sealed class PropertySymbol : Symbol
{
    public bool IsPublic { get; }
    public bool IsFinal  { get; set; }
    public bool IsStatic { get; }
    public ClassSymbol? OwnerClass { get; }
    public BpType? Type { get; set; }
    public bool HasCustomGetter { get; }
    public bool HasCustomSetter { get; }
    public bool IsShortForm { get; }
    public PropertyDef AstNode { get; }
    public PropertySymbol(string name, bool isPublic, bool isFinal, bool isStatic,
                         ClassSymbol? owner, PropertyDef ast)
        : base(name, ast.Line, ast.Column)
    {
        IsPublic = isPublic; IsFinal = isFinal; IsStatic = isStatic; OwnerClass = owner;
        HasCustomGetter = ast.Getter is not null;
        HasCustomSetter = ast.Setter is not null;
        IsShortForm = ast.IsShortForm;
        AstNode = ast; Decl = ast;
    }
}

// ============================================================
// Enums
// ============================================================
public sealed class EnumSymbol : Symbol
{
    public bool IsPublic { get; }
    public Dictionary<string, long> Values { get; } = new();
    public EnumSymbol(string name, bool isPublic, int line, int col) : base(name, line, col)
    {
        IsPublic = isPublic;
    }
}

// ============================================================
// Scope: tabla mutable name -> symbol con padre opcional
// ============================================================
public sealed class Scope
{
    public string Tag { get; }          // etiqueta de depuración
    public Scope? Parent { get; }
    private readonly Dictionary<string, Symbol> _symbols = new(StringComparer.Ordinal);

    public Scope(string name, Scope? parent = null) { Tag = name; Parent = parent; }

    public IEnumerable<Symbol> Symbols => _symbols.Values;

    /// <summary>Define un símbolo en este scope. Devuelve false si ya existía.</summary>
    public bool TryDefine(Symbol s)
    {
        if (_symbols.ContainsKey(s.Name)) return false;
        _symbols[s.Name] = s;
        return true;
    }

    /// <summary>Busca un símbolo solo en este scope.</summary>
    public bool TryLookup(string name, out Symbol symbol) =>
        _symbols.TryGetValue(name, out symbol!);

    /// <summary>Busca un símbolo subiendo por la cadena de scopes.</summary>
    public Symbol? Resolve(string name)
    {
        var s = this;
        while (s is not null)
        {
            if (s._symbols.TryGetValue(name, out var sym)) return sym;
            s = s.Parent;
        }
        return null;
    }
}
