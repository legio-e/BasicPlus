// Lo que no se traduce, y por qué (V8/H6, docs/V8_IDEAS.md §E9). Cuatro motivos:
//   - le falta a JC: una mejora de H7 (los nombres, los de #745, para que las cuentas casen con las de entonces);
//   - fuera: lo que Eduardo deja fuera (las lambdas, la herencia múltiple) o lo que la VM no tiene;
//   - el traductor todavía no: una regla que falta (no es culpa de JC);
//   - la biblioteca: un miembro de .NET que no está en la tabla (Biblioteca.cs);
//   - un fallo de JC, con su ficha: la traducción es buena y JC la haría mal (se quita al cerrar la ficha).
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;

namespace Cs2Jc
{
    sealed class Rechazo
    {
        public int Linea;
        public string Que;      // el trozo de C#, corto y en una línea
        public string Porque;
    }

    static class Porque
    {
        public static string H7(string mejora) => "le falta a JC: " + mejora;
        public static string Fuera(string que) => "fuera: " + que;
        public static string Pendiente(string regla) => "el traductor todavía no: " + regla;
        public static string Biblioteca(string miembro) => "la biblioteca: " + miembro;

        /// Un miembro o un tipo de .NET que no está en la tabla: su motivo, como en el censo de #745 (clasifica.py): la
        /// reflexión, los hilos, las fechas… quedan fuera; los genéricos y los delegados, de H7; lo demás, la biblioteca.
        public static string DeNet(string nombre)
        {
            string t = nombre.StartsWith("tipo ") ? nombre.Substring(5)
                     : nombre.StartsWith("new ") ? nombre.Substring(4)
                     : nombre.StartsWith("hereda de ") ? nombre.Substring(10)
                     : nombre.StartsWith("implementa ") ? nombre.Substring(11)
                     : nombre.StartsWith("catch (") ? nombre.Substring(7).TrimEnd(')') : nombre;
            bool Empieza(params string[] p) => p.Any(x => t.StartsWith(x));
            if (Empieza("System.Reflection", "System.Type", "System.Activator", "System.Attribute", "System.RuntimeTypeHandle",
                        "System.RuntimeFieldHandle", "System.RuntimeMethodHandle", "System.Object.GetType", "System.Enum.",
                        "System.Enum", "System.AttributeUsage", "System.AttributeTargets", "System.ObsoleteAttribute",
                        "System.Diagnostics.ConditionalAttribute", "System.FlagsAttribute", "System.CLSCompliant",
                        "System.Runtime.CompilerServices", "System.TypedReference", "System.Diagnostics.FileVersionInfo",
                        "System.Version"))
                return Fuera(Reflexion);
            if (Empieza("System.Runtime.InteropServices", "System.IntPtr", "System.UIntPtr", "System.Marshal")) return Fuera(Interop);
            if (Empieza("System.Collections.Generic", "System.Func", "System.Action", "System.Predicate", "System.Comparison",
                        "System.Nullable", "System.Lazy", "System.Tuple", "System.ValueTuple", "System.IEquatable", "System.IComparable<"))
                return H7(Genericos);
            if (Empieza("System.Delegate", "System.MulticastDelegate", "System.EventHandler", "System.AsyncCallback", "System.IAsyncResult"))
                return H7(Delegados);
            if (Empieza("System.Threading.Tasks", "System.Runtime.CompilerServices.TaskAwaiter")) return Fuera(Async);
            if (Empieza("System.Threading", "System.AggregateException")) return Fuera("los hilos de .NET");
            if (Empieza("System.DateTime", "System.TimeSpan", "System.DayOfWeek", "System.Diagnostics.Stopwatch")) return Fuera("las fechas y horas de .NET");
            if (Empieza("System.UInt32", "System.UInt64", "System.Decimal")) return Fuera(SinSigno);
            if (Empieza("Microsoft.CSharp.RuntimeBinder", "System.Dynamic")) return Fuera(Dynamic);
            if (Empieza("System.Linq")) return Fuera(Linq);
            if (Empieza("System.Collections.IEnumerable", "System.Collections.IEnumerator")) return H7(ForeachPropio);
            if (Empieza("System.NullReferenceException", "System.IndexOutOfRangeException", "System.DivideByZeroException",
                        "System.InvalidCastException", "System.OverflowException", "System.ArithmeticException",
                        "System.StackOverflowException", "System.OutOfMemoryException", "System.ArrayTypeMismatchException"))
                return H7(ErroresVm);
            return Biblioteca(nombre);
        }
        public static string Fallo(string ficha, string que) => "un fallo de JC: " + ficha + ", " + que;

        // Los fallos de JC que salen al traducir (FICHAS.md)
        public const string SuperProp = "una propiedad por base va a la de la hija";
        public const string ClaseDespues = "construir (o is, o convertir a, o atrapar) una clase declarada más abajo";
        public const string SuperObject = "super() en una clase sin padre escrito (el suyo es Object)";

        // Las mejoras de H7 (#745)
        public const string Genericos = "genéricos";
        public const string Struct = "struct (tipos por valor)";
        public const string RefOut = "ref / out";
        public const string Delegados = "delegados y eventos de C# (sin lambdas)";
        public const string As = "as";
        public const string NuloYPunto = "?? y ?. (con referencias)";
        public const string Indexadores = "indexadores this[i]";
        public const string Operadores = "sobrecarga de operadores";
        public const string ArraysVarias = "arrays de varias dimensiones";
        public const string ArraysDeArrays = "arrays de arrays";
        public const string Patrones = "patrones (is T x, switch con patrones)";
        public const string CtorEstatico = "constructor estático";
        public const string NoVirtuales = "métodos no virtuales (new)";
        public const string Formatos = "formatos {0:X} y {0,5}";
        public const string ThrowExpresion = "throw como expresión / filtro en catch";
        public const string IncEnExpresion = "++, -- y = dentro de una expresión";
        public const string ErroresVm = "errores de la VM con su tipo";
        public const string ForeachPropio = "foreach sobre colecciones propias";
        // las que salieron al traducir (H6, 31-oct)
        public const string BoolBits = "& | ^ sobre bool (sin atajo)";
        public const string ColorColor = "un nombre que es a la vez variable y tipo (el Color Color de C#)";
        public const string NullCadena = "null en una cadena (en BP un string no es nulo)";
        public const string ConstanteEstrecha = "una constante que cabe, en un argumento o un return estrecho (como en una asignación)";
        public const string Guiones = "los _ de un número, como en C# (varios seguidos, o en el exponente)";
        public const string EnumNegativo = "un enum con valores negativos (o que no caben en un int)";
        public const string EnumPorDefecto = "un enum como valor por defecto de un parámetro (JC sólo admite un literal)";
        public const string NullPorDefecto = "null como valor por defecto de un parámetro (JC sólo admite un literal)";
        public const string NullConTipo = "un null con su tipo, (X) null, para elegir una sobrecarga";
        public const string IgualdadRefs = "== y != entre referencias de dos tipos distintos (una clase y su hija, Object)";
        public const string ExplicitaDoble = "la implementación explícita de verdad: dos interfaces con el mismo método, cada una con su cuerpo";
        public const string ArrayDeVarios = "un array de valores de otro tipo que el suyo (una clase hija, o mezclados)";
        public const string ArrayConTamano = "crear un array de un tipo con su tamaño (Core: newIntArray, newByteArray, newLongArray, newDoubleArray y newObjArray)";

        // Lo que queda fuera
        public const string Lambdas = "lambdas y métodos anónimos";
        public const string Linq = "LINQ";
        public const string Yield = "iteradores (yield)";
        public const string Async = "async/await";
        public const string Unsafe = "unsafe y punteros";
        public const string SinSigno = "sin signo y decimal";
        public const string Nullable = "nullable (int?)";
        public const string Checked = "checked";
        public const string Goto = "goto";
        public const string Reflexion = "reflexión (typeof, GetType, atributos)";
        public const string Anonimos = "tipos anónimos";
        public const string Interop = "interop y extern";
        public const string Dynamic = "dynamic";
        public const string EnteroAEnum = "un entero que se convierte en un enum (JC no lo deja, a propósito: #522)";

        /// Los nodos que el traductor no traduce y de los que se sabe por qué (#745, clasifica.py).
        public static readonly Dictionary<SyntaxKind, string> PorNodo = new Dictionary<SyntaxKind, string>
        {
            [SyntaxKind.AsExpression] = H7(As),
            [SyntaxKind.CoalesceExpression] = H7(NuloYPunto),
            [SyntaxKind.ConditionalAccessExpression] = H7(NuloYPunto),
            [SyntaxKind.MemberBindingExpression] = H7(NuloYPunto),
            [SyntaxKind.ElementBindingExpression] = H7(NuloYPunto),
            [SyntaxKind.CoalesceAssignmentExpression] = H7(NuloYPunto),
            [SyntaxKind.IndexerDeclaration] = H7(Indexadores),
            [SyntaxKind.OperatorDeclaration] = H7(Operadores),
            [SyntaxKind.ConversionOperatorDeclaration] = H7(Operadores),
            [SyntaxKind.GenericName] = H7(Genericos),
            [SyntaxKind.TypeParameterList] = H7(Genericos),
            [SyntaxKind.StructDeclaration] = H7(Struct),
            [SyntaxKind.RecordStructDeclaration] = H7(Struct),
            [SyntaxKind.DeclarationExpression] = H7(RefOut),
            [SyntaxKind.RefExpression] = H7(RefOut),
            [SyntaxKind.RefType] = H7(RefOut),
            [SyntaxKind.RefTypeExpression] = H7(RefOut),
            [SyntaxKind.MakeRefExpression] = H7(RefOut),
            [SyntaxKind.RefValueExpression] = H7(RefOut),
            [SyntaxKind.IsPatternExpression] = H7(Patrones),
            [SyntaxKind.SwitchExpression] = H7(Patrones),
            [SyntaxKind.CasePatternSwitchLabel] = H7(Patrones),
            [SyntaxKind.ThrowExpression] = H7(ThrowExpresion),
            [SyntaxKind.CatchFilterClause] = H7(ThrowExpresion),
            [SyntaxKind.DelegateDeclaration] = H7(Delegados),
            [SyntaxKind.EventFieldDeclaration] = H7(Delegados),
            [SyntaxKind.EventDeclaration] = H7(Delegados),
            [SyntaxKind.AnonymousMethodExpression] = Fuera(Lambdas),
            [SyntaxKind.ParenthesizedLambdaExpression] = Fuera(Lambdas),
            [SyntaxKind.SimpleLambdaExpression] = Fuera(Lambdas),
            [SyntaxKind.QueryExpression] = Fuera(Linq),
            [SyntaxKind.YieldReturnStatement] = Fuera(Yield),
            [SyntaxKind.YieldBreakStatement] = Fuera(Yield),
            [SyntaxKind.AwaitExpression] = Fuera(Async),
            [SyntaxKind.PointerType] = Fuera(Unsafe),
            [SyntaxKind.AddressOfExpression] = Fuera(Unsafe),
            [SyntaxKind.PointerIndirectionExpression] = Fuera(Unsafe),
            [SyntaxKind.PointerMemberAccessExpression] = Fuera(Unsafe),
            [SyntaxKind.FixedStatement] = Fuera(Unsafe),
            [SyntaxKind.UnsafeStatement] = Fuera(Unsafe),
            [SyntaxKind.StackAllocArrayCreationExpression] = Fuera(Unsafe),
            [SyntaxKind.SizeOfExpression] = Fuera(Unsafe),
            [SyntaxKind.FunctionPointerType] = Fuera(Unsafe),
            [SyntaxKind.NullableType] = Fuera(Nullable),
            [SyntaxKind.CheckedExpression] = Fuera(Checked),
            [SyntaxKind.CheckedStatement] = Fuera(Checked),
            [SyntaxKind.GotoStatement] = Fuera(Goto),
            [SyntaxKind.GotoCaseStatement] = Fuera(Goto),
            [SyntaxKind.GotoDefaultStatement] = Fuera(Goto),
            [SyntaxKind.LabeledStatement] = Fuera(Goto),
            [SyntaxKind.TypeOfExpression] = Fuera(Reflexion),
            [SyntaxKind.AnonymousObjectCreationExpression] = Fuera(Anonimos),
            [SyntaxKind.ArgListExpression] = Fuera(Interop),
        };
    }
}
