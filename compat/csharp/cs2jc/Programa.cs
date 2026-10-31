// cs2jc — el traductor de C# a JC (V8/H6, docs/V8_IDEAS.md §E9). Una prueba de C# entra, y sale su JC o «no se
// traduce» con el porqué; y la ejecuta en las dos VMs: funciona, o falla y por qué.
//
//   cs2jc <prueba.cs> [--salida carpeta] [--plazo s]   una prueba: el .jc, compilarlo, ejecutarlo y el veredicto
//   cs2jc --lote <carpeta|lista> [--hilos n] […]        todas, en paralelo, y el informe (y informe.jsonl en la salida)
//   cs2jc --casos [carpeta] [--salida carpeta]          los casos del traductor (compat/csharp/casos): cada .cs da su
//                                                       .jc (o sus .rechazos), y el .jc funciona
//
// La salida (el .jc, el .mod y la stdlib que necesitan las VMs al lado), por defecto en %TEMP%\cs2jc. Sale con 0 si la
// prueba funciona (o si todos los casos están bien), con 1 si no, y con 2 si no se pudo ni empezar.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;

namespace Cs2Jc
{
    enum Clase { Funciona, NoSeTraduce, Falla, NoValida }

    sealed class Veredicto
    {
        public string Fichero, Modulo, Detalle;
        public Clase Clase;
        public List<Rechazo> Rechazos = new List<Rechazo>();
        public List<string> Avisos = new List<string>();

        public string Informe()
        {
            var r = new StringBuilder();
            r.Append(Fichero).Append(Modulo != null && Clase != Clase.NoSeTraduce ? " → " + Modulo + ".jc" : "").Append('\n');
            switch (Clase)
            {
                case Clase.Funciona: r.Append("✅ funciona: ").Append(Detalle).Append('\n'); break;
                case Clase.Falla: r.Append("❌ falla: ").Append(Detalle).Append('\n'); break;
                case Clase.NoValida: r.Append("· no válida: ").Append(Detalle).Append('\n'); break;
                case Clase.NoSeTraduce:
                    r.Append("⛔ no se traduce (").Append(Rechazos.Count).Append("):\n");
                    foreach (var x in Rechazos) r.Append("   ").Append(Programa.Formato(x)).Append('\n');
                    break;
            }
            foreach (var a in Avisos) r.Append("   aviso: ").Append(a).Append('\n');
            return r.ToString();
        }
    }

    static class Programa
    {
        static int Main(string[] args)
        {
            Console.OutputEncoding = new UTF8Encoding(false);
            string raiz = Raiz();
            if (raiz == null)
            {
                Console.Error.WriteLine("cs2jc: no encuentro la raíz del repositorio (con lexer-java, miVM y bpgenvm-c)");
                return 2;
            }
            string salida = Opcion(args, "--salida") ?? Path.Combine(Path.GetTempPath(), "cs2jc");
            int plazo = int.Parse(Opcion(args, "--plazo") ?? "10");
            string libre = Libre(args);
            if (args.Contains("--casos"))
                return Casos(libre ?? Path.Combine(raiz, "compat", "csharp", "casos"), new Ejecutor(raiz, salida, plazo));
            if (args.Contains("--lote") && libre != null)
                return Lote.Corre(libre, new Ejecutor(raiz, salida, plazo),
                                  int.Parse(Opcion(args, "--hilos") ?? Math.Max(1, Environment.ProcessorCount / 2).ToString()));
            if (libre == null)
            {
                Console.Error.WriteLine("uso: cs2jc <prueba.cs> [--salida carpeta] [--plazo s]  |  cs2jc --lote <carpeta|lista> [--hilos n]"
                                        + "  |  cs2jc --casos [carpeta]");
                return 2;
            }
            var v = Prueba(libre, new Ejecutor(raiz, salida, plazo));
            Console.Write(v.Informe());
            return v.Clase == Clase.Funciona ? 0 : 1;
        }

        /// Las opciones del compilador de Mono que piden otros ficheros o cambian el lenguaje (#745): la prueba no vale.
        static readonly Regex OpcionesFuera = new Regex(@"(^|\s)[-/](r|reference|addmodule|t|target|unsafe|lib|keyfile|res|linkresource|pkg|noconfig|nostdlib|main|out|codepage|mod|checked)\b");

        /// Una prueba, entera: leerla, traducirla, compilarla, ejecutarla y el veredicto.
        internal static Veredicto Prueba(string ruta, Ejecutor ej)
        {
            var v = new Veredicto { Fichero = Path.GetFileName(ruta) };
            var (comp, arbol, errores) = Lee(ruta);
            if (errores.Count > 0) { v.Clase = Clase.NoValida; v.Detalle = "no compila en .NET: " + errores[0]; return v; }
            var opciones = Regex.Match(arbol.ToString(), @"//\s*Compiler options:\s*(.*)");
            if (opciones.Success && OpcionesFuera.IsMatch(" " + opciones.Groups[1].Value))
            {
                v.Clase = Clase.NoValida;
                v.Detalle = "pide opciones del compilador: " + opciones.Groups[1].Value.Trim();
                return v;
            }
            var tr = new Traductor(comp, arbol).Traduce(NombreModulo(ruta));
            v.Modulo = tr.Modulo;
            v.Avisos = tr.Avisos;
            if (tr.Rechazos.Count > 0) { v.Clase = Clase.NoSeTraduce; v.Rechazos = tr.Rechazos; return v; }
            File.WriteAllText(Path.Combine(ej.Trabajo, tr.Modulo + ".jc"), tr.Jc, new UTF8Encoding(false));
            Juzga(v, tr, ej.Ejecuta(tr.Modulo), ej.Plazo);
            return v;
        }

        static void Juzga(Veredicto v, Traduccion tr, Ejecucion e, int plazo)
        {
            v.Clase = Clase.Falla;
            if (!e.Compila) { v.Detalle = "no compila en JC:\n   " + e.ErrorCompila.Replace("\n", "\n   "); return; }
            if (e.PlazoJava || e.PlazoC)
            {
                v.Detalle = "no acaba en " + plazo + " s (" + (e.PlazoJava && e.PlazoC ? "las dos VMs" : e.PlazoJava ? "la VM-Java" : "la VM-C") + ")";
                return;
            }
            if (e.RcJava != e.RcC) { v.Detalle = "las dos VMs no coinciden: sale con " + e.RcJava + " en la VM-Java y con " + e.RcC + " en la VM-C"; return; }
            if (e.SalidaJava != e.SalidaC) { v.Detalle = "las dos VMs no coinciden: " + PrimeraDiferencia(e.SalidaJava, e.SalidaC); return; }
            int lineas = e.SalidaJava.Count(c => c == '\n');
            string misma = "; la misma salida en las dos VMs (" + lineas + (lineas == 1 ? " línea)" : " líneas)");
            switch (e.RcJava)
            {
                case 0:
                    v.Clase = Clase.Funciona;
                    v.Detalle = (tr.MainDevuelveEntero ? "se comprueba sola (Main devuelve 0)" : "sólo corre (acaba sin excepción)") + misma;
                    return;
                case 1:
                {
                    var d = Regex.Match(e.ErrC + "\n" + e.ErrJava, @"Main devuelve (-?\d+)");
                    if (d.Success) { v.Detalle = "Main devuelve " + d.Groups[1].Value + misma; return; }
                    v.Detalle = "salta una excepción: " + Excepcion(e.ErrC) + misma;
                    return;
                }
                default:
                    v.Detalle = "la VM sale con " + e.RcJava + (e.RcJava == 2 ? " (no se pudo cargar)" : e.RcJava == 3 ? " (fallo interno)"
                                : e.RcJava == 4 ? " (sin memoria)" : "") + ": " + Excepcion(e.ErrC);
                    return;
            }
        }

        /// La excepción que la VM-C dice no atrapada (la nombra con su clase), o la primera línea que diga algo.
        static string Excepcion(string err)
        {
            var lineas = err.Split('\n').Select(l => l.Trim()).Where(l => l.Length > 0).ToList();
            return lineas.FirstOrDefault(l => l.Contains("no atrapada")) ?? lineas.FirstOrDefault(l => !l.StartsWith("[bpvm")) ?? "(sin mensaje)";
        }

        /// Una línea con lo que no se ve escrito: un `\r` al final la distingue de otra igual sin él.
        static string Visible(string s) => s.Replace("\r", "\\r").Replace("\t", "\\t");

        static string PrimeraDiferencia(string a, string b)
        {
            var la = a.Split('\n');
            var lb = b.Split('\n');
            for (int i = 0; i < Math.Max(la.Length, lb.Length); i++)
            {
                string x = i < la.Length ? la[i] : "(nada)", y = i < lb.Length ? lb[i] : "(nada)";
                if (x != y) return "línea " + (i + 1) + ": «" + Visible(x) + "» en la VM-Java, «" + Visible(y) + "» en la VM-C";
            }
            return "(iguales)";
        }

        // ------------------------------------------------------------------------------------------------ los casos

        /// Los casos del traductor: cada `X.cs` da su `X.jc` (y ese JC funciona), o sus `X.rechazos`, línea a línea.
        /// Si no casa, el nuevo queda al lado, en `X.jc.nuevo`, para mirarlo.
        static int Casos(string dir, Ejecutor ej)
        {
            int bien = 0, mal = 0;
            foreach (var cs in Directory.GetFiles(dir, "*.cs").OrderBy(x => x, StringComparer.Ordinal))
            {
                string fallo = Caso(cs, ej);
                if (fallo == null) { bien++; Console.WriteLine("  ok   " + Path.GetFileName(cs)); }
                else { mal++; Console.WriteLine("  MAL  " + Path.GetFileName(cs) + " — " + fallo.Replace("\n", "\n         ")); }
            }
            Console.WriteLine("casos: " + bien + " bien, " + mal + " mal");
            return mal == 0 && bien > 0 ? 0 : 1;
        }

        static string Caso(string cs, Ejecutor ej)
        {
            var (comp, arbol, errores) = Lee(cs);
            if (errores.Count > 0) return "no compila en .NET: " + errores[0];
            var tr = new Traductor(comp, arbol).Traduce(NombreModulo(cs));
            string rechazos = Path.ChangeExtension(cs, ".rechazos");
            if (File.Exists(rechazos))
            {
                var esperado = File.ReadAllLines(rechazos).Where(l => l.Trim().Length > 0 && !l.StartsWith("#")).ToList();
                var dado = tr.Rechazos.Select(Formato).ToList();
                if (esperado.SequenceEqual(dado)) return null;
                return "los rechazos no son los esperados:\n" + string.Join("\n", dado);
            }
            if (tr.Rechazos.Count > 0) return "no se traduce:\n" + string.Join("\n", tr.Rechazos.Select(Formato));
            string jc = Path.ChangeExtension(cs, ".jc");
            string esperadoJc = File.Exists(jc) ? File.ReadAllText(jc).Replace("\r\n", "\n") : null;
            if (esperadoJc != tr.Jc)
            {
                File.WriteAllText(jc + ".nuevo", tr.Jc, new UTF8Encoding(false));
                return (esperadoJc == null ? "no tiene su .jc" : "el JC no es el esperado") + " (el que sale, en " + Path.GetFileName(jc) + ".nuevo)";
            }
            File.WriteAllText(Path.Combine(ej.Trabajo, tr.Modulo + ".jc"), tr.Jc, new UTF8Encoding(false));
            var v = new Veredicto();
            Juzga(v, tr, ej.Ejecuta(tr.Modulo), ej.Plazo);
            return v.Clase == Clase.Funciona ? null : v.Detalle;
        }

        public static string Formato(Rechazo r) => r.Linea + ": " + r.Porque + " — " + r.Que;

        // ------------------------------------------------------------------------------------------------ leer

        static (CSharpCompilation, SyntaxTree, List<string>) Lee(string ruta)
        {
            string texto = File.ReadAllText(ruta);
            var arbol = CSharpSyntaxTree.ParseText(texto, new CSharpParseOptions(LanguageVersion.Preview), path: ruta);
            var comp = CSharpCompilation.Create("prueba", new[] { arbol }, Referencias.Value,
                new CSharpCompilationOptions(OutputKind.ConsoleApplication, allowUnsafe: true));
            var errores = comp.GetDiagnostics().Where(d => d.Severity == DiagnosticSeverity.Error).Select(d => d.ToString()).ToList();
            return (comp, arbol, errores);
        }

        /// Los ensamblados de .NET contra los que se compila la prueba: los del runtime con el que corre el traductor.
        static readonly Lazy<List<MetadataReference>> Referencias = new Lazy<List<MetadataReference>>(() =>
        {
            string dir = Path.GetDirectoryName(typeof(object).Assembly.Location);
            return ((string) AppContext.GetData("TRUSTED_PLATFORM_ASSEMBLIES")).Split(Path.PathSeparator)
                .Where(p => string.Equals(Path.GetDirectoryName(p), dir, StringComparison.OrdinalIgnoreCase))
                .Select(p => (MetadataReference) MetadataReference.CreateFromFile(p)).ToList();
        });

        /// `test-6.cs` es el módulo `CsTest6`.
        static string NombreModulo(string fichero)
        {
            var r = new StringBuilder("Cs");
            bool mayuscula = true;
            foreach (char c in Path.GetFileNameWithoutExtension(fichero))
            {
                if (c < 128 && char.IsLetterOrDigit(c)) { r.Append(mayuscula ? char.ToUpperInvariant(c) : c); mayuscula = false; }
                else mayuscula = true;
            }
            return r.ToString();
        }

        static string Raiz()
        {
            foreach (var inicio in new[] { AppContext.BaseDirectory, Directory.GetCurrentDirectory() })
                for (var d = new DirectoryInfo(inicio); d != null; d = d.Parent)
                    if (Directory.Exists(Path.Combine(d.FullName, "lexer-java")) && Directory.Exists(Path.Combine(d.FullName, "miVM"))
                        && Directory.Exists(Path.Combine(d.FullName, "bpgenvm-c")))
                        return d.FullName;
            return null;
        }

        static string Opcion(string[] args, string nombre)
        {
            int i = Array.IndexOf(args, nombre);
            return i >= 0 && i + 1 < args.Length ? args[i + 1] : null;
        }

        static string Libre(string[] args)
        {
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "--salida" || args[i] == "--plazo" || args[i] == "--hilos") { i++; continue; }
                if (!args[i].StartsWith("--")) return args[i];
            }
            return null;
        }
    }
}
