// ============================================================
// BpType.cs
// Sistema de tipos resueltos (semántico) de BASICPLUS.
//
// Diferenciamos tipo "BpType" del nodo "TypeRef" del AST: TypeRef
// es la sintaxis (lo que el usuario escribió), BpType es lo que
// el analizador semántico resuelve a partir de él.
// ============================================================

namespace BasicPlus.Lexer;

public abstract class BpType
{
    public abstract string Display { get; }

    /// <summary>true si this y other denotan exactamente el mismo tipo.</summary>
    public abstract bool SameAs(BpType other);

    /// <summary>
    /// true si una expresión de tipo 'source' puede asignarse a una
    /// variable de tipo 'this' (o pasarse como argumento, devolverse).
    /// Cubre la jerarquía de clases (una subclase es asignable a su base)
    /// y la nullabilidad (null se acepta en referencias, no en escalares).
    /// </summary>
    public abstract bool IsAssignableFrom(BpType source);

    /// <summary>true para integer/float/string/boolean.</summary>
    public virtual bool IsScalar => false;

    /// <summary>true para clases y arrays (admiten null).</summary>
    public virtual bool IsReference => false;

    /// <summary>true para integer y float.</summary>
    public virtual bool IsNumeric => false;

    /// <summary>true si es enum.</summary>
    public virtual bool IsEnum => false;

    public override string ToString() => Display;
}

// ---------- Tipos primitivos ----------
public sealed class PrimitiveType : BpType
{
    public enum Kind { Integer, Float, String, Boolean }

    public Kind Tag { get; }
    private PrimitiveType(Kind k) { Tag = k; }

    public static readonly PrimitiveType Integer = new(Kind.Integer);
    public static readonly PrimitiveType Float   = new(Kind.Float);
    public static readonly PrimitiveType String  = new(Kind.String);
    public static readonly PrimitiveType Boolean = new(Kind.Boolean);

    public override string Display => Tag.ToString().ToLowerInvariant();
    public override bool IsScalar  => true;
    public override bool IsNumeric => Tag is Kind.Integer or Kind.Float;

    public override bool SameAs(BpType other) =>
        other is PrimitiveType p && p.Tag == Tag;

    public override bool IsAssignableFrom(BpType source)
    {
        if (source is ErrorType) return true; // pacificamos cascada
        if (SameAs(source)) return true;
        // Promoción integer -> float (asignación numérica permisiva).
        if (Tag == Kind.Float && source is PrimitiveType { Tag: Kind.Integer })
            return true;
        return false;
    }
}

// ---------- Clases ----------
public sealed class ClassType : BpType
{
    public ClassSymbol Class { get; }
    public ClassType(ClassSymbol cls) { Class = cls; }

    public override string Display => Class.Name;
    public override bool IsReference => true;

    public override bool SameAs(BpType other) =>
        other is ClassType c && ReferenceEquals(c.Class, Class);

    public override bool IsAssignableFrom(BpType source)
    {
        if (source is ErrorType) return true;
        if (source is NullType) return true;
        if (source is not ClassType c) return false;
        // Subclase es asignable a la base (covariante en clases).
        var cur = c.Class;
        while (cur is not null)
        {
            if (ReferenceEquals(cur, Class)) return true;
            cur = cur.BaseClass;
        }
        return false;
    }
}

// ---------- Enums ----------
public sealed class EnumType : BpType
{
    public EnumSymbol Enum { get; }
    public EnumType(EnumSymbol e) { Enum = e; }

    public override string Display => Enum.Name;
    public override bool IsEnum => true;
    public override bool IsScalar => true;   // un enum es un entero con nombre

    public override bool SameAs(BpType other) =>
        other is EnumType e && ReferenceEquals(e.Enum, Enum);

    public override bool IsAssignableFrom(BpType source)
    {
        if (source is ErrorType) return true;
        if (SameAs(source)) return true;
        return false;
    }
}

// ---------- Arrays ----------
public sealed class ArrayType : BpType
{
    public BpType Element { get; }
    public ArrayType(BpType element) { Element = element; }

    public override string Display => $"{Element.Display}[]";
    public override bool IsReference => true;

    public override bool SameAs(BpType other) =>
        other is ArrayType a && a.Element.SameAs(Element);

    public override bool IsAssignableFrom(BpType source)
    {
        if (source is ErrorType) return true;
        if (source is NullType) return true;
        if (source is ArrayType a) return Element.SameAs(a.Element); // invariante
        return false;
    }
}

// ---------- Tipo del literal null ----------
public sealed class NullType : BpType
{
    public static readonly NullType Instance = new();
    private NullType() { }
    public override string Display => "null";
    public override bool SameAs(BpType other) => other is NullType;
    public override bool IsAssignableFrom(BpType source) =>
        source is NullType || source is ErrorType;
}

// ---------- Tipo void (return sin valor, sentencias) ----------
public sealed class VoidType : BpType
{
    public static readonly VoidType Instance = new();
    private VoidType() { }
    public override string Display => "void";
    public override bool SameAs(BpType other) => other is VoidType;
    public override bool IsAssignableFrom(BpType source) =>
        source is VoidType || source is ErrorType;
}

// ---------- Tipo de error (recuperación tras error semántico) ----------
public sealed class ErrorType : BpType
{
    public static readonly ErrorType Instance = new();
    private ErrorType() { }
    public override string Display => "<error>";
    public override bool SameAs(BpType other) => true;
    public override bool IsAssignableFrom(BpType source) => true;
}
