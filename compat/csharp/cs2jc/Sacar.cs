// `++`, `--` y `=` dentro de una expresión, partida en dos (V8/H8, tema 1; 2-nov; Eduardo: «tendríamos que partir la
// expresión en 2»; docs/V8_IDEAS.md §E9). JC no deja que una expresión cambie una variable: el cambio sale a una
// sentencia propia —delante, o detrás si es `x++` y la sentencia lo deja— y en su sitio queda el valor, o una temporal.
// Es exacto (la segunda regla de fondo: no cambia lo que se calcula ni su orden) cuando nada más de la sentencia lee lo
// que cambia; si es un campo o un elemento, cuando ninguna otra llamada de la sentencia podría verlo; y no bajo &&, ||,
// ?? o ?:, que sólo calculan a veces. Si no, sigue «le falta a JC».
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// La sentencia (o la condición) que se está escribiendo: lo que se saca delante y detrás de ella.
        sealed class Sacar
        {
            public SyntaxNode Raiz;
            public bool Despues;                                   // ¿se puede poner algo detrás? (no tras un return, un if…)
            public readonly List<string> Antes = new List<string>(), Tras = new List<string>();
            public readonly Dictionary<SyntaxNode, string> Hechos = new Dictionary<SyntaxNode, string>();
        }

        Sacar sacar;

        /// Escribe una sentencia (`emite`) dejando sacar lo que cambia en sus expresiones: lo de delante, antes de sus
        /// líneas; lo de detrás, después.
        void EmiteConSacar(SyntaxNode raiz, bool despues, Action emite)
        {
            var previo = sacar;
            var s = sacar = new Sacar { Raiz = raiz, Despues = despues };
            int inicio = sb.Length;
            try { emite(); }
            finally { sacar = previo; }
            if (s.Antes.Count > 0)
                sb.Insert(inicio, string.Concat(s.Antes.Select(l => new string(' ', 3 * (sangria + 1)) + l + "\n")));
            foreach (var l in s.Tras) Linea(l);
        }

        /// La condición de un if, un bucle o un switch, y lo que hay que sacar delante de ella (detrás no se puede).
        (string, List<string>) Condicion(ExpressionSyntax c)
        {
            var previo = sacar;
            var s = sacar = new Sacar { Raiz = c, Despues = false };
            try { return (Expr(c), s.Antes); }
            finally { sacar = previo; }
        }

        int sacados;

        /// `x++`, `++x`, `x = e` o `x op= e` dentro de una expresión: sacado a su sentencia, y en su sitio, su valor.
        string Sacado(ExpressionSyntax n)
        {
            if (sacar == null) return Rechaza(n, Porque.H7(Porque.IncEnExpresion));
            if (sacar.Hechos.TryGetValue(n, out var hecho)) return hecho;
            var objetivo = n is AssignmentExpressionSyntax a0 ? a0.Left
                         : n is PostfixUnaryExpressionSyntax po0 ? po0.Operand : ((PrefixUnaryExpressionSyntax) n).Operand;
            if (BajoCondicional(n) || Repetida(n, objetivo)) return Rechaza(n, Porque.H7(Porque.IncEnExpresion));
            var s = m.GetSymbolInfo(objetivo).Symbol;
            bool local = s is ILocalSymbol || (s is IParameterSymbol ps && ps.RefKind == RefKind.None);
            bool propiedad = s is IPropertySymbol pp && !pp.IsIndexer;
            bool campoOElemento = s is IFieldSymbol || (objetivo is ElementAccessExpressionSyntax && m.GetTypeInfo(((ElementAccessExpressionSyntax) objetivo).Expression).Type is IArrayTypeSymbol);
            if (!local && !propiedad && !campoOElemento) return Rechaza(n, Porque.H7(Porque.IncEnExpresion));
            if (!ObjetivoSimple(objetivo) || (!local && OtrosEfectos(n))) return Rechaza(n, Porque.H7(Porque.IncEnExpresion));
            string r;
            if (propiedad)
            {
                // una propiedad: sólo `P = e`, por una temporal (leerla otra vez llamaría a su get)
                if (!(n is AssignmentExpressionSyntax a) || !a.IsKind(SyntaxKind.SimpleAssignmentExpression))
                    return Rechaza(n, Porque.H7(Porque.IncEnExpresion));
                string t = Temporal();
                sacar.Antes.Add(Tipo(m.GetTypeInfo(objetivo).Type, objetivo) + " " + t + " = " + Expr(a.Right) + ";");
                sacar.Antes.Add(Expr(objetivo) + " = " + t + ";");
                r = t;
            }
            else if (n is PostfixUnaryExpressionSyntax)
            {
                string sentencia = ExprSentencia(n);
                if (sacar.Despues)
                {
                    r = Expr(objetivo);                                // su valor de antes; el cambio, detrás
                    sacar.Tras.Add(sentencia + ";");
                }
                else
                {
                    string t = Temporal();                             // detrás no se puede: su valor, en una temporal
                    sacar.Antes.Add(Tipo(m.GetTypeInfo(objetivo).Type, objetivo) + " " + t + " = " + Expr(objetivo) + ";");
                    sacar.Antes.Add(sentencia + ";");
                    r = t;
                }
            }
            else
            {
                sacar.Antes.Add(ExprSentencia(n) + ";");               // `++x`, `x = e`, `x += e`: el cambio, delante
                r = Expr(objetivo);                                    // y su valor, el nuevo
            }
            sacar.Hechos[n] = r;
            return r;
        }

        string Temporal() => "valor_s" + (++sacados);

        /// ¿Está `n` donde sólo se calcula a veces: a la derecha de &&, || o ??, o en una rama de ?:?
        bool BajoCondicional(SyntaxNode n)
        {
            for (SyntaxNode h = n, p = n.Parent; p != null && h != sacar.Raiz; h = p, p = p.Parent)
            {
                if (p is BinaryExpressionSyntax b && b.Right == h && (b.IsKind(SyntaxKind.LogicalAndExpression)
                    || b.IsKind(SyntaxKind.LogicalOrExpression) || b.IsKind(SyntaxKind.CoalesceExpression))) return true;
                if (p is ConditionalExpressionSyntax c && (c.WhenTrue == h || c.WhenFalse == h)) return true;
            }
            return false;
        }

        /// ¿Se lee en otro sitio de la sentencia lo que cambia (la variable, el campo, o el array del elemento)?
        bool Repetida(SyntaxNode n, ExpressionSyntax objetivo)
        {
            var e = objetivo;
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            if (e is ElementAccessExpressionSyntax ea) e = ea.Expression;
            var s = m.GetSymbolInfo(e).Symbol;
            if (s == null) return true;
            return sacar.Raiz.DescendantNodesAndSelf().OfType<IdentifierNameSyntax>()
                .Any(id => !n.Span.Contains(id.Span) && SymbolEqualityComparer.Default.Equals(m.GetSymbolInfo(id).Symbol, s));
        }

        /// ¿Hay en la sentencia, fuera de `n`, otra cosa que pueda cambiar o mirar un campo: una llamada, un `new`, otra
        /// asignación, otro `++`?
        bool OtrosEfectos(SyntaxNode n) =>
            sacar.Raiz.DescendantNodesAndSelf().Any(x => !x.Span.Contains(n.Span) && !n.Span.Contains(x.Span)
                && (x is InvocationExpressionSyntax || x is ObjectCreationExpressionSyntax || x is AssignmentExpressionSyntax
                    || x is PostfixUnaryExpressionSyntax pu && !pu.IsKind(SyntaxKind.SuppressNullableWarningExpression)
                    || x is PrefixUnaryExpressionSyntax pr && (pr.IsKind(SyntaxKind.PreIncrementExpression) || pr.IsKind(SyntaxKind.PreDecrementExpression))));
    }
}
