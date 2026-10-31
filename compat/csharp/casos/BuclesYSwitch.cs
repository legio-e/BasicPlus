// Caso del traductor C# -> JC (V8/H6): los `for` que no son el de JC (como `while`, con los incrementos también delante
// de cada `continue` suyo), el `for` con `<` y un límite constante (`i <= 9`), y el `switch` (varias etiquetas en un
// `case`, el `default` en medio, un `case` calculado, sobre un enum y sobre una cadena). Se comprueba solo.
using System;

enum Fruta { Pera, Manzana, Uva }

class Programa
{
    const int Tope = 3;

    static string Nombre(Fruta f)
    {
        switch (f)
        {
            case Fruta.Pera: return "pera";
            default: return "otra";
            case Fruta.Uva: return "uva";
        }
    }

    static int Codigo(string s)
    {
        int r = 0;
        switch (s)
        {
            case "a":
            case "b":
                r = 1;
                break;
            case "c":
                r = 2;
                if (s.Length == 1) break;
                r = 99;
                break;
            case "d": { r = 4; break; }
        }
        return r;
    }

    static int Main()
    {
        int n = 0;
        for (int i = 0; i < 10; i++) n += i;                     // el de JC: i <= 9
        if (n != 45) return 1;

        int pares = 0;
        for (int i = 0, j = 10; i < j; i += 2, j--)             // dos variables: como while
        {
            if (i == 4) continue;                                // el continue hace antes los incrementos
            pares++;
        }
        if (pares != 3) return 2;

        int k = 0;
        for (; ; )                                               // sin nada: while (true)
        {
            k++;
            if (k == 5) break;
        }
        if (k != 5) return 3;

        int m = 0;
        for (int i = 1; i < Tope * 4; i *= 2) m += i;            // el paso no es el de JC: 1 + 2 + 4 + 8
        if (m != 15) return 4;

        if (Nombre(Fruta.Pera) != "pera" || Nombre(Fruta.Manzana) != "otra" || Nombre(Fruta.Uva) != "uva") return 5;
        if (Codigo("b") != 1 || Codigo("c") != 2 || Codigo("d") != 4 || Codigo("z") != 0) return 6;

        int t = 0;
        switch (k)
        {
            case Tope + 2: t = 1; break;                         // un case calculado: case 5
            case 1: t = 2; break;
        }
        if (t != 1) return 7;
        Console.WriteLine("bucles y switch, bien");
        return 0;
    }
}
