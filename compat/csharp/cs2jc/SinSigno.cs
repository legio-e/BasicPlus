// uint en JC (V8/H6, 2-nov; docs/V8_IDEAS.md §E9): un uint es un long, con su valor (0 … 4 294 967 295). C# ya sube a
// long lo que mezcla un uint con un int; el traductor sólo pone la máscara `& 4294967295L` donde C# da la vuelta (+, -,
// *, << y ~ entre uint), y `& 31` a la cuenta de un desplazamiento (C# usa 5 bits en un tipo de 32; un long, 6).
// Comparar, /, % y >> salen bien sin nada: el valor nunca es negativo. `(int) u` se queda con los 32 bits de abajo, como
// C#. En una caja, un Long. decimal sigue fuera.
//
// ulong en JC (V8/H8, 5-nov; Eduardo: «No soportamos operaciones con ulong. Lo que podemos es permitir declarar ulong, leer
// y escribir haciendo conversiones de long <-> ulong»; «las 2 primeras filas»; docs/V8_IDEAS.md §E9): un ulong es un long con
// sus 64 bits tal cual (`ulong.MaxValue` es -1L), sólo en el traductor: JC no tiene ulong. Se declara, se pasa, se lee y se
// escribe; y lo que da los mismos bits en un long sale sin nada, también de 2^63 para arriba y dando la vuelta: +, -, *, &,
// |, ^, ~, <<, ++, --, == y != y un switch. Lo que no (comparar, /, %, >>, escribirlo, pasarlo a un real o a una caja) no se
// traduce: de 2^63 para arriba daría otro resultado, y en silencio.
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Cs2Jc
{
    sealed partial class Traductor
    {
        const string MascaraUInt = "4294967295L";

        static bool EsUInt(ITypeSymbol t) => t?.SpecialType == SpecialType.System_UInt32;

        static bool EsULong(ITypeSymbol t) => t?.SpecialType == SpecialType.System_UInt64;

        static bool EsReal(ITypeSymbol t) => t?.SpecialType == SpecialType.System_Single || t?.SpecialType == SpecialType.System_Double;

        /// Una operación entre ulong que en un long no da lo mismo: comparar, /, %, >>.
        static bool ULongNoSale(SyntaxKind k)
        {
            switch (k)
            {
                case SyntaxKind.LessThanExpression:
                case SyntaxKind.LessThanOrEqualExpression:
                case SyntaxKind.GreaterThanExpression:
                case SyntaxKind.GreaterThanOrEqualExpression:
                case SyntaxKind.DivideExpression:
                case SyntaxKind.ModuloExpression:
                case SyntaxKind.RightShiftExpression:
                case SyntaxKind.UnsignedRightShiftExpression:
                case SyntaxKind.DivideAssignmentExpression:
                case SyntaxKind.ModuloAssignmentExpression:
                case SyntaxKind.RightShiftAssignmentExpression: return true;
                default: return false;
            }
        }

        /// Una operación entre dos ulong (Binaria): si C# ya sabe lo que da, su valor; la que en un long no da lo mismo, no se
        /// traduce; null, las demás: como están.
        string BinariaULong(BinaryExpressionSyntax b)
        {
            if (!(m.GetSymbolInfo(b).Symbol is IMethodSymbol op) || op.Parameters.Length != 2 || !EsULong(op.Parameters[0].Type)) return null;
            var cv = m.GetConstantValue(b);
            if (cv.HasValue) return LiteralDeConstante(cv.Value, b);
            return ULongNoSale(b.Kind()) ? Rechaza(b, Porque.Fuera(Porque.ULong)) : null;
        }

        /// ¿Un ulong que aquí se convierte en un real sin escribirlo (`double d = u;`)? De 2^63 para arriba, el número sería
        /// otro. Una constante que cabe en un long, no.
        bool ULongAReal(ExpressionSyntax e)
        {
            var ti = m.GetTypeInfo(e);
            if (!EsULong(ti.Type) || !EsReal(ti.ConvertedType)) return false;
            return !(m.GetConstantValue(e).Value is ulong u && u <= long.MaxValue);
        }

        /// Lo que da una operación entre uint, recortado a sus 32 bits.
        static string RecortaUInt(string x) => "((" + x + ") & " + MascaraUInt + ")";

        /// La cuenta de un desplazamiento de un uint, como C#: sus 5 bits de abajo (una constante de 0 a 31, tal cual).
        string CuentaUInt(ExpressionSyntax n)
        {
            var cv = m.GetConstantValue(n);
            if (cv.HasValue && cv.Value is int k && k >= 0 && k <= 31) return Expr(n);
            return "(" + Expr(n) + " & 31)";
        }

        /// Una operación que da un uint (Binaria): +, - y *, recortadas; << con su cuenta, recortada; >> con su cuenta. Las
        /// demás (/, %, &, |, ^), null: como están.
        string BinariaUInt(BinaryExpressionSyntax b)
        {
            switch (b.Kind())
            {
                case SyntaxKind.AddExpression:
                case SyntaxKind.SubtractExpression:
                case SyntaxKind.MultiplyExpression:
                    return RecortaUInt(Expr(b.Left) + " " + b.OperatorToken.Text + " " + Expr(b.Right));
                case SyntaxKind.LeftShiftExpression: return RecortaUInt(Expr(b.Left) + " << " + CuentaUInt(b.Right));
                case SyntaxKind.RightShiftExpression: return Expr(b.Left) + " >> " + CuentaUInt(b.Right);
                default: return null;
            }
        }

        /// `u += y` (y `-=`, `*=`, `<<=`, `>>=`) con `u` un uint (ExprSentencia): `u = ((u + y) & 4294967295L)`; `u` va dos
        /// veces, así que tiene que no calcular nada. Las demás (/=, %=, &=, |=, ^=), null: como están.
        string AsignaUInt(AssignmentExpressionSyntax a)
        {
            string op;
            switch (a.Kind())
            {
                case SyntaxKind.AddAssignmentExpression: op = "+"; break;
                case SyntaxKind.SubtractAssignmentExpression: op = "-"; break;
                case SyntaxKind.MultiplyAssignmentExpression: op = "*"; break;
                case SyntaxKind.LeftShiftAssignmentExpression: op = "<<"; break;
                case SyntaxKind.RightShiftAssignmentExpression: return Expr(a.Left) + " >>= " + CuentaUInt(a.Right);
                default: return null;
            }
            if (!ObjetivoSimple(a.Left)) return Rechaza(a, Porque.Pendiente("una asignación compuesta de un uint a algo que calcula"));
            string x = Expr(a.Left);
            return x + " = " + RecortaUInt(x + " " + op + " " + (op == "<<" ? CuentaUInt(a.Right) : Expr(a.Right)));
        }

        /// `u++` y `u--` (y `++u`, `--u`) como sentencia: `u = ((u + 1) & 4294967295L)`.
        string IncUInt(ExpressionSyntax operando, bool mas, SyntaxNode donde)
        {
            if (!ObjetivoSimple(operando)) return Rechaza(donde, Porque.Pendiente("++ o -- de un uint en algo que calcula"));
            string x = Expr(operando);
            return x + " = " + RecortaUInt(x + (mas ? " + 1" : " - 1"));
        }

        /// `(uint) x`: de un entero con signo o de un real, sus 32 bits de abajo, en un long; de uno sin signo (byte,
        /// ushort, char) o de un enum (en JC, sin negativos), su número. null si no es de esos.
        string AUInt(CastExpressionSyntax c, ITypeSymbol de)
        {
            string x = Atomo(c.Expression);
            switch (de?.SpecialType)
            {
                case SpecialType.System_Int64:
                case SpecialType.System_UInt64: return RecortaUInt(x);
                case SpecialType.System_Int32:
                case SpecialType.System_Int16:
                case SpecialType.System_SByte:
                case SpecialType.System_Single:
                case SpecialType.System_Double: return RecortaUInt("(long) " + x);
                case SpecialType.System_Byte:
                case SpecialType.System_UInt16:
                case SpecialType.System_Char: return "(long) " + x;
            }
            return EsEnum(de) ? "(long) " + x : null;
        }

        /// `(byte) u` y los demás estrechos: sus bits de abajo, como C# (sin `checked`); el cast de JC a uno estrecho mira
        /// el rango, así que el valor le llega ya recortado.
        string DeUIntAEstrecho(CastExpressionSyntax c, ITypeSymbol a)
        {
            string x = Atomo(c.Expression);
            string t = Tipo(a, c.Type);
            switch (a.SpecialType)
            {
                case SpecialType.System_Byte: return "(" + t + ") ((int) (" + x + " & 255L))";
                case SpecialType.System_UInt16: return "(" + t + ") ((int) (" + x + " & 65535L))";
                case SpecialType.System_SByte: return "(" + t + ") ((((int) (" + x + " & 255L)) ^ 128) - 128)";
                case SpecialType.System_Int16: return "(" + t + ") ((((int) (" + x + " & 65535L)) ^ 32768) - 32768)";
            }
            return null;
        }
    }
}
