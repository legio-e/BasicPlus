// `&`, `|` y `^` entre bool (H8, tema 2): calculan los dos lados, y en JC son los de Java. Se comprueba solo.
using System;

class Prueba
{
    static int llamadas = 0;

    static bool G (bool v)
    {
        llamadas++;
        return v;
    }

    public static int Main ()
    {
        bool f = false, t = true;
        if (f & G (true) || llamadas != 1)
            return 1;
        if (!(t | G (false)) || llamadas != 2)
            return 2;
        if (!(t ^ f) || (t ^ t) || (f ^ f))
            return 3;

        bool c = t;
        c &= f;
        if (c)
            return 4;
        c |= t;
        if (!c)
            return 5;
        c ^= t;
        if (c)
            return 6;

        // con comparaciones, en C# el & va detrás del ==
        int a = 1, b = 2;
        if (!(a == 1 & b == 2))
            return 7;
        return 0;
    }
}
