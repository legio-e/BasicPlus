// Las expresiones y los tipos de C# en JC (V8/H6, docs/V8_IDEAS.md §E9). Los tipos de Roslyn dicen dónde va cada
// reescritura: un miembro de la clase lleva `this.` o el nombre de su clase delante; una caja, que en BP es a mano, no
// se tapa; lo que el traductor todavía no escribe (char, enum…) se rechaza por su tipo, antes de copiar nada.
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// Una expresión en su sitio: con la conversión que C# le aplica ahí (una caja, una conversión propia…).
        string Expr(ExpressionSyntax e)
        {
            var conv = m.GetConversion(e);
            if (conv.IsBoxing || conv.IsUnboxing) return Rechaza(e, Porque.Pendiente("las cajas (object)"));
            if (conv.IsUserDefined) return Rechaza(e, Porque.H7(Porque.Operadores));
            if (conv.IsNullable) return Rechaza(e, Porque.Fuera(Porque.Nullable));
            if (conv.IsEnumeration) return Rechaza(e, Porque.Pendiente("enum"));
            if (conv.IsMethodGroup) return Rechaza(e, Porque.H7(Porque.Delegados));
            // una constante que cabe en un byte, un short…: JC la acepta en una declaración o una asignación, no en un
            // argumento ni en un return
            if (conv.IsConstantExpression && EsEstrecho(m.GetTypeInfo(e).ConvertedType)
                && (e.Parent is ArgumentSyntax || e.Parent is ReturnStatementSyntax || e.Parent is ArrowExpressionClauseSyntax))
                return Rechaza(e, Porque.H7(Porque.ConstanteEstrecha));
            return ExprBase(e);
        }

        static bool EsEstrecho(ITypeSymbol t)
        {
            switch (t?.SpecialType)
            {
                case SpecialType.System_SByte:
                case SpecialType.System_Byte:
                case SpecialType.System_Int16:
                case SpecialType.System_UInt16: return true;
                default: return false;
            }
        }

        static bool EsBool(ITypeSymbol t) => t != null && t.SpecialType == SpecialType.System_Boolean;

        /// Una expresión por sí misma, sin la conversión de su sitio: la usan `print` y la suma de cadenas, donde C# mete
        /// un número en una caja para unirlo y JC lo une sin ella.
        string ExprBase(ExpressionSyntax e)
        {
            string porTipo = PorTipo(m.GetTypeInfo(e).Type);
            if (porTipo != null) return Rechaza(e, porTipo);
            switch (e)
            {
                case LiteralExpressionSyntax l: return Literal(l);
                case IdentifierNameSyntax id: return Nombre(id);
                case MemberAccessExpressionSyntax ma when ma.IsKind(SyntaxKind.SimpleMemberAccessExpression): return Miembro(ma);
                case InvocationExpressionSyntax inv: return Llamada(inv);
                case ObjectCreationExpressionSyntax oc: return Nuevo(oc);
                case BinaryExpressionSyntax b: return Binaria(b);
                case PrefixUnaryExpressionSyntax pu: return Prefijo(pu);
                case PostfixUnaryExpressionSyntax po:
                    if (po.IsKind(SyntaxKind.SuppressNullableWarningExpression)) return Expr(po.Operand);
                    return Rechaza(po, Porque.H7(Porque.IncEnExpresion));
                case ParenthesizedExpressionSyntax p: return "(" + Expr(p.Expression) + ")";
                case CastExpressionSyntax c: return Conversion(c);
                case ConditionalExpressionSyntax c: return Expr(c.Condition) + " ? " + Expr(c.WhenTrue) + " : " + Expr(c.WhenFalse);
                case AssignmentExpressionSyntax a: return Rechaza(a, Porque.H7(Porque.IncEnExpresion));
                case ElementAccessExpressionSyntax ea: return Elemento(ea);
                case ThisExpressionSyntax _: return "this";
                default: return RechazaNodo(e);
            }
        }

        /// Una expresión como sentencia: una llamada, un `new`, una asignación o `x++` (`++x;` es `x++;`).
        string ExprSentencia(ExpressionSyntax e)
        {
            switch (e)
            {
                case AssignmentExpressionSyntax a:
                    if (a.IsKind(SyntaxKind.CoalesceAssignmentExpression)) return Rechaza(a, Porque.H7(Porque.NuloYPunto));
                    if (a.IsKind(SyntaxKind.UnsignedRightShiftAssignmentExpression)) return Rechaza(a, Porque.Pendiente(">>>="));
                    if (m.GetSymbolInfo(a.Left).Symbol is IEventSymbol) return Rechaza(a, Porque.H7(Porque.Delegados));
                    if ((a.IsKind(SyntaxKind.AndAssignmentExpression) || a.IsKind(SyntaxKind.OrAssignmentExpression)
                         || a.IsKind(SyntaxKind.ExclusiveOrAssignmentExpression)) && EsBool(m.GetTypeInfo(a.Left).Type))
                        return Rechaza(a, Porque.H7(Porque.BoolBits));
                    if (a.Left is TupleExpressionSyntax || a.Left is DeclarationExpressionSyntax) return Rechaza(a, Porque.Pendiente("desempaquetado"));
                    // `s += 5`, con `s` una cadena: sin la caja de C#, como en la suma
                    bool unir = a.IsKind(SyntaxKind.AddAssignmentExpression) && EsCadena(m.GetTypeInfo(a.Left).Type);
                    return Expr(a.Left) + " " + a.OperatorToken.Text + " " + (unir ? ExprBase(a.Right) : Expr(a.Right));
                case PostfixUnaryExpressionSyntax p when p.IsKind(SyntaxKind.PostIncrementExpression) || p.IsKind(SyntaxKind.PostDecrementExpression):
                    return Expr(p.Operand) + p.OperatorToken.Text;
                case PrefixUnaryExpressionSyntax p when p.IsKind(SyntaxKind.PreIncrementExpression) || p.IsKind(SyntaxKind.PreDecrementExpression):
                    return Expr(p.Operand) + p.OperatorToken.Text;
                case InvocationExpressionSyntax _:
                case ObjectCreationExpressionSyntax _:
                    return Expr(e);
                default:
                    return RechazaNodo(e);
            }
        }

        /// Lo que el traductor no escribe, o JC no tiene, por el tipo de una expresión: el motivo, o null.
        static string PorTipo(ITypeSymbol t)
        {
            if (t == null) return null;
            switch (t.SpecialType)
            {
                case SpecialType.System_Char: return Porque.Pendiente("char (como entero)");
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64:
                case SpecialType.System_Decimal:
                case SpecialType.System_IntPtr:
                case SpecialType.System_UIntPtr: return Porque.Fuera(Porque.SinSigno);
            }
            if (t.TypeKind == TypeKind.Enum) return Porque.Pendiente("enum");
            if (t.TypeKind == TypeKind.Dynamic) return Porque.Fuera(Porque.Dynamic);
            if (t.TypeKind == TypeKind.Pointer || t.TypeKind == TypeKind.FunctionPointer) return Porque.Fuera(Porque.Unsafe);
            if (t.TypeKind == TypeKind.Delegate) return Porque.H7(Porque.Delegados);
            if (t.OriginalDefinition.SpecialType == SpecialType.System_Nullable_T) return Porque.Fuera(Porque.Nullable);
            if (t is IArrayTypeSymbol at) return PorTipo(at.ElementType);
            return null;
        }

        static bool EsCadena(ITypeSymbol t) => t != null && t.SpecialType == SpecialType.System_String;

        string Literal(LiteralExpressionSyntax l)
        {
            switch (l.Kind())
            {
                case SyntaxKind.NumericLiteralExpression:
                {
                    var v = l.Token.Value;
                    if (v is uint || v is ulong || v is decimal) return Rechaza(l, Porque.Fuera(Porque.SinSigno));
                    string txt = l.Token.Text;
                    if (GuionesQueJcNoLee(txt)) return Rechaza(l, Porque.H7(Porque.Guiones));
                    return txt.StartsWith(".") ? "0" + txt : txt;      // `.5`: el lexer de JC quiere la cifra delante
                }
                case SyntaxKind.NullLiteralExpression when EsCadena(m.GetTypeInfo(l).ConvertedType):
                    return Rechaza(l, Porque.H7(Porque.NullCadena));
                case SyntaxKind.StringLiteralExpression:
                    // `@"…"` y `"""…"""`, que JC no tiene, como una cadena normal; las demás, tal cual (JC lee los
                    // escapes de C#, punto 19)
                    if (l.Token.Text.StartsWith("@") || l.Token.Text.StartsWith("\"\"\"")) return Cadena((string) l.Token.Value);
                    return l.Token.Text;
                case SyntaxKind.TrueLiteralExpression: return "true";
                case SyntaxKind.FalseLiteralExpression: return "false";
                case SyntaxKind.NullLiteralExpression: return "null";
                default: return RechazaNodo(l);
            }
        }

        /// El lexer de JC lee un `_` entre dos cifras (`1_000`), no varios seguidos, ni tras `0x`, ni en el exponente.
        static bool GuionesQueJcNoLee(string t)
        {
            if (t.IndexOf('_') < 0) return false;
            string l = t.ToLowerInvariant();
            if (l.Contains("__") || l.StartsWith("0x_") || l.StartsWith("0b_")) return true;
            int e = l.StartsWith("0x") ? -1 : l.IndexOf('e');
            return e >= 0 && l.IndexOf('_', e) >= 0;
        }

        string Nombre(IdentifierNameSyntax id)
        {
            var s = m.GetSymbolInfo(id).Symbol;
            switch (s)
            {
                case ILocalSymbol _:
                case IParameterSymbol _:
                    return Ident(s.Name);
                case IFieldSymbol f:
                    if (!DelPrograma(f)) return Rechaza(id, Porque.Biblioteca(NombreCompleto(f.ContainingType) + "." + f.Name));
                    CompruebaAcceso(f, id);
                    return (f.IsStatic || f.IsConst ? Receptor(f.ContainingType, id) : "this") + "." + Ident(f.Name);
                case IPropertySymbol p:
                    if (!DelPrograma(p)) return Rechaza(id, Porque.Biblioteca(NombreCompleto(p.ContainingType) + "." + p.Name));
                    CompruebaAcceso(p, id);
                    return (p.IsStatic ? Receptor(p.ContainingType, id) : "this") + "." + Ident(p.Name);
                case INamedTypeSymbol nt:
                    return Tipo(nt, id);
                case IMethodSymbol _:
                case IEventSymbol _:
                    return Rechaza(id, Porque.H7(Porque.Delegados));
                default:
                    return Rechaza(id, Porque.Pendiente("el nombre " + id.Identifier.ValueText));
            }
        }

        string Miembro(MemberAccessExpressionSyntax ma)
        {
            var s = m.GetSymbolInfo(ma).Symbol;
            switch (s)
            {
                case null: return Rechaza(ma, Porque.Pendiente("el miembro " + ma.Name.Identifier.ValueText));
                case INamedTypeSymbol nt: return Tipo(nt, ma);            // N.A, o A.B: una anidada
                case INamespaceSymbol _: return Rechaza(ma, Porque.Pendiente("un namespace como valor"));
            }
            if (!DelPrograma(s)) return BibliotecaMiembro(ma, s);
            if (s is IMethodSymbol || s is IEventSymbol) return Rechaza(ma, Porque.H7(Porque.Delegados));
            if (s is IPropertySymbol p && p.IsIndexer) return Rechaza(ma, Porque.H7(Porque.Indexadores));
            CompruebaAcceso(s, ma);
            if (s.IsStatic || (s is IFieldSymbol f && f.IsConst)) return Receptor(s.ContainingType, ma) + "." + Ident(s.Name);
            if (ma.Expression is BaseExpressionSyntax)
            {
                if (s is IPropertySymbol) return Rechaza(ma, Porque.Fallo("#744", Porque.SuperProp));
                return "base." + Ident(s.Name);
            }
            return Expr(ma.Expression) + "." + Ident(s.Name);
        }

        string Llamada(InvocationExpressionSyntax inv)
        {
            var ms = m.GetSymbolInfo(inv).Symbol as IMethodSymbol;
            if (ms == null)
            {
                var cv = m.GetConstantValue(inv);                        // nameof(x): su cadena
                if (cv.HasValue && cv.Value is string nom) return Cadena(nom);
                return Rechaza(inv, Porque.Pendiente("una llamada sin resolver"));
            }
            if (ms.MethodKind == MethodKind.DelegateInvoke) return Rechaza(inv, Porque.H7(Porque.Delegados));
            if (ms.MethodKind == MethodKind.LocalFunction) return Rechaza(inv, Porque.Pendiente("funciones locales"));
            if (ms.IsGenericMethod) return Rechaza(inv, Porque.H7(Porque.Genericos));
            if (ms.ReducedFrom != null || ms.IsExtensionMethod) return Rechaza(inv, Porque.Pendiente("métodos de extensión"));
            if (!DelPrograma(ms)) return BibliotecaLlamada(inv, ms);
            CompruebaAcceso(ms, inv);
            string args = Argumentos(inv.ArgumentList, ms);
            string nombre = NombreMetodo(ms);
            switch (inv.Expression)
            {
                case IdentifierNameSyntax _:
                    return (ms.IsStatic ? Receptor(ms.ContainingType, inv) : "this") + "." + nombre + "(" + args + ")";
                case MemberAccessExpressionSyntax ma:
                    if (ms.IsStatic) return Receptor(ms.ContainingType, inv) + "." + nombre + "(" + args + ")";
                    if (ma.Expression is BaseExpressionSyntax) return "base." + nombre + "(" + args + ")";
                    return Expr(ma.Expression) + "." + nombre + "(" + args + ")";
                default:
                    return Rechaza(inv, Porque.H7(Porque.Delegados));
            }
        }

        string Argumentos(BaseArgumentListSyntax args, IMethodSymbol ms)
        {
            if (args == null) return "";
            foreach (var a in args.Arguments)
            {
                if (a.NameColon != null) return Rechaza(a, Porque.Pendiente("argumentos con nombre"));
                if (!a.RefKindKeyword.IsKind(SyntaxKind.None)) return Rechaza(a, Porque.H7(Porque.RefOut));
            }
            if (Expandido(ms, args)) return Rechaza(args, Porque.Pendiente("params"));
            return string.Join(", ", args.Arguments.Select(a => Expr(a.Expression)));
        }

        /// ¿Una llamada a un método `params` con los argumentos sueltos (o ninguno), en vez del array?
        bool Expandido(IMethodSymbol ms, BaseArgumentListSyntax args)
        {
            if (ms == null || ms.Parameters.Length == 0 || !ms.Parameters.Last().IsParams) return false;
            int n = ms.Parameters.Length;
            if (args.Arguments.Count != n) return true;
            var convertido = m.GetTypeInfo(args.Arguments[n - 1].Expression).ConvertedType;
            return !SymbolEqualityComparer.Default.Equals(convertido, ms.Parameters.Last().Type);
        }

        string Nuevo(ObjectCreationExpressionSyntax oc)
        {
            var ctor = m.GetSymbolInfo(oc).Symbol as IMethodSymbol;
            var tipo = m.GetTypeInfo(oc).Type as INamedTypeSymbol;
            if (ctor == null || tipo == null) return Rechaza(oc, Porque.Pendiente("un new sin resolver"));
            if (oc.Initializer != null) return Rechaza(oc.Initializer, Porque.Pendiente("inicializadores de objetos"));
            if (!DelPrograma(tipo)) return BibliotecaNuevo(oc, ctor, tipo);
            CompruebaAcceso(ctor, oc);
            CompruebaOrden(tipo, oc);
            return "new " + Tipo(tipo, oc.Type) + "(" + Argumentos(oc.ArgumentList, ctor) + ")";
        }

        string Binaria(BinaryExpressionSyntax b)
        {
            switch (b.Kind())
            {
                case SyntaxKind.AsExpression:
                case SyntaxKind.CoalesceExpression:
                    return RechazaNodo(b);
                case SyntaxKind.UnsignedRightShiftExpression:
                    return Rechaza(b, Porque.Pendiente(">>>"));
                case SyntaxKind.IsExpression:
                {
                    var t = m.GetTypeInfo(b.Right).Type;
                    if (t == null || t.IsValueType || EsCadena(t)
                        || (!DelPrograma(t) && !EsExcepcionNet(t) && t.SpecialType != SpecialType.System_Object))
                        return Rechaza(b, Porque.Pendiente("is " + NombreCompleto(t)));
                    CompruebaOrden(t, b);
                    return Expr(b.Left) + " is " + Tipo(t, b.Right);
                }
                case SyntaxKind.BitwiseAndExpression:
                case SyntaxKind.BitwiseOrExpression:
                case SyntaxKind.ExclusiveOrExpression:
                    if (EsBool(m.GetTypeInfo(b.Left).Type)) return Rechaza(b, Porque.H7(Porque.BoolBits));
                    break;
            }
            // una cadena con lo que sea: C# mete lo que no es cadena en una caja para unirlo; JC lo une sin ella
            if (b.IsKind(SyntaxKind.AddExpression) && EsCadena(m.GetTypeInfo(b).Type))
                return ExprBase(b.Left) + " + " + ExprBase(b.Right);
            return Expr(b.Left) + " " + b.OperatorToken.Text + " " + Expr(b.Right);
        }

        string Prefijo(PrefixUnaryExpressionSyntax pu)
        {
            switch (pu.Kind())
            {
                case SyntaxKind.UnaryMinusExpression:
                {
                    // -2147483648 y -9223372036854775808: el literal, solo, no cabe (es uint, ulong); juntos, el mínimo
                    if (pu.Operand is LiteralExpressionSyntax lit)
                    {
                        if (lit.Token.Value is uint u && u == 2147483648u) return "(-2147483647 - 1)";
                        if (lit.Token.Value is ulong ul && ul == 9223372036854775808UL) return "(-9223372036854775807L - 1L)";
                    }
                    string x = Expr(pu.Operand);
                    return x.StartsWith("-") ? "- " + x : "-" + x;      // `- -x` no es `--x`
                }
                case SyntaxKind.LogicalNotExpression: return "!" + Expr(pu.Operand);
                case SyntaxKind.UnaryPlusExpression: return Expr(pu.Operand);    // +x es x
                case SyntaxKind.BitwiseNotExpression:
                    // JC no tiene el ~ de C# (BP no tiene el no de bits): ~x es x ^ -1, en complemento a dos lo mismo
                    return "(" + Expr(pu.Operand) + " ^ -1)";
                case SyntaxKind.PreIncrementExpression:
                case SyntaxKind.PreDecrementExpression:
                    return Rechaza(pu, Porque.H7(Porque.IncEnExpresion));
                default:
                    return RechazaNodo(pu);
            }
        }

        string Conversion(CastExpressionSyntax c)
        {
            var de = m.GetTypeInfo(c.Expression).Type;
            var a = m.GetTypeInfo(c).Type;
            if (a == null) return Rechaza(c, Porque.Pendiente("una conversión sin resolver"));
            if (EsCadena(a) && c.Expression is LiteralExpressionSyntax nulo && nulo.IsKind(SyntaxKind.NullLiteralExpression))
                return Rechaza(c, Porque.H7(Porque.NullCadena));                // `(string) null`
            if (de != null)
            {
                if (de.IsValueType != a.IsValueType) return Rechaza(c, Porque.Pendiente("las cajas (object)"));
                if (m.GetConversion(c).IsUserDefined) return Rechaza(c, Porque.H7(Porque.Operadores));
            }
            CompruebaOrden(a, c);
            return "(" + Tipo(a, c.Type) + ") " + Expr(c.Expression);
        }

        string Elemento(ElementAccessExpressionSyntax ea)
        {
            var t = m.GetTypeInfo(ea.Expression).Type;
            if (t is IArrayTypeSymbol)
            {
                if (ea.ArgumentList.Arguments.Count != 1) return Rechaza(ea, Porque.H7(Porque.ArraysVarias));
                return Expr(ea.Expression) + "[" + Expr(ea.ArgumentList.Arguments[0].Expression) + "]";
            }
            if (EsCadena(t)) return Rechaza(ea, Porque.Pendiente("s[i] (char)"));
            if (m.GetSymbolInfo(ea).Symbol is IPropertySymbol p && p.IsIndexer)
                return Rechaza(ea, DelPrograma(p) ? Porque.H7(Porque.Indexadores) : Porque.Biblioteca(NombreCompleto(p.ContainingType) + "[i]"));
            return RechazaNodo(ea);
        }

        /// Al sacar una clase anidada al módulo, lo privado de la de fuera deja de verse: en C# sí se ve.
        void CompruebaAcceso(ISymbol s, SyntaxNode donde)
        {
            if (claseActual == null || s.ContainingType == null || !DelPrograma(s)) return;
            var dueno = s.ContainingType.OriginalDefinition;
            var aqui = claseActual.OriginalDefinition;
            switch (s.DeclaredAccessibility)
            {
                case Accessibility.Private:
                    if (!SymbolEqualityComparer.Default.Equals(dueno, aqui))
                        Rechaza(donde, Porque.Pendiente("una clase anidada usa lo privado de la de fuera"));
                    break;
                case Accessibility.Protected:
                case Accessibility.ProtectedAndInternal:
                    if (!Hereda(aqui, dueno))
                        Rechaza(donde, Porque.Pendiente("una clase anidada usa lo protegido de la de fuera"));
                    break;
            }
        }

        static bool Hereda(INamedTypeSymbol hija, INamedTypeSymbol padre)
        {
            for (var b = hija; b != null; b = b.BaseType)
                if (SymbolEqualityComparer.Default.Equals(b.OriginalDefinition, padre)) return true;
            return false;
        }

        /// Un tipo de C# en JC.
        string Tipo(ITypeSymbol ty, SyntaxNode donde)
        {
            if (ty == null) return Rechaza(donde, Porque.Pendiente("un tipo sin resolver"));
            switch (ty.SpecialType)
            {
                case SpecialType.System_Int32: return "int";
                case SpecialType.System_Int64: return "long";
                case SpecialType.System_Int16: return "short";
                case SpecialType.System_Byte: return "byte";
                case SpecialType.System_SByte: return "sbyte";
                case SpecialType.System_UInt16: return "ushort";
                case SpecialType.System_Single: return "float";
                case SpecialType.System_Double: return "double";
                case SpecialType.System_Boolean: return "bool";
                case SpecialType.System_String: return "string";
                case SpecialType.System_Object: return "Object";
                case SpecialType.System_Void: return "void";
            }
            string p = PorTipo(ty);
            if (p != null) return Rechaza(donde, p);
            if (ty is IArrayTypeSymbol at)
            {
                if (at.Rank > 1) return Rechaza(donde, Porque.H7(Porque.ArraysVarias));
                if (at.ElementType is IArrayTypeSymbol) return Rechaza(donde, Porque.H7(Porque.ArraysDeArrays));
                return Tipo(at.ElementType, donde) + "[]";
            }
            if (ty is ITypeParameterSymbol) return Rechaza(donde, Porque.H7(Porque.Genericos));
            if (ty is INamedTypeSymbol nt)
            {
                if (nt.IsGenericType) return Rechaza(donde, Porque.H7(Porque.Genericos));
                if (DelPrograma(nt))
                {
                    switch (nt.TypeKind)
                    {
                        case TypeKind.Class: return NombreTipo(nt);
                        case TypeKind.Struct: return Rechaza(donde, Porque.H7(Porque.Struct));
                        case TypeKind.Interface: return Rechaza(donde, Porque.Pendiente("interfaces simples, como clases base"));
                        default: return Rechaza(donde, Porque.Pendiente("tipo " + nt.TypeKind));
                    }
                }
                if (EsExcepcionNet(nt)) return "Exception";
                return Rechaza(donde, Porque.Biblioteca("tipo " + NombreCompleto(nt)));
            }
            return Rechaza(donde, Porque.Pendiente("tipo " + NombreCompleto(ty)));
        }
    }
}
