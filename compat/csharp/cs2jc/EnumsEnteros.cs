// Un enum que se opera (V8/H8, tema 5; 5-nov; Eduardo: «no podemos dejarlos como enum y convertirlos a enteros?»; docs/V8_IDEAS.md §E9). JC no deja convertir
// un entero en un enum (#522, a propósito), y C# lo hace al operar: `A.x | A.y`, `~e`, `e + 1`, `(E) n`, `E e = 0`. El enum se queda como enum, y lo que se opera
// se convierte a entero: sus variables son enteros, y un miembro, `(int) E.A`. JC no pierde nada: sólo el enum convierte a entero, nunca al revés. Vale para
// todo el enum, en todo el programa. Un enum con valores que uno de JC no lleva (negativos, o que no caben en un int) no se escribe, y sus miembros van con
// su número. Su nombre en una cadena, con una función de cada enum que lo pide (Cs_Nombre_E); los demás enums, como siempre (Enum.getName).
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        readonly HashSet<ITypeSymbol> enumsEnteros = new HashSet<ITypeSymbol>(SymbolEqualityComparer.Default);
        readonly HashSet<ITypeSymbol> enumsSinEscribir = new HashSet<ITypeSymbol>(SymbolEqualityComparer.Default);
        readonly List<INamedTypeSymbol> nombresDeEnum = new List<INamedTypeSymbol>();

        static bool EsEnumCs(ITypeSymbol t) => t != null && t.TypeKind == TypeKind.Enum;

        bool EsEnumEntero(ITypeSymbol t) => t != null && enumsEnteros.Contains(t);

        /// ¿Qué enums del programa se operan? Los que C# convierte desde un número o usa en una cuenta; y los que JC no sabría escribir.
        void BuscaEnumsEnteros(SyntaxNode raiz)
        {
            void Pon(ITypeSymbol t) { if (EsEnumCs(t) && DelPrograma(t)) enumsEnteros.Add(t); }
            ITypeSymbol Tipo(ExpressionSyntax e) => m.GetTypeInfo(e).Type;
            foreach (var n in raiz.DescendantNodes())
            {
                switch (n)
                {
                    case CastExpressionSyntax c when !SymbolEqualityComparer.Default.Equals(Tipo(c), Tipo(c.Expression)):
                        Pon(Tipo(c));
                        break;
                    case BinaryExpressionSyntax b when EsCuenta(b.Kind()):
                        Pon(Tipo(b));
                        Pon(Tipo(b.Left));
                        Pon(Tipo(b.Right));
                        break;
                    case PrefixUnaryExpressionSyntax p when !p.IsKind(SyntaxKind.LogicalNotExpression):
                        Pon(Tipo(p.Operand));
                        break;
                    case PostfixUnaryExpressionSyntax p when p.IsKind(SyntaxKind.PostIncrementExpression) || p.IsKind(SyntaxKind.PostDecrementExpression):
                        Pon(Tipo(p.Operand));
                        break;
                    case AssignmentExpressionSyntax a when !a.IsKind(SyntaxKind.SimpleAssignmentExpression):
                        Pon(Tipo(a.Left));
                        break;
                    case EnumDeclarationSyntax d:
                        foreach (var v in d.Members)
                        {
                            var f = m.GetDeclaredSymbol(v);
                            bool grande = f.ConstantValue is ulong u && u > long.MaxValue;
                            if (grande || Convert.ToInt64(f.ConstantValue) < 0 || Convert.ToInt64(f.ConstantValue) > int.MaxValue)
                            {
                                Pon(f.ContainingType);
                                enumsSinEscribir.Add(f.ContainingType);
                            }
                        }
                        break;
                }
                if (n is ExpressionSyntax e && m.GetConversion(e).IsEnumeration) Pon(m.GetTypeInfo(e).ConvertedType);
            }
        }

        static bool EsCuenta(SyntaxKind k)
        {
            switch (k)
            {
                case SyntaxKind.AddExpression:
                case SyntaxKind.SubtractExpression:
                case SyntaxKind.MultiplyExpression:
                case SyntaxKind.BitwiseAndExpression:
                case SyntaxKind.BitwiseOrExpression:
                case SyntaxKind.ExclusiveOrExpression: return true;
                default: return false;
            }
        }

        /// El entero de un enum que se opera: `long` si no cabe en un int; `int` para los demás, que es en lo que C# hace sus cuentas.
        static string TipoDeEnumEntero(ITypeSymbol t)
        {
            switch (((INamedTypeSymbol) t).EnumUnderlyingType?.SpecialType)
            {
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64: return "long";
                default: return "int";
            }
        }

        /// Un miembro de un enum que se opera, como entero: `((int) E.A)`; si el enum no se escribe, su número.
        string MiembroEntero(IFieldSymbol f, SyntaxNode donde)
        {
            if (enumsSinEscribir.Contains(f.ContainingType)) return NumeroDeEnum(f.ConstantValue, f.ContainingType);
            return "((" + TipoDeEnumEntero(f.ContainingType) + ") " + NombreTipo(f.ContainingType) + "." + Ident(f.Name) + ")";
        }

        /// Un valor de un enum que se opera, como número: JC sólo admite un literal en una constante.
        static string NumeroDeEnum(object v, ITypeSymbol t)
        {
            long n = v is ulong u ? unchecked((long) u) : Convert.ToInt64(v);
            return n.ToString(System.Globalization.CultureInfo.InvariantCulture) + (TipoDeEnumEntero(t) == "long" ? "L" : "");
        }

        /// Cómo se escribe un tipo en una firma de JC, a efectos de que dos sobrecargas choquen.
        string EnFirma(ITypeSymbol t)
        {
            if (EsEnumEntero(t)) return TipoDeEnumEntero(t);
            switch (t.SpecialType)
            {
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64: return "long";
                case SpecialType.System_Int32:
                case SpecialType.System_Char: return "int";
                default: return t.ToDisplayString();
            }
        }

        /// Dos sobrecargas que en JC serían la misma (`Test (int)` y `Test (E)`, con E un enum que se opera; `f (long)` y `f (uint)`): Roslyn dice a cuál
        /// va cada llamada, y la segunda lleva otro nombre, como un miembro oculto (Explicitas.cs).
        void BuscaSobrecargasIguales(IEnumerable<INamedTypeSymbol> tipos)
        {
            foreach (var c in tipos)
            {
                var vistas = new Dictionary<string, int>();
                foreach (var me in c.GetMembers().OfType<IMethodSymbol>().Where(x => x.MethodKind == MethodKind.Ordinary && x.OverriddenMethod == null))
                {
                    string firma = me.Name + "(" + string.Join(",", me.Parameters.Select(p => EnFirma(p.Type))) + ")";
                    vistas.TryGetValue(firma, out int cuantas);
                    vistas[firma] = cuantas + 1;
                    if (cuantas > 0 && !ocultos.ContainsKey(me.OriginalDefinition)) ocultos[me.OriginalDefinition] = Ident(me.Name) + "_" + cuantas;
                }
            }
        }

        /// El nombre de un valor en una cadena: lo que escribe C#, con la función de su enum.
        string NombreDeEnumEntero(ExpressionSyntax e, ITypeSymbol t)
        {
            var en = (INamedTypeSymbol) t;
            if (!nombresDeEnum.Contains(en, SymbolEqualityComparer.Default)) nombresDeEnum.Add(en);
            return "Cs_Nombre_" + NombreTipo(en) + "(" + ExprBase(e) + ")";
        }

        /// `Cs_Nombre_E(v)`, de cada enum que lo pide: su nombre, o su número si no lo tiene, como `ToString` en C#. Uno [Flags], los nombres de sus bits
        /// separados por una coma, del menor al mayor.
        void EmiteNombresDeEnums()
        {
            foreach (var en in nombresDeEnum)
            {
                var miembros = en.GetMembers().OfType<IFieldSymbol>().Where(f => f.IsConst).ToList();
                var ci = System.Globalization.CultureInfo.InvariantCulture;
                string Valor(IFieldSymbol f) => f.ConstantValue is ulong u ? unchecked((long) u).ToString(ci) : Convert.ToInt64(f.ConstantValue).ToString(ci);
                string l = TipoDeEnumEntero(en) == "long" ? "L" : "";
                Linea("");
                Linea("public function string Cs_Nombre_" + NombreTipo(en) + "(" + TipoDeEnumEntero(en) + " v)");   // public: lo llaman las clases del módulo
                Linea("{");
                sangria++;
                var vistos = new HashSet<string>();
                foreach (var f in miembros)
                    if (vistos.Add(Valor(f)))
                        Linea("if (v == " + Valor(f) + l + ") return " + Cadena(f.Name) + ";");
                if (en.GetAttributes().Any(a => a.AttributeClass?.Name == "FlagsAttribute"))
                {
                    Linea("string nombres = \"\";");
                    Linea(TipoDeEnumEntero(en) + " queda = v;");
                    foreach (var f in miembros.Where(f => Valor(f) != "0").OrderByDescending(f => Convert.ToInt64(Valor(f))))
                        Linea("if ((queda & " + Valor(f) + l + ") == " + Valor(f) + l + ") { queda -= " + Valor(f) + l + "; nombres = " + Cadena(f.Name)
                              + " + (nombres == \"\" ? \"\" : \", \" + nombres); }");
                    Linea("if (queda == 0 && nombres != \"\") return nombres;");
                }
                Linea("return \"\" + v;");
                sangria--;
                Linea("}");
            }
        }
    }
}
