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

        // ----------------------------------------------------------------------------------- las excepciones de .NET

        /// Las de la librería CsSystem (`compat/csharp/lib/CsSystem.jc`, el escalón 1: V8_IDEAS §E9), con la jerarquía
        /// y los mensajes de .NET; el número, cuántas cadenas admiten sus constructores como mucho: (), (string) y, las
        /// de los argumentos, (string, string).
        static readonly Dictionary<string, int> ExcepcionesDeCsSystem = new Dictionary<string, int>
        {
            ["System.ApplicationException"] = 1,
            ["System.ArgumentException"] = 2,
            ["System.ArgumentNullException"] = 2,
            ["System.ArgumentOutOfRangeException"] = 2,
            ["System.InvalidOperationException"] = 1,
            ["System.NotImplementedException"] = 1,
            ["System.NotSupportedException"] = 1,
            ["System.FormatException"] = 1,
        };

        /// El nombre en JC de una excepción de .NET que se sabe pasar, o null: `System.Exception`, la `Exception` de
        /// Core; `System.SystemException`, `RuntimeError` —lo que lanza la VM: un `catch (SystemException)` atrapa
        /// también un `null` o un índice fuera, como en .NET—; las de la librería, `CsSystem.X`, con su import. Las que
        /// lanza la VM (NullReferenceException…) no: un catch suyo no atraparía el RuntimeError de la VM.
        string ExcepcionJc(ITypeSymbol t)
        {
            if (t == null) return null;
            string n = NombreCompleto(t);
            if (n == "System.Exception") return "Exception";
            if (n == "System.SystemException") return "RuntimeError";
            if (!ExcepcionesDeCsSystem.ContainsKey(n)) return null;
            imports.Add("CsSystem");
            return "CsSystem." + t.Name;
        }

        /// El mensaje que pone .NET a una excepción creada sin él, cuando su padre de BP no tiene constructor sin
        /// argumentos (la `Exception` y el `RuntimeError` de Core piden el suyo): «Exception of type 'N.A+B' was
        /// thrown.» y, de `SystemException`, «System error.». Las de la librería lo tienen: null.
        static string MensajeSinArgumentos(ITypeSymbol padre, INamedTypeSymbol clase)
        {
            switch (NombreCompleto(padre))
            {
                case "System.Exception": return MensajePorDefecto(clase);
                case "System.SystemException": return "System error.";
            }
            return null;
        }

        /// ¿Tiene la librería el constructor de su excepción al que llama C#? Los suyos sólo llevan cadenas; con la causa
        /// (una `Exception`), el traductor todavía no; otro, la biblioteca. Si no, deja el rechazo y devuelve false.
        bool ConstructorDeCsSystem(IMethodSymbol ctor, ITypeSymbol tipo, SyntaxNode donde)
        {
            if (ctor == null) { Rechaza(donde, Porque.Pendiente("un constructor sin resolver")); return false; }
            if (ctor.Parameters.Any(p => ExcepcionJc(p.Type) != null || NombreCompleto(p.Type) == "System.Exception"))
            {
                Rechaza(donde, Porque.Pendiente("una excepción con su causa (innerException)"));
                return false;
            }
            if (ctor.Parameters.All(p => p.Type.SpecialType == SpecialType.System_String)
                && ctor.Parameters.Length <= ExcepcionesDeCsSystem[NombreCompleto(tipo)])
                return true;
            Rechaza(donde, Porque.DeNet("new " + NombreCompleto(tipo) + "("
                + string.Join(", ", ctor.Parameters.Select(p => NombreCompleto(p.Type))) + ")"));
            return false;
        }

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
            return Rechaza(inv, Porque.DeNet(clave));
        }

        string BibliotecaMiembro(MemberAccessExpressionSyntax ma, ISymbol s)
        {
            string clave = NombreCompleto(s.ContainingType) + "." + s.Name;
            // el mensaje de una excepción: el msg de la de BP (ArgumentException y sus hijas sobrescriben Message en .NET:
            // sale con su clase)
            if (s.Name == "Message" && ExcepcionJc(s.ContainingType) != null) return Expr(ma.Expression) + ".msg";
            switch (clave)
            {
                case "System.Exception.Message": return Expr(ma.Expression) + ".msg";
                case "System.ArgumentException.ParamName": return Expr(ma.Expression) + ".ParamName";
                case "System.Array.Length": return "len(" + Expr(ma.Expression) + ")";
                // en caracteres; C# cuenta unidades de UTF-16: lo mismo salvo los de fuera del plano básico
                case "System.String.Length": return "strlen(" + Expr(ma.Expression) + ")";
                // el fin de línea de la plataforma (#755): el eol() de Core
                case "System.Environment.NewLine": return "eol()";
            }
            return Rechaza(ma, Porque.DeNet(clave));
        }

        string BibliotecaNuevo(ObjectCreationExpressionSyntax oc, IMethodSymbol ctor, INamedTypeSymbol tipo)
        {
            if (ExcepcionJc(tipo) is string ex)
            {
                // Exception y RuntimeError (SystemException) piden su mensaje: el de .NET, si C# no lo da
                if (MensajeSinArgumentos(tipo, tipo) is string porDefecto)
                    return "new " + ex + "(" + ArgsExcepcion(oc.ArgumentList, porDefecto) + ")";
                if (!ConstructorDeCsSystem(ctor, tipo, oc)) return Hueco;
                return "new " + ex + "(" + (oc.ArgumentList != null ? Argumentos(oc.ArgumentList, ctor) : "") + ")";
            }
            return Rechaza(oc, Porque.DeNet("new " + NombreCompleto(tipo)));
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
