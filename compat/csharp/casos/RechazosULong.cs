// Lo de un ulong que no se traduce (H8, 5-nov): lo que en un long no sale igual de 2^63 para arriba. Todas se dicen, no sólo la primera.
using System;

class M
{
    static int Main ()
    {
        ulong a = 5, b = 3;
        bool x = a < b;
        x = a <= b;
        x = a > b;
        x = a >= b;
        ulong c = a / b;
        c = a % b;
        c = a >> 1;
        c /= 2;
        c %= 2;
        c >>= 1;
        double d = a;
        d = (double) a;
        float f = (float) a;
        a = (ulong) d;
        object o = a;
        Console.WriteLine (a);
        string s = "x" + a;
        s = a.ToString ();
        s = $"{a}";
        ulong m = Math.Max (a, b);
        ulong? n = null;
        x = n == null;
        return 0;
    }
}
