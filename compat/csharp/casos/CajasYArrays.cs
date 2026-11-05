// Caso del traductor C# -> JC (V8/H6): las cajas (en BP, a mano: `new Integer(5)` y `.value()`), crear arrays (con su
// tamaño, con `new int[n]` y los suyos, y con valores, `[1, 2]`), `Length` y `params`. Se comprueba solo: Main devuelve 0.
using System;

class Programa
{
    static int Suma(params int[] xs)
    {
        int s = 0;
        foreach (int x in xs) s += x;
        return s;
    }

    static int Cuenta(string que, params string[] cosas)
    {
        return cosas.Length;
    }

    static int Main()
    {
        object o = 5;
        if (!(o is int)) return 1;
        int n = (int) o;
        if (n != 5) return 2;
        object l = 7L;
        if ((long) l != 7L) return 3;
        object b = true;
        if (!(bool) b) return 4;
        object d = 2.5;
        if ((double) d != 2.5) return 5;
        Console.WriteLine("{0} {1} {2}", o, l, d);

        int[] a = new int[4];
        a[2] = 9;
        if (a.Length != 4 || a[2] != 9 || a[0] != 0) return 6;
        long[] g = { 1, 2, 3 };
        if (g.Length != 3 || g[2] != 3L) return 7;
        double[] r = new double[] { 1, 2.5 };
        if (r[0] != 1.0 || r[1] != 2.5) return 8;
        string[] s = { "uno", "dos" };
        if (s[1] != "dos" || s.Length != 2) return 9;
        var v = new[] { 10, 20, 30 };
        if (v[1] != 20) return 10;
        object[] cajas = new object[2];
        cajas[0] = "texto";
        cajas[1] = 3;
        if ((int) cajas[1] != 3) return 11;

        if (Suma() != 0) return 12;
        if (Suma(1, 2, 3) != 6) return 13;
        if (Suma(new int[] { 4, 5 }) != 9) return 14;
        if (Cuenta("x", "a", "b") != 2) return 15;
        if ("hola".Length != 4) return 16;
        return 0;
    }
}
