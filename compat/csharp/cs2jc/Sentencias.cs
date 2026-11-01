// Las sentencias de C# en JC (V8/H6, docs/V8_IDEAS.md §E9). El cuerpo de un if, un while o un for, siempre entre
// llaves; un bloque suelto, que JC no tiene, con su `if (true)` (el mismo ámbito).
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        void Bloque(BlockSyntax b, string primera = null)
        {
            Linea("{");
            sangria++;
            if (primera != null) Linea(primera);
            foreach (var s in b.Statements) Sentencia(s);
            sangria--;
            Linea("}");
        }

        /// El cuerpo de un if, un while, un for…: siempre con llaves.
        void CuerpoDe(StatementSyntax s)
        {
            if (s is BlockSyntax b) { Bloque(b); return; }
            Linea("{");
            sangria++;
            Sentencia(s);
            sangria--;
            Linea("}");
        }

        void Sentencia(StatementSyntax s)
        {
            switch (s)
            {
                case BlockSyntax b:
                    // JC no tiene el bloque suelto («un bloque va con su if, while, for…»): con `if (true)`, el mismo ámbito
                    Linea("if (true)");
                    Bloque(b);
                    break;
                case LocalDeclarationStatementSyntax ld: Local(ld); break;
                case ExpressionStatementSyntax es: Linea(ExprSentencia(es.Expression) + ";"); break;
                case IfStatementSyntax i: If(i); break;
                case WhileStatementSyntax w:
                    Linea("while (" + Expr(w.Condition) + ")");
                    CuerpoDe(w.Statement);
                    break;
                case DoStatementSyntax d:
                    Linea("do");
                    CuerpoDe(d.Statement);
                    Linea("while (" + Expr(d.Condition) + ");");
                    break;
                case ForStatementSyntax f: For(f); break;
                case ForEachStatementSyntax fe: Foreach(fe); break;
                case ReturnStatementSyntax r: Linea(r.Expression == null ? "return;" : "return " + Expr(r.Expression) + ";"); break;
                case BreakStatementSyntax _: Linea("break;"); break;
                case ContinueStatementSyntax co:
                    if (BucleDe(co) is ForStatementSyntax fw && forComoWhile.Contains(fw))
                        foreach (var i in fw.Incrementors) Linea(ExprSentencia(i) + ";");
                    Linea("continue;");
                    break;
                case SwitchStatementSyntax sw: Switch(sw); break;
                case UsingStatementSyntax u: Using(u); break;
                case LockStatementSyntax lo:
                    // un programa de un hilo: el bloque; lo que se cierra, si no es simple, no se calcula: no se traduce
                    if (!(lo.Expression is ThisExpressionSyntax || lo.Expression is IdentifierNameSyntax || lo.Expression is TypeOfExpressionSyntax))
                        Rechaza(lo.Expression, Porque.Pendiente("lock sobre algo que hay que calcular"));
                    Linea("if (true)");
                    CuerpoDe(lo.Statement);
                    break;
                case CheckedStatementSyntax ch when ch.IsKind(SyntaxKind.UncheckedStatement):
                    Linea("if (true)");                                      // unchecked: lo de siempre en C# y en BP
                    Bloque(ch.Block);
                    break;
                case ThrowStatementSyntax th: Throw(th); break;
                case TryStatementSyntax tr: Try(tr); break;
                case EmptyStatementSyntax _: break;
                default: RechazaNodo(s); break;
            }
        }

        void Local(LocalDeclarationStatementSyntax ld)
        {
            if (!ld.UsingKeyword.IsKind(SyntaxKind.None)) { Rechaza(ld, Porque.Pendiente("using")); return; }
            Declaracion(ld.Declaration, ld.IsConst);
        }

        /// `T a = x, b;` (la de un `for` también): `var`, con su tipo.
        void Declaracion(VariableDeclarationSyntax d, bool constante)
        {
            if (d.Type is RefTypeSyntax) { Rechaza(d.Type, Porque.H7(Porque.RefOut)); return; }
            var primero = (ILocalSymbol) m.GetDeclaredSymbol(d.Variables[0]);
            string tipo = Tipo(primero.Type, d.Type);
            // `T x = new T(…) { A = 1 };`: el objeto, y luego sus asignaciones, en orden (como C#). Sólo en una declaración,
            // donde `x` no existía antes; en otro sitio, el nuevo valor se vería antes de tiempo
            if (d.Variables.Count == 1 && d.Variables[0].Initializer?.Value is ObjectCreationExpressionSyntax oc
                && oc.Initializer != null && oc.Initializer.IsKind(SyntaxKind.ObjectInitializerExpression))
            {
                string x = Ident(d.Variables[0].Identifier.ValueText);
                Linea(tipo + " " + x + " = " + NuevoSinInicializador(oc) + ";");
                foreach (var e in oc.Initializer.Expressions)
                {
                    if (e is AssignmentExpressionSyntax a && a.IsKind(SyntaxKind.SimpleAssignmentExpression)
                        && a.Left is IdentifierNameSyntax miembro && !(a.Right is InitializerExpressionSyntax))
                    {
                        var s = m.GetSymbolInfo(miembro).Symbol;
                        if (s == null || !DelPrograma(s)) { Rechaza(e, Porque.Pendiente("un inicializador de algo de la biblioteca")); continue; }
                        Linea(x + "." + Ident(s.Name) + " = " + Expr(a.Right) + ";");
                    }
                    else Rechaza(e, Porque.Pendiente("un inicializador anidado, o de una colección"));
                }
                return;
            }
            var partes = d.Variables.Select(v => Ident(v.Identifier.ValueText)
                                                 + (v.Initializer != null ? " = " + Expr(v.Initializer.Value) : "")).ToList();
            if (constante)
                foreach (var p in partes) Linea("const " + tipo + " " + p + ";");      // JC: una constante por declaración
            else
                Linea(tipo + " " + string.Join(", ", partes) + ";");
        }

        /// `using (R r = …) cuerpo`: `R r = …; try { cuerpo } finally { if (r != null) r.Dispose(); }`, lo que hace C#,
        /// dentro de un `if (true)` que da a `r` su ámbito.
        void Using(UsingStatementSyntax u)
        {
            string nombre;
            Linea("if (true)");
            Linea("{");
            sangria++;
            if (u.Declaration != null)
            {
                if (u.Declaration.Variables.Count != 1) { Rechaza(u, Porque.Pendiente("using con varias variables")); }
                var v = (ILocalSymbol) m.GetDeclaredSymbol(u.Declaration.Variables[0]);
                nombre = Ident(v.Name);
                Declaracion(u.Declaration, false);
                CompruebaDispose(v.Type, u);
            }
            else
            {
                var t = m.GetTypeInfo(u.Expression).Type;
                nombre = "recurso_cs" + (++nombresNuevos);
                Linea(Tipo(t, u.Expression) + " " + nombre + " = " + Expr(u.Expression) + ";");
                CompruebaDispose(t, u);
            }
            Linea("try");
            CuerpoDe(u.Statement);
            Linea("finally");
            Linea("{");
            Linea("   if (" + nombre + " != null)");
            Linea("   {");
            Linea("      " + nombre + ".Dispose();");
            Linea("   }");
            Linea("}");
            sangria--;
            Linea("}");
        }

        /// Un `using` sólo con una clase del programa que tenga su `Dispose()`.
        void CompruebaDispose(ITypeSymbol t, SyntaxNode donde)
        {
            if (t == null || !DelPrograma(t) || t.TypeKind != TypeKind.Class)
                Rechaza(donde, Porque.Pendiente("using de algo que no es una clase del programa"));
            else if (!t.GetMembers("Dispose").OfType<IMethodSymbol>().Any(x => x.Parameters.Length == 0)
                     && !t.GetMembers().OfType<IMethodSymbol>().Any(x => x.ExplicitInterfaceImplementations.Any(i => i.Name == "Dispose")))
                Rechaza(donde, Porque.Pendiente("using de una clase cuyo Dispose está en su padre"));
        }

        void If(IfStatementSyntax i)
        {
            Linea("if (" + Expr(i.Condition) + ")");
            CuerpoDe(i.Statement);
            var e = i.Else;
            while (e != null)
            {
                if (e.Statement is IfStatementSyntax ei)
                {
                    Linea("else if (" + Expr(ei.Condition) + ")");
                    CuerpoDe(ei.Statement);
                    e = ei.Else;
                }
                else
                {
                    Linea("else");
                    CuerpoDe(e.Statement);
                    e = null;
                }
            }
        }

        /// Los `for` que se escriben como `while`: sus `continue` llevan delante los incrementos.
        readonly HashSet<ForStatementSyntax> forComoWhile = new HashSet<ForStatementSyntax>();

        void For(ForStatementSyntax f)
        {
            string cabecera = ForDeJc(f);
            if (cabecera != null)
            {
                Linea(cabecera);
                CuerpoDe(f.Statement);
                return;
            }
            // Los demás, como `while`, dentro de un `if (true)` que da a la variable el ámbito del `for`; un `continue` del
            // `for` hace antes sus incrementos, como en C#.
            forComoWhile.Add(f);
            Linea("if (true)");
            Linea("{");
            sangria++;
            if (f.Declaration != null) Declaracion(f.Declaration, false);
            foreach (var i in f.Initializers) Linea(ExprSentencia(i) + ";");
            Linea("while (" + (f.Condition != null ? Expr(f.Condition) : "true") + ")");
            Linea("{");
            sangria++;
            if (f.Statement is BlockSyntax b) foreach (var s in b.Statements) Sentencia(s);
            else Sentencia(f.Statement);
            foreach (var i in f.Incrementors) Linea(ExprSentencia(i) + ";");
            sangria--;
            Linea("}");
            sangria--;
            Linea("}");
        }

        /// El bucle al que salta un `continue` (el `switch` no lo para).
        static SyntaxNode BucleDe(ContinueStatementSyntax c)
        {
            foreach (var a in c.Ancestors())
                if (a is ForStatementSyntax || a is ForEachStatementSyntax || a is WhileStatementSyntax || a is DoStatementSyntax)
                    return a;
            return null;
        }

        void Switch(SwitchStatementSyntax sw)
        {
            // el de JC: `case 1, 2:`, sin caer de un caso al siguiente (C# tampoco), y el `default` al final (en C# el
            // orden no cambia nada, porque no se cae)
            // Una local declarada en un case y usada en otro: en C# su ámbito es el switch entero; en JC, el case.
            var deSeccion = new Dictionary<ISymbol, SwitchSectionSyntax>(SymbolEqualityComparer.Default);
            foreach (var sec in sw.Sections)
                foreach (var ld in sec.Statements.OfType<LocalDeclarationStatementSyntax>())
                    foreach (var v in ld.Declaration.Variables)
                        deSeccion[m.GetDeclaredSymbol(v)] = sec;
            foreach (var sec in sw.Sections)
                foreach (var id in sec.DescendantNodes().OfType<IdentifierNameSyntax>())
                    if (m.GetSymbolInfo(id).Symbol is ILocalSymbol l && deSeccion.TryGetValue(l, out var suya) && suya != sec)
                        Rechaza(id, Porque.Pendiente("una variable de un case usada en otro (en C#, su ámbito es el switch)"));
            Linea("switch (" + Expr(sw.Expression) + ")");
            Linea("{");
            sangria++;
            SwitchSectionSyntax porDefecto = null;
            foreach (var sec in sw.Sections)
            {
                foreach (var l in sec.Labels.Where(l => !(l is CaseSwitchLabelSyntax) && !(l is DefaultSwitchLabelSyntax)))
                    RechazaNodo(l);                                           // los patrones, de H7
                if (sec.Labels.Any(l => l is DefaultSwitchLabelSyntax)) { porDefecto = sec; continue; }   // `case 1: default:` es default
                Linea("case " + string.Join(", ", sec.Labels.OfType<CaseSwitchLabelSyntax>().Select(l => ValorDeCase(l.Value))) + ":");
                SentenciasDeCase(sec);
            }
            if (porDefecto != null)
            {
                Linea("default:");
                SentenciasDeCase(porDefecto);
            }
            sangria--;
            Linea("}");
        }

        void SentenciasDeCase(SwitchSectionSyntax sec)
        {
            sangria++;
            foreach (var s in sec.Statements) Sentencia(s);
            sangria--;
        }

        /// El valor de un `case`: el de un enum, por su nombre; los demás, el que calcula C# (`case 1 + 2:` es `case 3:`).
        string ValorDeCase(ExpressionSyntax e)
        {
            var t = m.GetTypeInfo(e).Type;
            if (EsEnum(t)) return Expr(e);
            var cv = m.GetConstantValue(e);
            if (!cv.HasValue) return Rechaza(e, Porque.Pendiente("un case que no es constante"));
            if (cv.Value == null) return Rechaza(e, EsCadena(m.GetTypeInfo(e).ConvertedType) ? Porque.H7(Porque.NullCadena) : Porque.Pendiente("case null"));
            if (e is LiteralExpressionSyntax) return Expr(e);
            switch (cv.Value)
            {
                case string s: return Cadena(s);
                case bool bo: return bo ? "true" : "false";
                case int i: return i == int.MinValue ? "(-2147483647 - 1)" : i.ToString(System.Globalization.CultureInfo.InvariantCulture);
                case long l: return l.ToString(System.Globalization.CultureInfo.InvariantCulture) + "L";
                default: return Expr(e);
            }
        }

        /// La forma del `for` de JC (punto 11): `for (int i = a; i <= b; i++)`, `i >= b` con `i--`, o `i += n` /
        /// `i -= n` con `n` un número; el límite, fijo (BP lo calcula una vez), y la variable, sin tocar en el cuerpo.
        /// Si no es ésa, null.
        string ForDeJc(ForStatementSyntax f)
        {
            if (f.Declaration == null || f.Declaration.Variables.Count != 1 || f.Initializers.Count != 0) return null;
            var v = f.Declaration.Variables[0];
            if (v.Initializer == null) return null;
            var simbolo = (ILocalSymbol) m.GetDeclaredSymbol(v);
            if (simbolo.Type.SpecialType != SpecialType.System_Int32) return null;
            string nombre = v.Identifier.ValueText;
            if (!(f.Condition is BinaryExpressionSyntax c) || !(c.Left is IdentifierNameSyntax cl) || cl.Identifier.ValueText != nombre)
                return null;
            if (f.Incrementors.Count != 1) return null;
            int dir;
            string paso;
            var inc = f.Incrementors[0];
            if (inc is PostfixUnaryExpressionSyntax pu && pu.Operand is IdentifierNameSyntax pn && pn.Identifier.ValueText == nombre)
            {
                dir = pu.IsKind(SyntaxKind.PostIncrementExpression) ? 1 : -1;
                paso = dir == 1 ? "++" : "--";
            }
            else if (inc is PrefixUnaryExpressionSyntax pr && pr.Operand is IdentifierNameSyntax prn && prn.Identifier.ValueText == nombre
                     && (pr.IsKind(SyntaxKind.PreIncrementExpression) || pr.IsKind(SyntaxKind.PreDecrementExpression)))
            {
                dir = pr.IsKind(SyntaxKind.PreIncrementExpression) ? 1 : -1;
                paso = dir == 1 ? "++" : "--";
            }
            else if (inc is AssignmentExpressionSyntax ae && ae.Left is IdentifierNameSyntax an && an.Identifier.ValueText == nombre
                     && ae.Right is LiteralExpressionSyntax lit && lit.IsKind(SyntaxKind.NumericLiteralExpression)
                     && lit.Token.Value is int
                     && (ae.IsKind(SyntaxKind.AddAssignmentExpression) || ae.IsKind(SyntaxKind.SubtractAssignmentExpression)))
            {
                dir = ae.IsKind(SyntaxKind.AddAssignmentExpression) ? 1 : -1;
                paso = (dir == 1 ? " += " : " -= ") + lit.Token.Text;
            }
            else return null;
            if (f.Statement.DescendantNodesAndSelf().Any(n => Asigna(n, nombre))) return null;
            string limite;
            if ((c.IsKind(SyntaxKind.LessThanOrEqualExpression) && dir == 1) || (c.IsKind(SyntaxKind.GreaterThanOrEqualExpression) && dir == -1))
            {
                if (!LimiteFijo(c.Right, f.Statement)) return null;
                limite = Ident(nombre) + " " + c.OperatorToken.Text + " " + Expr(c.Right);
            }
            else if ((c.IsKind(SyntaxKind.LessThanExpression) && dir == 1) || (c.IsKind(SyntaxKind.GreaterThanExpression) && dir == -1))
            {
                // `i < 10`, con un límite constante: `i <= 9` (calculado aquí, y sin pasarse del int)
                var cv = m.GetConstantValue(c.Right);
                if (!cv.HasValue || !(cv.Value is int lim) || (dir == 1 && lim == int.MinValue) || (dir == -1 && lim == int.MaxValue))
                    return null;
                limite = Ident(nombre) + (dir == 1 ? " <= " + (lim - 1) : " >= " + (lim + 1));
            }
            else return null;
            return "for (int " + Ident(nombre) + " = " + Expr(v.Initializer.Value) + "; " + limite + "; " + Ident(nombre) + paso + ")";
        }

        /// ¿Se asigna `nombre` en este nodo?
        static bool Asigna(SyntaxNode n, string nombre)
        {
            ExpressionSyntax objetivo = null;
            if (n is AssignmentExpressionSyntax a) objetivo = a.Left;
            else if (n is PostfixUnaryExpressionSyntax p && (p.IsKind(SyntaxKind.PostIncrementExpression) || p.IsKind(SyntaxKind.PostDecrementExpression))) objetivo = p.Operand;
            else if (n is PrefixUnaryExpressionSyntax q && (q.IsKind(SyntaxKind.PreIncrementExpression) || q.IsKind(SyntaxKind.PreDecrementExpression))) objetivo = q.Operand;
            else if (n is ArgumentSyntax g && !g.RefKindKeyword.IsKind(SyntaxKind.None)) objetivo = g.Expression;
            return objetivo is IdentifierNameSyntax id && id.Identifier.ValueText == nombre;
        }

        /// ¿El límite da lo mismo en cada vuelta? Una constante, o una local o un parámetro que el cuerpo no asigna (o
        /// sumas y restas de eso). El `for` de BP lo calcula una vez; el de C#, en cada vuelta.
        bool LimiteFijo(ExpressionSyntax lim, StatementSyntax cuerpo)
        {
            if (m.GetConstantValue(lim).HasValue) return true;
            if (lim is ParenthesizedExpressionSyntax par) return LimiteFijo(par.Expression, cuerpo);
            if (lim is IdentifierNameSyntax id)
            {
                var s = m.GetSymbolInfo(id).Symbol;
                if (s is ILocalSymbol || s is IParameterSymbol)
                    return !cuerpo.DescendantNodesAndSelf().Any(n => Asigna(n, id.Identifier.ValueText));
                return false;
            }
            if (lim is BinaryExpressionSyntax b && (b.IsKind(SyntaxKind.SubtractExpression) || b.IsKind(SyntaxKind.AddExpression)))
                return LimiteFijo(b.Left, cuerpo) && LimiteFijo(b.Right, cuerpo);
            return false;
        }

        void Foreach(ForEachStatementSyntax fe)
        {
            var tc = m.GetTypeInfo(fe.Expression).Type;
            var variable = m.GetDeclaredSymbol(fe);
            if (tc is IArrayTypeSymbol at && at.Rank == 1)
            {
                if (!SymbolEqualityComparer.Default.Equals(variable.Type, at.ElementType))
                {
                    Rechaza(fe.Type, Porque.Pendiente("foreach con otro tipo que el de los elementos"));
                    return;
                }
                Linea("foreach (var " + Ident(variable.Name) + " in " + Expr(fe.Expression) + ")");
                CuerpoDe(fe.Statement);
                return;
            }
            if (tc != null && tc.SpecialType == SpecialType.System_String) { Rechaza(fe, Porque.Pendiente("foreach sobre una cadena")); return; }
            if (tc is IArrayTypeSymbol) { Rechaza(fe, Porque.H7(Porque.ArraysVarias)); return; }
            Rechaza(fe, Porque.Pendiente("foreach sobre " + NombreCompleto(tc)));
        }

        void Throw(ThrowStatementSyntax th)
        {
            if (th.Expression == null)
            {
                // `throw;`, el de C#, relanza la del catch: en JC, `throw e;` (punto 28)
                if (catchs.Count == 0 || catchs.Peek() == null) { Rechaza(th, Porque.Pendiente("throw; sin catch")); return; }
                Linea("throw " + catchs.Peek() + ";");
                return;
            }
            Linea("throw " + Expr(th.Expression) + ";");
        }

        void Try(TryStatementSyntax tr)
        {
            Linea("try");
            Bloque(tr.Block);
            foreach (var c in tr.Catches)
            {
                if (c.Filter != null) Rechaza(c.Filter, Porque.H7(Porque.ThrowExpresion));
                if (c.Declaration == null)
                {
                    // `catch` a secas: en JC no tiene nombre; si dentro hay un `throw;`, `catch (Exception e)`, que en BP
                    // también lo atrapa todo
                    if (Relanza(c.Block))
                    {
                        string n = NombreNuevo();
                        Linea("catch (Exception " + n + ")");
                        catchs.Push(n);
                    }
                    else
                    {
                        Linea("catch");
                        catchs.Push(null);
                    }
                }
                else
                {
                    var tipo = m.GetTypeInfo(c.Declaration.Type).Type;
                    string jt = TipoExcepcion(tipo, c.Declaration.Type);
                    bool sinNombre = c.Declaration.Identifier.IsKind(SyntaxKind.None);
                    string nombre = sinNombre ? NombreNuevo() : Ident(c.Declaration.Identifier.ValueText);
                    // `throw;` relanza la que llegó, aunque el catch haya cambiado su variable (`ex = new …; throw;`):
                    // entonces la que llegó, en una variable del traductor, y la del programa, una local con su valor
                    if (!sinNombre && Relanza(c.Block)
                        && c.Block.DescendantNodes().Any(n => Asigna(n, c.Declaration.Identifier.ValueText)))
                    {
                        string llegada = NombreNuevo();
                        Linea("catch (" + jt + " " + llegada + ")");
                        catchs.Push(llegada);
                        Bloque(c.Block, jt + " " + nombre + " = " + llegada + ";");
                        catchs.Pop();
                        continue;
                    }
                    Linea("catch (" + jt + " " + nombre + ")");
                    catchs.Push(nombre);
                }
                Bloque(c.Block);
                catchs.Pop();
            }
            if (tr.Finally != null)
            {
                Linea("finally");
                Bloque(tr.Finally.Block);
            }
        }

        /// ¿Hay en el bloque de un catch un `throw;` que sea suyo (no el de un catch de dentro)?
        static bool Relanza(BlockSyntax b) =>
            b.DescendantNodes().OfType<ThrowStatementSyntax>().Any(th => th.Expression == null
                && th.Ancestors().OfType<CatchClauseSyntax>().First().Block == b);

        /// El tipo de un catch: Exception, SystemException (el RuntimeError de BP), una de la librería CsSystem, una
        /// excepción del programa; las de la VM (NullReferenceException…), por su tipo, no: la VM lanza RuntimeError.
        string TipoExcepcion(ITypeSymbol t, SyntaxNode donde)
        {
            if (ExcepcionJc(t) is string ex) return ex;
            if (DelPrograma(t))
            {
                CompruebaOrden(t, donde);
                return Tipo(t, donde);
            }
            switch (NombreCompleto(t))
            {
                case "System.NullReferenceException":
                case "System.IndexOutOfRangeException":
                case "System.DivideByZeroException":
                case "System.InvalidCastException":
                case "System.OverflowException":
                case "System.ArithmeticException":
                case "System.ArrayTypeMismatchException":
                case "System.StackOverflowException":
                case "System.OutOfMemoryException":
                    return Rechaza(donde, Porque.H7(Porque.ErroresVm));
            }
            return Rechaza(donde, Porque.DeNet("catch (" + NombreCompleto(t) + ")"));
        }
    }
}
