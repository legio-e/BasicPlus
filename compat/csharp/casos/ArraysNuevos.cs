// `new T[n]`, de lo que sea (H8): una clase, una interfaz, un enum, string, bool y los números. Sus casillas nacen a null o
// a cero, como en C#; y el tamaño, si es un long, lleva su conversión. Se comprueba solo: Main devuelve 0.
using System;

interface IForma
{
    int Area ();
}

class Punto : IForma
{
    public int x;

    public Punto (int x)
    {
        this.x = x;
    }

    public int Area ()
    {
        return x * 2;
    }
}

enum Color
{
    Rojo,
    Verde = 7
}

class M
{
    Punto[] campo = new Punto [2];
    static string[] nombres = new string [3];

    static Punto[] Crea (int n)
    {
        return new Punto [n + 1];
    }

    public static int Main ()
    {
        Punto[] a = new Punto [3];
        if (a.Length != 3 || a [0] != null)
            return 1;
        a [1] = new Punto (6);
        if (a [1].Area () != 12)
            return 2;

        string[] s = new string [2];
        if (s [0] != null)
            return 3;
        s [1] = "hola";
        if (s [1].Length != 4)
            return 4;

        bool[] b = new bool [2];
        if (b [1])
            return 5;

        Color[] c = new Color [2];
        c [1] = Color.Verde;
        if (c [0] != Color.Rojo || c [1] != Color.Verde)
            return 6;

        IForma[] f = new IForma [1];
        f [0] = new Punto (3);
        if (f [0].Area () != 6)
            return 7;

        float[] r = new float [2];
        short[] h = new short [2];
        sbyte[] y = new sbyte [2];
        ushort[] u = new ushort [2];
        if (r [1] != 0 || h [1] != 0 || y [1] != 0 || u [1] != 0)
            return 8;

        long n = 4;
        int[] e = new int [n];
        if (e.Length != 4)
            return 9;

        if (Crea (2).Length != 3)
            return 10;
        if (new M ().campo.Length != 2 || nombres.Length != 3)
            return 11;

        int?[] o = new int? [2];
        if (o [0] != null)
            return 12;

        return 0;
    }
}
