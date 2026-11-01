// Caso del traductor C# -> JC (V8/H6, el escalón 1): las constantes de .NET (su valor, escrito: int.MaxValue, long.MinValue,
// ushort.MaxValue, Math.PI, double.NaN y los infinitos, string.Empty, `PI` con `using static`) y los métodos estáticos de
// la librería CsSystem (Math, double.IsNaN, int.Parse con su FormatException). Se comprueba solo: Main devuelve 0.
using System;
using static System.Math;

class Programa
{
    static int Main()
    {
        if (int.MaxValue != 2147483647 || int.MinValue + 1 != -2147483647) return 1;
        long l = long.MinValue;
        if (l + 1 != -9223372036854775807L) return 2;
        if (ushort.MaxValue != 65535 || short.MinValue != -32768) return 3;
        if (Math.PI < 3.14159 || PI > 3.1416) return 4;
        double nan = double.NaN;
        if (!double.IsNaN(nan) || double.IsNaN(1.5)) return 5;
        if (!(double.PositiveInfinity > double.MaxValue) || !(double.NegativeInfinity < double.MinValue)) return 6;
        if (string.Empty.Length != 0) return 7;
        if (Math.Abs(-5) != 5 || Math.Abs(-2.5) != 2.5 || Math.Max(3, 9) != 9 || Math.Min(-1L, 4L) != -1L) return 8;
        if (Math.Round(2.5) != 2.0 || Math.Round(3.5) != 4.0 || Math.Round(-2.5) != -2.0 || Math.Round(2.6) != 3.0) return 9;
        if (Math.Floor(-1.5) != -2.0 || Math.Ceiling(1.2) != 2.0) return 10;
        if (!double.IsNaN(Math.Max(1.0, nan))) return 11;
        if (int.Parse("-42") != -42 || long.Parse("9000000000") != 9000000000L) return 12;
        try
        {
            int.Parse("4x");
            return 13;
        }
        catch (FormatException)
        {
        }
        Console.WriteLine("numeros bien");
        return 0;
    }
}
