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
            Sentencias(b);
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
                case LocalDeclarationStatementSyntax ld: EmiteConSacar(ld, true, () => Local(ld)); break;
                // la llamada a un método partial sin cuerpo: C# la quita, con sus argumentos sin calcular (el escalón 1)
                case ExpressionStatementSyntax es when es.Expression is InvocationExpressionSyntax llamada
                                                       && m.GetSymbolInfo(llamada).Symbol is IMethodSymbol pm
                                                       && pm.OriginalDefinition.IsPartialDefinition && pm.OriginalDefinition.PartialImplementationPart == null:
                    Linea("// " + pm.Name + "(…): un método partial sin cuerpo; C# quita la llamada");
                    break;
                // la de un método [Conditional("S")] sin S definido: lo mismo (H8, tema 1)
                case ExpressionStatementSyntax es when es.Expression is InvocationExpressionSyntax llamada
                                                       && m.GetSymbolInfo(llamada).Symbol is IMethodSymbol cm
                                                       && CondicionalSinDefinir(cm, llamada) is string simbolos:
                    Linea("// " + cm.Name + "(…): [Conditional(" + simbolos + ")], sin definir; C# quita la llamada");
                    break;
                // `this = x;`: sólo en un struct, y uno sin estado (H8, tema 2) no tiene nada que copiar; si `x` calcula algo, se calcula
                case ExpressionStatementSyntax es when es.Expression is AssignmentExpressionSyntax ta && ta.Left is ThisExpressionSyntax:
                    if (EsSimple(ta.Right)) Linea("// this = …: un struct sin estado no tiene nada que copiar");
                    else EmiteConSacar(es, true, () => Linea(ExprSentencia(ta.Right) + ";"));
                    break;
                // `new T () { A = 1 };`: el objeto y sus asignaciones van delante, y aquí no queda nada (gtest-autoproperty-22)
                case ExpressionStatementSyntax es when es.Expression is ObjectCreationExpressionSyntax nuevo && nuevo.Initializer != null:
                    EmiteConSacar(es, true, () => { Expr(nuevo); Linea("// un new con su inicializador, que nadie guarda"); });
                    break;
                case ExpressionStatementSyntax es: EmiteConSacar(es, true, () => Linea(ExprSentencia(es.Expression) + ";")); break;
                case IfStatementSyntax i: If(i); break;
                case WhileStatementSyntax w: While(w); break;
                case DoStatementSyntax d: Do(d); break;
                case ForStatementSyntax f: For(f); break;
                case ForEachStatementSyntax fe: Foreach(fe); break;
                case ReturnStatementSyntax r:
                    EmiteConSacar(r, false, () => Linea(r.Expression == null ? "return;" : "return " + Expr(r.Expression) + ";"));
                    break;
                case BreakStatementSyntax br: SaltoDeBucle(true, DestinoDe(br)); break;
                case ContinueStatementSyntax co: SaltoDeBucle(false, BucleDe(co)); break;
                case GotoStatementSyntax g: Goto(g); break;
                case LabeledStatementSyntax ls: Sentencia(ls.Statement); break;          // la etiqueta, en Saltos.cs
                case SwitchStatementSyntax sw: Switch(sw); break;
                case UsingStatementSyntax u: Using(u); break;
                case LockStatementSyntax lo: Lock(lo); break;
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

        /// Las locales de un case que se usan en otro (el escalón 1): declaradas antes del switch (Switch), así que su
        /// declaración, en el case, es una asignación.
        readonly HashSet<ISymbol> izadas = new HashSet<ISymbol>(SymbolEqualityComparer.Default);
        /// Su nombre en JC (`i_s1`), y cuántas van.
        readonly Dictionary<ISymbol, string> renombradas = new Dictionary<ISymbol, string>(SymbolEqualityComparer.Default);
        int subidas = 0;

        void Local(LocalDeclarationStatementSyntax ld)
        {
            if (!ld.UsingKeyword.IsKind(SyntaxKind.None)) { Rechaza(ld, Porque.Pendiente("using")); return; }
            // una local sin valor que nadie usa no hace nada (H8, tema 1): no se escribe, sea del tipo que sea
            // (`System.Net.IPAddress[] x;`, test-55); con un `typeof`, que no calcula nada, tampoco (test-static-using-10), ni con
            // un null (`Type t = (Type) null;`, test-624)
            if (ld.Declaration.Variables.All(v => (v.Initializer == null || v.Initializer.Value is TypeOfExpressionSyntax
                                                   || EsNullEscrito(v.Initializer.Value)) && SinUsar(v, ld)))
            {
                Linea("// " + string.Join(", ", ld.Declaration.Variables.Select(v => v.Identifier.ValueText)) + ": una local sin valor que nadie usa");
                return;
            }
            if (ld.Declaration.Variables.Any(v => izadas.Contains(m.GetDeclaredSymbol(v))))
            {
                if (ld.IsConst) return;                                       // ya está, con su valor, antes del switch
                foreach (var v in ld.Declaration.Variables.Where(v => v.Initializer != null))
                    Linea(renombradas[m.GetDeclaredSymbol(v)] + " = " + Expr(v.Initializer.Value) + ";");
                return;
            }
            Declaracion(ld.Declaration, ld.IsConst);
        }

        /// ¿No aparece la local `v` en ningún sitio del miembro donde se declara?
        bool SinUsar(VariableDeclaratorSyntax v, SyntaxNode donde)
        {
            var s = m.GetDeclaredSymbol(v);
            var cuerpo = donde.Ancestors().OfType<MemberDeclarationSyntax>().First();
            return !cuerpo.DescendantNodes().OfType<IdentifierNameSyntax>()
                .Any(id => id.Identifier.ValueText == v.Identifier.ValueText && SymbolEqualityComparer.Default.Equals(m.GetSymbolInfo(id).Symbol, s));
        }

        /// `lock (x) cuerpo`: un programa de un hilo, el bloque; pero lo que se cierra se calcula una vez, antes (una
        /// asignación, como sentencia; otra cosa, en una temporal), y si es null, ArgumentNullException, como C#
        /// (Monitor.Enter): test-524 y test-899. Sin mirarlo, lo que no puede ser null: this, typeof, un new, una cadena.
        void Lock(LockStatementSyntax lo)
        {
            var e = lo.Expression;
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            string objetivo = null;
            if (e is ThisExpressionSyntax || e is TypeOfExpressionSyntax) { }
            else if (e is AssignmentExpressionSyntax a && a.IsKind(SyntaxKind.SimpleAssignmentExpression))
            {
                Linea(ExprSentencia(a) + ";");
                if (!(a.Right is ObjectCreationExpressionSyntax)) objetivo = Expr(a.Left);
            }
            else if (e is IdentifierNameSyntax) objetivo = Expr(e);
            else
            {
                string tmp = "lock_s" + (++subidas);
                Linea("Object " + tmp + " = " + Expr(e) + ";");
                if (!(e is ObjectCreationExpressionSyntax)) objetivo = tmp;
            }
            if (objetivo != null && !EsCadena(m.GetTypeInfo(e).Type))
            {
                imports.Add("CsSystem");
                Linea("if (" + objetivo + " == null)");
                Linea("{");
                sangria++;
                Linea("throw new CsSystem.ArgumentNullException(\"obj\");");
                sangria--;
                Linea("}");
            }
            Linea("if (true)");
            CuerpoDe(lo.Statement);
        }

        /// Un valor del tipo para una local declarada antes de tiempo: C# no deja leerla sin asignarla, así que nadie lo ve.
        string ValorCualquiera(ITypeSymbol t, SyntaxNode donde)
        {
            switch (t.SpecialType)
            {
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64: return "0L";
                case SpecialType.System_Double: return "0.0";
                case SpecialType.System_Single: return "0.0f";
                case SpecialType.System_Boolean: return "false";
                case SpecialType.System_String: return "\"\"";
            }
            if (EsNumerico(t) || EsEnumEntero(t)) return "0";
            if (EsEnum(t) && t.GetMembers().OfType<IFieldSymbol>().FirstOrDefault(f => f.IsConst) is IFieldSymbol primero)
                return Tipo(t, donde) + "." + Ident(primero.Name);
            if (!t.IsValueType || EsNullable(t)) return "null";
            if (EsStructSinEstado(t)) return "new " + Tipo(t, donde) + "()";
            return Rechaza(donde, Porque.Pendiente("una variable de un case usada en otro (en C#, su ámbito es el switch)"));
        }

        /// #778 — ` = <el valor de su tipo>` para una local que C# declara sin valor (ValorCualquiera: null, 0, false, ""…), o
        /// nada si el tipo no tiene uno que se pueda escribir (entonces queda como en C#, y si JC la ve sin valor, lo dice).
        string SinValor(ITypeSymbol t, SyntaxNode donde)
        {
            bool tiene = EsNumerico(t) || t.SpecialType == SpecialType.System_Boolean || EsCadena(t) || EsNullable(t)
                         || !t.IsValueType || EsStructSinEstado(t)
                         || (EsEnum(t) && t.GetMembers().OfType<IFieldSymbol>().Any(f => f.IsConst));
            return tiene ? " = " + ValorCualquiera(t, donde) : "";
        }

        /// `A = 1`, de un `new T() { A = 1 }`, sobre el objeto `x`: la sentencia; null, si no se traduce (ya dicho).
        string AsignacionDeInicializador(string x, ExpressionSyntax e)
        {
            if (e is AssignmentExpressionSyntax a && a.IsKind(SyntaxKind.SimpleAssignmentExpression)
                && a.Left is IdentifierNameSyntax miembro && !(a.Right is InitializerExpressionSyntax))
            {
                var s = m.GetSymbolInfo(miembro).Symbol;
                if (s != null && DelPrograma(s)) return x + "." + NombreDe(s) + " = " + Expr(a.Right) + ";";
                Rechaza(e, Porque.Pendiente("un inicializador de algo de la biblioteca"));
                return null;
            }
            Rechaza(e, Porque.Pendiente("un inicializador anidado, o de una colección"));
            return null;
        }

        /// `T a = x, b;` (la de un `for` también): `var`, con su tipo.
        void Declaracion(VariableDeclarationSyntax d, bool constante)
        {
            if (d.Type is RefTypeSyntax) { Rechaza(d.Type, Porque.H7(Porque.RefOut)); return; }
            var primero = (ILocalSymbol) m.GetDeclaredSymbol(d.Variables[0]);
            string tipo = Tipo(primero.Type, d.Type);
            // H7.4: una local que se pasa por ref u out, en su caja (Cajas.cs); cada una en su línea, que el tipo ya no es el mismo
            if (!constante && d.Variables.Any(v => m.GetDeclaredSymbol(v) is ILocalSymbol lv && enCaja.Contains(lv)))
            {
                foreach (var v in d.Variables)
                {
                    var lv = (ILocalSymbol) m.GetDeclaredSymbol(v);
                    if (enCaja.Contains(lv)) Linea(DeclaraEnCaja(v, lv));
                    else Linea(tipo + " " + Ident(v.Identifier.ValueText)
                               + (v.Initializer != null ? " = " + Expr(v.Initializer.Value) : SinValor(lv.Type, v)) + ";");
                }
                return;
            }
            // `T x = new T(…) { A = 1 };`: el objeto, y luego sus asignaciones, en orden (como C#). Sólo en una declaración,
            // donde `x` no existía antes; en otro sitio, el nuevo valor se vería antes de tiempo
            if (d.Variables.Count == 1 && d.Variables[0].Initializer?.Value is ObjectCreationExpressionSyntax oc
                && oc.Initializer != null && oc.Initializer.IsKind(SyntaxKind.ObjectInitializerExpression))
            {
                string x = Ident(d.Variables[0].Identifier.ValueText);
                Linea(tipo + " " + x + " = " + NuevoSinInicializador(oc) + ";");
                foreach (var e in oc.Initializer.Expressions)
                    if (AsignacionDeInicializador(x, e) is string linea) Linea(linea);
                return;
            }
            // #778 (Eduardo, 3-nov: «el traductor puede asignarle un valor por defecto ( un null ). En Java ese código no
            // compilaría por utilizar una variable sin asignar») — una local que C# declara sin valor, con el de su tipo. C#
            // no deja leerla sin asignarla, y lo comprueba con más detalle que JC (un `switch` sobre una constante,
            // test-504): ese valor nadie lo lee. JC, como Java, pide que tenga uno en todos los caminos (#540).
            var partes = d.Variables.Select(v => Ident(v.Identifier.ValueText)
                                                 + (constante && EsEnumEntero(primero.Type) && m.GetDeclaredSymbol(v) is ILocalSymbol lc
                                                    ? " = " + NumeroDeEnum(lc.ConstantValue, primero.Type)
                                                    : v.Initializer != null ? " = " + Expr(v.Initializer.Value)
                                                    : SinValor(((ILocalSymbol) m.GetDeclaredSymbol(v)).Type, v))).ToList();
            // #777: una constante de C# que vale null (`const string x = null`): en JC una constante es un valor y no
            // puede serlo; se escribe como lo que hace, una variable que nadie cambia
            if (constante && d.Variables.Any(v => m.GetDeclaredSymbol(v) is ILocalSymbol ls && ls.HasConstantValue && ls.ConstantValue == null))
                constante = false;
            if (constante)
                foreach (var p in partes) Linea("const " + tipo + " " + p + ";");      // JC: una constante por declaración
            else
                Linea(tipo + " " + string.Join(", ", partes) + ";");
        }

        /// `using (R r = …) cuerpo`: `R r = …; try { cuerpo } finally { if (r != null) r.Dispose(); }`, lo que hace C#,
        /// dentro de un `if (true)` que da a `r` su ámbito. Con varias variables, una dentro de otra, como dice C#. El tipo,
        /// el que sea: `IDisposable` es una interfaz de CsSystem (H8, tema 2), y una clase que la implementa tiene su
        /// `Dispose`, suyo o de su padre.
        void Using(UsingStatementSyntax u)
        {
            Linea("if (true)");
            Linea("{");
            sangria++;
            if (u.Declaration != null) UsingVariable(u, 0);
            else
            {
                var t = m.GetTypeInfo(u.Expression).Type;
                if (t == null) Rechaza(u, Porque.Pendiente("using de un null"));
                string nombre = "recurso_cs" + (++nombresNuevos);
                Linea(Tipo(t, u.Expression) + " " + nombre + " = " + Expr(u.Expression) + ";");
                Linea("try");
                CuerpoDe(u.Statement);
                UsingFinally(nombre);
            }
            sangria--;
            Linea("}");
        }

        /// La variable `i` de un `using`, y dentro las que siguen y su cuerpo.
        void UsingVariable(UsingStatementSyntax u, int i)
        {
            var vars = u.Declaration.Variables;
            var v = (ILocalSymbol) m.GetDeclaredSymbol(vars[i]);
            if (vars.Count == 1) Declaracion(u.Declaration, false);
            else Linea(Tipo(v.Type, u.Declaration.Type) + " " + Ident(v.Name) + " = " + Expr(vars[i].Initializer.Value) + ";");
            Linea("try");
            if (i + 1 == vars.Count) CuerpoDe(u.Statement);
            else
            {
                Linea("{");
                sangria++;
                UsingVariable(u, i + 1);
                sangria--;
                Linea("}");
            }
            UsingFinally(Ident(v.Name));
        }

        void UsingFinally(string nombre)
        {
            Linea("finally");
            Linea("{");
            Linea("   if (" + nombre + " != null)");
            Linea("   {");
            Linea("      " + nombre + ".Dispose();");
            Linea("   }");
            Linea("}");
        }

        void If(IfStatementSyntax i)
        {
            var (cond, antes) = Condicion(i.Condition);
            foreach (var l in antes) Linea(l);
            IfDesde(i, cond);
        }

        /// `if (cond) cuerpo`, y su cadena de else if. Lo que saca la condición de un else if va delante de ella, y un
        /// else if no tiene «delante»: la cadena sigue dentro de un `else { …; if … }`.
        void IfDesde(IfStatementSyntax i, string cond)
        {
            Linea("if (" + cond + ")");
            CuerpoDe(i.Statement);
            var e = i.Else;
            while (e != null)
            {
                if (e.Statement is IfStatementSyntax ei)
                {
                    var (c2, antes2) = Condicion(ei.Condition);
                    if (antes2.Count > 0)
                    {
                        Linea("else");
                        Linea("{");
                        sangria++;
                        foreach (var l in antes2) Linea(l);
                        IfDesde(ei, c2);
                        sangria--;
                        Linea("}");
                        return;
                    }
                    Linea("else if (" + c2 + ")");
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

        /// `while (cond) cuerpo`. Si la condición saca algo: `while (true) { …; if (!(cond)) { break; } cuerpo }`, así lo
        /// sacado se hace en cada vuelta, también en la última, como en C#; un `continue` vuelve arriba, a la condición.
        void While(WhileStatementSyntax w)
        {
            var (cond, antes) = Condicion(w.Condition);
            if (antes.Count == 0)
            {
                Linea("while (" + cond + ")");
                CuerpoDe(w.Statement);
                return;
            }
            Linea("while (true)");
            Linea("{");
            sangria++;
            foreach (var l in antes) Linea(l);
            Sal(cond);
            if (w.Statement is BlockSyntax b) Sentencias(b);
            else Sentencia(w.Statement);
            sangria--;
            Linea("}");
        }

        /// `do cuerpo while (cond);`. Si la condición saca algo: `do { cuerpo; …; if (!(cond)) { break; } } while (true);`.
        /// Con un `continue` del cuerpo, que en C# va a la condición (y en esa forma se la saltaría): `bool primera = true;
        /// while (true) { if (!primera) { …; if (!(cond)) { break; } } primera = false; cuerpo }`.
        void Do(DoStatementSyntax d)
        {
            var (cond, antes) = Condicion(d.Condition);
            if (antes.Count == 0)
            {
                Linea("do");
                CuerpoDe(d.Statement);
                Linea("while (" + cond + ");");
                return;
            }
            if (d.Statement.DescendantNodesAndSelf().OfType<ContinueStatementSyntax>().Any(c => BucleDe(c) == d))
            {
                string primera = "primera_s" + (++sacados);
                Linea("bool " + primera + " = true;");
                Linea("while (true)");
                Linea("{");
                sangria++;
                Linea("if (!" + primera + ")");
                Linea("{");
                sangria++;
                foreach (var l in antes) Linea(l);
                Sal(cond);
                sangria--;
                Linea("}");
                Linea(primera + " = false;");
                if (d.Statement is BlockSyntax cb) Sentencias(cb);
                else Sentencia(d.Statement);
                sangria--;
                Linea("}");
                return;
            }
            Linea("do");
            Linea("{");
            sangria++;
            if (d.Statement is BlockSyntax b) Sentencias(b);
            else Sentencia(d.Statement);
            foreach (var l in antes) Linea(l);
            Sal(cond);
            sangria--;
            Linea("}");
            Linea("while (true);");
        }

        /// La salida de un bucle cuya condición sacó algo: `if (!(cond)) { break; }`.
        void Sal(string cond)
        {
            Linea("if (!(" + cond + "))");
            Linea("{");
            Linea("   break;");
            Linea("}");
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
            // `for` hace antes sus incrementos, como en C#. Sin variable que declarar (`for (i = 0; …)`), sin el `if (true)`:
            // no hace falta, y JC no ve que se entra, así que tras él no daría `i` por asignada (test-25, H8).
            forComoWhile.Add(f);
            bool ambito = f.Declaration != null;
            if (ambito)
            {
                Linea("if (true)");
                Linea("{");
                sangria++;
            }
            if (f.Declaration != null) Declaracion(f.Declaration, false);
            foreach (var i in f.Initializers) Linea(ExprSentencia(i) + ";");
            // la condición: si saca algo, dentro del bucle (While), y un `continue` del for hace antes sus incrementos
            var (cond, antes) = f.Condition != null ? Condicion(f.Condition) : ("true", new List<string>());
            Linea("while (" + (antes.Count == 0 ? cond : "true") + ")");
            Linea("{");
            sangria++;
            if (antes.Count > 0)
            {
                foreach (var l in antes) Linea(l);
                Sal(cond);
            }
            if (f.Statement is BlockSyntax b) Sentencias(b);
            else Sentencia(f.Statement);
            foreach (var i in f.Incrementors) Linea(ExprSentencia(i) + ";");
            sangria--;
            Linea("}");
            if (ambito)
            {
                sangria--;
                Linea("}");
            }
        }

        /// El bucle al que salta un `continue` (el `switch` no lo para).
        static SyntaxNode BucleDe(SyntaxNode c)
        {
            foreach (var a in c.Ancestors())
                if (a is ForStatementSyntax || a is ForEachStatementSyntax || a is WhileStatementSyntax || a is DoStatementSyntax)
                    return a;
            return null;
        }

        void Switch(SwitchStatementSyntax sw)
        {
            // sobre un T?, un null va al default: el switch de JC no lo hace solo
            if (EsNullable(m.GetTypeInfo(sw.Expression).Type)) { Rechaza(sw.Expression, Porque.Pendiente("switch sobre un T?")); return; }
            if (SwitchConSaltos(sw)) { EmiteSwitchConSaltos(sw); return; }             // con etiquetas o `goto case` (Saltos.cs)
            // el de JC: `case 1, 2:`, sin caer de un caso al siguiente (C# tampoco), y el `default` al final (en C# el
            // orden no cambia nada, porque no se cae)
            // Una local declarada en un case y usada en otro: en C# su ámbito es el switch entero; en JC, el case.
            var deSeccion = new Dictionary<ISymbol, SwitchSectionSyntax>(SymbolEqualityComparer.Default);
            foreach (var sec in sw.Sections)
                foreach (var ld in sec.Statements.OfType<LocalDeclarationStatementSyntax>())
                    foreach (var v in ld.Declaration.Variables)
                        deSeccion[m.GetDeclaredSymbol(v)] = sec;
            // (el escalón 1) se declaran antes del switch, y en su case su declaración es una asignación; usada en la
            // etiqueta de otro (`case x:`, x una constante), no cuenta: la etiqueta lleva su valor
            var antes = new List<ILocalSymbol>();
            foreach (var sec in sw.Sections)
                foreach (var id in sec.DescendantNodes().OfType<IdentifierNameSyntax>())
                    if (m.GetSymbolInfo(id).Symbol is ILocalSymbol l && deSeccion.TryGetValue(l, out var suya) && suya != sec
                        && !id.Ancestors().Any(x => x is CaseSwitchLabelSyntax) && !antes.Contains(l, SymbolEqualityComparer.Default))
                        antes.Add(l);
            // con un nombre propio (`i_s1`), para no chocar con otra del mismo nombre fuera del switch (en C#, otra puede
            // reusarlo en un bloque hermano); no dentro de un `if (true)`: JC no ve que se entra, y una variable que el
            // switch asigna en todos sus casos le parecería sin valor (la diferencia de test-901)
            foreach (var l in antes) IzaLocal(l);
            SwitchDentro(sw);
        }

        /// Una local declarada antes de tiempo, con su nombre propio; en su sitio, su declaración es una asignación (Local).
        void IzaLocal(ILocalSymbol l)
        {
            var decl = l.DeclaringSyntaxReferences.First().GetSyntax();
            string nuevo = Ident(l.Name) + "_s" + (++subidas);
            renombradas[l] = nuevo;
            if (l.IsConst) Linea("const " + Tipo(l.Type, decl) + " " + nuevo + " = " + LiteralDeConstante(l.ConstantValue, decl) + ";");
            else Linea(Tipo(l.Type, decl) + " " + nuevo + " = " + ValorCualquiera(l.Type, decl) + ";");
            izadas.Add(l);
        }

        void SwitchDentro(SwitchStatementSyntax sw)
        {
            var (que, antes) = Condicion(sw.Expression);
            foreach (var l in antes) Linea(l);
            Linea("switch (" + que + ")");
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
            if (EsEnum(t) || EsEnumEntero(t)) return Expr(e);
            var cv = m.GetConstantValue(e);
            if (!cv.HasValue) return Rechaza(e, Porque.Pendiente("un case que no es constante"));
            // `case null:` (en JC, un switch sobre un null va al default: #774) pediría partir el switch con un if delante
            if (cv.Value == null) return Rechaza(e, Porque.Pendiente("case null"));
            // el de un switch sobre un uint (en JC, un long): su valor, como long
            var sw = e.Ancestors().OfType<SwitchStatementSyntax>().FirstOrDefault();
            if (sw != null && EsUInt(m.GetTypeInfo(sw.Expression).Type))
                return System.Convert.ToInt64(cv.Value).ToString(System.Globalization.CultureInfo.InvariantCulture) + "L";
            // y sobre un ulong, sus 64 bits (SinSigno.cs)
            if (sw != null && EsULong(m.GetTypeInfo(sw.Expression).Type))
                return LiteralDeConstante(cv.Value is ulong ul ? unchecked((long) ul) : System.Convert.ToInt64(cv.Value), e);
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
                var (coleccion, antes) = Condicion(fe.Expression);
                foreach (var l in antes) Linea(l);
                Linea("foreach (var " + Ident(variable.Name) + " in " + coleccion + ")");
                CuerpoDe(fe.Statement);
                return;
            }
            if (tc != null && tc.SpecialType == SpecialType.System_String) { ForeachCadena(fe, variable); return; }
            if (tc is IArrayTypeSymbol) { Rechaza(fe, Porque.H7(Porque.ArraysVarias)); return; }
            if (EsLista(tc)) { ForeachLista(fe, variable); return; }
            var info = m.GetForEachStatementInfo(fe);
            if (info.GetEnumeratorMethod is IMethodSymbol ge && (DelPrograma(ge) || EsDeCsSystem(ge.ContainingType)))
            {
                ForeachPropio(fe, info, ge, variable);
                return;
            }
            Rechaza(fe, Porque.Pendiente("foreach sobre " + NombreCompleto(tc)));
        }

        /// `foreach (T x in lista)` sobre una `ArrayList`, que es la `List` de Core: el `foreach` de JC, y su elemento, un Object.
        void ForeachLista(ForEachStatementSyntax fe, ILocalSymbol variable)
        {
            var (lista, antes) = Condicion(fe.Expression);
            foreach (var l in antes) Linea(l);
            bool conTipo = SaleConSuTipo(fe.Expression);       // en una `List<X>`, ya es un X (Genericos.cs)
            if (variable.Type.SpecialType == SpecialType.System_Object || variable.Type is ITypeParameterSymbol
                || (conTipo && !variable.Type.IsValueType))
            {
                Linea("foreach (var " + Ident(variable.Name) + " in " + lista + ")");
                CuerpoDe(fe.Statement);
                return;
            }
            string x = "elemento_s" + (++sacados);
            Linea("foreach (var " + x + " in " + lista + ")");
            if (conTipo)
            {
                CuerpoConPrimera(fe.Statement, Tipo(variable.Type, fe.Type) + " " + Ident(variable.Name) + " = " + x + ".value();");
                return;
            }
            var objeto = m.Compilation.GetSpecialType(SpecialType.System_Object);
            CuerpoConPrimera(fe.Statement, Tipo(variable.Type, fe.Type) + " " + Ident(variable.Name) + " = " + Convierte(x, objeto, variable.Type, fe.Type) + ";");
        }

        /// `foreach (char c in s)`: sus caracteres, por su índice; en JC, un char es su código. La cadena se calcula una vez.
        void ForeachCadena(ForEachStatementSyntax fe, ILocalSymbol variable)
        {
            if (variable.Type.SpecialType != SpecialType.System_Char) { Rechaza(fe.Type, Porque.Pendiente("foreach con otro tipo que el de los elementos")); return; }
            var (cadena, antes) = Condicion(fe.Expression);
            foreach (var l in antes) Linea(l);
            string t = "cadena_s" + (++sacados), i = "indice_s" + sacados;
            Linea("string " + t + " = " + cadena + ";");
            Linea("for (int " + i + " = 0; " + i + " <= strlen(" + t + ") - 1; " + i + "++)");
            CuerpoConPrimera(fe.Statement, "int " + Ident(variable.Name) + " = charCodeAt(" + t + ", " + i + ");");
        }

        /// `foreach (T x in c)` sobre lo que tiene `GetEnumerator`, una clase del programa o una de CsSystem: el bucle que escribe
        /// C#. El enumerador se suelta al salir, por donde sea, si es `IDisposable`, o si puede serlo alguna de sus hijas (test-188).
        void ForeachPropio(ForEachStatementSyntax fe, ForEachStatementInfo info, IMethodSymbol ge, ILocalSymbol variable)
        {
            string tipo = Tipo(ge.ReturnType, fe.Expression);
            if (tipo == Hueco) return;
            var (coleccion, antes) = Condicion(fe.Expression);
            foreach (var l in antes) Linea(l);
            string e = "enumerador_s" + (++sacados);
            Linea(tipo + " " + e + " = " + coleccion + "." + (DelPrograma(ge) ? NombreMetodo(ge) : ge.Name) + "();");
            bool suelta = PuedeSoltarse(ge.ReturnType);
            if (suelta) { Linea("try"); Linea("{"); sangria++; }
            string actual = e + "." + (DelPrograma(info.CurrentProperty) ? NombreDe(info.CurrentProperty) : info.CurrentProperty.Name);
            Linea("while (" + e + "." + (DelPrograma(info.MoveNextMethod) ? NombreMetodo(info.MoveNextMethod) : info.MoveNextMethod.Name) + "())");
            string valor = Convierte(actual, info.ElementType, variable.Type, fe.Type);
            CuerpoConPrimera(fe.Statement, Tipo(variable.Type, fe.Type) + " " + Ident(variable.Name) + " = " + valor + ";");
            if (suelta)
            {
                imports.Add("CsSystem");
                sangria--;
                Linea("}");
                Linea("finally");
                Linea("{");
                Linea("   CsSystem.Foreach_Suelta(" + e + ");");
                Linea("}");
            }
        }

        /// El cuerpo de un bucle con una sentencia delante.
        void CuerpoConPrimera(StatementSyntax s, string primera)
        {
            if (s is BlockSyntax b) { Bloque(b, primera); return; }
            Linea("{");
            sangria++;
            Linea(primera);
            Sentencia(s);
            sangria--;
            Linea("}");
        }

        /// ¿Puede ser `IDisposable` lo que hay en una variable de este tipo? Él, o algo del programa que herede de él.
        bool PuedeSoltarse(ITypeSymbol t)
        {
            bool Disponible(ITypeSymbol x) => x.AllInterfaces.Any(i => i.SpecialType == SpecialType.System_IDisposable);
            if (Disponible(t)) return true;
            if (t.IsSealed) return false;
            return arbol.GetRoot().DescendantNodes().OfType<BaseTypeDeclarationSyntax>().Select(d => m.GetDeclaredSymbol(d)).OfType<INamedTypeSymbol>()
                .Any(c => Disponible(c) && (c.AllInterfaces.Contains(t, SymbolEqualityComparer.Default) || Hereda(c, t)));
        }

        static bool Hereda(ITypeSymbol c, ITypeSymbol padre)
        {
            for (var b = c.BaseType; b != null; b = b.BaseType)
                if (SymbolEqualityComparer.Default.Equals(b, padre)) return true;
            return false;
        }

        /// `x`, de tipo `de`, como un `a`, con lo que escribe una conversión de C# (la del elemento de un foreach).
        string Convierte(string x, ITypeSymbol de, ITypeSymbol a, SyntaxNode donde)
        {
            if (SymbolEqualityComparer.Default.Equals(de, a)) return x;
            if (!de.IsValueType && a.IsValueType && !EsEnum(a))
            {
                string caja = ClaseDeCaja(a);
                if (caja == null || de.SpecialType != SpecialType.System_Object)
                    return Rechaza(donde, Porque.Pendiente("sacar un " + NombreCompleto(a) + " de " + NombreCompleto(de)));
                return "((" + caja + ") " + x + ").value()";
            }
            if (!de.IsValueType && !a.IsValueType && a.SpecialType == SpecialType.System_Object) return x;   // subir: no se escribe
            return "(" + Tipo(a, donde) + ") " + x;
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
            EmiteConSacar(th, false, () => Linea("throw " + Expr(th.Expression) + ";"));
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
