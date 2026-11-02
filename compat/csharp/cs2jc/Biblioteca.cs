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

        // ------------------------------------------------------------------------- las funciones y las constantes

        /// Los métodos estáticos de .NET que tiene la librería, por su firma en C# (la sobrecarga que eligió C#, con sus
        /// tipos): `Math.Abs(int)` es `CsSystem.Math_Abs`, una función del módulo con el nombre de su tipo delante.
        static readonly HashSet<string> FuncionesDeCsSystem = new HashSet<string>
        {
            "System.Math.Abs(System.Int32)", "System.Math.Abs(System.Int64)", "System.Math.Abs(System.Double)",
            "System.Math.Abs(System.Single)",
            "System.Math.Max(System.Int32,System.Int32)", "System.Math.Max(System.Int64,System.Int64)",
            "System.Math.Max(System.Double,System.Double)",
            "System.Math.Min(System.Int32,System.Int32)", "System.Math.Min(System.Int64,System.Int64)",
            "System.Math.Min(System.Double,System.Double)",
            "System.Math.Floor(System.Double)", "System.Math.Ceiling(System.Double)", "System.Math.Round(System.Double)",
            "System.Math.Acos(System.Double)",
            "System.Double.IsNaN(System.Double)", "System.Single.IsNaN(System.Single)",
            "System.Int32.Parse(System.String)", "System.Int64.Parse(System.String)",
            "System.String.Join(System.String,System.String[])",
            "System.Char.IsDigit(System.Char)", "System.Char.IsLetter(System.Char)", "System.Char.IsLetterOrDigit(System.Char)",
            "System.Char.IsWhiteSpace(System.Char)", "System.Char.IsUpper(System.Char)", "System.Char.IsLower(System.Char)",
            "System.Char.ToUpper(System.Char)", "System.Char.ToLower(System.Char)",
        };

        /// Los métodos de instancia de .NET que se saben pasar, por su firma: la función de BP que los hace, con el objeto
        /// delante (`s.StartsWith(p)` es `startsWith(s, p)`): las de Core, y las de CsSystem con su módulo delante.
        static readonly Dictionary<string, string> MetodosNet = new Dictionary<string, string>
        {
            ["System.String.StartsWith(System.String)"] = "startsWith",
            ["System.String.EndsWith(System.String)"] = "endsWith",
            ["System.String.Contains(System.String)"] = "contains",
            ["System.String.IndexOf(System.String)"] = "indexOf",
            ["System.String.Replace(System.String,System.String)"] = "replace",
            ["System.String.ToUpper()"] = "upper",
            ["System.String.ToLower()"] = "lower",
            ["System.String.Trim()"] = "trim",
            ["System.String.Substring(System.Int32)"] = "CsSystem.String_Substring",
            ["System.String.Substring(System.Int32,System.Int32)"] = "CsSystem.String_Substring",
        };

        static string Firma(IMethodSymbol ms) => NombreCompleto(ms.ContainingType) + "." + ms.Name + "("
            + string.Join(",", ms.Parameters.Select(p => NombreCompleto(p.Type))) + ")";

        /// Una constante de .NET (`int.MaxValue`, `Math.PI`, `double.NaN`…), escrita: Roslyn sabe su valor. Como un valor
        /// por defecto, y además NaN y los infinitos, que en una expresión sí se escriben (`0.0 / 0.0`); un negativo,
        /// entre paréntesis; un `char`, su código (en JC, un entero).
        string LiteralDeConstante(object v, SyntaxNode donde)
        {
            var ci = System.Globalization.CultureInfo.InvariantCulture;
            string r;
            switch (v)
            {
                case bool b: return b ? "true" : "false";
                case string s: return Cadena(s);
                case char c: return ((int) c).ToString(ci);
                case int i: r = i == int.MinValue ? "-2147483647 - 1" : i.ToString(ci); break;
                case long l: r = l == long.MinValue ? "-9223372036854775807L - 1L" : l.ToString(ci) + "L"; break;
                // un tipo estrecho, con su tipo: en C# `ushort.MaxValue` es un ushort, y un 65535 a secas, en JC, un int
                // (`return 65535;` en una función ushort no compila: test-701)
                case short _: return "((short) " + System.Convert.ToString(v, ci) + ")";
                case sbyte _: return "((sbyte) " + System.Convert.ToString(v, ci) + ")";
                case byte _: return "((byte) " + System.Convert.ToString(v, ci) + ")";
                case ushort _: return "((ushort) " + System.Convert.ToString(v, ci) + ")";
                case double d:
                    if (double.IsNaN(d)) return "(0.0 / 0.0)";
                    if (double.IsInfinity(d)) return d > 0 ? "(1.0 / 0.0)" : "(-1.0 / 0.0)";
                    r = d.ToString("R", ci);
                    if (!r.Contains('.') && !r.Contains('E')) r += ".0";
                    break;
                case float f:
                    if (float.IsNaN(f)) return "(0.0f / 0.0f)";
                    if (float.IsInfinity(f)) return f > 0 ? "(1.0f / 0.0f)" : "(-1.0f / 0.0f)";
                    r = f.ToString("R", ci);
                    if (!r.Contains('.') && !r.Contains('E')) r += ".0";
                    r += "f";
                    break;
                case uint u: return u.ToString(ci) + "L";                        // un uint es un long (SinSigno.cs)
                case ulong _: case decimal _: return Rechaza(donde, Porque.Fuera(Porque.SinSigno));
                default: return Rechaza(donde, Porque.Pendiente("una constante " + v));
            }
            return r.StartsWith("-") ? "(" + r + ")" : r;
        }

        string BibliotecaLlamada(InvocationExpressionSyntax inv, IMethodSymbol ms)
        {
            if (EsNullable(ms.ContainingType)) return LlamadaDeNullable(inv, ms);      // un T? (Nullables.cs)
            string clave = NombreCompleto(ms.ContainingType) + "." + ms.Name;
            if (ms.IsStatic && FuncionesDeCsSystem.Contains(Firma(ms)))
            {
                imports.Add("CsSystem");
                return "CsSystem." + ms.ContainingType.Name + "_" + ms.Name + "(" + Argumentos(inv.ArgumentList, ms) + ")";
            }
            var objeto = inv.Expression as MemberAccessExpressionSyntax;
            // un método de instancia que se sabe pasar: la función de Core o de CsSystem, con el objeto delante
            if (!ms.IsStatic && objeto != null && MetodosNet.TryGetValue(Firma(ms), out var fn))
            {
                if (fn.StartsWith("CsSystem.")) imports.Add("CsSystem");
                string resto = Argumentos(inv.ArgumentList, ms);
                return fn + "(" + Expr(objeto.Expression) + (resto.Length > 0 ? ", " + resto : "") + ")";
            }
            switch (Firma(ms))
            {
                // el ToString de un entero: el número, como lo escribe BP (y .NET: en decimal, con su signo)
                case "System.Int32.ToString()": case "System.Int64.ToString()": case "System.Int16.ToString()":
                case "System.Byte.ToString()": case "System.SByte.ToString()": case "System.UInt16.ToString()":
                case "System.UInt32.ToString()":
                    if (objeto != null) return "(\"\" + (" + Expr(objeto.Expression) + "))";
                    break;
                // el de un bool: .NET escribe True y False
                case "System.Boolean.ToString()":
                    if (objeto != null) return "(" + Expr(objeto.Expression) + " ? \"True\" : \"False\")";
                    break;
                case "System.String.ToString()":
                    if (objeto != null) return Expr(objeto.Expression);
                    break;
                // el de un char: su carácter (en JC, el char es su código)
                case "System.Char.ToString()":
                    if (objeto != null) { imports.Add("Str"); return "Str.fromCharCode(" + Expr(objeto.Expression) + ")"; }
                    break;
                case "System.Char.ToString(System.Char)":
                    imports.Add("Str");
                    return "Str.fromCharCode(" + Expr(inv.ArgumentList.Arguments[0].Expression) + ")";
                case "System.String.Equals(System.String,System.String)":
                    return "(" + string.Join(" == ", inv.ArgumentList.Arguments.Select(a => Expr(a.Expression))) + ")";
                case "System.String.Equals(System.String)":
                    if (objeto != null) return "(" + Expr(objeto.Expression) + " == " + Expr(inv.ArgumentList.Arguments[0].Expression) + ")";
                    break;
            }
            switch (clave)
            {
                case "System.Console.WriteLine": return Escribe(inv, ms, "println");
                case "System.Console.Write": return Escribe(inv, ms, "print");
                case "System.String.Format": return Formatea(inv, ms);
                // Console.Out.WriteLine(x): lo mismo que Console.WriteLine(x)
                case "System.IO.TextWriter.WriteLine":
                case "System.IO.TextWriter.Write":
                    if (objeto != null && m.GetSymbolInfo(objeto.Expression).Symbol is IPropertySymbol salida
                        && NombreCompleto(salida.ContainingType) + "." + salida.Name == "System.Console.Out")
                        return Escribe(inv, ms, ms.Name == "WriteLine" ? "println" : "print");
                    break;
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
            if (EsNullable(s.ContainingType)) return MiembroDeNullable(ma, s);           // un T? (Nullables.cs)
            string clave = NombreCompleto(s.ContainingType) + "." + s.Name;
            var cv = m.GetConstantValue(ma);
            if (cv.HasValue) return LiteralDeConstante(cv.Value, ma);
            if (clave == "System.String.Empty") return "\"\"";
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
            // new object(): el Object de Core (comprobado en las dos VMs)
            if (tipo.SpecialType == SpecialType.System_Object && (oc.ArgumentList == null || oc.ArgumentList.Arguments.Count == 0)
                && oc.Initializer == null)
                return "new Object()";
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
                return fn + "(" + Texto(args[0].Expression) + ")";        // sin la caja de WriteLine(object); un char, su carácter
            }
            var piezas = Piezas(args, ms, inv);
            if (piezas == null) return Hueco;
            if (piezas.Count == 0) piezas.Add("\"\"");
            return fn + "(" + string.Join(", ", piezas) + ")";
        }

        /// `string.Format("{0}-{1}", a, b)`: la suma de sus piezas, `("" + a + "-" + b)`; con el `""` delante, la suma es
        /// de cadenas aunque las primeras sean números (el escalón 1).
        string Formatea(InvocationExpressionSyntax inv, IMethodSymbol ms)
        {
            var args = inv.ArgumentList.Arguments;
            if (args.Any(a => a.NameColon != null)) return Rechaza(inv, Porque.Pendiente("argumentos con nombre"));
            var piezas = Piezas(args, ms, inv);
            if (piezas == null) return Hueco;
            return "(\"\"" + string.Concat(piezas.Select(p => " + " + (p.StartsWith("\"") ? p : "(" + p + ")"))) + ")";
        }

        /// Un formato literal, troceado: `"{0}-{1}", a, b` son las piezas `a`, `"-"` y `b`. Sólo si usa cada argumento
        /// una vez y en orden: C# los calcula todos, de izquierda a derecha, antes de formatear (la segunda regla de
        /// fondo). Si no se puede, el rechazo, y null.
        List<string> Piezas(SeparatedSyntaxList<ArgumentSyntax> args, IMethodSymbol ms, SyntaxNode donde)
        {
            if (!(args[0].Expression is LiteralExpressionSyntax lit && lit.IsKind(SyntaxKind.StringLiteralExpression)))
            { Rechaza(args[0], Porque.H7(Porque.Formatos)); return null; }
            if (args.Count == 2 && ms.Parameters.Length == 2 && ms.Parameters[1].IsParams
                && SymbolEqualityComparer.Default.Equals(m.GetTypeInfo(args[1].Expression).ConvertedType, ms.Parameters[1].Type))
            { Rechaza(donde, Porque.Pendiente("un formato con sus argumentos en un array")); return null; }
            var piezas = new List<string>();
            var texto = new StringBuilder();
            string f = (string) lit.Token.Value;
            int siguiente = 0;
            for (int i = 0; i < f.Length; i++)
            {
                char c = f[i];
                if (c == '{' && i + 1 < f.Length && f[i + 1] == '{') { texto.Append('{'); i++; continue; }
                if (c == '}' && i + 1 < f.Length && f[i + 1] == '}') { texto.Append('}'); i++; continue; }
                if (c == '}') { Rechaza(args[0], Porque.Pendiente("un formato mal formado")); return null; }
                if (c != '{') { texto.Append(c); continue; }
                int cierre = f.IndexOf('}', i);
                if (cierre < 0) { Rechaza(args[0], Porque.Pendiente("un formato mal formado")); return null; }
                string dentro = f.Substring(i + 1, cierre - i - 1);
                if (dentro.Contains(':') || dentro.Contains(',')) { Rechaza(args[0], Porque.H7(Porque.Formatos)); return null; }
                if (!int.TryParse(dentro, out int n) || n + 1 >= args.Count)
                { Rechaza(args[0], Porque.Pendiente("un formato mal formado")); return null; }
                if (n != siguiente)
                { Rechaza(args[0], Porque.Pendiente("un formato que no usa sus argumentos una vez y en orden")); return null; }
                siguiente++;
                if (texto.Length > 0) { piezas.Add(Cadena(texto.ToString())); texto.Clear(); }
                piezas.Add(Texto(args[n + 1].Expression));
                i = cierre;
            }
            if (siguiente != args.Count - 1)
            { Rechaza(args[0], Porque.Pendiente("un formato que no usa sus argumentos una vez y en orden")); return null; }
            if (texto.Length > 0) piezas.Add(Cadena(texto.ToString()));
            return piezas;
        }
    }
}
