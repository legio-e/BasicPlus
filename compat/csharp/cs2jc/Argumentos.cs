// H8, tema 4 (7-nov; Eduardo: «OK, sigue con los 4 ajustes»; docs/V8_IDEAS.md §E9, «El tema 4») — lo que C# pone en una
// llamada y JC no escribe solo. Ninguno toca JC:
//   1. Los valores por defecto que JC no admite en la declaración (sólo un literal: no null, un enum o un T? con valor), y
//      los de un método con un params detrás de uno: se escriben en cada llamada, que es lo que hace C#, y la declaración
//      se queda sin ninguno.
//   2. Un null con su tipo (`(X) null`) donde el tipo elige la sobrecarga: una función que lo devuelve, `CsNull_X()`.
//   3. Un array de valores de otro tipo que el suyo (una clase hija, cosas mezcladas en un `params object[]`): una función
//      que lo crea con `new T[n]` y llena sus casillas, una por tipo y tamaño, `CsArray_T_n(…)`.
//   4. Los argumentos con nombre que cambian el orden de lo que se calcula: lo que calcula algo, a una temporal delante de
//      la sentencia, en el orden escrito (Sacar.cs); en la llamada, las temporales, en el de los parámetros.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Operations;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        // ---------------------------------------------------------------------- 1. los valores por defecto, en la llamada

        readonly Dictionary<IMethodSymbol, bool> enLaLlamada = new Dictionary<IMethodSymbol, bool>(SymbolEqualityComparer.Default);

        /// El método que declara los valores por defecto: de una llamada a un genérico, su definición; de un partial, la parte
        /// sin cuerpo; de un método de extensión llamado sobre su objeto, el estático.
        static IMethodSymbol DondeSeDeclara(IMethodSymbol ms)
        {
            var o = (ms.ReducedFrom ?? ms).OriginalDefinition;
            return o.PartialDefinitionPart ?? o;
        }

        /// ¿Se escriben en cada llamada sus valores por defecto? Los de un método del programa con uno que JC no admite en la
        /// declaración, con un params detrás de uno (en JC, uno sin valor no va tras uno con él) o con uno [Optional] sin
        /// valor. Entonces su declaración no lleva ninguno.
        bool DefectoEnLaLlamada(IMethodSymbol ms)
        {
            if (ms == null) return false;
            var d = DondeSeDeclara(ms);
            if (!DelPrograma(d)) return false;
            if (enLaLlamada.TryGetValue(d, out bool si)) return si;
            var ps = d.Parameters;
            si = ps.Any(p => p.HasExplicitDefaultValue && LiteralDeDefecto(p) == null)
                 || (ps.Any(p => p.IsParams) && ps.Any(p => p.HasExplicitDefaultValue))
                 || ps.Any(p => p.IsOptional && !p.HasExplicitDefaultValue && !p.IsParams);
            return enLaLlamada[d] = si;
        }

        /// El valor por defecto de un parámetro como un literal de JC, lo único que JC admite en una declaración; null si no lo
        /// es: null, un enum, un T? con valor, NaN…
        string LiteralDeDefecto(IParameterSymbol ps)
        {
            if (!ps.HasExplicitDefaultValue || PorTipo(ps.Type) != null || EsNullable(ps.Type) || EsEnum(ps.Type)) return null;
            return LiteralDeValor(ps.ExplicitDefaultValue);
        }

        /// Un valor constante como literal de JC; null si no lo tiene (null, NaN, un infinito, un decimal…).
        static string LiteralDeValor(object v)
        {
            if (v is ulong grande) v = unchecked((long) grande);       // un ulong, sus 64 bits (SinSigno.cs)
            var ci = System.Globalization.CultureInfo.InvariantCulture;
            switch (v)
            {
                case bool b: return b ? "true" : "false";
                case string s: return Cadena(s);
                case char ch: return ((int) ch).ToString(ci);           // un char, su código: en JC es un entero
                case int i: return i == int.MinValue ? "(-2147483647 - 1)" : i.ToString(ci);
                case long l: return l == long.MinValue ? "(-9223372036854775807L - 1L)" : l.ToString(ci) + "L";
                case uint u: return u.ToString(ci) + "L";                    // un uint es un long (SinSigno.cs)
                case short _: case sbyte _: case byte _: case ushort _: return Convert.ToString(v, ci);
                case double d:
                {
                    if (double.IsNaN(d) || double.IsInfinity(d)) return null;
                    string r = d.ToString("R", ci);
                    return r.Contains('.') || r.Contains('E') ? r : r + ".0";
                }
                case float f:
                {
                    if (float.IsNaN(f) || float.IsInfinity(f)) return null;
                    string r = f.ToString("R", ci);
                    return (r.Contains('.') || r.Contains('E') ? r : r + ".0") + "f";
                }
                default: return null;
            }
        }

        /// Los valores que una llamada se salta, desde el parámetro `dados`, escritos como los pone C#; ninguno si el método
        /// los tiene en su declaración (JC los pone).
        List<string> Faltan(IMethodSymbol ms, int dados, SyntaxNode donde) =>
            DefectoEnLaLlamada(ms) ? ValoresDesde(ms, dados, donde) : new List<string>();

        /// Los valores de los parámetros desde el `desde`, escritos como los pone C#.
        List<string> ValoresDesde(IMethodSymbol ms, int desde, SyntaxNode donde)
        {
            var r = new List<string>();
            for (int i = desde; i < ms.Parameters.Length; i++)
                r.Add(ms.Parameters[i].IsParams ? "[]" : ValorEnLaLlamada(ms.Parameters[i], ms, donde));
            return r;
        }

        /// El valor que pone C# a un parámetro que la llamada se salta: su valor por defecto, escrito; un null, con su tipo si
        /// el método tiene sobrecargas; un enum, su nombre; un T?, en su caja; uno [Optional] sin valor, el de su tipo.
        string ValorEnLaLlamada(IParameterSymbol p, IMethodSymbol ms, SyntaxNode donde)
        {
            var t = p.Type;
            if (PorTipo(t) is string porTipo) return Rechaza(donde, porTipo);
            object v = null;
            if (p.HasExplicitDefaultValue) v = p.ExplicitDefaultValue;
            else if (t.SpecialType == SpecialType.System_Object) return Rechaza(donde, Porque.Pendiente("un [Optional] de un object (Type.Missing)"));
            if (v == null && t is ITypeParameterSymbol tp && !tp.HasReferenceTypeConstraint)
                return Rechaza(donde, Porque.Pendiente("un default(T) escrito en la llamada"));
            if (EsNullable(t))
            {
                if (v == null) return NullEnLaLlamada(t, ms, donde);
                string caja = CajaDeNullable(t);
                string valor = LiteralDeValor(v);
                if (caja == null || valor == null) return Rechaza(donde, PorTipo(t) ?? Porque.Pendiente("un valor por defecto de " + NombreCompleto(t)));
                return "new " + caja + "(" + valor + ")";
            }
            if (EsEnum(t)) return MiembroDeEnum(t, v ?? 0, donde);
            if (EsEnumEntero(t)) return NumeroDeEnum(v ?? 0, t);
            if (v == null)
            {
                if (!t.IsValueType) return NullEnLaLlamada(t, ms, donde);
                if (!EsNumerico(t) && t.SpecialType != SpecialType.System_Boolean && !EsStructSinEstado(t))
                    return Rechaza(donde, Porque.Pendiente("el valor por defecto de " + NombreCompleto(t)));
                string cero = ValorCualquiera(t, donde);
                return EsEstrecho(t) ? "((" + Tipo(t, donde) + ") " + cero + ")" : cero;
            }
            string lit = LiteralDeValor(v);
            if (lit == null) return Rechaza(donde, Porque.Pendiente("un valor por defecto " + v));
            // un byte, un short…: en un argumento, JC (como Java) lo quiere con su conversión escrita
            return EsEstrecho(t) ? "((" + Tipo(t, donde) + ") " + lit + ")" : lit;
        }

        /// Un null en la llamada: con su tipo, si el método tiene otro con su nombre (un null suelto podría no decir a cuál va).
        string NullEnLaLlamada(ITypeSymbol t, IMethodSymbol ms, SyntaxNode donde) => Sobrecargado(ms) ? NullDe(t, donde) : "null";

        /// Un valor de un enum de JC, por su nombre (JC no convierte un entero en un enum, #522): el miembro que lo vale.
        string MiembroDeEnum(ITypeSymbol t, object v, SyntaxNode donde)
        {
            if (!DelPrograma(t)) return Rechaza(donde, Porque.DeNet("tipo " + NombreCompleto(t)));
            long n = Entero64(v);
            var f = t.GetMembers().OfType<IFieldSymbol>().FirstOrDefault(x => x.HasConstantValue && Entero64(x.ConstantValue) == n);
            if (f == null) return Rechaza(donde, Porque.Pendiente("un valor por defecto de un enum que no es uno de sus nombres"));
            return Receptor((INamedTypeSymbol) t, donde) + "." + NombreDe(f);
        }

        static long Entero64(object v) => v is ulong u ? unchecked((long) u) : Convert.ToInt64(v);

        /// El constructor de la base que C# llama solo, sin `: base (…)`, si tiene parámetros (todos con su valor por defecto,
        /// o un params): el `super()` implícito de JC sólo encuentra uno sin parámetros (`#804`), y la llamada se escribe, con
        /// los valores que pone C#.
        IMethodSymbol BaseConValores(ConstructorDeclarationSyntax c)
        {
            if (m.GetOperation(c) is IConstructorBodyOperation cuerpo && cuerpo.Initializer is IExpressionStatementOperation es
                && es.Operation is IInvocationOperation inv && inv.TargetMethod.Parameters.Length > 0 && DelPrograma(inv.TargetMethod))
                return inv.TargetMethod;
            return null;
        }

        /// Una clase sin constructores: el implícito de C# llama al de la base que se puede llamar sin argumentos; si ése tiene
        /// parámetros, el implícito de JC no lo encuentra (`#804`): se escribe uno, con la llamada y sus valores.
        void ImplicitoConValores(INamedTypeSymbol s)
        {
            var b = s.BaseType;
            if (s.TypeKind != TypeKind.Class || b == null || !DelPrograma(b) || !s.InstanceConstructors.All(c => c.IsImplicitlyDeclared)) return;
            var sinArgumentos = b.InstanceConstructors.Where(c => c.DeclaredAccessibility != Accessibility.Private
                                                                  && c.Parameters.All(p => p.IsOptional || p.IsParams)).ToList();
            if (sinArgumentos.Count == 0 || sinArgumentos.Any(c => c.Parameters.Length == 0)) return;
            if (sinArgumentos.Count > 1)
            {
                Rechaza(Declaracion(s), Porque.Pendiente("una clase sin constructor cuya base tiene varios que se llaman sin argumentos"));
                return;
            }
            Linea("public function " + NombreTipo(s) + "() : base(" + string.Join(", ", ValoresDesde(sinArgumentos[0], 0, Declaracion(s))) + ") { }");
        }

        // ---------------------------------------------------------------------- 2. un null con su tipo

        /// Las funciones de los null con su tipo: su nombre y el tipo, en JC.
        readonly SortedDictionary<string, string> nulos = new SortedDictionary<string, string>(StringComparer.Ordinal);

        /// `(X) null` donde el tipo elige la sobrecarga: JC no escribe un null con su tipo, y una función que devuelve un X lo
        /// es: `CsNull_X()`.
        string NullDe(ITypeSymbol t, SyntaxNode donde)
        {
            string jt = Tipo(t, donde);
            if (jt == Hueco) return Hueco;
            string nombre = "CsNull_" + Regex.Replace(jt, "[^A-Za-z0-9]", "_");
            nulos[nombre] = jt;
            return nombre + "()";
        }

        /// ¿Tiene el método otro con su nombre, en su clase o en una de sus bases (o, si es un constructor, otro constructor)?
        static bool Sobrecargado(IMethodSymbol ms)
        {
            if (ms.MethodKind == MethodKind.Constructor) return ms.ContainingType.InstanceConstructors.Length > 1;
            int n = 0;
            for (var t = ms.ContainingType; t != null; t = t.BaseType)
                n += t.GetMembers(ms.Name).OfType<IMethodSymbol>().Count(o => o.OverriddenMethod == null);
            return n > 1;
        }

        // ---------------------------------------------------------------------- 3. un array de valores de otro tipo

        /// Las funciones que crean un array de valores de otro tipo: su nombre, y el tipo de sus casillas en JC y cuántas son.
        readonly SortedDictionary<string, (string Tipo, int N)> arraysDeVarios = new SortedDictionary<string, (string, int)>(StringComparer.Ordinal);

        /// `new Animal[] { new Perro (), new Gato () }`, o `F (8, "a")` con `params object[]`: un array de valores de JC toma el
        /// tipo de sus valores, y éstos no lo tienen; una función que lo crea con su tipo y su tamaño y llena las casillas, en
        /// orden, sí: `CsArray_Animal_2(new Perro(), new Gato())`.
        string ArrayDeVarios(IArrayTypeSymbol at, List<string> valores, SyntaxNode donde)
        {
            if (at.ElementType is IArrayTypeSymbol || at.ElementType is ITypeParameterSymbol) return Rechaza(donde, Porque.H7(Porque.ArrayDeVarios));
            string el = Tipo(at.ElementType, donde);
            if (el == Hueco || valores.Contains(Hueco)) return Hueco;
            string nombre = "CsArray_" + Regex.Replace(el, "[^A-Za-z0-9]", "_") + "_" + valores.Count;
            arraysDeVarios[nombre] = (el, valores.Count);
            return nombre + "(" + string.Join(", ", valores) + ")";
        }

        /// Al final del módulo, las funciones que se usaron: las de los null con su tipo y las de los arrays.
        void EmiteFuncionesDeLlamadas()
        {
            foreach (var n in nulos)
            {
                Linea("");
                Linea("public function " + n.Value + " " + n.Key + "()");    // public: lo llaman las clases del módulo
                Linea("{");
                Linea("   return null;");
                Linea("}");
            }
            foreach (var a in arraysDeVarios)
            {
                string t = a.Value.Tipo;
                int n = a.Value.N;
                Linea("");
                Linea("public function " + t + "[] " + a.Key + "(" + string.Join(", ", Enumerable.Range(0, n).Select(i => t + " v" + i)) + ")");
                Linea("{");
                sangria++;
                Linea(t + "[] r = new " + t + "[" + n + "];");
                for (int i = 0; i < n; i++) Linea("r[" + i + "] = v" + i + ";");
                Linea("return r;");
                sangria--;
                Linea("}");
            }
        }

        // ---------------------------------------------------------------------- 4. los argumentos con nombre, en su orden

        /// `m (a: f (1), c: f (2), b: f (3))`: C# calcula los argumentos en el orden escrito, y JC en el de los parámetros. Lo
        /// que calcula algo, a una temporal delante de la sentencia, en el orden escrito (Sacar.cs); lo que no cambia (un
        /// literal, una constante, una local, un parámetro, `this`), en su sitio. Como un `++` partido: no bajo &&, ||, ?? o ?:,
        /// ni con algo que se calcule antes en la sentencia (el objeto de la llamada, otra llamada). null si no se puede.
        Dictionary<ExpressionSyntax, string> EnSuOrden(BaseArgumentListSyntax args, IMethodSymbol ms, ExpressionSyntax[] porParametro)
        {
            if (sacar == null || BajoCondicional(args) || AlgoAntes(args)) return null;
            var aSacar = new List<(ExpressionSyntax E, int P)>();
            foreach (var a in args.Arguments)
            {
                if (NoCambia(a.Expression)) continue;
                int p = Array.IndexOf(porParametro, a.Expression);
                if (p < 0 || ms.OriginalDefinition.Parameters[p].Type is ITypeParameterSymbol) return null;
                aSacar.Add((a.Expression, p));
            }
            var r = new Dictionary<ExpressionSyntax, string>();
            foreach (var (e, p) in aSacar)
            {
                if (!sacar.Hechos.TryGetValue(e, out var t))
                {
                    t = Temporal();
                    sacar.Antes.Add(Tipo(ms.Parameters[p].Type, e) + " " + t + " = " + ArgumentoDe(e, ms.Parameters[p]) + ";");
                    sacar.Hechos[e] = t;
                }
                r[e] = t;
            }
            return r;
        }

        /// ¿Una expresión que da lo mismo calcular antes o después de las otras: un literal, una constante, una local, un
        /// parámetro, `this`? (Un campo, no: una llamada de los otros argumentos puede cambiarlo.)
        bool NoCambia(ExpressionSyntax e)
        {
            switch (e)
            {
                case LiteralExpressionSyntax _:
                case ThisExpressionSyntax _: return true;
                case ParenthesizedExpressionSyntax p: return NoCambia(p.Expression);
                case PrefixUnaryExpressionSyntax u when u.IsKind(SyntaxKind.UnaryMinusExpression) || u.IsKind(SyntaxKind.LogicalNotExpression):
                    return NoCambia(u.Operand);
                case IdentifierNameSyntax id:
                {
                    var s = m.GetSymbolInfo(id).Symbol;
                    return (s is ILocalSymbol l && !enCaja.Contains(l)) || (s is IParameterSymbol ps && ps.RefKind == RefKind.None)
                           || m.GetConstantValue(id).HasValue;
                }
                default: return m.GetConstantValue(e).HasValue;
            }
        }
    }
}
