// Las implementaciones explícitas que chocan (V8/H8, tema 2; 4-nov; docs/V8_IDEAS.md §E9). En JC una implementación
// explícita es un método público con el nombre del de la interfaz; si la clase tiene otro miembro que se escribiría igual
// (`int IFoo.Hello()` y `public int Hello()`), u otra explícita de otra interfaz (`void IA.Stop()` y `void IB.Stop()`), son dos
// cuerpos para un nombre. Entonces lleva otro nombre el miembro de la INTERFAZ, en todo el programa: `IFoo_Hello`. La
// explícita se escribe con él, lo que llama por la interfaz también, y una clase que la implementa con un miembro normal
// añade un método que le reenvía. Lo de la clase no cambia de nombre: quien lo llama por la clase, o lo sobrescribe, no lo nota.
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// Los miembros de interfaz del programa que en JC llevan otro nombre.
        readonly Dictionary<ISymbol, string> deInterfaz = new Dictionary<ISymbol, string>(SymbolEqualityComparer.Default);
        /// Las explícitas de una interfaz que no es del programa (IDisposable) y chocan: ésa no se puede renombrar.
        readonly HashSet<ISymbol> explicitasDeFuera = new HashSet<ISymbol>(SymbolEqualityComparer.Default);

        /// El miembro de interfaz que implementa `x`, si es una implementación explícita; si no, null.
        static ISymbol InterfazDe(ISymbol x) =>
            (x as IMethodSymbol)?.ExplicitInterfaceImplementations.FirstOrDefault()
            ?? (ISymbol) (x as IPropertySymbol)?.ExplicitInterfaceImplementations.FirstOrDefault();

        /// Los miembros de una clase que ocultan uno de su padre (`new`, o sin él, con el aviso de C#). En JC todo método es
        /// virtual y lo que se llama igual en una hija sobrescribe: el que oculta lleva otro nombre (`Hija_M`). Qué miembro usa cada
        /// sitio lo dice Roslyn, así que `b.M()` con `b` del padre sigue yendo al del padre, y con `b` de la hija, al suyo.
        readonly Dictionary<ISymbol, string> ocultos = new Dictionary<ISymbol, string>(SymbolEqualityComparer.Default);

        /// El nombre en JC de un miembro: el suyo; o el que lleva si es de una interfaz y chocaba, si oculta a otro, o si
        /// sobrescribe a uno que lo llevaba.
        string NombreDe(ISymbol s)
        {
            for (var x = s; x != null; x = (x as IMethodSymbol)?.OverriddenMethod ?? (ISymbol) (x as IPropertySymbol)?.OverriddenProperty)
            {
                var o = x.OriginalDefinition;
                if (deInterfaz.TryGetValue(o, out var n) || ocultos.TryGetValue(o, out n)) return n;
            }
            return Ident(s.Name);
        }

        /// Antes de escribir nada: qué miembros ocultan a otro. Si uno implementa además algo de una interfaz, ése cambia de nombre
        /// también: con el suyo, sobrescribiría al del padre; la clase le reenvía (Reenvia).
        void BuscaOcultos(IEnumerable<INamedTypeSymbol> tipos)
        {
            foreach (var c in tipos.Where(t => t.TypeKind == TypeKind.Class))
            {
                foreach (var x in c.GetMembers().Where(x => OcultaAlPadre(x, c)))
                {
                    ocultos[x.OriginalDefinition] = NombreTipo(c) + "_" + Ident(x.Name);
                    foreach (var i in c.AllInterfaces.Where(DelPrograma))
                        foreach (var mi in i.GetMembers())
                            if (SymbolEqualityComparer.Default.Equals(c.FindImplementationForInterfaceMember(mi), x))
                                deInterfaz[mi.OriginalDefinition] = NombreTipo(i) + "_" + Ident(mi.Name);
                }
                // dos propiedades que sólo se diferencian en la primera letra (`p` y `P`, test-56): en JC, las dos, `getP`
                var vistas = new HashSet<string>();
                foreach (var x in c.GetMembers().Where(EsPropiedadEnJc).OrderBy(x => char.IsUpper(x.Name[0]) ? 0 : 1))
                    if (!vistas.Add(char.ToUpperInvariant(x.Name[0]) + x.Name.Substring(1)) && !ocultos.ContainsKey(x.OriginalDefinition))
                        ocultos[x.OriginalDefinition] = NombreTipo(c) + "_" + Ident(x.Name);
            }
        }

        /// ¿Es en JC una propiedad, con su `getX`? Las de C#, y los campos públicos («el estado sale por getters y setters»).
        bool EsPropiedadEnJc(ISymbol x) =>
            !x.IsImplicitlyDeclared && ((x is IPropertySymbol p && !p.IsIndexer) || (x is IFieldSymbol f && !f.IsConst && AccesoDe(f) == "public"));

        /// ¿Oculta `x` a un miembro de un padre del programa? Un método, con los mismos parámetros; lo privado no se ve, un
        /// tipo anidado se escribe aparte, y lo estático va con su clase delante (un campo también cambia: en JC es una propiedad).
        bool OcultaAlPadre(ISymbol x, INamedTypeSymbol c)
        {
            if (x.IsImplicitlyDeclared) return false;
            switch (x)
            {
                case IMethodSymbol ms when ms.MethodKind == MethodKind.Ordinary && !ms.IsStatic && !ms.IsOverride: break;
                case IPropertySymbol ps when !ps.IsStatic && !ps.IsOverride && !ps.IsIndexer && ps.ExplicitInterfaceImplementations.Length == 0: break;
                case IFieldSymbol _: break;
                default: return false;
            }
            for (var b = c.BaseType; b != null && DelPrograma(b); b = b.BaseType)
                foreach (var o in b.GetMembers(x.Name))
                {
                    if (o is INamedTypeSymbol || o.DeclaredAccessibility == Accessibility.Private) continue;
                    if (o is IMethodSymbol om && om.MethodKind != MethodKind.Ordinary) continue;
                    if (o.IsStatic && !(x is IFieldSymbol)) continue;
                    if (o is IMethodSymbol a && x is IMethodSymbol d && !MismosParametros(a, d)) continue;
                    return true;
                }
            return false;
        }

        /// Antes de escribir nada: qué miembros de interfaz cambian de nombre. Una clase y sus padres del programa: una hija
        /// sobrescribiría, sin querer, lo que en JC se llama igual en su padre.
        void BuscaChoques(IEnumerable<INamedTypeSymbol> tipos)
        {
            BuscaOcultos(tipos);
            foreach (var s in tipos.Where(t => t.TypeKind == TypeKind.Class || t.TypeKind == TypeKind.Struct))
            {
                var miembros = new List<ISymbol>();
                for (var c = s; c != null && DelPrograma(c); c = c.BaseType)
                    miembros.AddRange(c.GetMembers().Where(x => !x.IsImplicitlyDeclared
                        && ((x is IMethodSymbol ms && (ms.MethodKind == MethodKind.Ordinary || ms.MethodKind == MethodKind.ExplicitInterfaceImplementation))
                            || x is IPropertySymbol || x is IFieldSymbol)));
                foreach (var e in miembros.Where(x => InterfazDe(x) != null))
                {
                    if (!miembros.Any(o => Choca(e, o))) continue;
                    var i = InterfazDe(e);
                    if (DelPrograma(i)) deInterfaz[i.OriginalDefinition] = NombreTipo(i.ContainingType) + "_" + Ident(i.Name);
                    else explicitasDeFuera.Add(e);
                }
            }
        }

        /// ¿Se escribiría `o` igual que la explícita `e` en JC? El mismo nombre y, entre métodos, los mismos parámetros; la
        /// misma explícita en una hija es lo que C# llama reimplementarla, y en JC sobrescribe, que es lo que hace.
        static bool Choca(ISymbol e, ISymbol o)
        {
            var i = InterfazDe(e);
            var j = InterfazDe(o);
            if (SymbolEqualityComparer.Default.Equals(e, o) || (j ?? o).Name != i.Name) return false;
            if (j != null && SymbolEqualityComparer.Default.Equals(i.OriginalDefinition, j.OriginalDefinition)) return false;
            return !(e is IMethodSymbol a && o is IMethodSymbol b) || MismosParametros(a.Parameters, b.Parameters);
        }

        /// Lo que falta en una clase que implementa con un miembro normal uno de interfaz que cambió de nombre: un método (o
        /// una propiedad) con ese nombre, que le reenvía. Si su padre ya lo hace, y con el mismo, no hace falta.
        void Reenvia(INamedTypeSymbol s)
        {
            foreach (var i in s.AllInterfaces.Where(DelPrograma))
                foreach (var mi in i.GetMembers().Where(x => deInterfaz.ContainsKey(x.OriginalDefinition)))
                {
                    var impl = s.FindImplementationForInterfaceMember(mi);
                    if (impl == null || InterfazDe(impl) != null) continue;      // la explícita ya lleva ese nombre
                    var padre = s.BaseType;
                    if (padre != null && padre.AllInterfaces.Contains(i, SymbolEqualityComparer.Default)
                        && SymbolEqualityComparer.Default.Equals(padre.FindImplementationForInterfaceMember(mi), impl)) continue;
                    var donde = Declaracion(s);
                    Linea("@implement{ " + NombreTipo(i) + " }");
                    if (impl is IMethodSymbol ms)
                    {
                        Linea("public function " + Tipo(ms.ReturnType, donde) + " " + NombreDe(mi) + "("
                              + string.Join(", ", ms.Parameters.Select(p => Tipo(p.Type, donde) + " " + Ident(p.Name))) + ")");
                        Linea("{");
                        Linea("   " + (ms.ReturnsVoid ? "" : "return ") + "this." + NombreMetodo(ms) + "("
                              + string.Join(", ", ms.Parameters.Select(p => Ident(p.Name))) + ");");
                        Linea("}");
                    }
                    else if (impl is IPropertySymbol ps)
                    {
                        Linea("public " + Tipo(ps.Type, donde) + " " + NombreDe(mi));
                        Linea("{");
                        Linea("   get { return this." + NombreDe(ps) + "; }");
                        if (ps.SetMethod != null) Linea("   set { this." + NombreDe(ps) + " = value; }");
                        Linea("}");
                    }
                }
        }
    }
}
