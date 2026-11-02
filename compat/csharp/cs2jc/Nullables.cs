// int? y los demás T? de C# en JC (V8/H6, 2-nov; Eduardo: «int? es Integer (la clase)»; docs/V8_IDEAS.md §E9). Un T?
// es una referencia a la caja de BP de su T —Integer, Long, Float, Double o Boolean—, o null; los enteros estrechos y
// el char, en un Integer. Lo que C# hace con ellos (los operadores «elevados»: con un null, la aritmética da null y <,
// >, <= y >=, false; == compara los valores, y dos null son iguales; un null se escribe vacío) lo hacen las funciones
// Nullable_* de CsSystem, una por operación, con los operandos ya en su caja: cada uno se calcula una vez, como en C#.
// `??`, con el ternario: la derecha sólo se calcula si hace falta.
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Operations;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// El T de un T?, o null si no es un T?.
        static ITypeSymbol DeNullable(ITypeSymbol t) =>
            t is INamedTypeSymbol nt && nt.OriginalDefinition.SpecialType == SpecialType.System_Nullable_T ? nt.TypeArguments[0] : null;

        static bool EsNullable(ITypeSymbol t) => DeNullable(t) != null;

        /// La caja de BP que guarda un T?: la de su T; la de los enteros estrechos y el char, un Integer. null si no hay.
        static string CajaDeNullable(ITypeSymbol t)
        {
            switch (DeNullable(t)?.SpecialType)
            {
                case SpecialType.System_Int32:
                case SpecialType.System_Int16:
                case SpecialType.System_SByte:
                case SpecialType.System_Byte:
                case SpecialType.System_UInt16:
                case SpecialType.System_Char: return "Integer";
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32: return "Long";             // un uint es un long (SinSigno.cs)
                case SpecialType.System_Single: return "Float";
                case SpecialType.System_Double: return "Double";
                case SpecialType.System_Boolean: return "Boolean";
                default: return null;
            }
        }

        /// ¿Un uint?, cuyas cuentas (+, -, *, <<, >>, ~, ++) piden el recorte a 32 bits que las de su caja, un Long, no
        /// hacen? Ésas, todavía no; comparar, /, %, &, | y ^ salen bien.
        static bool EsUIntNullable(ITypeSymbol t) => EsUInt(DeNullable(t));

        /// Por qué no se traduce un T? que no tiene caja (PorTipo): el de su T, o el de lo que es.
        static string PorNullable(ITypeSymbol u)
        {
            if (EsEnum(u)) return Porque.Pendiente("un enum que puede ser null (E?)");
            string p = PorTipo(u);
            if (p != null) return p;
            if (u.TypeKind == TypeKind.Struct && u.Locations.Any(l => l.IsInSource)) return Porque.H7(Porque.Struct);
            return Porque.DeNet("tipo " + NombreCompleto(u) + "?");
        }

        /// ¿Es `e` un null (`null`, o `(int?) null`)?
        static bool EsNull(ExpressionSyntax e)
        {
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            if (e is CastExpressionSyntax c) return EsNull(c.Expression);
            return e is LiteralExpressionSyntax l && l.IsKind(SyntaxKind.NullLiteralExpression);
        }

        /// Un valor que no es un T?, en la caja del T? `a`: `new Integer(x)`. Uno más estrecho entra solo (JC lo sube).
        string MeteEnCaja(ExpressionSyntax e, ITypeSymbol a)
        {
            string caja = CajaDeNullable(a);
            if (caja == null) return Rechaza(e, PorTipo(a) ?? Porque.Pendiente("un " + NombreCompleto(a)));
            return "new " + caja + "(" + ExprBase(e) + ")";
        }

        /// Un T? de una caja a otra (`int?` a `long?`, `long?` a `int?`…): la función de CsSystem que lo hace; nada si
        /// va al mismo T, o a `int?` desde un entero estrecho (la misma caja, y el valor cabe).
        string CambiaDeCaja(string x, ITypeSymbol de, ITypeSymbol a, SyntaxNode donde)
        {
            var u = DeNullable(de);
            var v = DeNullable(a);
            string cd = CajaDeNullable(de), ca = CajaDeNullable(a);
            if (cd == null || ca == null) return Rechaza(donde, PorTipo(cd == null ? de : a) ?? Porque.Pendiente("de " + NombreCompleto(de) + " a " + NombreCompleto(a)));
            if (SymbolEqualityComparer.Default.Equals(u, v)) return x;
            string fn;
            switch (v.SpecialType)
            {
                case SpecialType.System_Int32: fn = cd == "Integer" ? null : "Nullable_AInteger"; break;
                case SpecialType.System_Int64: fn = cd == "Long" ? null : "Nullable_ALong"; break;
                case SpecialType.System_Single: fn = "Nullable_AFloat"; break;
                case SpecialType.System_Double: fn = "Nullable_ADouble"; break;
                case SpecialType.System_Byte: fn = "Nullable_AByte"; break;
                case SpecialType.System_SByte: fn = "Nullable_ASByte"; break;
                case SpecialType.System_Int16: fn = "Nullable_AShort"; break;
                case SpecialType.System_UInt16:
                case SpecialType.System_Char: fn = "Nullable_AUShort"; break;
                case SpecialType.System_UInt32: fn = "Nullable_AUInt"; break;
                default: return Rechaza(donde, Porque.Pendiente("de " + NombreCompleto(de) + " a " + NombreCompleto(a)));
            }
            if (fn == null) return x;
            if ((cd == "Float" || cd == "Double")
                && ((ca == "Integer" && v.SpecialType != SpecialType.System_Int32) || v.SpecialType == SpecialType.System_UInt32))
                return Rechaza(donde, Porque.Pendiente("de " + NombreCompleto(de) + " a " + NombreCompleto(a)));
            imports.Add("CsSystem");
            return "CsSystem." + fn + "(" + x + ")";
        }

        /// Lo que C# convierte, en un sitio, de un T a un T?, o de un T? a otro (Conversion.IsNullable).
        string ConversionNullable(ExpressionSyntax e, ITypeSymbol de, ITypeSymbol a)
        {
            string p = PorTipo(a);
            if (p != null) return Rechaza(e, p);
            if (EsNullable(de)) return CambiaDeCaja(ExprBase(e), de, a, e);
            if (EsNullable(a)) return MeteEnCaja(e, a);
            return Rechaza(e, Porque.Pendiente("de " + NombreCompleto(de) + " a " + NombreCompleto(a)));
        }

        /// El valor de un T? (`x.Value`, `(int) x`): el de CsSystem, que lanza InvalidOperationException si es null, como
        /// .NET; uno estrecho, con su tipo.
        string ValorDeNullable(string x, ITypeSymbol de, SyntaxNode donde)
        {
            imports.Add("CsSystem");
            string v = "CsSystem.Nullable_Value(" + x + ")";
            var u = DeNullable(de);
            return EsEstrecho(u) ? "((" + Tipo(u, donde) + ") " + v + ")" : v;
        }

        /// Una conversión escrita en la que hay un T?: `(int?) x`, `(int?) null`, `(int?) o`, `(long?) x`, `(int) x`,
        /// `(long) x`, `(object) x`.
        string ConversionEscritaNullable(CastExpressionSyntax c, ITypeSymbol de, ITypeSymbol a)
        {
            if (EsNullable(a))
            {
                string p = PorTipo(a);
                if (p != null) return Rechaza(c, p);
                if (de == null) return "null";                                          // (int?) null
                if (EsNullable(de)) return CambiaDeCaja(Expr(c.Expression), de, a, c);
                if (!de.IsValueType)
                {
                    // (int?) o: sacarlo de su caja (si no es la suya, la conversión lanza, como en C#), o null
                    if (de.SpecialType != SpecialType.System_Object || ClaseDeCaja(DeNullable(a)) == null)
                        return Rechaza(c, Porque.Pendiente("sacar un " + NombreCompleto(a) + " de " + NombreCompleto(de)));
                    return "((" + CajaDeNullable(a) + ") " + Atomo(c.Expression) + ")";
                }
                return MeteEnCaja(c.Expression, a);
            }
            if (!a.IsValueType)
            {
                // (object) x: la misma referencia, si su caja es la de su T (la de un byte? no lo es: un Integer)
                if (a.SpecialType != SpecialType.System_Object || ClaseDeCaja(DeNullable(de)) == null)
                    return Rechaza(c, Porque.Pendiente("una caja de " + NombreCompleto(de) + " como " + NombreCompleto(a)));
                return Expr(c.Expression);
            }
            // (int) x, (long) x: su valor, y la conversión de C# de su T al otro
            var u = DeNullable(de);
            string v = ValorDeNullable(Expr(c.Expression), de, c);
            if (SymbolEqualityComparer.Default.Equals(u, a)) return v;
            if (EsEnum(a)) return Rechaza(c, Porque.Fuera(Porque.EnteroAEnum));
            if (a.SpecialType == SpecialType.System_Char) return "(((int) " + v + ") & 65535)";
            return "(" + Tipo(a, c.Type) + ") " + v;
        }

        /// ¿Va la operación por las de un T? (los operadores elevados de C#)? Sus operandos, ya convertidos, lo dicen.
        static bool EsOperacionNullable(IBinaryOperation bo) =>
            bo != null && bo.Type?.SpecialType != SpecialType.System_String
            && (bo.IsLifted || EsNullable(bo.LeftOperand.Type) || EsNullable(bo.RightOperand.Type));

        /// Las funciones de CsSystem de los operadores elevados, y las cajas que tienen.
        static string FuncionNullable(SyntaxKind k, string caja)
        {
            bool numero = caja == "Integer" || caja == "Long" || caja == "Float" || caja == "Double";
            bool entero = caja == "Integer" || caja == "Long";
            switch (k)
            {
                case SyntaxKind.AddExpression: case SyntaxKind.AddAssignmentExpression: return numero ? "Suma" : null;
                case SyntaxKind.SubtractExpression: case SyntaxKind.SubtractAssignmentExpression: return numero ? "Resta" : null;
                case SyntaxKind.MultiplyExpression: case SyntaxKind.MultiplyAssignmentExpression: return numero ? "Mult" : null;
                case SyntaxKind.DivideExpression: case SyntaxKind.DivideAssignmentExpression: return numero ? "Div" : null;
                case SyntaxKind.ModuloExpression: case SyntaxKind.ModuloAssignmentExpression: return entero ? "Resto" : null;
                case SyntaxKind.BitwiseAndExpression: case SyntaxKind.AndAssignmentExpression: return entero || caja == "Boolean" ? "Y" : null;
                case SyntaxKind.BitwiseOrExpression: case SyntaxKind.OrAssignmentExpression: return entero || caja == "Boolean" ? "O" : null;
                case SyntaxKind.ExclusiveOrExpression: case SyntaxKind.ExclusiveOrAssignmentExpression: return entero || caja == "Boolean" ? "OEx" : null;
                case SyntaxKind.LeftShiftExpression: case SyntaxKind.LeftShiftAssignmentExpression: return entero ? "Izq" : null;
                case SyntaxKind.RightShiftExpression: case SyntaxKind.RightShiftAssignmentExpression: return entero ? "Der" : null;
                case SyntaxKind.LessThanExpression: return numero ? "Menor" : null;
                case SyntaxKind.GreaterThanExpression: return numero ? "Mayor" : null;
                case SyntaxKind.LessThanOrEqualExpression: return numero ? "MenorIgual" : null;
                case SyntaxKind.GreaterThanOrEqualExpression: return numero ? "MayorIgual" : null;
                default: return null;
            }
        }

        /// Una operación con T? (BinariaNullable): `x == null`, la referencia; `==` y `!=`, Nullable_Igual; las demás,
        /// su función de CsSystem.
        string BinariaNullable(BinaryExpressionSyntax b, IBinaryOperation bo)
        {
            bool igual = b.IsKind(SyntaxKind.EqualsExpression);
            if (igual || b.IsKind(SyntaxKind.NotEqualsExpression))
            {
                var otro = EsNull(b.Right) ? b.Left : EsNull(b.Left) ? b.Right : null;
                if (otro != null)
                {
                    // con null: un T?, su referencia; algo que no puede ser null (un int, un enum), C# da siempre false
                    // (o true con !=), y si calcula algo, se calcula: entonces, todavía no
                    if (EsNullable(m.GetTypeInfo(otro).Type)) return Atomo(otro) + (igual ? " == null" : " != null");
                    if (EsSimple(otro)) return igual ? "false" : "true";
                    return Rechaza(b, Porque.Pendiente("comparar con null algo que no puede serlo, y que calcula"));
                }
                imports.Add("CsSystem");
                string f = "CsSystem.Nullable_Igual(" + Expr(b.Left) + ", " + Expr(b.Right) + ")";
                return igual ? f : "!" + f;
            }
            string caja = CajaDeNullable(bo.LeftOperand.Type);
            if (caja == null) return Rechaza(b, PorTipo(bo.LeftOperand.Type) ?? Porque.Pendiente(b.OperatorToken.Text + " con un " + NombreCompleto(bo.LeftOperand.Type)));
            string fn = FuncionNullable(b.Kind(), caja);
            if (fn == null) return Rechaza(b, Porque.Pendiente(b.OperatorToken.Text + " con un " + NombreCompleto(bo.LeftOperand.Type)));
            if (EsUIntNullable(bo.LeftOperand.Type) && (fn == "Suma" || fn == "Resta" || fn == "Mult" || fn == "Izq" || fn == "Der"))
                return Rechaza(b, Porque.Pendiente("+, -, *, << y >> con un uint? (el recorte a 32 bits)"));
            if (EsNull(b.Left) || EsNull(b.Right)) return Rechaza(b, Porque.Pendiente("un null suelto en una operación con un T?"));
            imports.Add("CsSystem");
            return "CsSystem.Nullable_" + fn + "(" + Expr(b.Left) + ", " + Expr(b.Right) + ")";
        }

        /// `x ?? y`, con `x` un T?: el ternario, que sólo calcula `y` si hace falta; `x` va dos veces, así que tiene que no
        /// calcular nada. Si calcula y `y` es una constante, GetValueOrDefault (que la calcula siempre: no importa).
        string CoalesceNullable(BinaryExpressionSyntax b)
        {
            var tx = m.GetTypeInfo(b.Left).Type;
            var t = m.GetTypeInfo(b).Type;
            string p = PorTipo(tx);
            if (p != null) return Rechaza(b, p);
            if (!EsNullable(t) && !SymbolEqualityComparer.Default.Equals(t, DeNullable(tx)))
                return Rechaza(b, Porque.Pendiente("?? que cambia el tipo de un T?"));
            if (!EsSimple(b.Left))
            {
                if (EsNullable(t) || !m.GetConstantValue(b.Right).HasValue)
                    return Rechaza(b, Porque.Pendiente("?? con algo que calcula a la izquierda"));
                imports.Add("CsSystem");
                return "CsSystem.Nullable_GetValueOrDefault(" + Expr(b.Left) + ", " + Expr(b.Right) + ")";
            }
            string x = Atomo(b.Left);
            if (EsNullable(t)) return "(" + x + " != null ? " + x + " : " + Expr(b.Right) + ")";
            string v = x + ".value()";
            if (EsEstrecho(t)) v = "((" + Tipo(t, b) + ") " + v + ")";
            return "(" + x + " != null ? " + v + " : " + Expr(b.Right) + ")";
        }

        /// `-x`, `+x`, `~x` y `!x` con `x` un T?.
        string PrefijoNullable(PrefixUnaryExpressionSyntax pu, IUnaryOperation uo)
        {
            string caja = CajaDeNullable(uo.Operand.Type);
            if (caja == null && PorTipo(uo.Operand.Type) is string porTipo) return Rechaza(pu, porTipo);
            string fn = null;
            switch (pu.Kind())
            {
                case SyntaxKind.UnaryPlusExpression: return Expr(pu.Operand);
                case SyntaxKind.UnaryMinusExpression: fn = caja != null && caja != "Boolean" ? "Menos" : null; break;
                case SyntaxKind.BitwiseNotExpression:
                    fn = (caja == "Integer" || caja == "Long") && !EsUIntNullable(uo.Operand.Type) ? "NoBits" : null; break;
                case SyntaxKind.LogicalNotExpression: fn = caja == "Boolean" ? "No" : null; break;
                case SyntaxKind.PreIncrementExpression:
                case SyntaxKind.PreDecrementExpression: return Sacado(pu);
            }
            if (fn == null) return Rechaza(pu, Porque.Pendiente(pu.OperatorToken.Text + " con un " + NombreCompleto(uo.Operand.Type)));
            imports.Add("CsSystem");
            return "CsSystem.Nullable_" + fn + "(" + Expr(pu.Operand) + ")";
        }

        /// ¿Se puede escribir dos veces lo que se asigna, porque no calcula nada? Una variable, un campo o una propiedad
        /// (de `this`, de una clase, o de algo que no calcula), un elemento de un array que no calcula, con su índice.
        bool ObjetivoSimple(ExpressionSyntax e)
        {
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            switch (e)
            {
                case IdentifierNameSyntax _: return true;
                case MemberAccessExpressionSyntax ma:
                    return ma.Expression is ThisExpressionSyntax || ma.Expression is BaseExpressionSyntax || EsSimple(ma.Expression)
                           || m.GetSymbolInfo(ma.Expression).Symbol is INamedTypeSymbol;
                case ElementAccessExpressionSyntax ea:
                    return EsSimple(ea.Expression) && ea.ArgumentList.Arguments.All(x => EsSimple(x.Expression));
                default: return false;
            }
        }

        /// `x += y` (y los demás) con `x` un T?: `x = CsSystem.Nullable_Suma(x, y)`, con `y` en la caja de `x`.
        string AsignaNullable(AssignmentExpressionSyntax a)
        {
            var tx = m.GetTypeInfo(a.Left).Type;
            string caja = CajaDeNullable(tx);
            if (caja == null) return Rechaza(a, PorTipo(tx) ?? Porque.Pendiente("un " + NombreCompleto(tx)));
            if ((caja == "Integer" && DeNullable(tx).SpecialType != SpecialType.System_Int32) || EsUIntNullable(tx))
                return Rechaza(a, Porque.Pendiente("una asignación compuesta de un T? estrecho, o de un uint?"));
            if (!ObjetivoSimple(a.Left)) return Rechaza(a, Porque.Pendiente("una asignación compuesta de un T? a algo que calcula"));
            string fn = FuncionNullable(a.Kind(), caja);
            if (fn == null) return Rechaza(a, Porque.Pendiente(a.OperatorToken.Text + " con un " + NombreCompleto(tx)));
            // lo de la derecha, en la caja de la operación: la de `x`, o un int? en un desplazamiento
            bool desplaza = fn == "Izq" || fn == "Der";
            var ty = m.GetTypeInfo(a.Right).Type;
            string y;
            if (EsNull(a.Right)) return Rechaza(a, Porque.Pendiente("un null suelto en una operación con un T?"));
            // (la conversión de la derecha la hace aquí el traductor: ExprBase, sin la que diga Roslyn para ese sitio)
            if (EsNullable(ty)) y = desplaza ? (CajaDeNullable(ty) == "Integer" ? ExprBase(a.Right) : Rechaza(a.Right, Porque.Pendiente("un desplazamiento con otra cuenta")))
                                             : CambiaDeCaja(ExprBase(a.Right), ty, tx, a.Right);
            else y = "new " + (desplaza ? "Integer" : caja) + "(" + ExprBase(a.Right) + ")";
            imports.Add("CsSystem");
            string x = Expr(a.Left);
            return x + " = CsSystem.Nullable_" + fn + "(" + x + ", " + y + ")";
        }

        /// `x++`, `x--` (y `++x`, `--x`) como sentencia, con `x` un T?: `x = CsSystem.Nullable_Inc(x)`.
        string IncNullable(ExpressionSyntax operando, bool mas, SyntaxNode donde)
        {
            var tx = m.GetTypeInfo(operando).Type;
            string caja = CajaDeNullable(tx);
            if (caja == null) return Rechaza(donde, PorTipo(tx) ?? Porque.Pendiente("un " + NombreCompleto(tx)));
            if (caja == "Boolean" || (caja == "Integer" && DeNullable(tx).SpecialType != SpecialType.System_Int32) || EsUIntNullable(tx))
                return Rechaza(donde, Porque.Pendiente("++ o -- con un " + NombreCompleto(tx)));
            if (!ObjetivoSimple(operando)) return Rechaza(donde, Porque.Pendiente("++ o -- de un T? en algo que calcula"));
            imports.Add("CsSystem");
            string x = Expr(operando);
            return x + " = CsSystem.Nullable_" + (mas ? "Inc" : "Dec") + "(" + x + ")";
        }

        /// Un miembro de un T?: `x.HasValue`, `x != null`; `x.Value`, su valor.
        string MiembroDeNullable(MemberAccessExpressionSyntax ma, ISymbol s)
        {
            switch (s.Name)
            {
                case "HasValue": return "(" + Expr(ma.Expression) + " != null)";
                case "Value": return ValorDeNullable(Expr(ma.Expression), s.ContainingType, ma);
            }
            return Rechaza(ma, Porque.DeNet(NombreCompleto(s.ContainingType) + "." + s.Name));
        }

        /// Un método de un T?: `GetValueOrDefault()` y `GetValueOrDefault(d)`; `ToString()`, el de .NET (vacío si es null).
        string LlamadaDeNullable(InvocationExpressionSyntax inv, IMethodSymbol ms)
        {
            var objeto = inv.Expression as MemberAccessExpressionSyntax;
            string caja = CajaDeNullable(ms.ContainingType);
            if (objeto != null && caja != null)
            {
                string x = Expr(objeto.Expression);
                switch (ms.Name)
                {
                    case "GetValueOrDefault":
                    {
                        imports.Add("CsSystem");
                        string r = "CsSystem.Nullable_GetValueOrDefault(" + x + (ms.Parameters.Length == 1 ? ", " + Argumentos(inv.ArgumentList, ms) : "") + ")";
                        var u = DeNullable(ms.ContainingType);
                        return EsEstrecho(u) ? "((" + Tipo(u, inv) + ") " + r + ")" : r;
                    }
                    case "ToString" when ms.Parameters.Length == 0 && (caja == "Integer" || caja == "Long" || caja == "Boolean"):
                        imports.Add("CsSystem");
                        return "CsSystem.Nullable_ToString(" + x + ")";
                }
            }
            return Rechaza(inv, Porque.DeNet(NombreCompleto(ms.ContainingType) + "." + ms.Name));
        }
    }
}
