// IDisposable, la interfaz de CsSystem (H8, tema 2): una variable de su tipo, una clase que la implementa y una hija que
// hereda su Dispose, una interfaz del programa que hereda de ella, una implementación explícita, is, as y una conversión,
// y un using con varias variables. Se comprueba solo.
using System;

class Recurso : IDisposable
{
    public static int Vivos = 0;
    public static string Orden = "";
    public string Nombre;

    public Recurso (string nombre)
    {
        Nombre = nombre;
        Vivos++;
    }

    public void Dispose ()
    {
        Vivos--;
        Orden += Nombre;
    }
}

class Hija : Recurso
{
    public Hija () : base ("h")
    {
    }
}

interface IConAlgo : IDisposable
{
    int Algo ();
}

class ConAlgo : IConAlgo
{
    public static int Cerrados = 0;

    public int Algo ()
    {
        return 7;
    }

    void IDisposable.Dispose ()
    {
        Cerrados++;
    }
}

class Prueba
{
    static IDisposable Dame ()
    {
        return new Recurso ("d");
    }

    static int Sale ()
    {
        using (IDisposable d = Dame ()) {
            return 3;
        }
    }

    public static int Main ()
    {
        using (Recurso r = new Recurso ("a")) {
            if (Recurso.Vivos != 1)
                return 1;
        }
        if (Recurso.Vivos != 0 || Recurso.Orden != "a")
            return 2;

        if (Sale () != 3 || Recurso.Orden != "ad")
            return 3;

        using (Hija h = new Hija ())
            ;
        if (Recurso.Orden != "adh")
            return 4;

        using (IConAlgo c = new ConAlgo ()) {
            if (c.Algo () != 7)
                return 5;
        }
        if (ConAlgo.Cerrados != 1)
            return 6;

        // la de después se cierra antes
        using (Recurso x = new Recurso ("x"), y = new Recurso ("y"))
            ;
        if (Recurso.Orden != "adhyx")
            return 7;

        // sin variable: lo que se cierra, en una temporal; null no se cierra
        Recurso nulo = null;
        using (nulo)
            ;
        using (Dame ())
            ;
        if (Recurso.Orden != "adhyxd")
            return 8;

        object o = new Recurso ("o");
        if (!(o is IDisposable))
            return 9;
        IDisposable i = (IDisposable) o;
        i.Dispose ();
        if (Recurso.Orden != "adhyxdo")
            return 10;
        IDisposable j = o as IDisposable;
        if (j == null)
            return 11;
        object nada = new object ();
        IDisposable k = nada as IDisposable;
        if (k != null)
            return 12;

        Console.WriteLine (Recurso.Orden);
        return 0;
    }
}
