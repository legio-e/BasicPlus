// Caso del traductor C# -> JC (V8/H8, tema 1; 2-nov): los formatos que se saben pasar. El ancho (`{x,6}`, `{x,-6}`), el
// hexadecimal (`{x:x}`, `{x:X4}`; un negativo, en complemento a dos), un formato a medida de ceros y un punto
// (`{x:0.00}`), y en una cadena, que no tiene formatos, ninguno; en `$"…"`, en `string.Format` y en `Console.WriteLine`.
// Se comprueba solo: Main devuelve 0.
using System;

class Programa
{
    static int Main()
    {
        int n = 255;
        if ($"{n:x}" != "ff" || $"{n:X4}" != "00FF") return 1;
        int menos = -1;
        if ($"{menos:x}" != "ffffffff") return 2;
        long grande = -2;
        if ($"{grande:X}" != "FFFFFFFFFFFFFFFE") return 3;
        short corto = -1;
        if ($"{corto:x}" != "ffff") return 4;
        if ($"{1:0.00}" != "1.00" || $"{2.6:0}" != "3" || $"{-1.234:0.0}" != "-1.2" || $"{3.14159:00.000}" != "03.142") return 5;
        string s = "hola";
        if ($"[{s,6}]" != "[  hola]" || $"[{s,-6}]" != "[hola  ]") return 6;
        if ($"[{n,5}]" != "[  255]" || $"[{n,-5:x}]" != "[ff   ]") return 7;
        if ($@"{s:\u007B}" != "hola") return 8;          // una cadena no tiene formatos: .NET lo ignora
        if (string.Format("{0,4}|{1:x2}", 7, 10) != "   7|0a") return 9;
        Console.WriteLine("{0,-6}|{1,6}|", "ab", 12);
        return 0;
    }
}
