// Las sentencias de C# en JC (V8/H6, docs/V8_IDEAS.md §E9). El cuerpo de un if, un while o un for, siempre entre
// llaves; un bloque suelto, que JC no tiene, con su `if (true)` (el mismo ámbito).
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
                case ContinueStatementSyntax _: Linea("continue;"); break;
                case ThrowStatementSyntax th: Throw(th); break;
                case TryStatementSyntax tr: Try(tr); break;
                case EmptyStatementSyntax _: break;
                default: RechazaNodo(s); break;
            }
        }

        void Local(LocalDeclarationStatementSyntax ld)
        {
            if (!ld.UsingKeyword.IsKind(SyntaxKind.None)) { Rechaza(ld, Porque.Pendiente("using")); return; }
            var d = ld.Declaration;
            if (d.Type is RefTypeSyntax) { Rechaza(d.Type, Porque.H7(Porque.RefOut)); return; }
            var primero = (ILocalSymbol) m.GetDeclaredSymbol(d.Variables[0]);
            string tipo = Tipo(primero.Type, d.Type);      // `var`, con su tipo
            var partes = d.Variables.Select(v => Ident(v.Identifier.ValueText)
                                                 + (v.Initializer != null ? " = " + Expr(v.Initializer.Value) : ""));
            Linea((ld.IsConst ? "const " : "") + tipo + " " + string.Join(", ", partes) + ";");
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

        void For(ForStatementSyntax f)
        {
            string cabecera = ForDeJc(f);
            if (cabecera == null) { Rechaza(f, Porque.Pendiente("un for que no es el de JC")); return; }
            Linea(cabecera);
            CuerpoDe(f.Statement);
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
            if (!(c.IsKind(SyntaxKind.LessThanOrEqualExpression) && dir == 1) && !(c.IsKind(SyntaxKind.GreaterThanOrEqualExpression) && dir == -1))
                return null;
            if (!LimiteFijo(c.Right, f.Statement)) return null;
            return "for (int " + Ident(nombre) + " = " + Expr(v.Initializer.Value) + "; " + Ident(nombre) + " "
                   + c.OperatorToken.Text + " " + Expr(c.Right) + "; " + Ident(nombre) + paso + ")";
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

        /// El tipo de un catch: Exception, una excepción del programa; las de la VM (NullReferenceException…), por su
        /// tipo, no: la VM lanza RuntimeError.
        string TipoExcepcion(ITypeSymbol t, SyntaxNode donde)
        {
            if (EsExcepcionNet(t)) return "Exception";
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
            return Rechaza(donde, Porque.Biblioteca("catch (" + NombreCompleto(t) + ")"));
        }
    }
}
