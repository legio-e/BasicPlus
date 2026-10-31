// Lo que no se traduce, y por qué (V8/H6, docs/V8_IDEAS.md §E9). Cuatro motivos:
//   - le falta a JC: una mejora de H7 (los nombres, los de #745, para que las cuentas casen con las de entonces);
//   - fuera: lo que Eduardo deja fuera (las lambdas, la herencia múltiple) o lo que la VM no tiene;
//   - el traductor todavía no: una regla que falta (no es culpa de JC);
//   - la biblioteca: un miembro de .NET que no está en la tabla (Biblioteca.cs);
//   - un fallo de JC, con su ficha: la traducción es buena y JC la haría mal (se quita al cerrar la ficha).
using System.Collections.Generic;
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
        public const string EnumEntero = "enum <-> int";
        // las que salieron al traducir (H6, 31-oct)
        public const string BoolBits = "& | ^ sobre bool (sin atajo)";
        public const string ColorColor = "un nombre que es a la vez variable y tipo (el Color Color de C#)";
        public const string NullCadena = "null en una cadena (en BP un string no es nulo)";
        public const string ConstanteEstrecha = "una constante que cabe, en un argumento o un return estrecho (como en una asignación)";
        public const string Guiones = "los _ de un número, como en C# (varios seguidos, o en el exponente)";

        // Lo que queda fuera
        public const string Lambdas = "lambdas y métodos anónimos";
        public const string HerenciaMultiple = "interfaces (herencia múltiple)";
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
