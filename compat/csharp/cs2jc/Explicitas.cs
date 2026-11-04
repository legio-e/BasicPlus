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

        /// El nombre en JC de un miembro: el suyo, o el que lleva si es de una interfaz y chocaba.
        string NombreDe(ISymbol s) => deInterfaz.TryGetValue(s.OriginalDefinition, out var n) ? n : Ident(s.Name);

        /// Antes de escribir nada: qué miembros de interfaz cambian de nombre. Una clase y sus padres del programa: una hija
        /// sobrescribiría, sin querer, lo que en JC se llama igual en su padre.
        void BuscaChoques(IEnumerable<INamedTypeSymbol> tipos)
        {
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
                        Linea("   get { return this." + Ident(ps.Name) + "; }");
                        if (ps.SetMethod != null) Linea("   set { this." + Ident(ps.Name) + " = value; }");
                        Linea("}");
                    }
                }
        }
    }
}
