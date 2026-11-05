// Los indexadores: `T this[…]` son dos métodos en JC, y `x[i]` es `x.get(i)`, y `x[i] = v`, `x.set(i, v)` (#794).
using System;

interface ILee
{
    int this[int i] { get; }
}

interface IEscribe : ILee
{
    new int this[int i] { get; set; }
}

abstract class Figura
{
    public abstract int this[int i] { get; }

    public virtual string this[string k]
    {
        get { return "figura " + k; }
    }
}

class Tabla : Figura, ILee
{
    int[] d = new int[4];

    public override int this[int i]
    {
        get { return d[i]; }
    }

    public int Pon(int i, int v)
    {
        d[i] = v;
        return v;
    }
}

class Hija : Tabla
{
    public override string this[string k] => "hija y " + base[k];

    public override int this[int i]
    {
        get { return base[i] * 2; }
    }
}

class Cuenta : IEscribe
{
    int[] d = new int[3];
    public int escritos;

    public int this[int i]
    {
        set { d[i] = value; escritos++; }
        get { return d[i]; }
    }

    public int this[string k]
    {
        get { return k.Length; }
    }

    public int Suma => this[0] + this[1];
}

class Malla
{
    int[] d = new int[6];

    public int this[int x, int y]
    {
        get { return d[y * 3 + x]; }
        set { d[y * 3 + x] = value; }
    }
}

class Tiene
{
    object guardado;

    public object this[object llave]
    {
        get { return guardado; }
        set { guardado = value; }
    }
}

class X
{
    static int llamadas;

    static int Cual()
    {
        llamadas++;
        return 1;
    }

    static int Main()
    {
        var t = new Tabla();
        t.Pon(1, 7);
        Console.WriteLine(t[1]);
        Console.WriteLine(t["x"]);
        Figura f = new Hija();
        ((Tabla) f).Pon(2, 4);
        Console.WriteLine(f[2]);
        Console.WriteLine(f["y"]);
        ILee l = t;
        Console.WriteLine(l[1]);

        var c = new Cuenta();
        c[0] = 5;
        c[1] = 6;
        c[1] += 4;
        c[Cual()] *= 2;
        c[2]++;
        Console.WriteLine(c.Suma + " " + c[2] + " " + c["hola"] + " " + c.escritos + " " + llamadas);
        IEscribe e = c;
        e[0] = 9;
        Console.WriteLine(e[0]);

        var m = new Malla();
        m[1, 1] = 3;
        Console.WriteLine(m[1, 1] + m[0, 0]);

        var g = new Tiene();
        g[1] = "uno";
        Console.WriteLine(g[2]);
        if (c[1] != 20 || llamadas != 1) return 1;
        return 0;
    }
}
