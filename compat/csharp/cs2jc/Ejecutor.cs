// Compilar el JC y ejecutarlo en las dos VMs (V8/H6, docs/V8_IDEAS.md §E9), como compat.sh: la stdlib al lado del
// .mod, cada VM desde su carpeta, la salida sin el banner de la VM, y el código de salida también es paridad.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace Cs2Jc
{
    sealed class Ejecucion
    {
        public bool Compila;
        public string ErrorCompila;
        public int RcJava, RcC;
        public string SalidaJava, SalidaC, ErrJava, ErrC;
        public bool PlazoJava, PlazoC;            // no acabó en el tiempo máximo
    }

    sealed class Ejecutor
    {
        readonly string fe, vmJava, vmC;
        public readonly string Trabajo;
        public readonly int Plazo;                // segundos

        public Ejecutor(string raiz, string trabajo, int plazo)
        {
            fe = Path.Combine(raiz, "lexer-java", "target", "basicplus-frontend.jar");
            vmJava = Path.Combine(raiz, "miVM", "target", "bpgenvm-1.0.jar");
            vmC = Path.Combine(raiz, "bpgenvm-c", "build", "bpgenvm-c.exe");
            Trabajo = Path.GetFullPath(trabajo);
            Plazo = plazo;
            Directory.CreateDirectory(Trabajo);
            // la stdlib al lado: el compilador la lee de ahí (--stdlibDir) y las VMs la cargan junto al .mod
            foreach (var f in Directory.GetFiles(Path.Combine(raiz, "bpstdlib"), "*.mod"))
                File.Copy(f, Path.Combine(Trabajo, Path.GetFileName(f)), true);
            // y las librerías CSharp (compat/csharp/lib, el escalón 1: V8_IDEAS §E9), lo de .NET que piden las pruebas:
            // compiladas aquí, como un módulo más; CsSystem primero, que las demás pueden usarla
            string lib = Path.Combine(raiz, "compat", "csharp", "lib");
            if (Directory.Exists(lib))
                foreach (var f in Directory.GetFiles(lib, "*.jc").OrderBy(f => Path.GetFileName(f) == "CsSystem.jc" ? 0 : 1)
                                                               .ThenBy(f => f, StringComparer.Ordinal))
                {
                    string mod = Path.Combine(Trabajo, Path.GetFileNameWithoutExtension(f) + ".mod");
                    File.Delete(mod);
                    var c = Corre("java", new[] { "-Dfile.encoding=UTF-8", "-jar", fe, f, "--compile", Trabajo, "--backend=mivm",
                                                  "--stdlibDir=" + Trabajo }, Trabajo, 120);
                    if (c.Rc != 0 || !File.Exists(mod))
                        throw new InvalidOperationException("la librería " + Path.GetFileName(f) + " no compila (rc " + c.Rc + "):\n"
                                                            + c.Salida + c.Error);
                }
        }

        public Ejecucion Ejecuta(string modulo)
        {
            var r = new Ejecucion();
            string mod = Path.Combine(Trabajo, modulo + ".mod");
            File.Delete(mod);
            var c = Corre("java", new[] { "-Dfile.encoding=UTF-8", "-jar", fe, Path.Combine(Trabajo, modulo + ".jc"),
                                          "--compile", Trabajo, "--backend=mivm", "--stdlibDir=" + Trabajo }, Trabajo, 120);
            if (c.Rc != 0 || !File.Exists(mod))
            {
                // los errores del emisor van sangrados bajo su cabecera, «-- Errores del emisor mivm (N) --», y no dicen
                // «error»: sin esto quedaba sólo el «NO compilado: … (1 errores del emisor…)», sin el porqué (#753)
                var errores = new List<string>();
                bool delEmisor = false;
                foreach (var l in (c.Salida + "\n" + c.Error).Split('\n').Select(x => x.TrimEnd('\r')))
                {
                    if (l.StartsWith("-- Errores del emisor")) { delEmisor = true; continue; }
                    if (delEmisor && l.StartsWith("  ")) { errores.Add("error del emisor: " + l.Trim()); continue; }
                    delEmisor = false;
                    if (l.IndexOf("error", StringComparison.OrdinalIgnoreCase) >= 0 && !l.StartsWith("compilación abortada")
                        && !l.StartsWith("NO compilado:")       // el resumen de los del emisor, que ya van arriba
                        && !l.StartsWith("--"))                 // «-- Diagnósticos semánticos (1 errores…) --» es una cabecera
                        errores.Add(l);
                }
                errores = errores.Take(5).ToList();
                r.ErrorCompila = errores.Count > 0 ? string.Join("\n", errores) : "(rc " + c.Rc + ", sin mensaje de error)";
                return r;
            }
            r.Compila = true;
            var j = Corre("java", new[] { "-Djava.awt.headless=true", "-jar", vmJava, modulo + ".mod" }, Trabajo, Plazo);
            var k = Corre(vmC, new[] { modulo + ".mod" }, Trabajo, Plazo);
            r.RcJava = j.Rc; r.SalidaJava = Filtra(j.Salida); r.ErrJava = j.Error; r.PlazoJava = j.FueraDePlazo;
            r.RcC = k.Rc; r.SalidaC = Filtra(k.Salida); r.ErrC = k.Error; r.PlazoC = k.FueraDePlazo;
            return r;
        }

        sealed class Proceso
        {
            public int Rc;
            public string Salida, Error;
            public bool FueraDePlazo;
        }

        static Proceso Corre(string exe, IEnumerable<string> args, string dir, int plazo)
        {
            var psi = new ProcessStartInfo(exe)
            {
                WorkingDirectory = dir,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                StandardOutputEncoding = new UTF8Encoding(false),
                StandardErrorEncoding = new UTF8Encoding(false),
            };
            foreach (var a in args) psi.ArgumentList.Add(a);
            using (var p = Process.Start(psi))
            {
                var o = p.StandardOutput.ReadToEndAsync();
                var e = p.StandardError.ReadToEndAsync();
                bool acabo = p.WaitForExit(plazo * 1000);
                if (!acabo)
                {
                    try { p.Kill(true); } catch (Exception) { }
                    p.WaitForExit();
                }
                return new Proceso { Rc = acabo ? p.ExitCode : -1, Salida = o.Result, Error = e.Result, FueraDePlazo = !acabo };
            }
        }

        static readonly Regex Banner = new Regex("INICIANDO|FIN DE|heapStart|^config:");

        /// La salida sin el banner de la VM, y sin las líneas en blanco de delante y de detrás (el `filt` de compat.sh).
        static string Filtra(string s)
        {
            var r = new StringBuilder();
            var pendientes = new List<string>();
            bool vista = false;
            var lineas = s.Split('\n');
            int n = lineas.Length;
            if (n > 0 && lineas[n - 1].Length == 0) n--;
            for (int i = 0; i < n; i++)
            {
                string l = lineas[i];
                if (Banner.IsMatch(l)) { pendientes.Clear(); continue; }
                bool blanca = l.Trim().Length == 0;
                if (!vista) { if (blanca) continue; vista = true; }
                if (blanca) { pendientes.Add(l); continue; }
                foreach (var x in pendientes) r.Append(x).Append('\n');
                pendientes.Clear();
                r.Append(l).Append('\n');
            }
            return r.ToString();
        }
    }
}
