// Las expresiones y los tipos de C# en JC (V8/H6, docs/V8_IDEAS.md §E9). Los tipos de Roslyn dicen dónde va cada
// reescritura: un miembro de la clase lleva `this.` o el nombre de su clase delante; una caja, que en BP es a mano, no
// se tapa; lo que el traductor todavía no escribe (char, enum…) se rechaza por su tipo, antes de copiar nada.
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Operations;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        /// Una expresión en su sitio: con la conversión que C# le aplica ahí (una caja, una conversión propia…).
        string Expr(ExpressionSyntax e)
        {
            var conv = m.GetConversion(e);
            if (conv.IsBoxing) return Caja(e, m.GetTypeInfo(e).Type, m.GetTypeInfo(e).ConvertedType);
            if (conv.IsUnboxing) return Rechaza(e, Porque.Pendiente("sacar de una caja sin conversión"));
            if (conv.IsUserDefined) return Rechaza(e, Porque.H7(Porque.Operadores));
            if (conv.IsNullable) return ConversionNullable(e, m.GetTypeInfo(e).Type, m.GetTypeInfo(e).ConvertedType);   // un T?
            if (conv.IsEnumeration) return Rechaza(e, Porque.Fuera(Porque.EnteroAEnum));      // `Color c = 0;`
            if (conv.IsMethodGroup) return Rechaza(e, Porque.H7(Porque.Delegados));
            // una constante que cabe en un byte, un short…: JC (como Java) la acepta en una declaración o una asignación,
            // no en un argumento ni en un return; ahí, con su conversión escrita, `((byte) 5)` (H8, tema 1)
            if (conv.IsConstantExpression && EsEstrecho(m.GetTypeInfo(e).ConvertedType)
                && (e.Parent is ArgumentSyntax || e.Parent is ReturnStatementSyntax || e.Parent is ArrowExpressionClauseSyntax))
            {
                string v = ExprBase(e);
                return v == Hueco ? Hueco : "((" + Tipo(m.GetTypeInfo(e).ConvertedType, e) + ") " + v + ")";
            }
            return ExprBase(e);
        }

        static bool EsEstrecho(ITypeSymbol t)
        {
            switch (t?.SpecialType)
            {
                case SpecialType.System_SByte:
                case SpecialType.System_Byte:
                case SpecialType.System_Int16:
                case SpecialType.System_UInt16: return true;
                default: return false;
            }
        }

        static bool EsBool(ITypeSymbol t) => t != null && t.SpecialType == SpecialType.System_Boolean;

        static bool EsEnum(ITypeSymbol t) => t != null && t.TypeKind == TypeKind.Enum;

        /// La caja de BP de un valor (en BP las cajas son a mano: `Integer(5)`, y se sacan con `.value()`); null si no hay.
        static string ClaseDeCaja(ITypeSymbol t)
        {
            switch (t?.SpecialType)
            {
                case SpecialType.System_Int32: return "Integer";
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32: return "Long";             // un uint es un long (SinSigno.cs)
                case SpecialType.System_Double: return "Double";
                case SpecialType.System_Single: return "Float";
                case SpecialType.System_Boolean: return "Boolean";
                default: return null;
            }
        }

        /// `object o = 5;`: `Object o = new Integer(5);`. Sólo las de las cajas de BP, y sólo como `object`.
        string Caja(ExpressionSyntax e, ITypeSymbol de, ITypeSymbol a)
        {
            if (EsNullable(de))
            {
                // un T? como object: la misma referencia (su caja ya es un Object), si es la de su T (la de un byte?, un
                // Integer, no lo es)
                if (ClaseDeCaja(DeNullable(de)) == null) return Rechaza(e, PorTipo(de) ?? Porque.Pendiente("una caja de " + NombreCompleto(de)));
                if (a == null || a.SpecialType != SpecialType.System_Object) return Rechaza(e, Porque.Pendiente("una caja como " + NombreCompleto(a)));
                return ExprBase(e);
            }
            string clase = ClaseDeCaja(de);
            if (clase == null) return Rechaza(e, Porque.Pendiente("una caja de " + NombreCompleto(de)));
            if (a == null || a.SpecialType != SpecialType.System_Object) return Rechaza(e, Porque.Pendiente("una caja como " + NombreCompleto(a)));
            return "new " + clase + "(" + ExprBase(e) + ")";
        }

        static bool EsNumerico(ITypeSymbol t)
        {
            switch (t?.SpecialType)
            {
                case SpecialType.System_Int32:
                case SpecialType.System_Int64:
                case SpecialType.System_Int16:
                case SpecialType.System_Byte:
                case SpecialType.System_SByte:
                case SpecialType.System_UInt16:
                case SpecialType.System_UInt32:
                case SpecialType.System_Char:
                case SpecialType.System_Single:
                case SpecialType.System_Double: return true;
                default: return false;
            }
        }

        /// `new int[n]`: las funciones de Core que crean un array con su tamaño; null si no hay para ese tipo.
        static string CreadorDeArray(ITypeSymbol t)
        {
            switch (t?.SpecialType)
            {
                case SpecialType.System_Int32:
                case SpecialType.System_Char: return "newIntArray";        // un char es un entero
                case SpecialType.System_Byte: return "newByteArray";
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32: return "newLongArray";      // un uint es un long
                case SpecialType.System_Double: return "newDoubleArray";
                case SpecialType.System_Object: return "newObjArray";
                default: return null;
            }
        }

        string CreaArray(ArrayCreationExpressionSyntax ac)
        {
            if (!(m.GetTypeInfo(ac).Type is IArrayTypeSymbol at)) return Rechaza(ac, Porque.Pendiente("un array sin resolver"));
            string tipo = Tipo(at, ac);                                  // los de varias dimensiones, de H7
            if (tipo == Hueco) return Hueco;
            if (ac.Initializer != null) return ValoresDeArray(ac.Initializer.Expressions, at, ac);
            string fn = CreadorDeArray(at.ElementType);
            if (fn == null) return Rechaza(ac, Porque.H7(Porque.ArrayConTamano));
            // el tamaño, un entero: C# convierte uno long (`new char[len]`, test-138); el creador de Core pide un integer
            var tam = ac.Type.RankSpecifiers[0].Sizes[0];
            if (m.GetTypeInfo(tam).Type?.SpecialType == SpecialType.System_Int64 || EsUInt(m.GetTypeInfo(tam).Type))
                return fn + "((int) " + Atomo(tam) + ")";
            return fn + "(" + Expr(tam) + ")";
        }

        /// `{ 1, 2 }`: `[1, 2]`. Un array de valores de JC toma el tipo de sus valores: uno que no sea del tipo del array,
        /// si es un número, con su conversión (`(long) 1`); si no (una clase hija, cosas mezcladas), JC no lo escribe.
        string ValoresDeArray(IEnumerable<ExpressionSyntax> valores, IArrayTypeSymbol at, SyntaxNode donde)
        {
            string tipo = Tipo(at, donde);
            if (tipo == Hueco) return Hueco;
            var partes = new List<string>();
            var lista = valores.ToList();
            foreach (var x in lista)
            {
                var tx = m.GetTypeInfo(x).Type;
                if (SymbolEqualityComparer.Default.Equals(tx, at.ElementType)) partes.Add(Expr(x));
                else if (tx == null && !at.ElementType.IsValueType && !EsCadena(at.ElementType)) partes.Add(Expr(x));   // null
                else if (EsNullable(at.ElementType)) partes.Add(Expr(x));            // un T?: null, o el valor en su caja
                else if (EsNumerico(tx) && EsNumerico(at.ElementType)) partes.Add("(" + Tipo(at.ElementType, x) + ") " + Atomo(x));
                else return Rechaza(donde, Porque.H7(Porque.ArrayDeVarios));
            }
            // un null vale si otro valor le da al array su tipo: `[null]` solo, en JC, no tiene ninguno
            if (partes.Count > 0 && partes.All(x => x == "null")) return Rechaza(donde, Porque.H7(Porque.ArrayDeVarios));
            return "[" + string.Join(", ", partes) + "]";
        }

        /// El tipo que ve JC: el de C#, salvo tras una conversión hacia arriba, que se quita (Conversion): el de dentro.
        ITypeSymbol TipoEnJc(ExpressionSyntax e)
        {
            while (e is ParenthesizedExpressionSyntax p) e = p.Expression;
            if (e is CastExpressionSyntax c)
            {
                var de = m.GetTypeInfo(c.Expression).Type;
                var a = m.GetTypeInfo(c).Type;
                if (de != null && a != null && !de.IsValueType && !a.IsValueType && m.ClassifyConversion(c.Expression, a).IsImplicit
                    && !(RamaDeTernario(c) && a.TypeKind == TypeKind.Interface))   // #761: ésa se conserva (Conversion)
                    return TipoEnJc(c.Expression);
            }
            return m.GetTypeInfo(e).Type;
        }

        /// #761 — ¿es `e` (entre paréntesis o no) una de las dos ramas de un ternario `c ? a : b`?
        static bool RamaDeTernario(ExpressionSyntax e)
        {
            SyntaxNode n = e;
            while (n.Parent is ParenthesizedExpressionSyntax) n = n.Parent;
            return n.Parent is ConditionalExpressionSyntax t && (t.WhenTrue == n || t.WhenFalse == n);
        }

        /// Una expresión que va a llevar algo delante (`(int) e`): entre paréntesis si no es simple.
        string Atomo(ExpressionSyntax e)
        {
            string x = Expr(e);
            bool simple = e is IdentifierNameSyntax || e is MemberAccessExpressionSyntax || e is InvocationExpressionSyntax
                          || e is ElementAccessExpressionSyntax || e is ParenthesizedExpressionSyntax || e is LiteralExpressionSyntax
                          || e is ThisExpressionSyntax;
            return simple ? x : "(" + x + ")";
        }

        /// Una expresión como texto (`print`, la suma de cadenas, un formato): la de por sí (ExprBase), y un `char`, que
        /// en JC es su código, como su carácter: `Str.fromCharCode(c)`.
        string Texto(ExpressionSyntax e)
        {
            // `"x" + null`, `(string) null + null`, `(object) null + "x"`: en C#, el null se une como "". En JC, el null
            // escrito tal cual en una suma de cadenas es un error a propósito (#774), así que se escribe lo que C# dice
            if (EsNullEscrito(e)) return "\"\"";
            if (EsNullable(m.GetTypeInfo(e).Type))
            {
                // un T?: vacío si es null, como C#
                string x = ExprBase(e);
                if (x == Hueco) return Hueco;
                imports.Add("CsSystem");
                return "CsSystem.Nullable_Texto(" + x + ")";
            }
            if (m.GetTypeInfo(e).Type?.SpecialType == SpecialType.System_Char)
            {
                imports.Add("Str");
                return "Str.fromCharCode(" + ExprBase(e) + ")";
            }
            // #778 (Eduardo, 3-nov: «Para los Enum una función nueva, algo como Enum.getName(miEnum.enun4)») — un enum en
            // una cadena, su nombre, como en C# (`X.A + "a"` es "Aa"; en JC, sin más, su número): `Enum.getName(x)`, que el
            // compilador de JC resuelve con los nombres del enum. Uno [Flags] da en C# «A, B» con dos a la vez: no se traduce.
            var tipo = m.GetTypeInfo(e).Type;
            if (EsEnum(tipo))
            {
                if (tipo.GetAttributes().Any(a => a.AttributeClass?.Name == "FlagsAttribute"))
                    return Rechaza(e, Porque.Pendiente("el nombre de un enum [Flags] en una cadena"));
                return "Enum.getName(" + ExprBase(e) + ")";
            }
            return ExprBase(e);
        }

        /// ¿Es un null escrito? `null`, con casts o paréntesis delante (`(string) null`), o el `default` de una referencia.
        bool EsNullEscrito(ExpressionSyntax e)
        {
            while (e is ParenthesizedExpressionSyntax || e is CastExpressionSyntax)
                e = e is ParenthesizedExpressionSyntax p ? p.Expression : ((CastExpressionSyntax) e).Expression;
            if (e.IsKind(SyntaxKind.NullLiteralExpression)) return true;
            var t = m.GetTypeInfo(e).Type;
            return (e is DefaultExpressionSyntax || e.IsKind(SyntaxKind.DefaultLiteralExpression)) && t != null && !t.IsValueType;
        }

        /// Una expresión por sí misma, sin la conversión de su sitio: la usan `print` y la suma de cadenas, donde C# mete
        /// un número en una caja para unirlo y JC lo une sin ella.
        string ExprBase(ExpressionSyntax e)
        {
            string porTipo = PorTipo(m.GetTypeInfo(e).Type);
            if (porTipo != null) return Rechaza(e, porTipo);
            // una cuenta constante que da un uint (`(uint) int.MaxValue + 1`), su valor: lo calcula C#
            if (EsUInt(m.GetTypeInfo(e).Type) && (e is BinaryExpressionSyntax || e is PrefixUnaryExpressionSyntax || e is CastExpressionSyntax)
                && m.GetConstantValue(e) is var cu && cu.HasValue)
                return LiteralDeConstante(cu.Value, e);
            switch (e)
            {
                case LiteralExpressionSyntax l: return Literal(l);
                case IdentifierNameSyntax id: return Nombre(id);
                case MemberAccessExpressionSyntax ma when ma.IsKind(SyntaxKind.SimpleMemberAccessExpression): return Miembro(ma);
                case InvocationExpressionSyntax inv: return Llamada(inv);
                case ObjectCreationExpressionSyntax oc: return Nuevo(oc);
                case BinaryExpressionSyntax b: return Binaria(b);
                case PrefixUnaryExpressionSyntax pu: return Prefijo(pu);
                case PostfixUnaryExpressionSyntax po:
                    if (po.IsKind(SyntaxKind.SuppressNullableWarningExpression)) return Expr(po.Operand);
                    return Sacado(po);                                     // partido en dos, si se puede (Sacar.cs)
                case ParenthesizedExpressionSyntax p: return "(" + Expr(p.Expression) + ")";
                case CastExpressionSyntax c: return Conversion(c);
                case ConditionalExpressionSyntax c: return Expr(c.Condition) + " ? " + Expr(c.WhenTrue) + " : " + Expr(c.WhenFalse);
                case AssignmentExpressionSyntax a: return Sacado(a);              // partido en dos, si se puede (Sacar.cs)
                case ElementAccessExpressionSyntax ea: return Elemento(ea);
                case ThisExpressionSyntax _: return "this";
                case ArrayCreationExpressionSyntax ac: return CreaArray(ac);
                case ImplicitArrayCreationExpressionSyntax ia:
                    return m.GetTypeInfo(ia).Type is IArrayTypeSymbol iat ? ValoresDeArray(ia.Initializer.Expressions, iat, ia) : RechazaNodo(ia);
                case InitializerExpressionSyntax ie when ie.IsKind(SyntaxKind.ArrayInitializerExpression):
                    return TipoDelInicializador(ie) is IArrayTypeSymbol iet ? ValoresDeArray(ie.Expressions, iet, ie) : RechazaNodo(ie);
                case DefaultExpressionSyntax _: return PorDefecto(e);
                case InterpolatedStringExpressionSyntax ise: return Interpolada(ise);
                case CheckedExpressionSyntax ce when ce.IsKind(SyntaxKind.UncheckedExpression):
                    return "(" + Expr(ce.Expression) + ")";                    // unchecked: lo de siempre, en C# y en BP
                default: return RechazaNodo(e);
            }
        }

        /// `default(T)` y `default`: el valor que les da C#, escrito.
        string PorDefecto(ExpressionSyntax e)
        {
            var t = m.GetTypeInfo(e).ConvertedType ?? m.GetTypeInfo(e).Type;
            switch (t?.SpecialType)
            {
                case SpecialType.System_Int32:
                case SpecialType.System_Int16:
                case SpecialType.System_Byte:
                case SpecialType.System_SByte:
                case SpecialType.System_UInt16:
                case SpecialType.System_Char: return "0";
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32: return "0L";
                case SpecialType.System_Double: return "0.0";
                case SpecialType.System_Single: return "0.0f";
                case SpecialType.System_Boolean: return "false";
                case SpecialType.System_String: return "null";            // #774: una variable string de JC puede valer null
            }
            if (EsNullable(t)) return PorTipo(t) is string p ? Rechaza(e, p) : "null";
            if (t == null || t.IsValueType) return Rechaza(e, Porque.Pendiente("default de " + NombreCompleto(t)));
            return "null";
        }

        /// `$"a{x}b"`: `("a" + x + "b")`, la suma de C#; un formato (`{x:F2}`) o un ancho (`{x,5}`), no (H7).
        string Interpolada(InterpolatedStringExpressionSyntax ise)
        {
            var partes = new List<string>();
            foreach (var c in ise.Contents)
            {
                if (c is InterpolatedStringTextSyntax txt) partes.Add(Cadena(txt.TextToken.ValueText));
                else if (c is InterpolationSyntax it)
                {
                    if (it.AlignmentClause != null || it.FormatClause != null)
                    {
                        // `{x,5}` y `{x:X}` (H8, tema 1): con su ancho y su formato (PiezaConFormato); el ancho, una constante
                        int? ancho = null;
                        if (it.AlignmentClause != null)
                        {
                            var cv = m.GetConstantValue(it.AlignmentClause.Value);
                            if (!(cv.HasValue && cv.Value is int a)) return Rechaza(it, Porque.H7(Porque.Formatos));
                            ancho = a;
                        }
                        partes.Add("(" + PiezaConFormato(it.Expression, ancho, it.FormatClause?.FormatStringToken.ValueText, it) + ")");
                        continue;
                    }
                    string x = Texto(it.Expression);                       // sin la caja de C#, como en la suma
                    bool simple = it.Expression is IdentifierNameSyntax || it.Expression is MemberAccessExpressionSyntax
                                  || it.Expression is InvocationExpressionSyntax || it.Expression is LiteralExpressionSyntax
                                  || it.Expression is ElementAccessExpressionSyntax || it.Expression is ParenthesizedExpressionSyntax;
                    partes.Add(simple ? x : "(" + x + ")");
                }
            }
            if (partes.Count == 0) return "\"\"";
            if (!(ise.Contents[0] is InterpolatedStringTextSyntax)) partes.Insert(0, "\"\"");   // que sea una suma de cadenas
            return "(" + string.Join(" + ", partes) + ")";
        }

        /// El tipo de `{ 1, 2 }` en `int[] a = { 1, 2 };`: el de lo que se declara.
        ITypeSymbol TipoDelInicializador(InitializerExpressionSyntax ie)
        {
            var ti = m.GetTypeInfo(ie);
            if (ti.ConvertedType != null) return ti.ConvertedType;
            if (ti.Type != null) return ti.Type;
            if (ie.Parent is EqualsValueClauseSyntax ev && ev.Parent is VariableDeclaratorSyntax vd)
                return (m.GetDeclaredSymbol(vd) as ILocalSymbol)?.Type ?? (m.GetDeclaredSymbol(vd) as IFieldSymbol)?.Type;
            return null;
        }

        /// Una expresión como sentencia: una llamada, un `new`, una asignación o `x++` (`++x;` es `x++;`).
        string ExprSentencia(ExpressionSyntax e)
        {
            switch (e)
            {
                case AssignmentExpressionSyntax a:
                    if (a.IsKind(SyntaxKind.CoalesceAssignmentExpression)) return Rechaza(a, Porque.H7(Porque.NuloYPunto));
                    if (a.IsKind(SyntaxKind.UnsignedRightShiftAssignmentExpression)) return Rechaza(a, Porque.Pendiente(">>>="));
                    if (m.GetSymbolInfo(a.Left).Symbol is IEventSymbol) return Rechaza(a, Porque.H7(Porque.Delegados));
                    if (!a.IsKind(SyntaxKind.SimpleAssignmentExpression) && EsNullable(m.GetTypeInfo(a.Left).Type)) return AsignaNullable(a);
                    if (EsUInt(m.GetTypeInfo(a.Left).Type) && AsignaUInt(a) is string au) return au;    // `u += y`, recortado
                    if ((a.IsKind(SyntaxKind.AndAssignmentExpression) || a.IsKind(SyntaxKind.OrAssignmentExpression)
                         || a.IsKind(SyntaxKind.ExclusiveOrAssignmentExpression)) && EsBool(m.GetTypeInfo(a.Left).Type))
                        return Rechaza(a, Porque.H7(Porque.BoolBits));
                    if (a.Left is TupleExpressionSyntax || a.Left is DeclarationExpressionSyntax) return Rechaza(a, Porque.Pendiente("desempaquetado"));
                    if (!a.IsKind(SyntaxKind.SimpleAssignmentExpression) && EsEnum(m.GetTypeInfo(a.Left).Type))
                        return Rechaza(a, Porque.Fuera(Porque.EnteroAEnum));       // `e |= f`: da un enum desde su entero
                    // `s += 5`, con `s` una cadena: sin la caja de C#, como en la suma
                    bool unir = a.IsKind(SyntaxKind.AddAssignmentExpression) && EsCadena(m.GetTypeInfo(a.Left).Type);
                    return Expr(a.Left) + " " + a.OperatorToken.Text + " " + (unir ? ExprBase(a.Right) : Expr(a.Right));
                case PostfixUnaryExpressionSyntax p when p.IsKind(SyntaxKind.PostIncrementExpression) || p.IsKind(SyntaxKind.PostDecrementExpression):
                    if (EsEnum(m.GetTypeInfo(p.Operand).Type)) return Rechaza(p, Porque.Fuera(Porque.EnteroAEnum));
                    if (EsNullable(m.GetTypeInfo(p.Operand).Type)) return IncNullable(p.Operand, p.IsKind(SyntaxKind.PostIncrementExpression), p);
                    if (EsUInt(m.GetTypeInfo(p.Operand).Type)) return IncUInt(p.Operand, p.IsKind(SyntaxKind.PostIncrementExpression), p);
                    return Expr(p.Operand) + p.OperatorToken.Text;
                case PrefixUnaryExpressionSyntax p when p.IsKind(SyntaxKind.PreIncrementExpression) || p.IsKind(SyntaxKind.PreDecrementExpression):
                    if (EsEnum(m.GetTypeInfo(p.Operand).Type)) return Rechaza(p, Porque.Fuera(Porque.EnteroAEnum));
                    if (EsNullable(m.GetTypeInfo(p.Operand).Type)) return IncNullable(p.Operand, p.IsKind(SyntaxKind.PreIncrementExpression), p);
                    if (EsUInt(m.GetTypeInfo(p.Operand).Type)) return IncUInt(p.Operand, p.IsKind(SyntaxKind.PreIncrementExpression), p);
                    return Expr(p.Operand) + p.OperatorToken.Text;
                case InvocationExpressionSyntax _:
                case ObjectCreationExpressionSyntax _:
                    return Expr(e);
                default:
                    return RechazaNodo(e);
            }
        }

        /// Lo que el traductor no escribe, o JC no tiene, por el tipo de una expresión: el motivo, o null.
        static string PorTipo(ITypeSymbol t)
        {
            if (t == null) return null;
            switch (t.SpecialType)
            {
                case SpecialType.System_UInt64:
                case SpecialType.System_Decimal:
                case SpecialType.System_IntPtr:
                case SpecialType.System_UIntPtr: return Porque.Fuera(Porque.SinSigno);
            }
            if (t.TypeKind == TypeKind.Dynamic) return Porque.Fuera(Porque.Dynamic);
            if (t.TypeKind == TypeKind.Pointer || t.TypeKind == TypeKind.FunctionPointer) return Porque.Fuera(Porque.Unsafe);
            if (t.TypeKind == TypeKind.Delegate) return Porque.H7(Porque.Delegados);
            if (DeNullable(t) is ITypeSymbol u) return CajaDeNullable(t) != null ? null : PorNullable(u);   // un T?: su caja
            if (t is IArrayTypeSymbol at) return PorTipo(at.ElementType);
            return null;
        }

        static bool EsCadena(ITypeSymbol t) => t != null && t.SpecialType == SpecialType.System_String;

        string Literal(LiteralExpressionSyntax l)
        {
            switch (l.Kind())
            {
                case SyntaxKind.NumericLiteralExpression:
                {
                    var v = l.Token.Value;
                    if (v is uint u) return u.ToString(System.Globalization.CultureInfo.InvariantCulture) + "L";   // un uint es un long
                    if (v is ulong || v is decimal) return Rechaza(l, Porque.Fuera(Porque.SinSigno));
                    string txt = l.Token.Text;
                    if (GuionesQueJcNoLee(txt)) txt = txt.Replace("_", "");          // `1__000` es `1000` (H8, tema 1)
                    return txt.StartsWith(".") ? "0" + txt : txt;      // `.5`: el lexer de JC quiere la cifra delante
                }
                case SyntaxKind.StringLiteralExpression:
                    // `@"…"` y `"""…"""`, que JC no tiene, como una cadena normal; las demás, tal cual (JC lee los
                    // escapes de C#, punto 19)
                    if (l.Token.Text.StartsWith("@") || l.Token.Text.StartsWith("\"\"\"")) return Cadena((string) l.Token.Value);
                    return l.Token.Text;
                case SyntaxKind.CharacterLiteralExpression:
                {
                    // un carácter es un entero en JC, su código (17-oct; el escalón 1): `'a'` tal cual si se lee bien, y
                    // si no (una comilla, la barra, uno de control, de fuera de ASCII), el número
                    char ch = (char) l.Token.Value;
                    if (ch >= 32 && ch < 127 && ch != '\'' && ch != '\\') return "'" + ch + "'";
                    return ((int) ch).ToString(System.Globalization.CultureInfo.InvariantCulture);
                }
                case SyntaxKind.TrueLiteralExpression: return "true";
                case SyntaxKind.FalseLiteralExpression: return "false";
                case SyntaxKind.NullLiteralExpression: return "null";
                case SyntaxKind.DefaultLiteralExpression: return PorDefecto(l);
                default: return RechazaNodo(l);
            }
        }

        /// El lexer de JC lee un `_` entre dos cifras (`1_000`), no varios seguidos, ni tras `0x`, ni en el exponente.
        static bool GuionesQueJcNoLee(string t)
        {
            if (t.IndexOf('_') < 0) return false;
            string l = t.ToLowerInvariant();
            if (l.Contains("__") || l.StartsWith("0x_") || l.StartsWith("0b_")) return true;
            int e = l.StartsWith("0x") ? -1 : l.IndexOf('e');
            return e >= 0 && l.IndexOf('_', e) >= 0;
        }

        string Nombre(IdentifierNameSyntax id)
        {
            var s = m.GetSymbolInfo(id).Symbol;
            switch (s)
            {
                case ILocalSymbol _ when renombradas.TryGetValue(s, out var nuevo):
                    return nuevo;                                  // una local de un case subida antes del switch
                case ILocalSymbol _:
                case IParameterSymbol _:
                    return Ident(s.Name);
                case IFieldSymbol f:
                    if (!DelPrograma(f) && m.GetConstantValue(id) is var cv && cv.HasValue)
                        return LiteralDeConstante(cv.Value, id);      // `PI` con `using static System.Math`
                    if (!DelPrograma(f)) return Rechaza(id, Porque.DeNet(NombreCompleto(f.ContainingType) + "." + f.Name));
                    CompruebaAcceso(f, id);
                    return (f.IsStatic || f.IsConst ? Receptor(f.ContainingType, id) : "this") + "." + Ident(f.Name);
                case IPropertySymbol p:
                    if (!DelPrograma(p)) return Rechaza(id, Porque.DeNet(NombreCompleto(p.ContainingType) + "." + p.Name));
                    CompruebaAcceso(p, id);
                    return (p.IsStatic ? Receptor(p.ContainingType, id) : "this") + "." + Ident(p.Name);
                case INamedTypeSymbol nt:
                    return Tipo(nt, id);
                case IMethodSymbol _:
                case IEventSymbol _:
                    return Rechaza(id, Porque.H7(Porque.Delegados));
                default:
                    return Rechaza(id, Porque.Pendiente("el nombre " + id.Identifier.ValueText));
            }
        }

        string Miembro(MemberAccessExpressionSyntax ma)
        {
            var s = m.GetSymbolInfo(ma).Symbol;
            switch (s)
            {
                case null: return Rechaza(ma, Porque.Pendiente("el miembro " + ma.Name.Identifier.ValueText));
                case INamedTypeSymbol nt: return Tipo(nt, ma);            // N.A, o A.B: una anidada
                case INamespaceSymbol _: return Rechaza(ma, Porque.Pendiente("un namespace como valor"));
            }
            if (!DelPrograma(s)) return BibliotecaMiembro(ma, s);
            if (s is IMethodSymbol || s is IEventSymbol) return Rechaza(ma, Porque.H7(Porque.Delegados));
            if (s is IPropertySymbol p && p.IsIndexer) return Rechaza(ma, Porque.H7(Porque.Indexadores));
            CompruebaAcceso(s, ma);
            if (s.IsStatic || (s is IFieldSymbol f && f.IsConst)) return Receptor(s.ContainingType, ma) + "." + Ident(s.Name);
            if (ma.Expression is BaseExpressionSyntax)
            {
                return "base." + Ident(s.Name);
            }
            return Expr(ma.Expression) + "." + Ident(s.Name);
        }

        string Llamada(InvocationExpressionSyntax inv)
        {
            var ms = m.GetSymbolInfo(inv).Symbol as IMethodSymbol;
            if (ms == null)
            {
                var cv = m.GetConstantValue(inv);                        // nameof(x): su cadena
                if (cv.HasValue && cv.Value is string nom) return Cadena(nom);
                return Rechaza(inv, Porque.Pendiente("una llamada sin resolver"));
            }
            if (ms.MethodKind == MethodKind.DelegateInvoke) return Rechaza(inv, Porque.H7(Porque.Delegados));
            // la de un [Conditional] sin definir, como sentencia, la quita Sentencias; en otro sitio (un for), todavía no
            if (CondicionalSinDefinir(ms, inv) != null) return Rechaza(inv, Porque.Pendiente("una llamada que C# quita ([Conditional]) en este sitio"));
            if (ms.MethodKind == MethodKind.LocalFunction) return Rechaza(inv, Porque.Pendiente("funciones locales"));
            if (ms.IsGenericMethod) return Rechaza(inv, Porque.H7(Porque.Genericos));
            if (!DelPrograma(ms)) return BibliotecaLlamada(inv, ms);
            if (ms.ReducedFrom != null)
            {
                // `x.Doble(a)`, un método de extensión: la llamada estática que es, `Ext.Doble(x, a)` (en el mismo orden)
                if (!(inv.Expression is MemberAccessExpressionSyntax mx)) return Rechaza(inv, Porque.Pendiente("un método de extensión sin receptor"));
                string resto = Argumentos(inv.ArgumentList, ms);
                return Receptor(ms.ContainingType, inv) + "." + Ident(ms.Name) + "(" + Expr(mx.Expression)
                       + (resto.Length > 0 ? ", " + resto : "") + ")";
            }
            CompruebaAcceso(ms, inv);
            string args = Argumentos(inv.ArgumentList, ms);
            string nombre = NombreMetodo(ms);
            switch (inv.Expression)
            {
                case IdentifierNameSyntax _:
                    return (ms.IsStatic ? Receptor(ms.ContainingType, inv) : "this") + "." + nombre + "(" + args + ")";
                case MemberAccessExpressionSyntax ma:
                    if (ms.IsStatic) return Receptor(ms.ContainingType, inv) + "." + nombre + "(" + args + ")";
                    if (ma.Expression is BaseExpressionSyntax) return "base." + nombre + "(" + args + ")";
                    return Expr(ma.Expression) + "." + nombre + "(" + args + ")";
                default:
                    return Rechaza(inv, Porque.H7(Porque.Delegados));
            }
        }

        string Argumentos(BaseArgumentListSyntax args, IMethodSymbol ms)
        {
            if (args == null) return "";
            foreach (var a in args.Arguments)
                if (!a.RefKindKeyword.IsKind(SyntaxKind.None)) return Rechaza(a, Porque.H7(Porque.RefOut));
            if (args.Arguments.Any(a => a.NameColon != null)) return ConNombre(args, ms);
            if (Expandido(ms, args))
            {
                // `f(1, 2, 3)` con `params int[]`: el array, escrito, `f([1, 2, 3])` (C# lo crea igual, en orden)
                int fijos = ms.Parameters.Length - 1;
                if (args.Arguments.Count < fijos) return Rechaza(args, Porque.Pendiente("params sin todos los fijos"));
                var partes = args.Arguments.Take(fijos).Select(a => Expr(a.Expression)).ToList();
                partes.Add(ValoresDeArray(args.Arguments.Skip(fijos).Select(a => a.Expression).ToList(),
                                          (IArrayTypeSymbol) ms.Parameters.Last().Type, args));
                return string.Join(", ", partes);
            }
            return string.Join(", ", args.Arguments.Select(a => Expr(a.Expression)));
        }

        /// `f(b: 2, a: 1)`: en el orden de los parámetros, `f(1, 2)`. C# los calcula en el orden escrito: si cambia, sólo
        /// cuando ninguno calcula nada (la segunda regla de fondo). Uno que se salta, en medio, con su valor por defecto.
        string ConNombre(BaseArgumentListSyntax args, IMethodSymbol ms)
        {
            if (ms == null) return Rechaza(args, Porque.Pendiente("argumentos con nombre sin resolver"));
            var porParametro = new ExpressionSyntax[ms.Parameters.Length];
            for (int i = 0; i < args.Arguments.Count; i++)
            {
                var a = args.Arguments[i];
                int p = a.NameColon == null ? i
                      : ms.Parameters.IndexOf(ms.Parameters.FirstOrDefault(x => x.Name == a.NameColon.Name.Identifier.ValueText));
                if (p < 0 || p >= porParametro.Length || ms.Parameters[p].IsParams)
                    return Rechaza(a, Porque.Pendiente("un argumento con nombre de params"));
                porParametro[p] = a.Expression;
            }
            var escritos = args.Arguments.Select(a => a.Expression).ToList();
            if (!escritos.SequenceEqual(porParametro.Where(x => x != null)) && !escritos.All(EsSimple))
                return Rechaza(args, Porque.Pendiente("argumentos con nombre que cambian el orden de lo que se calcula"));
            int ultimo = Array.FindLastIndex(porParametro, x => x != null);
            var partes = new List<string>();
            for (int p = 0; p <= ultimo; p++)
                partes.Add(porParametro[p] != null ? ArgumentoDe(porParametro[p], ms.Parameters[p]) : ConstanteDe(ms.Parameters[p], args));
            return string.Join(", ", partes);
        }

        /// V8 #783 (Eduardo, 3-nov: «783 OK») — un argumento de una llamada con nombres, con el tipo de su parámetro
        /// cuando es otro número. C# elige la sobrecarga por los nombres; JC, por los tipos de lo que se le pasa en cada
        /// posición, la más específica (#772). Por posición, `d.Foo(2, 1)` con `Foo(int x, long y, string a = "a")` y
        /// `Foo(long y, int x)` es ambigua (cada una gana en un argumento); con el tipo, `d.Foo((long) 2, 1)`, es la de C#.
        string ArgumentoDe(ExpressionSyntax e, IParameterSymbol p)
        {
            var te = m.GetTypeInfo(e).Type;
            if (te != null && !SymbolEqualityComparer.Default.Equals(te, p.Type) && EsNumerico(te) && EsNumerico(p.Type))
                return "(" + Tipo(p.Type, e) + ") " + Atomo(e);
            return Expr(e);
        }

        /// ¿Una expresión que no calcula nada, y que por eso puede ir antes o después? Un literal, una constante, una
        /// local, un parámetro, un campo, `this`.
        bool EsSimple(ExpressionSyntax e)
        {
            switch (e)
            {
                case LiteralExpressionSyntax _:
                case ThisExpressionSyntax _: return true;
                case ParenthesizedExpressionSyntax p: return EsSimple(p.Expression);
                case PrefixUnaryExpressionSyntax u when u.IsKind(SyntaxKind.UnaryMinusExpression) || u.IsKind(SyntaxKind.LogicalNotExpression):
                    return EsSimple(u.Operand);
                case IdentifierNameSyntax id:
                {
                    var s = m.GetSymbolInfo(id).Symbol;
                    return s is ILocalSymbol || s is IParameterSymbol || s is IFieldSymbol;
                }
                case MemberAccessExpressionSyntax ma:
                {
                    var s = m.GetSymbolInfo(ma).Symbol;
                    return s is IFieldSymbol f && (f.IsStatic || f.IsConst || EsSimple(ma.Expression));
                }
                default: return m.GetConstantValue(e).HasValue;
            }
        }

        /// ¿Una llamada a un método `params` con los argumentos sueltos (o ninguno), en vez del array?
        bool Expandido(IMethodSymbol ms, BaseArgumentListSyntax args)
        {
            if (ms == null || ms.Parameters.Length == 0 || !ms.Parameters.Last().IsParams) return false;
            int n = ms.Parameters.Length;
            if (args.Arguments.Count != n) return true;
            var convertido = m.GetTypeInfo(args.Arguments[n - 1].Expression).ConvertedType;
            return !SymbolEqualityComparer.Default.Equals(convertido, ms.Parameters.Last().Type);
        }

        /// `new T(…)`, sin mirar su inicializador (`{ A = 1 }`): lo escribe la declaración, detrás (Sentencias.Declaracion).
        string NuevoSinInicializador(ObjectCreationExpressionSyntax oc) => Nuevo(oc, true);

        string Nuevo(ObjectCreationExpressionSyntax oc, bool sinInicializador = false)
        {
            var ctor = m.GetSymbolInfo(oc).Symbol as IMethodSymbol;
            var tipo = m.GetTypeInfo(oc).Type as INamedTypeSymbol;
            if (EsNullable(tipo))
            {
                // `new int?()` es null; `new int?(5)`, su caja
                if (PorTipo(tipo) is string p) return Rechaza(oc, p);
                if (oc.ArgumentList == null || oc.ArgumentList.Arguments.Count == 0) return "null";
                return MeteEnCaja(oc.ArgumentList.Arguments[0].Expression, tipo);
            }
            if (ctor == null || tipo == null) return Rechaza(oc, Porque.Pendiente("un new sin resolver"));
            // `new IFoo()`: una interfaz COM ([CoClass]); una interfaz no se construye
            if (tipo.TypeKind == TypeKind.Interface) return Rechaza(oc, Porque.Fuera(Porque.Interop));
            if (oc.Initializer != null && !sinInicializador) return Rechaza(oc.Initializer, Porque.Pendiente("inicializadores de objetos, fuera de una declaración"));
            if (!DelPrograma(tipo)) return BibliotecaNuevo(oc, ctor, tipo);
            CompruebaAcceso(ctor, oc);
            return "new " + Tipo(tipo, oc.Type) + "(" + Argumentos(oc.ArgumentList, ctor) + ")";
        }

        string Binaria(BinaryExpressionSyntax b)
        {
            // con un T?: sus funciones de CsSystem (Nullables.cs); `x ?? y`, el ternario
            if (b.IsKind(SyntaxKind.CoalesceExpression) && EsNullable(m.GetTypeInfo(b.Left).Type)) return CoalesceNullable(b);
            if (m.GetOperation(b) is IBinaryOperation bo && EsOperacionNullable(bo)) return BinariaNullable(b, bo);
            switch (b.Kind())
            {
                case SyntaxKind.AsExpression: return Como(b);
                case SyntaxKind.CoalesceExpression:
                    return RechazaNodo(b);
                case SyntaxKind.UnsignedRightShiftExpression:
                    return Rechaza(b, Porque.Pendiente(">>>"));
                case SyntaxKind.IsExpression:
                {
                    var t = m.GetTypeInfo(b.Right).Type;
                    // `o is int`: ¿lleva una caja de int? En BP, `o is Integer`
                    var izq = m.GetTypeInfo(b.Left).Type;
                    // `x is int`, con `x` un int?: si tiene valor
                    if (EsNullable(izq) && SymbolEqualityComparer.Default.Equals(DeNullable(izq), t)) return "(" + Expr(b.Left) + " != null)";
                    // `o is uint`: en su caja, un uint es un Long, como un long; no se distinguen
                    if (EsUInt(t)) return Rechaza(b, Porque.Pendiente("is uint (en una caja, un uint es un Long, como un long)"));
                    if (t != null && t.IsValueType && ClaseDeCaja(t) != null && izq != null && !izq.IsValueType)
                        return Expr(b.Left) + " is " + ClaseDeCaja(t);
                    if (t == null || t.IsValueType || EsCadena(t)
                        || (!DelPrograma(t) && ExcepcionJc(t) == null && !EsDeCsSystem(t) && t.SpecialType != SpecialType.System_Object))
                        return Rechaza(b, Porque.Pendiente("is " + NombreCompleto(t)));
                    return Expr(b.Left) + " is " + Tipo(t, b.Right);
                }
                case SyntaxKind.BitwiseAndExpression:
                case SyntaxKind.BitwiseOrExpression:
                case SyntaxKind.ExclusiveOrExpression:
                    if (EsBool(m.GetTypeInfo(b.Left).Type)) return Rechaza(b, Porque.H7(Porque.BoolBits));
                    break;
            }
            // Un enum en JC: `==` y `!=` tal cual, y en una cadena, su nombre (`Texto`: `Enum.getName`, #778). Lo que en C# opera con su entero, con
            // `(int)` delante (Eduardo, 31-oct: «(int)Enum.mivalor»): `a < b` es `(int) a < (int) b`, y `a - b`, dos enums,
            // `(int) a - (int) b`. Lo que da un enum desde un entero (`a | b`, `a + 1`) no: #522.
            bool enumIzq = EsEnum(m.GetTypeInfo(b.Left).Type), enumDer = EsEnum(m.GetTypeInfo(b.Right).Type);
            if ((enumIzq || enumDer) && !b.IsKind(SyntaxKind.EqualsExpression) && !b.IsKind(SyntaxKind.NotEqualsExpression)
                && !(b.IsKind(SyntaxKind.AddExpression) && EsCadena(m.GetTypeInfo(b).Type)))
            {
                if (EsEnum(m.GetTypeInfo(b).Type)) return Rechaza(b, Porque.Fuera(Porque.EnteroAEnum));
                string izq = enumIzq ? "(int) " + Atomo(b.Left) : Expr(b.Left);
                string der = enumDer ? "(int) " + Atomo(b.Right) : Expr(b.Right);
                return izq + " " + b.OperatorToken.Text + " " + der;
            }
            // `==` entre referencias: en JC, del mismo tipo (o con null); C# deja comparar una clase con su hija, con Object…
            if (b.IsKind(SyntaxKind.EqualsExpression) || b.IsKind(SyntaxKind.NotEqualsExpression))
            {
                var ti = TipoEnJc(b.Left);
                var td = TipoEnJc(b.Right);
                // H7.3 (#756): con una interfaz, JC compara con cualquier objeto (la identidad), como C#
                bool conInterfaz = ti?.TypeKind == TypeKind.Interface || td?.TypeKind == TypeKind.Interface;
                // una cadena con otra referencia (`o == "x"`, con o un object): en C#, la identidad; JC no las compara
                // (test-111). Dos cadenas, el texto, en los dos.
                bool unaCadena = EsCadena(ti) != EsCadena(td);
                if (ti != null && td != null && !ti.IsValueType && !td.IsValueType
                    && (unaCadena || (!EsCadena(ti) && !EsCadena(td) && !SymbolEqualityComparer.Default.Equals(ti, td) && !conInterfaz)))
                    return Rechaza(b, Porque.H7(Porque.IgualdadRefs));
            }
            // una cadena con lo que sea: C# mete lo que no es cadena en una caja para unirlo; JC lo une sin ella
            if (b.IsKind(SyntaxKind.AddExpression) && EsCadena(m.GetTypeInfo(b).Type))
                return Texto(b.Left) + " + " + Texto(b.Right);
            // lo que da un uint: recortado a sus 32 bits donde C# da la vuelta (SinSigno.cs)
            if (EsUInt(m.GetTypeInfo(b).Type) && BinariaUInt(b) is string u) return u;
            return Expr(b.Left) + " " + b.OperatorToken.Text + " " + Expr(b.Right);
        }

        /// `x as T` (H8, tema 1; JC, como Java, no tiene `as`): `(x is T ? (T) x : null)`; si `x` ya es un T (subir), `x`.
        /// `x` va dos veces, así que tiene que no calcular nada. Un T? de una caja de BP: `(x is Integer ? (Integer) x :
        /// null)`. Una cadena (en BP no es un objeto) o un tipo de .NET, todavía no.
        string Como(BinaryExpressionSyntax b)
        {
            var t = m.GetTypeInfo(b.Right).Type;
            var de = m.GetTypeInfo(b.Left).Type;
            if (t == null || de == null || de.IsValueType || EsCadena(t) || EsCadena(de) || !EsSimple(b.Left))
                return Rechaza(b, Porque.H7(Porque.As));
            string tipo;
            if (EsNullable(t))
            {
                if (ClaseDeCaja(DeNullable(t)) == null || EsUInt(DeNullable(t))) return Rechaza(b, Porque.H7(Porque.As));
                tipo = CajaDeNullable(t);
            }
            else if (t.SpecialType == SpecialType.System_Object || (t is INamedTypeSymbol nt && (DelPrograma(nt) || ExcepcionJc(nt) != null || EsDeCsSystem(nt))))
            {
                // subir: el mismo objeto (o null), como la conversión hacia arriba, que JC no escribe
                if (m.ClassifyConversion(b.Left, t).IsImplicit) return Expr(b.Left);
                tipo = Tipo(t, b.Right);
            }
            else return Rechaza(b, Porque.H7(Porque.As));
            if (tipo == Hueco) return Hueco;
            string x = Atomo(b.Left);
            return "(" + x + " is " + tipo + " ? (" + tipo + ") " + x + " : null)";
        }

        string Prefijo(PrefixUnaryExpressionSyntax pu)
        {
            if (m.GetOperation(pu) is IUnaryOperation uo && (uo.IsLifted || EsNullable(uo.Operand.Type))) return PrefijoNullable(pu, uo);
            if (EsEnum(m.GetTypeInfo(pu.Operand).Type)) return Rechaza(pu, Porque.Fuera(Porque.EnteroAEnum));   // `~e`: otro enum
            switch (pu.Kind())
            {
                case SyntaxKind.UnaryMinusExpression:
                {
                    // -2147483648 y -9223372036854775808: el literal, solo, no cabe (es uint, ulong); juntos, el mínimo
                    if (pu.Operand is LiteralExpressionSyntax lit)
                    {
                        if (lit.Token.Value is uint u && u == 2147483648u) return "(-2147483647 - 1)";
                        if (lit.Token.Value is ulong ul && ul == 9223372036854775808UL) return "(-9223372036854775807L - 1L)";
                    }
                    string x = Expr(pu.Operand);
                    return x.StartsWith("-") ? "- " + x : "-" + x;      // `- -x` no es `--x`
                }
                case SyntaxKind.LogicalNotExpression: return "!" + Expr(pu.Operand);
                case SyntaxKind.UnaryPlusExpression: return Expr(pu.Operand);    // +x es x
                case SyntaxKind.BitwiseNotExpression:
                    // JC no tiene el ~ de C# (BP no tiene el no de bits): ~x es x ^ -1, en complemento a dos lo mismo; el
                    // de un uint, con sus 32 bits (SinSigno.cs)
                    if (EsUInt(m.GetTypeInfo(pu).Type)) return "(" + Expr(pu.Operand) + " ^ " + MascaraUInt + ")";
                    return "(" + Expr(pu.Operand) + " ^ -1)";
                case SyntaxKind.PreIncrementExpression:
                case SyntaxKind.PreDecrementExpression:
                    return Sacado(pu);                                     // partido en dos, si se puede (Sacar.cs)
                default:
                    return RechazaNodo(pu);
            }
        }

        string Conversion(CastExpressionSyntax c)
        {
            var de = m.GetTypeInfo(c.Expression).Type;
            var a = m.GetTypeInfo(c).Type;
            if (a == null) return Rechaza(c, Porque.Pendiente("una conversión sin resolver"));
            if (c.Expression is LiteralExpressionSyntax nulo && nulo.IsKind(SyntaxKind.NullLiteralExpression))
            {
                // `(string) null` (#774: una variable string de JC puede valer null), `(object) null`, `(X[]) null`: null,
                // que en JC no se convierte (ni hace falta)
                // como argumento de un método con sobrecargas, el tipo del null elige cuál (`Foo((X[]) null)`): JC no lo
                // escribe, y sin él la llamada es ambigua
                if (c.Parent is ArgumentSyntax arg && arg.Parent?.Parent is ExpressionSyntax llamada
                    && m.GetSymbolInfo(llamada).Symbol is IMethodSymbol ms
                    && ms.ContainingType.GetMembers(ms.Name).OfType<IMethodSymbol>().Count(x => x.Parameters.Length == ms.Parameters.Length) > 1)
                    return Rechaza(c, Porque.H7(Porque.NullConTipo));
                if (!a.IsValueType) return "null";
            }
            if (EsNullable(a) || EsNullable(de)) return ConversionEscritaNullable(c, de, a);      // un T? (Nullables.cs)
            // a un uint, sus 32 bits de abajo; de un uint a uno estrecho, sus bits de abajo (SinSigno.cs)
            if (EsUInt(a) && de != null && de.IsValueType && !EsUInt(de))
            {
                var cu = m.GetConstantValue(c);
                if (cu.HasValue) return LiteralDeConstante(cu.Value, c);
                if (AUInt(c, de) is string au) return au;
            }
            if (EsUInt(de) && DeUIntAEstrecho(c, a) is string ae) return ae;
            if (EsEnum(a) && !SymbolEqualityComparer.Default.Equals(a, de)) return Rechaza(c, Porque.Fuera(Porque.EnteroAEnum));
            // `(int) Color.Rojo`, `(long) c`: se copia (Eduardo, 31-oct: «es el compilador el que ha de hacer la
            // conversión»); a un tipo estrecho, todavía no
            if (EsEnum(de) && !SymbolEqualityComparer.Default.Equals(a, de) && EsEstrecho(a))
                return Rechaza(c, Porque.Pendiente("un enum convertido a un tipo estrecho"));
            // `(I) y`, `(Animal) perro`: subir no se escribe en JC (sólo se baja); el objeto es el mismo y los métodos,
            // virtuales: se quita. Salvo una cadena como object: en C#, `(object) s == (object) t` compara referencias.
            if (de != null && !de.IsValueType && !a.IsValueType && !SymbolEqualityComparer.Default.Equals(a, de)
                && m.ClassifyConversion(c.Expression, a).IsImplicit)
            {
                if (EsCadena(de)) return Rechaza(c, Porque.Pendiente("una cadena como object (su referencia)"));
                // #761 — en una rama del ternario, la conversión a una interfaz da su tipo (en C#, el ternario tiene el de
                // la rama a la que se convierte la otra) y se conserva: JC la escribe, `(IA) x`. A una clase no se
                // puede: en JC, `Animal(perro)` sería construirla.
                if (RamaDeTernario(c) && a.TypeKind == TypeKind.Interface) return "(" + Tipo(a, c.Type) + ") " + Atomo(c.Expression);
                return Expr(c.Expression);
            }
            if (de != null)
            {
                if (m.GetConversion(c).IsUserDefined) return Rechaza(c, Porque.H7(Porque.Operadores));
                if (de.IsValueType && !a.IsValueType && !EsEnum(de)) return Caja(c.Expression, de, a);     // `(object) 5`
                if (!de.IsValueType && a.IsValueType && !EsEnum(a))
                {
                    // `(int) o`: sacarlo de su caja, `((Integer) o).value()`; si no es un int, la conversión lanza, como en C#
                    string clase = ClaseDeCaja(a);
                    if (clase == null || de.SpecialType != SpecialType.System_Object)
                        return Rechaza(c, Porque.Pendiente("sacar un " + NombreCompleto(a) + " de " + NombreCompleto(de)));
                    return "((" + clase + ") " + Atomo(c.Expression) + ").value()";
                }
            }
            // `(char) n`: los 16 bits de abajo, como C#; una constante, su valor (`(char) 65` es 'A')
            if (a.SpecialType == SpecialType.System_Char && de?.SpecialType != SpecialType.System_Char)
            {
                var cv = m.GetConstantValue(c);
                if (cv.HasValue) return LiteralDeConstante(cv.Value, c);
                return "(((int) " + Atomo(c.Expression) + ") & 65535)";
            }
            return "(" + Tipo(a, c.Type) + ") " + Expr(c.Expression);
        }

        string Elemento(ElementAccessExpressionSyntax ea)
        {
            var t = m.GetTypeInfo(ea.Expression).Type;
            if (t is IArrayTypeSymbol)
            {
                if (ea.ArgumentList.Arguments.Count != 1) return Rechaza(ea, Porque.H7(Porque.ArraysVarias));
                return Expr(ea.Expression) + "[" + Indice(ea.ArgumentList.Arguments[0].Expression) + "]";
            }
            // `s[i]`: su carácter, que en JC es su código (el escalón 1)
            if (EsCadena(t) && ea.ArgumentList.Arguments.Count == 1)
                return "charCodeAt(" + Expr(ea.Expression) + ", " + Indice(ea.ArgumentList.Arguments[0].Expression) + ")";
            if (m.GetSymbolInfo(ea).Symbol is IPropertySymbol p && p.IsIndexer)
                return Rechaza(ea, DelPrograma(p) ? Porque.H7(Porque.Indexadores) : Porque.DeNet(NombreCompleto(p.ContainingType) + "[i]"));
            return RechazaNodo(ea);
        }

        /// Un índice: un uint (en JC, un long), con `(int)` delante; JC indexa con un entero.
        string Indice(ExpressionSyntax i) => EsUInt(m.GetTypeInfo(i).Type) ? "(int) " + Atomo(i) : Expr(i);

        /// Al sacar una clase anidada al módulo, lo privado de la de fuera deja de verse: en C# sí se ve.
        void CompruebaAcceso(ISymbol s, SyntaxNode donde)
        {
            if (claseActual == null || s.ContainingType == null || !DelPrograma(s)) return;
            if (abiertos.Contains(s.OriginalDefinition)) return;          // en JC, public: lo usa una anidada
            var dueno = s.ContainingType.OriginalDefinition;
            var aqui = claseActual.OriginalDefinition;
            switch (s.DeclaredAccessibility)
            {
                case Accessibility.Private:
                    if (!SymbolEqualityComparer.Default.Equals(dueno, aqui))
                        Rechaza(donde, Porque.Pendiente("una clase anidada usa lo privado de la de fuera"));
                    break;
                case Accessibility.Protected:
                case Accessibility.ProtectedAndInternal:
                    if (!Hereda(aqui, dueno))
                        Rechaza(donde, Porque.Pendiente("una clase anidada usa lo protegido de la de fuera"));
                    break;
            }
        }

        static bool Hereda(INamedTypeSymbol hija, INamedTypeSymbol padre)
        {
            for (var b = hija; b != null; b = b.BaseType)
                if (SymbolEqualityComparer.Default.Equals(b.OriginalDefinition, padre)) return true;
            return false;
        }

        /// Un tipo de C# en JC.
        string Tipo(ITypeSymbol ty, SyntaxNode donde)
        {
            if (ty == null) return Rechaza(donde, Porque.Pendiente("un tipo sin resolver"));
            switch (ty.SpecialType)
            {
                case SpecialType.System_Int32: return "int";
                case SpecialType.System_Int64: return "long";
                case SpecialType.System_Int16: return "short";
                case SpecialType.System_Byte: return "byte";
                case SpecialType.System_SByte: return "sbyte";
                case SpecialType.System_UInt16: return "ushort";
                case SpecialType.System_UInt32: return "long";         // un uint es un long (SinSigno.cs)
                case SpecialType.System_Char: return "int";            // en JC, char es un entero (Eduardo, 1-nov)
                case SpecialType.System_Single: return "float";
                case SpecialType.System_Double: return "double";
                case SpecialType.System_Boolean: return "bool";
                case SpecialType.System_String: return "string";
                case SpecialType.System_Object: return "Object";
                case SpecialType.System_Void: return "void";
            }
            string p = PorTipo(ty);
            if (p != null) return Rechaza(donde, p);
            if (CajaDeNullable(ty) is string caja) return caja;                // un T?, su caja (Nullables.cs)
            if (ty is IArrayTypeSymbol at)
            {
                if (at.Rank > 1) return Rechaza(donde, Porque.H7(Porque.ArraysVarias));
                if (at.ElementType is IArrayTypeSymbol) return Rechaza(donde, Porque.H7(Porque.ArraysDeArrays));
                return Tipo(at.ElementType, donde) + "[]";
            }
            if (ty is ITypeParameterSymbol) return Rechaza(donde, Porque.H7(Porque.Genericos));
            if (ty is INamedTypeSymbol nt)
            {
                if (nt.IsGenericType) return Rechaza(donde, Porque.H7(Porque.Genericos));
                if (DelPrograma(nt))
                {
                    switch (nt.TypeKind)
                    {
                        case TypeKind.Class:
                        case TypeKind.Interface:                       // H7.3 (#756): la interfaz de JC
                        case TypeKind.Enum: return NombreTipo(nt);
                        case TypeKind.Struct: return Rechaza(donde, Porque.H7(Porque.Struct));
                        default: return Rechaza(donde, Porque.Pendiente("tipo " + nt.TypeKind));
                    }
                }
                if (ExcepcionJc(nt) is string ex) return ex;              // Exception, RuntimeError o la de CsSystem
                if (InterfazDeCsSystem(nt) is string ci) return ci;       // IDisposable (H8, tema 2)
                if (nt.TypeKind == TypeKind.Interface) return Rechaza(donde, PorInterfazAjena(nt));
                return Rechaza(donde, Porque.DeNet("tipo " + NombreCompleto(nt)));
            }
            return Rechaza(donde, Porque.Pendiente("tipo " + NombreCompleto(ty)));
        }
    }
}
