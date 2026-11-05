// El tipo de un objeto (V8/H8, tema 2; 4-nov; docs/V8_IDEAS.md §E9). Eduardo: «El compilador ya conoce el tipo entonces, no se
// puede comparar si un type es igual?»; «El 'instanceof' no puede sustituir la expresion?»; y de dos Object con un Integer y un
// Float dentro: «eso no lo puede resolver el compilador, tendría que ser la VM». No hay un `Type` al ejecutar: el traductor ve el
// programa entero, y lo que sabe lo escribe él —un `is`, un `true`, el nombre como texto—; lo que sólo se sabe al ejecutar, se lo
// pregunta a la VM (`CsSystem.Type_Misma`, por el nombre de la clase), o a `Cs_TipoDe`, una cadena de `is` con las clases del
// programa. Un `Type` en una variable sigue sin traducirse.
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// `typeof(T)` (Escrito), o `x.GetType()` (De; null, de `this`).
        sealed class TipoDe
        {
            public ITypeSymbol Escrito;
            public ExpressionSyntax De;
        }

        /// Todos los tipos del programa (Traduce), y si hay que escribir `Cs_TipoDe`.
        List<INamedTypeSymbol> tiposDelPrograma = new List<INamedTypeSymbol>();
        bool usaTipoDe;

        TipoDe ExprDeTipo(ExpressionSyntax e)
        {
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            if (e is TypeOfExpressionSyntax to)
            {
                var t = m.GetTypeInfo(to.Type).Type;
                return t == null || PideArgumentos(t) ? null : new TipoDe { Escrito = t };
            }
            if (e is InvocationExpressionSyntax inv && inv.ArgumentList.Arguments.Count == 0
                && m.GetSymbolInfo(inv).Symbol is IMethodSymbol ms && ms.Name == "GetType"
                && ms.ContainingType.SpecialType == SpecialType.System_Object)
            {
                var de = (inv.Expression as MemberAccessExpressionSyntax)?.Expression;
                var s = de == null || de is BaseExpressionSyntax || de is ThisExpressionSyntax ? claseActual : m.GetTypeInfo(de).Type;
                if (s != null && PideArgumentos(s)) return null;          // de un `T` o de una `C<X>`, sin sus argumentos no se sabe
                return new TipoDe { De = de is BaseExpressionSyntax ? null : de };
            }
            return null;
        }

        IEnumerable<INamedTypeSymbol> Hijas(INamedTypeSymbol t) =>
            tiposDelPrograma.Where(x => SymbolEqualityComparer.Default.Equals(x.BaseType?.OriginalDefinition, t.OriginalDefinition));

        /// El tipo que tiene seguro al ejecutar, o null si depende: el escrito, el de un `new`, un valor o una cadena, y una
        /// clase del programa sin hijas.
        ITypeSymbol Exacto(TipoDe t)
        {
            if (t.Escrito != null) return t.Escrito;
            if (t.De == null) return !claseActual.IsAbstract && !Hijas(claseActual).Any() ? claseActual : null;
            var e = t.De;
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            var s = m.GetTypeInfo(e).Type;
            if (s == null || EsNullable(s)) return null;
            if (e is ObjectCreationExpressionSyntax || s.IsValueType || EsCadena(s)) return s;
            if (s is IArrayTypeSymbol a) return a.ElementType.IsValueType ? s : null;
            if (s is INamedTypeSymbol nt && nt.TypeKind == TypeKind.Class && DelPrograma(nt) && !nt.IsAbstract && !Hijas(nt).Any()) return s;
            return null;
        }

        /// `x.GetType()` lanza si `x` es null: la prueba de que no lo es, o null si no hace falta (no puede serlo). Si `x` es un
        /// valor que calcula algo, hay que calcularlo, y no hay dónde: no se traduce.
        string Hay(TipoDe t, SyntaxNode donde)
        {
            if (t.De == null) return null;
            var s = m.GetTypeInfo(t.De).Type;
            if (s != null && s.IsValueType)
                return EsSimple(t.De) ? null : Rechaza(donde, Porque.Pendiente("GetType de un valor que calcula"));
            if (NoPuedeSerNull(t.De)) return null;
            imports.Add("CsSystem");
            return "CsSystem.Type_Hay(" + Expr(t.De) + ")";
        }

        static string Y(params string[] partes)
        {
            var q = partes.Where(x => x != null).ToList();
            return q.Count == 1 ? q[0] : "(" + string.Join(" && ", q) + ")";
        }

        /// `a == b` o `a != b`, con `a` y `b` un `typeof` o un `GetType()`.
        string ComparaTipos(BinaryExpressionSyntax b, TipoDe i, TipoDe d)
        {
            string r = MismoTipo(i, d, b);
            if (b.IsKind(SyntaxKind.EqualsExpression) || r == Hueco) return r;
            return r == "true" ? "false" : r == "false" ? "true" : "!" + r;
        }

        string MismoTipo(TipoDe a, TipoDe b, SyntaxNode donde)
        {
            var ea = Exacto(a);
            var eb = Exacto(b);
            if (ea != null && eb != null)
                return Y(Hay(a, donde), Hay(b, donde), SymbolEqualityComparer.Default.Equals(ea, eb) ? "true" : "false");
            // uno seguro: el otro es ése si `is` lo dice; lo seguro se mira delante, y por eso no puede calcular nada
            if (ea != null || eb != null)
            {
                var seguro = ea != null ? a : b;
                if (seguro.De != null && !EsSimple(seguro.De)) return Rechaza(donde, Porque.Pendiente("GetType de algo que calcula, en este sitio"));
                return Y(Hay(seguro, donde), Es(ea != null ? b : a, ea ?? eb, donde));
            }
            // los dos dependen de lo que lleven dentro: lo sabe la VM
            if (HayCajaCompartida()) return Rechaza(donde, Porque.Pendiente(CajaCompartida));
            if (HayGenericas()) return Rechaza(donde, Porque.Fuera(Porque.SinT));
            imports.Add("CsSystem");
            return "CsSystem.Type_Misma(" + Valor(a) + ", " + Valor(b) + ")";
        }

        string Valor(TipoDe t) => t.De == null ? "this" : Expr(t.De);

        /// ¿Es `t` exactamente de ese tipo, sin saberlo al traducir? `x is T`, y si `T` tiene hijas, que no sea ninguna: `is`
        /// también dice sí con ellas, y `GetType` no. Una interfaz o una clase abstracta nunca son el tipo de un objeto, y dos
        /// clases sin relación, tampoco.
        string Es(TipoDe t, ITypeSymbol tipo, SyntaxNode donde)
        {
            var nt = tipo as INamedTypeSymbol;
            var de = t.De != null ? m.GetTypeInfo(t.De).Type as INamedTypeSymbol : claseActual;
            if (nt != null && (nt.TypeKind == TypeKind.Interface || nt.IsAbstract)) return Y(Hay(t, donde), "false");
            if (nt != null && de != null && de.TypeKind == TypeKind.Class && nt.TypeKind == TypeKind.Class
                && !Hereda(nt, de.OriginalDefinition) && !Hereda(de, nt.OriginalDefinition))
                return Y(Hay(t, donde), "false");
            string clase;
            var hijas = new List<INamedTypeSymbol>();
            if (ClaseDeCaja(tipo) != null && !EsUInt(tipo))
            {
                if (tipo.SpecialType == SpecialType.System_Int64 && HayCajaCompartida()) return Rechaza(donde, Porque.Pendiente(CajaCompartida));
                clase = ClaseDeCaja(tipo);
            }
            else if (nt != null && DelPrograma(nt) && (nt.TypeKind == TypeKind.Class || EsStructSinEstado(nt)))
            {
                clase = Tipo(nt, donde);
                hijas = Hijas(nt).ToList();
            }
            else return Rechaza(donde, Porque.Fuera(Porque.Reflexion));
            string x = UnaVez(t, donde);
            if (x == Hueco) return Hueco;
            var partes = new List<string>();
            if (t.De != null && !NoPuedeSerNull(t.De))
            {
                imports.Add("CsSystem");
                partes.Add("CsSystem.Type_Hay(" + x + ")");
            }
            partes.Add(x + " is " + clase);
            partes.AddRange(hijas.Select(h => "!(" + x + " is " + Tipo(h, donde) + ")"));
            return Y(partes.ToArray());
        }

        /// Lo de delante de un `GetType()`, que va a escribirse varias veces: tal cual si no calcula nada; si calcula, en una
        /// temporal delante de la sentencia (Sacar), si no cambia el orden.
        string UnaVez(TipoDe t, SyntaxNode n)
        {
            if (t.De == null) return "this";
            if (EsSimple(t.De)) return Expr(t.De);
            if (sacar == null || BajoCondicional(n) || AlgoAntes(n)) return Rechaza(n, Porque.Pendiente("GetType de algo que calcula, en este sitio"));
            if (sacar.Hechos.TryGetValue(t.De, out var hecho)) return hecho;
            string v = Temporal();
            sacar.Antes.Add(Tipo(m.GetTypeInfo(t.De).Type, t.De) + " " + v + " = " + Expr(t.De) + ";");
            return sacar.Hechos[t.De] = v;
        }

        const string CajaCompartida = "comparar tipos con algo en una caja que comparte con otro tipo (un uint va en un Long, como un long)";
        bool? cajaCompartida;

        /// ¿Mete el programa en una caja algo que comparte caja con otro tipo? Entonces el tipo se pierde, y ni el traductor ni
        /// la VM saben decirlo: mejor no traducirlo que dar una respuesta equivocada. El programa se ve entero.
        bool HayCajaCompartida()
        {
            if (cajaCompartida == null)
                cajaCompartida = arbol.GetRoot().DescendantNodes().OfType<ExpressionSyntax>()
                    .Any(e => m.GetTypeInfo(e).Type is ITypeSymbol t && (EsUInt(t) || EsEstrecho(t) || t.SpecialType == SpecialType.System_Char)
                              && m.GetConversion(e).IsBoxing);
            return cajaCompartida.Value;
        }

        // -------------------------------------------------------------------------------------------------- los nombres

        static string NombreCorto(ITypeSymbol t) => t is IArrayTypeSymbol a ? NombreCorto(a.ElementType) + "[]" : t.Name;

        string NombreEntero(ITypeSymbol t)
        {
            if (t is IArrayTypeSymbol a) return NombreEntero(a.ElementType) + "[]";
            return t is INamedTypeSymbol nt && DelPrograma(nt) ? NombreNet(nt) : NombreCompleto(t);
        }

        /// El nombre del tipo: `FullName` (también al imprimirlo) o `Name`. Si se sabe, como texto; si no, lo busca `Cs_TipoDe`.
        string NombreDeTipo(TipoDe t, bool entero, SyntaxNode donde)
        {
            var ex = Exacto(t);
            if (ex != null)
            {
                string texto = Cadena(entero ? NombreEntero(ex) : NombreCorto(ex));
                string hay = Hay(t, donde);
                return hay == null ? texto : "(" + hay + " ? " + texto + " : \"\")";
            }
            if (HayCajaCompartida()) return Rechaza(donde, Porque.Pendiente(CajaCompartida));
            if (HayGenericas()) return Rechaza(donde, Porque.Fuera(Porque.SinT));
            usaTipoDe = true;
            string nombre = "Cs_TipoDe(" + Valor(t) + ")";
            if (entero) return nombre;
            imports.Add("CsSystem");
            return "CsSystem.Type_Name(" + nombre + ")";
        }

        /// Lo que se pide a un tipo (`.Name`, `.FullName`, `.IsAbstract`…): lo que dice su declaración.
        string MiembroDeTipo(MemberAccessExpressionSyntax ma, TipoDe t, ISymbol s)
        {
            if (s.Name == "FullName") return NombreDeTipo(t, true, ma);
            if (s.Name == "Name") return NombreDeTipo(t, false, ma);
            var ex = Exacto(t) as INamedTypeSymbol;
            if (ex == null) return Rechaza(ma, Porque.Fuera(Porque.Reflexion));
            bool v;
            switch (s.Name)
            {
                case "IsAbstract": v = ex.IsAbstract || ex.IsStatic || ex.TypeKind == TypeKind.Interface; break;
                case "IsSealed": v = ex.IsSealed || ex.IsStatic || ex.IsValueType; break;
                case "IsClass": v = ex.TypeKind == TypeKind.Class; break;
                case "IsInterface": v = ex.TypeKind == TypeKind.Interface; break;
                case "IsEnum": v = ex.TypeKind == TypeKind.Enum; break;
                case "IsValueType": v = ex.IsValueType; break;
                default: return Rechaza(ma, Porque.Fuera(Porque.Reflexion));
            }
            return Y(Hay(t, ma), v ? "true" : "false");
        }

        /// `Cs_TipoDe(o)`: el nombre del tipo de un objeto, cuando sólo se sabe al ejecutar. De las hijas a los padres, porque
        /// `is` también dice sí con una hija; las cajas de BP, con el suyo de .NET; y si no es de nadie, un `new object()`.
        void EmiteTipoDe()
        {
            int Hondo(INamedTypeSymbol c) { int n = 0; for (var b = c.BaseType; b != null; b = b.BaseType) n++; return n; }
            var clases = tiposDelPrograma.Where(c => !c.IsGenericType && !c.IsAbstract && !c.IsStatic
                                                     && (c.TypeKind == TypeKind.Class || EsStructSinEstado(c)))
                                         .OrderByDescending(Hondo).ToList();
            imports.Add("CsSystem");
            Linea("");
            Linea("public function string Cs_TipoDe(Object o)");       // public: lo llaman las clases del módulo
            Linea("{");
            sangria++;
            Linea("CsSystem.Type_Hay(o);");
            foreach (var c in clases) Si("o is " + NombreTipo(c), NombreNet(c));
            Si("o is Integer", "System.Int32");
            Si("o is Long", "System.Int64");
            Si("o is Double", "System.Double");
            Si("o is Float", "System.Single");
            Si("o is Boolean", "System.Boolean");
            Si("CsSystem.Type_Clase(o) == CsSystem.Type_Clase(\"\")", "System.String");     // JC no escribe `o is string`
            Linea("return \"System.Object\";");
            sangria--;
            Linea("}");
        }

        /// ¿Hay clases genéricas? Sin sus argumentos, `Cs_TipoDe` y `CsSystem.Type_Misma` no las distinguen.
        bool HayGenericas() => tiposDelPrograma.Any(c => c.IsGenericType && c.TypeKind == TypeKind.Class);

        void Si(string condicion, string nombre)
        {
            Linea("if (" + condicion + ")");
            Linea("{");
            Linea("   return " + Cadena(nombre) + ";");
            Linea("}");
        }
    }
}
