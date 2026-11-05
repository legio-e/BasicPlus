// Los genéricos de C# en JC (V8/H8, `#793`; docs/V8_IDEAS.md §E9). JC no lleva una variable `T`: una clase se compila con su tipo
// base, y `C<X>` es una orden al compilador. Aquí se borra `T` y se escribe lo que hay:
//   - `class C<T>` es `@element{B} class C`, con `Object` o `B` (la restricción) donde C# dice `T`;
//   - `C<Sensor>` se escribe tal cual, y `C<int>`, `C<Integer>`: en JC, entre `<` y `>` va una clase;
//   - con dos parámetros o más, y en los métodos genéricos, no hay nada que escribir: la clase a secas, y el cast en cada uso;
//   - lo que entra por un `T`, si es un número, en su caja; lo que sale, fuera de ella, y con su cast si JC no sabe el tipo;
//   - lo que pide saber `T` al ejecutar (`new T()`, `default(T)`, `x is T`, `T[]`, `x is C<int>`) no se traduce: el borrado no lo da;
//   - JC lee X donde la firma dice el tipo base: si ahí hay un `object` de verdad, o un `U` ya sin escribir, la clase va a secas;
//   - dos sobrecargas que ya sin `T` son la misma, como las de los enums: la segunda lleva otro nombre (BuscaSobrecargasIguales);
//   - lo que redefine o implementa con otra firma lo que se declaró con `T` (`int Get ()` y `T Get ()`) no se traduce: en JC no es
//     el mismo método.
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// La expresión que se está escribiendo como destino de una asignación: ahí no se saca nada de la caja.
        ExpressionSyntax destino;

        /// Las clases genéricas ya escritas, por su nombre: `C` y `C<T>` son dos clases en C#, y en JC tendrían el mismo.
        readonly HashSet<string> nombresDeGenericas = new HashSet<string>();

        /// Lo que dice {@link LlevaElemento} de cada clase genérica.
        readonly Dictionary<INamedTypeSymbol, bool> conElemento = new Dictionary<INamedTypeSymbol, bool>(SymbolEqualityComparer.Default);

        const string SinArrays = "un array como T: en JC, un array no es un objeto";
        const string CompuestoDeT = "un compuesto sobre lo que se declaró con T: en JC, es un Object";

        static bool EsListaGenerica(ITypeSymbol t) =>
            t != null && t.OriginalDefinition.ToDisplayString() == "System.Collections.Generic.List<T>";

        /// ¿Menciona algún parámetro de tipo? `T`, `List<T>`, `T[]`.
        static bool ConParametros(ITypeSymbol t)
        {
            if (t is ITypeParameterSymbol) return true;
            if (t is IArrayTypeSymbol a) return ConParametros(a.ElementType);
            return t is INamedTypeSymbol n && n.TypeArguments.Any(ConParametros);
        }

        /// `T`, en JC: su restricción, si es una clase o una interfaz del programa; si no, `Object`.
        string TipoDeParametro(ITypeParameterSymbol tp, SyntaxNode donde)
        {
            if (tp.ConstraintTypes.Length > 1) return Rechaza(donde, Porque.Pendiente("un parámetro de tipo con varias restricciones"));
            foreach (var c in tp.ConstraintTypes)
            {
                if (c is ITypeParameterSymbol u) return TipoDeParametro(u, donde);
                if (c is INamedTypeSymbol n && DelPrograma(n)) return NombreTipo(n);
                return Rechaza(donde, Porque.DeNet("restricción " + c.OriginalDefinition.ToDisplayString()));
            }
            return "Object";
        }

        /// Lo que va entre `<` y `>`: una clase, una interfaz o `string`, y un número, su caja. null si no se escribe: dentro de
        /// lo genérico (`C<T>`) JC no sabe más que la clase.
        string ArgumentoDeTipo(ITypeSymbol a, SyntaxNode donde)
        {
            if (ConParametros(a)) return null;
            if (a is IArrayTypeSymbol) return Rechaza(donde, Porque.Pendiente(SinArrays));
            if (!a.IsValueType) return Tipo(a, donde);
            return ClaseDeCaja(a) ?? Rechaza(donde, Porque.Pendiente("un genérico de " + NombreCompleto(a)));
        }

        /// Un tipo genérico en JC, o null si no es de los que se escriben.
        string TipoGenerico(INamedTypeSymbol nt, SyntaxNode donde)
        {
            if (EsListaGenerica(nt))
            {
                string arg = ArgumentoDeTipo(nt.TypeArguments[0], donde);
                return arg == null ? "List" : "List<" + arg + ">";
            }
            if (!DelPrograma(nt)) return null;
            string n = NombreTipo(nt);
            if (!LlevaElemento(nt)) return n;
            string x = ArgumentoDeTipo(nt.TypeArguments[0], donde);
            return x == null ? n : n + "<" + x + ">";
        }

        /// ¿Es `C<X>` en JC? Una clase del programa con un solo parámetro, que no sea la anidada de otra genérica, y en la que JC lea
        /// lo que hay: donde la firma dice el tipo base, lee X, y un `object` de verdad no es un X.
        bool LlevaElemento(INamedTypeSymbol nt)
        {
            var d = nt.OriginalDefinition;
            if (d.TypeKind != TypeKind.Class || d.TypeParameters.Length != 1 || !DelPrograma(d)
                || (d.ContainingType != null && d.ContainingType.IsGenericType)) return false;
            if (!conElemento.TryGetValue(d, out bool si))
                conElemento[d] = si = HeredaSuT(d) && SoloTDiceElTipoBase(d);
            return si;
        }

        /// Sus antepasados, si guardan algo, guardan lo mismo: `class Hija<T> : Caja<T>`. En `class Mezcla<T> : Caja<int>`, de `Saca`
        /// sale un int, y `Mezcla<X>` leería un X.
        bool HeredaSuT(INamedTypeSymbol d)
        {
            for (var b = d.BaseType; b != null && DelPrograma(b); b = b.BaseType)
                if (LlevaElemento(b) && !SymbolEqualityComparer.Default.Equals(b.TypeArguments[0], d.TypeArguments[0])) return false;
            return true;
        }

        /// ¿Sólo `T` dice el tipo base? `void F (object o)` en `class C<T>`, y `U G<U> (U u)`, ya sin `U`: en JC son la misma firma
        /// que `T`, y `C<X>` la leería como X: a `G` no le entraría lo que no fuese un X. `Test (int)` junto a `Test (T)`, lo mismo
        /// (el `add(5)` de una lista): a `C<X>.Test (3)`, JC le pide que 3 quepa en un X.
        bool SoloTDiceElTipoBase(INamedTypeSymbol d)
        {
            var t = d.TypeArguments[0];
            string tipoBase = EnFirma(t);
            bool EsT(ITypeSymbol x) => SymbolEqualityComparer.Default.Equals(x, t);
            bool Choca(ITypeSymbol x) => !EsT(x) && EnFirma(x) == tipoBase;
            var metodos = d.GetMembers().OfType<IMethodSymbol>().Where(x => x.MethodKind == MethodKind.Ordinary && !x.IsStatic).ToList();
            foreach (var s in d.GetMembers().Where(x => !x.IsStatic))
                if (s is IFieldSymbol f && Choca(f.Type)) return false;
                else if (s is IPropertySymbol p && (Choca(p.Type) || p.Parameters.Any(a => Choca(a.Type)))) return false;
            foreach (var a in metodos)
            {
                if (Choca(a.ReturnType) || a.Parameters.Any(x => Choca(x.Type))) return false;
                for (int i = 0; i < a.Parameters.Length; i++)
                    if (EsT(a.Parameters[i].Type) && metodos.Any(o => !SymbolEqualityComparer.Default.Equals(o, a) && o.Name == a.Name
                                                                     && o.Parameters.Length == a.Parameters.Length && o.Parameters[i].Type.IsValueType))
                        return false;
            }
            return true;
        }

        /// Lo que una clase genérica no puede ser en JC. Sus campos estáticos: en C# hay unos por cada `C<X>`.
        bool GenericaSeEscribe(INamedTypeSymbol s, SyntaxNode donde)
        {
            if (!s.IsGenericType) return true;
            string n = NombreTipo(s);
            if (!nombresDeGenericas.Add(n) || tiposDelPrograma.Count(t => NombreTipo(t) == n) > 1)
            {
                Rechaza(donde, Porque.Pendiente("dos clases con el mismo nombre y distintos parámetros de tipo"));
                return false;
            }
            if (s.GetMembers().Any(x => x.IsStatic && (x is IFieldSymbol f && !f.IsConst || x is IPropertySymbol)))
            {
                Rechaza(donde, Porque.Pendiente("lo estático de una clase genérica: en C# hay uno por cada C<X>"));
                return false;
            }
            return true;
        }

        /// Lo que dice un método en JC, ya sin `T`: `Object Get ()` en `I<T>`, e `int Get ()` en una `class X : I<int>`.
        string FirmaBorrada(IMethodSymbol ms) => EnFirma(ms.OriginalDefinition.ReturnType) + " " + FirmaEnJc(ms.OriginalDefinition);

        /// ¿Implementa algo de una interfaz con otra firma? `class X : I<int>`: el `int Get ()` de `X`, en JC, no es el `Object Get ()`
        /// de la interfaz.
        bool ImplementaConOtraFirma(INamedTypeSymbol s) =>
            s.AllInterfaces.Where(i => DelPrograma(i)).SelectMany(i => i.GetMembers().OfType<IMethodSymbol>())
             .Any(im => s.FindImplementationForInterfaceMember(im) is IMethodSymbol cm && FirmaBorrada(cm) != FirmaBorrada(im));

        /// ¿Redefine con otra firma lo que el padre declara con `T`? `int Get ()` en una hija de `A<int>`: en JC, el padre dice
        /// `Object Get ()`, y no es el mismo método.
        bool RedefineConOtraFirma(ISymbol s)
        {
            switch (s)
            {
                case IMethodSymbol ms when ms.OverriddenMethod != null && DelPrograma(ms.OverriddenMethod):
                    return FirmaBorrada(ms) != FirmaBorrada(ms.OverriddenMethod);
                case IPropertySymbol ps when ps.OverriddenProperty != null && DelPrograma(ps.OverriddenProperty):
                    return EnFirma(ps.OriginalDefinition.Type) != EnFirma(ps.OverriddenProperty.OriginalDefinition.Type);
                default:
                    return false;
            }
        }

        /// Un tipo sin sus argumentos, como se escribe en un cast: la `C` a secas entra en una `C<X>`.
        string TipoPlano(ITypeSymbol t, SyntaxNode donde)
        {
            if (!(t is INamedTypeSymbol n && n.IsGenericType)) return Tipo(t, donde);
            return EsListaGenerica(n) ? "List" : DelPrograma(n) ? NombreTipo(n) : Tipo(t, donde);
        }

        /// ¿Hay que saber los argumentos al ejecutar? Un `T`; y `x is C<int>`: sin argumentos, una `C<int>` no se distingue de una
        /// `C<string>`.
        bool PideArgumentos(ITypeSymbol t) =>
            ConParametros(t) || (t is INamedTypeSymbol n && n.TypeArguments.Length > 0 && (DelPrograma(n) || EsListaGenerica(n)));

        /// ¿Se declaró con `T`, y aquí es lo que `T` vale? `p.Segundo` en un `Par<string, int>`: en JC, un Object.
        bool EsDeT(ExpressionSyntax e) =>
            Declarado(m.GetSymbolInfo(e).Symbol) is ITypeParameterSymbol tp && SeBorra(tp) && !(m.GetTypeInfo(e).Type is ITypeParameterSymbol);

        /// ¿Es el `T` de algo del programa, o el de `List<T>`? El de lo demás (`Nullable<T>`) lo resuelve la biblioteca.
        bool SeBorra(ITypeSymbol t) =>
            t is ITypeParameterSymbol tp
            && (tp.DeclaringMethod != null ? DelPrograma(tp.DeclaringMethod) : DelPrograma(tp.DeclaringType) || EsListaGenerica(tp.DeclaringType));

        /// El tipo con el que se declaró lo que da esta expresión, sin sustituir nada.
        static ITypeSymbol Declarado(ISymbol s)
        {
            switch (s?.OriginalDefinition)
            {
                case IMethodSymbol ms: return ms.ReturnType;
                case IPropertySymbol ps: return ps.Type;
                case IFieldSymbol f: return f.Type;
                default: return null;
            }
        }

        /// El objeto de `r.m()`, `r.p` o `r[i]`.
        static ExpressionSyntax ReceptorDe(ExpressionSyntax e)
        {
            switch (e)
            {
                case InvocationExpressionSyntax inv: return (inv.Expression as MemberAccessExpressionSyntax)?.Expression;
                case MemberAccessExpressionSyntax ma: return ma.Expression;
                case ElementAccessExpressionSyntax ea: return ea.Expression;
                case ParenthesizedExpressionSyntax p: return ReceptorDe(p.Expression);
                default: return null;
            }
        }

        /// ¿JC sabe de esta expresión que es un `C<X>`? Lo sabe de lo que se declaró así, de un `new`, y de lo que sale de otro
        /// `C<X>`. De lo que se declaró con `T`, no: en JC, ahí hay un `Object`.
        bool JcConArgumentos(ExpressionSyntax e)
        {
            switch (e)
            {
                case ParenthesizedExpressionSyntax p: return JcConArgumentos(p.Expression);
                case ObjectCreationExpressionSyntax _: return true;
                case CastExpressionSyntax _: return false;
            }
            var s = m.GetSymbolInfo(e).Symbol;
            ITypeSymbol declarado;
            switch (s)
            {
                case ILocalSymbol l: declarado = l.Type; break;
                case IParameterSymbol p: declarado = p.OriginalDefinition.Type; break;
                default: declarado = Declarado(s); break;
            }
            if (declarado == null) return false;
            if (!ConParametros(declarado)) return true;
            return declarado is ITypeParameterSymbol tp && JcLoLee(tp, e);
        }

        /// Lo que sale de `e`, que se declaró con `tp`: ¿lo lee JC como un X? Si lo declara una clase que lo guarda, y sale de un
        /// `C<X>` que JC conoce.
        bool JcLoLee(ITypeParameterSymbol tp, ExpressionSyntax e) =>
            tp.TypeParameterKind == TypeParameterKind.Type && (LlevaElemento(tp.DeclaringType) || EsListaGenerica(tp.DeclaringType))
            && ReceptorDe(e) is ExpressionSyntax r && SaleConSuTipo(r);

        /// ¿Lo que sale de este objeto ya es un X en JC? Si su tipo es `C<X>`, y JC lo sabe.
        bool SaleConSuTipo(ExpressionSyntax receptor)
        {
            var t = m.GetTypeInfo(receptor).Type as INamedTypeSymbol;
            return t != null && (LlevaElemento(t) || EsListaGenerica(t)) && !ConParametros(t) && JcConArgumentos(receptor);
        }

        /// Lo que sale de un `T`: fuera de su caja, y con su cast si JC no sabe el tipo.
        string SaleDeGenerico(ExpressionSyntax e, string x)
        {
            if (x == Hueco || ReferenceEquals(e, destino)) return x;
            if (!(e is InvocationExpressionSyntax || e is MemberAccessExpressionSyntax || e is ElementAccessExpressionSyntax
                  || e is IdentifierNameSyntax)) return x;
            var s = m.GetSymbolInfo(e).Symbol;
            if (!(Declarado(s) is ITypeParameterSymbol tp) || !SeBorra(tp)) return x;
            var real = m.GetTypeInfo(e).Type;
            if (real == null || real is ITypeParameterSymbol || real.SpecialType == SpecialType.System_Void) return x;
            bool conTipo = JcLoLee(tp, e);
            if (real.IsValueType)
            {
                string caja = ClaseDeCaja(real);
                if (caja == null) return Rechaza(e, Porque.Pendiente("un genérico de " + NombreCompleto(real)));
                return conTipo ? x + ".value()" : "((" + caja + ") " + x + ").value()";
            }
            if (conTipo || real.SpecialType == SpecialType.System_Object) return x;
            return "((" + TipoPlano(real, e) + ") " + x + ")";
        }

        /// Un argumento o un valor que va a un `T`: un número, en su caja.
        string EntraEnGenerico(ExpressionSyntax e, ITypeSymbol declarado)
        {
            var t = m.GetTypeInfo(e).ConvertedType ?? m.GetTypeInfo(e).Type;
            if (SeBorra(declarado) && t is IArrayTypeSymbol) return Rechaza(e, Porque.Pendiente(SinArrays));
            if (!SeBorra(declarado) || t == null || !t.IsValueType || t is ITypeParameterSymbol) return Expr(e);
            string caja = ClaseDeCaja(t);
            return caja == null ? Rechaza(e, Porque.Pendiente("un genérico de " + NombreCompleto(t))) : "new " + caja + "(" + Expr(e) + ")";
        }

        /// Los argumentos de lo que se declaró con `T`.
        string ArgumentosEnGenerico(IEnumerable<ArgumentSyntax> args, IEnumerable<IParameterSymbol> declarados)
        {
            var ps = declarados.ToList();
            return string.Join(", ", args.Select((a, i) => EntraEnGenerico(a.Expression, i < ps.Count ? ps[i].Type : null)));
        }

        /// El lado izquierdo de una asignación, o una llamada como sentencia: se escribe lo que hay, sin sacar nada de una caja.
        string TalCual(ExpressionSyntax e)
        {
            var antes = destino;
            destino = e;
            try { return Expr(e); }
            finally { destino = antes; }
        }
    }
}
