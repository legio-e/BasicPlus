// Un lote de pruebas (V8/H6, docs/V8_IDEAS.md §E9): todas las de una carpeta (o las de una lista), traducidas y
// ejecutadas en paralelo, y un informe: cuántas de cada veredicto; de las que no se traducen, qué piden, contado; de las
// que fallan, por qué. Cada prueba, una línea de informe.jsonl en la carpeta de salida.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Cs2Jc
{
    static class Lote
    {
        public static int Corre(string entrada, Ejecutor ej, int hilos)
        {
            var ficheros = (Directory.Exists(entrada)
                    ? Directory.GetFiles(entrada, "*.cs")
                    : File.ReadAllLines(entrada).Select(l => l.Trim()).Where(l => l.Length > 0 && !l.StartsWith("#"))
                        .Select(l => Path.IsPathRooted(l) ? l : Path.Combine(Path.GetDirectoryName(Path.GetFullPath(entrada)), l)))
                .OrderBy(f => f, StringComparer.Ordinal).ToArray();
            var veredictos = new Veredicto[ficheros.Length];
            int hechas = 0;
            Parallel.For(0, ficheros.Length, new ParallelOptions { MaxDegreeOfParallelism = hilos }, i =>
            {
                try { veredictos[i] = Programa.Prueba(ficheros[i], ej); }
                catch (Exception e)
                {
                    veredictos[i] = new Veredicto { Fichero = Path.GetFileName(ficheros[i]), Clase = Clase.Falla,
                                                    Detalle = "el traductor se rompe: " + e.GetType().Name + ": " + e.Message };
                }
                int n = System.Threading.Interlocked.Increment(ref hechas);
                if (n % 100 == 0) Console.Error.WriteLine("  … " + n + " de " + ficheros.Length);
            });
            using (var w = new StreamWriter(Path.Combine(ej.Trabajo, "informe.jsonl"), false, new UTF8Encoding(false)))
                foreach (var v in veredictos)
                    w.WriteLine(JsonSerializer.Serialize(new Dictionary<string, object>
                    {
                        ["fichero"] = v.Fichero,
                        ["veredicto"] = v.Clase.ToString(),
                        ["detalle"] = v.Detalle,
                        ["rechazos"] = v.Rechazos.Select(Programa.Formato).ToList(),
                        ["avisos"] = v.Avisos,
                    }));
            Console.Write(Informe(veredictos));
            return 0;
        }

        static string Informe(Veredicto[] vs)
        {
            var r = new StringBuilder();
            var validas = vs.Where(v => v.Clase != Clase.NoValida).ToList();
            r.Append("lote: ").Append(vs.Length).Append(" pruebas; válidas, ").Append(validas.Count).Append('\n');
            var bien = vs.Where(v => v.Clase == Clase.Funciona).ToList();
            r.Append("  ✅ funciona:        ").Append(bien.Count)
             .Append(" (se comprueba sola, ").Append(bien.Count(v => v.Detalle.StartsWith("se comprueba"))).Append("; sólo corre, ")
             .Append(bien.Count(v => v.Detalle.StartsWith("sólo corre"))).Append(")\n");
            r.Append("  ❌ falla:           ").Append(vs.Count(v => v.Clase == Clase.Falla)).Append('\n');
            r.Append("  ⛔ no se traduce:   ").Append(vs.Count(v => v.Clase == Clase.NoSeTraduce)).Append('\n');
            r.Append("  ·  no válida:       ").Append(vs.Count(v => v.Clase == Clase.NoValida)).Append('\n');

            // Lo que piden las que no se traducen: por motivo, en cuántas pruebas sale, y en cuántas es lo único.
            var nst = vs.Where(v => v.Clase == Clase.NoSeTraduce).ToList();
            var porMotivo = new Dictionary<string, (int pruebas, int solo)>();
            foreach (var v in nst)
            {
                var motivos = v.Rechazos.Select(x => x.Porque).Distinct().ToList();
                foreach (var mo in motivos)
                {
                    porMotivo.TryGetValue(mo, out var c);
                    porMotivo[mo] = (c.pruebas + 1, c.solo + (motivos.Count == 1 ? 1 : 0));
                }
            }
            r.Append("\nLo que piden las que no se traducen (pruebas que lo piden; a las que sólo les falta eso):\n");
            foreach (var kv in porMotivo.OrderByDescending(kv => kv.Value.pruebas).ThenBy(kv => kv.Key, StringComparer.Ordinal))
                r.Append(string.Format("  {0,5} {1,5}  {2}\n", kv.Value.pruebas, kv.Value.solo, kv.Key));

            var fallan = vs.Where(v => v.Clase == Clase.Falla).ToList();
            if (fallan.Count > 0)
            {
                r.Append("\nPor qué fallan:\n");
                foreach (var g in fallan.GroupBy(v => Motivo(v.Detalle)).OrderByDescending(g => g.Count()).ThenBy(g => g.Key, StringComparer.Ordinal))
                    r.Append(string.Format("  {0,5}  {1}  ({2})\n", g.Count(), g.Key,
                        string.Join(", ", g.Select(v => v.Fichero).Take(6)) + (g.Count() > 6 ? ", …" : "")));
            }
            return r.ToString();
        }

        /// El motivo de un fallo, sin lo que cambia de una prueba a otra (números, nombres, líneas).
        static string Motivo(string detalle)
        {
            string d = detalle.Split('\n')[0];
            if (detalle.StartsWith("no compila en JC"))
            {
                var linea = detalle.Split('\n').Skip(1).Select(l => l.Trim()).FirstOrDefault(l => l.Length > 0) ?? "";
                linea = Regex.Replace(linea, @"^\[\d+:\d+\]\s*", "");
                linea = Regex.Replace(linea, @"'[^']*'", "'…'");
                return "no compila en JC: " + linea;
            }
            if (d.StartsWith("Main devuelve")) return "Main devuelve otra cosa";
            d = Regex.Replace(d, @"; la misma salida en las dos VMs.*$", "");
            d = Regex.Replace(d, @"«[^»]*»", "«…»");
            return Regex.Replace(d, @"\d+", "N");
        }
    }
}
