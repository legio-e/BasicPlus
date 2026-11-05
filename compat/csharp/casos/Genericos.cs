// Los genéricos (#793): `class C<T>` es `@element{B} class C`, sin `T`; `C<Sensor>`, tal cual; `C<int>`, `C<Integer>`; un número
// entra en su caja y sale de ella, y con dos parámetros o en un método genérico, el cast lo escribe el traductor.
using System;
using System.Collections.Generic;

interface IValor
{
    int Valor ();
}

class Sensor : IValor
{
    int v;

    public Sensor (int x)
    {
        v = x;
    }

    public int Valor ()
    {
        return v;
    }
}

class Caja<T>
{
    T dentro;

    public Caja (T x)
    {
        dentro = x;
    }

    public T Saca ()
    {
        return dentro;
    }

    public void Mete (T x)
    {
        dentro = x;
    }

    public T Dentro
    {
        get { return dentro; }
        set { dentro = value; }
    }
}

class Hija<T> : Caja<T>
{
    public Hija (T x) : base (x)
    {
    }

    public T Otra ()
    {
        return Saca ();
    }
}

class Fija : Caja<int>
{
    public Fija () : base (7)
    {
    }
}

class Par<A, B>
{
    public A Primero;
    public B Segundo;

    public Par (A a, B b)
    {
        Primero = a;
        Segundo = b;
    }
}

class Medida<T> where T : IValor
{
    T x;

    public Medida (T v)
    {
        x = v;
    }

    public int Cuanto ()
    {
        return x.Valor () * 2;
    }

    public T Quien ()
    {
        return x;
    }
}

class Nodo<T>
{
    public T Valor;
    public Nodo<T> Siguiente;

    public Nodo (T v, Nodo<T> s)
    {
        Valor = v;
        Siguiente = s;
    }
}

class Pila<T>
{
    Nodo<T> cima;
    public int Cuantos;

    public void Mete (T x)
    {
        cima = new Nodo<T> (x, cima);
        Cuantos++;
    }

    public T Saca ()
    {
        T r = cima.Valor;
        cima = cima.Siguiente;
        Cuantos--;
        return r;
    }

    public bool Vacia ()
    {
        return cima == null;
    }
}

class X
{
    static T Igual<T> (T x)
    {
        return x;
    }

    static Caja<int> Crea ()
    {
        return new Caja<int> (9);
    }

    static int Main ()
    {
        var c = new Caja<Sensor> (new Sensor (3));
        Console.WriteLine (c.Saca ().Valor ());
        c.Mete (new Sensor (4));
        Console.WriteLine (c.Dentro.Valor ());
        c.Dentro = new Sensor (5);
        Console.WriteLine (c.Saca ().Valor ());

        var n = new Caja<int> (6);
        int v = n.Saca () + 1;
        n.Mete (v);
        n.Dentro = n.Dentro + 1;
        Console.WriteLine (n.Saca ());

        var s = new Caja<string> ("uno");
        Console.WriteLine (s.Saca () + "!");

        var h = new Hija<string> ("hija");
        Console.WriteLine (h.Otra () + h.Saca ());

        var f = new Fija ();
        Console.WriteLine (f.Saca () + 1);
        Console.WriteLine (Crea ().Saca ());

        var p = new Par<string, int> ("a", 1);
        Console.WriteLine (p.Primero + p.Segundo);
        p.Segundo = 2;
        Console.WriteLine (p.Segundo + 1);

        var m = new Medida<Sensor> (new Sensor (8));
        Console.WriteLine (m.Cuanto () + m.Quien ().Valor ());

        var pi = new Pila<int> ();
        pi.Mete (1);
        pi.Mete (2);
        int t = 0;
        while (!pi.Vacia ())
            t = t * 10 + pi.Saca ();
        Console.WriteLine (t);

        Console.WriteLine (Igual<int> (5) + Igual (6));
        Console.WriteLine (Igual ("x") + Igual<string> ("y"));
        Console.WriteLine (Igual (new Sensor (2)).Valor ());
        Console.WriteLine (Igual (new Pila<int> ()).Vacia ());
        Console.WriteLine (Igual (new Caja<int> (3)).Saca ());

        var l = new List<int> ();
        l.Add (1);
        l.Add (2);
        l[0] = 5;
        int suma = 0;
        foreach (int x in l)
            suma += x;
        Console.WriteLine (suma + l[1] + l.Count);

        var ls = new List<Sensor> ();
        ls.Add (new Sensor (1));
        ls.Add (new Sensor (2));
        foreach (Sensor x in ls)
            Console.WriteLine (x.Valor ());
        Console.WriteLine (ls[1].Valor ());

        var lc = new List<Caja<string>> ();
        lc.Add (new Caja<string> ("dentro"));
        Console.WriteLine (lc[0].Saca ());
        return 0;
    }
}
