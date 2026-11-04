// `goto` (V8/H8; 4-nov; docs/V8_IDEAS.md §E9). Eduardo: «No vamos a ñadir labels ni goto. Pero se puede reemplazar por do ... loop y
// break». JC no tiene etiquetas: el bloque con etiquetas se parte por ellas en trozos, y se escribe un bucle con nombre y una variable
// que dice por qué trozo sigue. `if (pc <= k)` corre el trozo `k` y, al seguir, los de detrás, en orden, como en C#; un `goto` es dar
// valor a la variable y `continue` a ese bucle, desde donde sea: en C# una etiqueta se ve en su bloque y en los de dentro, así que un
// `goto` nunca entra en un bloque, se queda en el de la etiqueta o sale hacia él. Un `switch` con etiquetas o con `goto case` se parte
// igual: el `switch` elige el trozo de cada `case`. Una sola regla para todo: hacia delante, hacia atrás y cuando se cruzan.
//
// El bucle de la máquina no es de C#: un `break` o un `continue` de un bucle de fuera lo cruzaría. Sale por una variable (`fuga`), y el
// salto se hace detrás de la máquina. El `break` del propio `switch` es salir de su máquina.
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// Un bloque, o un `switch`, escrito como un bucle: `Nombre` es el del bucle y el de su variable.
        sealed class Maquina
        {
            public string Nombre, Fuga;
            public SyntaxNode Dueno;
        }

        readonly Stack<Maquina> maquinas = new Stack<Maquina>();
        /// De cada etiqueta y de cada sección de un `switch`, su máquina y su trozo.
        readonly Dictionary<ISymbol, (Maquina, int)> etiquetas = new Dictionary<ISymbol, (Maquina, int)>(SymbolEqualityComparer.Default);
        readonly Dictionary<SwitchSectionSyntax, (Maquina, int)> secciones = new Dictionary<SwitchSectionSyntax, (Maquina, int)>();
        HashSet<ISymbol> etiquetasUsadas;

        /// ¿Va algún `goto` a esta etiqueta? Una a la que no va ninguno no hace nada.
        bool Usada(ISymbol etiqueta)
        {
            if (etiquetasUsadas == null)
                etiquetasUsadas = new HashSet<ISymbol>(arbol.GetRoot().DescendantNodes().OfType<GotoStatementSyntax>()
                    .Where(g => g.IsKind(SyntaxKind.GotoStatement)).Select(g => m.GetSymbolInfo(g.Expression).Symbol).Where(s => s != null),
                    SymbolEqualityComparer.Default);
            return etiquetasUsadas.Contains(etiqueta);
        }

        /// `a: b: x;`: las etiquetas de una sentencia a las que va un `goto`.
        List<ISymbol> EtiquetasDe(StatementSyntax s)
        {
            var r = new List<ISymbol>();
            for (; s is LabeledStatementSyntax l; s = l.Statement)
            {
                var e = m.GetDeclaredSymbol(l);
                if (e != null && Usada(e)) r.Add(e);
            }
            return r;
        }

        Maquina NuevaMaquina(SyntaxNode dueno) => new Maquina { Nombre = "salto_s" + (++subidas), Dueno = dueno };

        /// Las sentencias de un bloque: una tras otra, o partidas por sus etiquetas.
        void Sentencias(BlockSyntax b)
        {
            if (!b.Statements.Any(s => EtiquetasDe(s).Count > 0))
            {
                foreach (var s in b.Statements) Sentencia(s);
                return;
            }
            var q = NuevaMaquina(b);
            var trozos = new List<List<StatementSyntax>> { new List<StatementSyntax>() };
            foreach (var s in b.Statements)
            {
                var suyas = EtiquetasDe(s);
                if (suyas.Count > 0)
                {
                    trozos.Add(new List<StatementSyntax>());
                    foreach (var e in suyas) etiquetas[e] = (q, trozos.Count - 1);
                }
                trozos[trozos.Count - 1].Add(s);
            }
            IzaDeTrozos(trozos);
            Linea("int " + q.Nombre + " = 0;");
            EmiteMaquina(q, trozos, !m.AnalyzeControlFlow(b.Statements.First(), b.Statements.Last()).EndPointIsReachable);
        }

        /// Las locales de un trozo que se usan en otro: delante de la máquina, como las de un `case` que se usan en otro.
        void IzaDeTrozos(List<List<StatementSyntax>> trozos)
        {
            var antes = new List<ILocalSymbol>();
            for (int i = 0; i < trozos.Count; i++)
                foreach (var s in trozos[i])
                {
                    var x = s;
                    while (x is LabeledStatementSyntax l) x = l.Statement;
                    if (!(x is LocalDeclarationStatementSyntax ld)) continue;
                    foreach (var v in ld.Declaration.Variables)
                    {
                        var simbolo = (ILocalSymbol) m.GetDeclaredSymbol(v);
                        bool fuera = trozos.Where((t, j) => j != i).Any(t => t.Any(o => o.DescendantNodesAndSelf().OfType<IdentifierNameSyntax>()
                            .Any(id => id.Identifier.ValueText == v.Identifier.ValueText && SymbolEqualityComparer.Default.Equals(m.GetSymbolInfo(id).Symbol, simbolo))));
                        if (fuera) antes.Add(simbolo);
                    }
                }
            foreach (var l in antes) IzaLocal(l);
        }

        /// El bucle: un `if` por trozo, y al acabar, fuera. Detrás, los saltos que cruzaban la máquina; y si C# dice que no se llega
        /// al final (un `return` en todos los caminos), que JC tampoco lo crea: el bucle tiene un `break`.
        void EmiteMaquina(Maquina q, List<List<StatementSyntax>> trozos, bool finInalcanzable)
        {
            bool saleBreak = q.Dueno is BlockSyntax && q.Dueno.DescendantNodes().OfType<BreakStatementSyntax>().Any(x => !q.Dueno.Contains(DestinoDe(x)));
            bool saleContinue = q.Dueno.DescendantNodes().OfType<ContinueStatementSyntax>().Any(x => !q.Dueno.Contains(BucleDe(x)));
            if (saleBreak || saleContinue)
            {
                q.Fuga = "fuga_s" + (++subidas);
                Linea("int " + q.Fuga + " = 0;");
            }
            Linea("do " + q.Nombre);
            Linea("{");
            sangria++;
            maquinas.Push(q);
            for (int k = 0; k < trozos.Count; k++)
            {
                if (trozos[k].Count == 0) continue;
                Linea("if (" + q.Nombre + " <= " + k + ")");
                Linea("{");
                sangria++;
                foreach (var s in trozos[k]) Sentencia(s);
                sangria--;
                Linea("}");
            }
            maquinas.Pop();
            Linea("break " + q.Nombre + ";");
            sangria--;
            Linea("}");
            Linea("while (true);");
            if (saleBreak) SaltoDetras(q, 1, true, DestinoDe(q.Dueno));
            if (saleContinue) SaltoDetras(q, 2, false, BucleDe(q.Dueno));
            if (finInalcanzable) Linea("throw new RuntimeError(\"inalcanzable\");");
        }

        void SaltoDetras(Maquina q, int valor, bool esBreak, SyntaxNode destino)
        {
            Linea("if (" + q.Fuga + " == " + valor + ")");
            Linea("{");
            sangria++;
            SaltoDeBucle(esBreak, destino);
            sangria--;
            Linea("}");
        }

        /// Un `break` o un `continue` de C#, que van a `destino`. Si hay una máquina por medio, sale por ella; el `break` del
        /// `switch` que es una máquina, sale de ella; y si no, el de siempre.
        void SaltoDeBucle(bool esBreak, SyntaxNode destino)
        {
            if (maquinas.Count > 0)
            {
                var q = maquinas.Peek();
                if (esBreak && q.Dueno == destino) { Linea("break " + q.Nombre + ";"); return; }
                if (q.Dueno != destino && destino.Contains(q.Dueno))
                {
                    Linea(q.Fuga + " = " + (esBreak ? 1 : 2) + ";");
                    Linea("break " + q.Nombre + ";");
                    return;
                }
            }
            if (esBreak) { Linea("break;"); return; }
            if (destino is ForStatementSyntax fw && forComoWhile.Contains(fw))
                foreach (var i in fw.Incrementors) Linea(ExprSentencia(i) + ";");
            Linea("continue;");
        }

        /// A lo que va un `break`: el bucle o el `switch` más cercano.
        static SyntaxNode DestinoDe(SyntaxNode n)
        {
            foreach (var a in n.Ancestors())
                if (a is ForStatementSyntax || a is ForEachStatementSyntax || a is WhileStatementSyntax || a is DoStatementSyntax || a is SwitchStatementSyntax)
                    return a;
            return null;
        }

        /// `goto x;`, `goto case 1;` y `goto default;`: su trozo, y volver a empezar.
        void Goto(GotoStatementSyntax g)
        {
            (Maquina q, int k) d = default;
            bool hay;
            if (g.IsKind(SyntaxKind.GotoStatement))
                hay = m.GetSymbolInfo(g.Expression).Symbol is ISymbol e && etiquetas.TryGetValue(e, out d);
            else
            {
                var sec = SeccionDe(g);
                hay = sec != null && secciones.TryGetValue(sec, out d);
            }
            if (!hay) { Rechaza(g, Porque.Fuera(Porque.Goto)); return; }
            Linea(d.q.Nombre + " = " + d.k + ";");
            Linea("continue " + d.q.Nombre + ";");
        }

        /// La sección a la que va un `goto case` o un `goto default`, del `switch` más cercano.
        SwitchSectionSyntax SeccionDe(GotoStatementSyntax g)
        {
            var sw = g.Ancestors().OfType<SwitchStatementSyntax>().First();
            if (g.IsKind(SyntaxKind.GotoDefaultStatement)) return sw.Sections.FirstOrDefault(s => s.Labels.Any(l => l is DefaultSwitchLabelSyntax));
            var valor = m.GetConstantValue(g.Expression);
            if (!valor.HasValue) return null;
            return sw.Sections.FirstOrDefault(s => s.Labels.OfType<CaseSwitchLabelSyntax>().Any(l => MismaConstante(m.GetConstantValue(l.Value).Value, valor.Value)));
        }

        static bool MismaConstante(object a, object b)
        {
            if (a == null || b == null || a is string || b is string || a is bool || b is bool) return Equals(a, b);
            if (a is char ca) a = (int) ca;                  // `goto case 'x'`: Convert no pasa un char a decimal
            if (b is char cb) b = (int) cb;
            return System.Convert.ToDecimal(a) == System.Convert.ToDecimal(b);
        }

        /// ¿Hay que partir este `switch`? Una etiqueta en una sección, o un `goto case` suyo.
        bool SwitchConSaltos(SwitchStatementSyntax sw) =>
            sw.Sections.Any(sec => sec.Statements.Any(s => EtiquetasDe(s).Count > 0))
            || sw.DescendantNodes().OfType<GotoStatementSyntax>().Any(g => !g.IsKind(SyntaxKind.GotoStatement)
                                                                         && g.Ancestors().OfType<SwitchStatementSyntax>().First() == sw);

        /// Un `switch` partido: cada sección empieza un trozo, y cada etiqueta, otro; el `switch` de JC sólo elige por cuál se
        /// empieza. De una sección no se cae en la siguiente (C# no lo deja), así que `pc <= k` vale igual.
        void EmiteSwitchConSaltos(SwitchStatementSyntax sw)
        {
            var q = NuevaMaquina(sw);
            var trozos = new List<List<StatementSyntax>>();
            foreach (var sec in sw.Sections)
            {
                trozos.Add(new List<StatementSyntax>());
                secciones[sec] = (q, trozos.Count - 1);
                foreach (var s in sec.Statements)
                {
                    var suyas = EtiquetasDe(s);
                    if (suyas.Count > 0)
                    {
                        if (trozos[trozos.Count - 1].Count > 0) trozos.Add(new List<StatementSyntax>());
                        foreach (var e in suyas) etiquetas[e] = (q, trozos.Count - 1);
                    }
                    trozos[trozos.Count - 1].Add(s);
                }
            }
            IzaDeTrozos(trozos);
            Linea("int " + q.Nombre + " = " + trozos.Count + ";");      // sin un case que case, ningún trozo
            var (que, antes) = Condicion(sw.Expression);
            foreach (var l in antes) Linea(l);
            Linea("switch (" + que + ")");
            Linea("{");
            sangria++;
            SwitchSectionSyntax porDefecto = null;
            foreach (var sec in sw.Sections)
            {
                foreach (var l in sec.Labels.Where(l => !(l is CaseSwitchLabelSyntax) && !(l is DefaultSwitchLabelSyntax)))
                    RechazaNodo(l);
                if (sec.Labels.Any(l => l is DefaultSwitchLabelSyntax)) { porDefecto = sec; continue; }
                Linea("case " + string.Join(", ", sec.Labels.OfType<CaseSwitchLabelSyntax>().Select(l => ValorDeCase(l.Value))) + ":");
                Linea("   " + q.Nombre + " = " + secciones[sec].Item2 + ";");
            }
            if (porDefecto != null)
            {
                Linea("default:");
                Linea("   " + q.Nombre + " = " + secciones[porDefecto].Item2 + ";");
            }
            sangria--;
            Linea("}");
            EmiteMaquina(q, trozos, !m.AnalyzeControlFlow(sw).EndPointIsReachable);
        }
    }
}
