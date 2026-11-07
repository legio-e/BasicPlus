// H7.4 (7-nov) — `ref` y `out` de C#, en JC: los parámetros `var`, a los que se pasa `objeto::property` (decidido por
// Eduardo: en JC no hay referencias a variables, sólo a properties). Una local que C# pasa por `ref` u `out` vive aquí en
// una CAJA desde que se declara: una clase con una property `v`, una por tipo (`CsRef_int`…), que se escriben al final del
// módulo; sus usos son `x.v`, y en la llamada se pasa `x::v`. Un parámetro `ref` que se reenvía, tal cual. Lo demás (un
// campo, una casilla, un parámetro por valor, `out var`), todavía no.
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// Las locales que C# pasa por `ref` u `out`: van en una caja.
        readonly HashSet<ILocalSymbol> enCaja = new HashSet<ILocalSymbol>(SymbolEqualityComparer.Default);
        /// Las cajas que hacen falta: su nombre y el tipo de su `v`, en JC.
        readonly SortedDictionary<string, string> cajas = new SortedDictionary<string, string>(System.StringComparer.Ordinal);

        static bool EsRefOut(ArgumentSyntax a) =>
            a.RefKindKeyword.IsKind(SyntaxKind.RefKeyword) || a.RefKindKeyword.IsKind(SyntaxKind.OutKeyword);

        static bool EsRefOut(IParameterSymbol p) => p.RefKind == RefKind.Ref || p.RefKind == RefKind.Out;

        /// Antes de escribir nada: las locales que se pasan por `ref` u `out`.
        void BuscaCajas(SyntaxNode raiz)
        {
            foreach (var a in raiz.DescendantNodes().OfType<ArgumentSyntax>())
                if (EsRefOut(a) && a.Expression is IdentifierNameSyntax id && m.GetSymbolInfo(id).Symbol is ILocalSymbol l)
                    enCaja.Add(l);
        }

        /// La caja de un tipo: su nombre (`CsRef_int`), y queda apuntada para escribirla al final.
        string Caja(ITypeSymbol t, SyntaxNode donde)
        {
            string jt = Tipo(t, donde);
            string nombre = "CsRef_" + Regex.Replace(jt, "[^A-Za-z0-9]", "_");
            cajas[nombre] = jt;
            return nombre;
        }

        /// Una local en caja, al declararla: `CsRef_int x = new CsRef_int(valor);`.
        string DeclaraEnCaja(VariableDeclaratorSyntax v, ILocalSymbol l)
        {
            string caja = Caja(l.Type, v);
            string valor = v.Initializer != null ? Expr(v.Initializer.Value) : SinValor(l.Type, v);
            if (valor.StartsWith(" = ")) valor = valor.Substring(3);
            if (valor == "") return Rechaza(v, Porque.Pendiente("una local en caja sin un valor de su tipo"));
            // a un parámetro estrecho no entra un entero sin convertir (test-41: `sbyte c = 1`)
            string jt = cajas[caja];
            if (jt == "sbyte" || jt == "byte" || jt == "short" || jt == "ushort") valor = "(" + jt + ") (" + valor + ")";
            return caja + " " + Ident(v.Identifier.ValueText) + " = new " + caja + "(" + valor + ");";
        }

        /// El argumento de un `ref` u `out`: la property de la caja (`x::v`), u otro parámetro var, tal cual.
        string ArgumentoRef(ArgumentSyntax a)
        {
            if (a.Expression is IdentifierNameSyntax id)
            {
                var s = m.GetSymbolInfo(id).Symbol;
                if (s is IParameterSymbol p && EsRefOut(p)) return Ident(p.Name);
                if (s is ILocalSymbol l && enCaja.Contains(l) && !renombradas.ContainsKey(l)) return Ident(l.Name) + "::v";
            }
            return Rechaza(a, Porque.Pendiente("ref u out de un campo, una casilla, un parámetro por valor o out var"));
        }

        /// ¿Otro método con ese nombre y los mismos tipos que difiere en `ref`, `out` o nada, en la clase o en un padre? En JC
        /// los dos son `var` (o uno no lo es): JC no deja sobrecargas que sólo difieran en eso, y entre una clase y su padre
        /// uno sobreescribiría al otro (test-798: `Foo(out int)` y, en la hija, `Foo(ref int)`).
        static bool SoloDifiereEnRef(IMethodSymbol metodo)
        {
            for (var t = metodo.ContainingType; t != null; t = t.BaseType)
                foreach (var o in t.GetMembers(metodo.Name).OfType<IMethodSymbol>())
                    if (!SymbolEqualityComparer.Default.Equals(o.OriginalDefinition, metodo.OriginalDefinition)
                        && !SymbolEqualityComparer.Default.Equals(o, metodo.OverriddenMethod)
                        && o.Parameters.Length == metodo.Parameters.Length
                        && o.Parameters.Select(x => x.Type).SequenceEqual(metodo.Parameters.Select(x => x.Type), SymbolEqualityComparer.Default)
                        && !o.Parameters.Select(x => x.RefKind).SequenceEqual(metodo.Parameters.Select(x => x.RefKind)))
                        return true;
            return false;
        }

        /// Al final del módulo, cada caja que se usó.
        void EmiteCajas()
        {
            foreach (var c in cajas)
            {
                Linea("");
                Linea("public class " + c.Key);
                Linea("{");
                sangria++;
                Linea("public " + c.Value + " v { get; set; }");
                Linea("");
                Linea("public function " + c.Key + "(" + c.Value + " v)");
                Linea("{");
                sangria++;
                Linea("this.v = v;");
                sangria--;
                Linea("}");
                sangria--;
                Linea("}");
            }
        }
    }
}
