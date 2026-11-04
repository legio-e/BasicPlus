// `a ?? b` y `x ??= y` con referencias (H8, tema 2): la derecha, sólo si hace falta; lo de la izquierda, una vez. Se
// comprueba solo. Lo que no se puede adelantar sin cambiar el orden, en el caso RechazosJc.
using System;

class A
{
}

class B : A
{
}

interface I
{
    int V ();
}

class C : I
{
    public int V ()
    {
        return 1;
    }
}

class Prueba
{
    static int llamadas = 0;
    static string campo;

    static A Cuenta (A x)
    {
        llamadas++;
        return x;
    }

    static string Texto (string s)
    {
        llamadas++;
        return s;
    }

    public static int Main ()
    {
        A nada = null;
        A algo = new A ();
        A a = nada ?? algo;
        if (a != algo)
            return 1;

        // la derecha no se calcula
        A b = algo ?? Cuenta (new A ());
        if (b != algo || llamadas != 0)
            return 2;

        // la izquierda se calcula una vez
        A c = Cuenta (nada) ?? algo;
        if (c != algo || llamadas != 1)
            return 3;

        A d = null ?? algo;
        A e = new A () ?? algo;
        if (d != algo || e == algo)
            return 4;

        string s = null;
        string t = s ?? "x";
        if (t != "x" || (campo ?? "w") != "w")
            return 5;
        if ((Texto ("y") ?? "z") != "y" || llamadas != 2)
            return 6;

        // una hija a la derecha, y una interfaz
        A f = nada ?? new B ();
        if (!(f is B))
            return 7;
        I i = null;
        I j = i ?? new C ();
        if (j.V () != 1)
            return 8;

        // encadenado
        string u = s ?? campo ?? "v";
        if (u != "v")
            return 9;

        s ??= "n";
        if (s != "n")
            return 10;
        s ??= "m";
        if (s != "n")
            return 11;

        return 0;
    }
}
