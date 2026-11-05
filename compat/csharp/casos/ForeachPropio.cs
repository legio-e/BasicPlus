// `foreach` sobre lo que no es un array (H8, tema 3): una clase con `GetEnumerator`, `MoveNext` y `Current`, sin interfaces, como C#; una que implementa
// `IEnumerable` e `IEnumerator`; y una cadena. El enumerador se suelta al salir, por donde sea, si es `IDisposable`, aunque lo sea sólo la hija que devuelve.
// Se comprueba solo: Main devuelve 0.
using System;
using System.Collections;

class Pasos
{
    int n;

    public Pasos (int n)
    {
        this.n = n;
    }

    public bool MoveNext ()
    {
        return n-- > 0;
    }

    public int Current {
        get { return n; }
    }
}

class Cuenta
{
    public Pasos GetEnumerator ()
    {
        return new Pasos (3);
    }
}

class Suelta : Pasos, IDisposable
{
    public static int soltados = 0;

    public Suelta () : base (2)
    {
    }

    public void Dispose ()
    {
        soltados++;
    }
}

class ConSuelta
{
    public Pasos GetEnumerator ()
    {
        return new Suelta ();
    }
}

class Trozos : IEnumerable
{
    public IEnumerator GetEnumerator ()
    {
        return new Paso ();
    }

    class Paso : IEnumerator
    {
        int i = -1;

        public bool MoveNext ()
        {
            i++;
            return i < 2;
        }

        public void Reset ()
        {
            i = -1;
        }

        public object Current {
            get { return i == 0 ? "uno" : "dos"; }
        }
    }
}

class M
{
    public static int Main ()
    {
        int t = 0;
        foreach (int x in new Cuenta ())
            t = t * 10 + x;
        if (t != 210)
            return 1;

        // con break, con continue y dentro de otro
        t = 0;
        foreach (int x in new Cuenta ()) {
            if (x == 2)
                continue;
            if (x == 0)
                break;
            foreach (int y in new Cuenta ())
                t += y + 1;
        }
        if (t != 6)
            return 2;

        foreach (int x in new ConSuelta ()) {
            if (x == 0)
                break;
        }
        if (Suelta.soltados != 1)
            return 3;

        string todo = "";
        foreach (string s in new Trozos ())
            todo += s;
        if (todo != "unodos")
            return 4;

        foreach (object o in new Trozos ())
            todo += (string) o;
        if (todo != "unodosunodos")
            return 5;

        int suma = 0;
        foreach (char c in "abc") {
            if (c == 'b')
                continue;
            suma += c;
        }
        if (suma != 'a' + 'c')
            return 6;

        return 0;
    }
}
