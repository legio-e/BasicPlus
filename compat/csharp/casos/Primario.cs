// El constructor primario (H8, tema 3): `class D (string arg) : Base (arg)`. Un constructor con esos parámetros, que llama a la base y da los valores
// que los miran, en orden. `class X ()` es el implícito; otro constructor llega a él con `: this (…)`. Se comprueba solo: Main devuelve 0.
using System;

abstract class Base (object obj)
{
    public string Prop { get; } = obj.ToString ();
    public int Fija = 3;
}

class D (string arg, int n) : Base (arg)
{
    int doble = n * 2;
    public string Copia { get; } = arg + "!";

    public int Doble {
        get { return doble; }
    }
}

class Vacia () : IDisposable
{
    public void Dispose ()
    {
    }
}

class DosCaminos (int a)
{
    public int A { get; } = a;

    public DosCaminos (string s) : this (s.Length)
    {
    }
}

class M
{
    public static int Main ()
    {
        var d = new D ("hola", 21);
        if (d.Prop != "hola")
            return 1;
        if (d.Doble != 42 || d.Copia != "hola!" || d.Fija != 3)
            return 2;
        using (var v = new Vacia ()) {
        }
        if (new DosCaminos (7).A != 7 || new DosCaminos ("abc").A != 3)
            return 3;
        return 0;
    }
}
