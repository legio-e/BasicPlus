// La biblioteca de .NET en la de BP (V8/H6, docs/V8_IDEAS.md §E9): una tabla de lo que el traductor sabe pasar. Lo
// que no está en ella es «no se traduce», con su nombre: así el informe de un bloque dice qué se pide más.
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// El nombre entero de un tipo de .NET, como en el censo de #745: `System.String`, no `string`.
        static readonly SymbolDisplayFormat Completo = new SymbolDisplayFormat(
            typeQualificationStyle: SymbolDisplayTypeQualificationStyle.NameAndContainingTypesAndNamespaces,
            genericsOptions: SymbolDisplayGenericsOptions.IncludeTypeParameters);

        static string NombreCompleto(ITypeSymbol t) => t == null ? "?" : t.ToDisplayString(Completo);

        string BibliotecaLlamada(InvocationExpressionSyntax inv, IMethodSymbol ms)
        {
            string clave = NombreCompleto(ms.ContainingType) + "." + ms.Name;
            switch (clave)
            {
                case "System.Console.WriteLine": return Escribe(inv, ms, "println");
                case "System.Console.Write": return Escribe(inv, ms, "print");
                case "System.Object.ToString":
                    // el de Object (la clase no lo sobrescribe): el toString de BP (sale `object@…`, no el nombre)
                    if (inv.ArgumentList.Arguments.Count == 0)
                    {
                        if (inv.Expression is IdentifierNameSyntax) return "this.toString()";
                        if (inv.Expression is MemberAccessExpressionSyntax ma && !(m.GetTypeInfo(ma.Expression).Type?.IsValueType ?? true))
                            return (ma.Expression is BaseExpressionSyntax ? "base" : Expr(ma.Expression)) + ".toString()";
                    }
                    break;
            }
            return Rechaza(inv, Porque.Biblioteca(clave));
        }

        string BibliotecaMiembro(MemberAccessExpressionSyntax ma, ISymbol s)
        {
            string clave = NombreCompleto(s.ContainingType) + "." + s.Name;
            switch (clave)
            {
                case "System.Exception.Message": return Expr(ma.Expression) + ".msg";
            }
            return Rechaza(ma, Porque.Biblioteca(clave));
        }

        string BibliotecaNuevo(ObjectCreationExpressionSyntax oc, IMethodSymbol ctor, INamedTypeSymbol tipo)
        {
            if (EsExcepcionNet(tipo)) return "new Exception(" + ArgsExcepcion(oc.ArgumentList, tipo) + ")";
            return Rechaza(oc, Porque.Biblioteca("new " + NombreCompleto(tipo)));
        }

        /// `Console.WriteLine` y `Console.Write`: `println(…)` y `print(…)`. Con un formato literal, troceado aquí:
        /// `WriteLine("{0}-{1}", a, b)` es `println(a, "-", b)`. Sólo si usa cada argumento una vez y en orden: C# los
        /// calcula todos, de izquierda a derecha, antes de escribir (la segunda regla de fondo).
        string Escribe(InvocationExpressionSyntax inv, IMethodSymbol ms, string fn)
        {
            var args = inv.ArgumentList.Arguments;
            if (args.Any(a => a.NameColon != null)) return Rechaza(inv, Porque.Pendiente("argumentos con nombre"));
            if (args.Count == 0) return fn + "()";
            if (args.Count == 1)
            {
                if (ms.Parameters[0].Type is IArrayTypeSymbol) return Rechaza(inv, Porque.Pendiente("Console.Write de un char[]"));
                return fn + "(" + ExprBase(args[0].Expression) + ")";     // sin la caja de WriteLine(object)
            }
            if (!(args[0].Expression is LiteralExpressionSyntax lit && lit.IsKind(SyntaxKind.StringLiteralExpression)))
                return Rechaza(args[0], Porque.H7(Porque.Formatos));
            if (args.Count == 2 && ms.Parameters.Length == 2 && ms.Parameters[1].IsParams
                && SymbolEqualityComparer.Default.Equals(m.GetTypeInfo(args[1].Expression).ConvertedType, ms.Parameters[1].Type))
                return Rechaza(inv, Porque.Pendiente("un formato con sus argumentos en un array"));
            var piezas = new List<string>();
            var texto = new StringBuilder();
            string f = (string) lit.Token.Value;
            int siguiente = 0;
            for (int i = 0; i < f.Length; i++)
            {
                char c = f[i];
                if (c == '{' && i + 1 < f.Length && f[i + 1] == '{') { texto.Append('{'); i++; continue; }
                if (c == '}' && i + 1 < f.Length && f[i + 1] == '}') { texto.Append('}'); i++; continue; }
                if (c == '}') return Rechaza(args[0], Porque.Pendiente("un formato mal formado"));
                if (c != '{') { texto.Append(c); continue; }
                int cierre = f.IndexOf('}', i);
                if (cierre < 0) return Rechaza(args[0], Porque.Pendiente("un formato mal formado"));
                string dentro = f.Substring(i + 1, cierre - i - 1);
                if (dentro.Contains(':') || dentro.Contains(',')) return Rechaza(args[0], Porque.H7(Porque.Formatos));
                if (!int.TryParse(dentro, out int n) || n + 1 >= args.Count) return Rechaza(args[0], Porque.Pendiente("un formato mal formado"));
                if (n != siguiente) return Rechaza(args[0], Porque.Pendiente("un formato que no usa sus argumentos una vez y en orden"));
                siguiente++;
                if (texto.Length > 0) { piezas.Add(Cadena(texto.ToString())); texto.Clear(); }
                piezas.Add(ExprBase(args[n + 1].Expression));
                i = cierre;
            }
            if (siguiente != args.Count - 1) return Rechaza(args[0], Porque.Pendiente("un formato que no usa sus argumentos una vez y en orden"));
            if (texto.Length > 0) piezas.Add(Cadena(texto.ToString()));
            if (piezas.Count == 0) piezas.Add("\"\"");
            return fn + "(" + string.Join(", ", piezas) + ")";
        }
    }
}
