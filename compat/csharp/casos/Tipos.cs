// El tipo de un objeto (H8, tema 2): no hay un `Type` al ejecutar. Lo que se sabe al traducir, lo escribe el traductor (un `is`,
// un `true`, el nombre como texto); lo que no, lo pregunta a la VM. Se comprueba solo.
using System;

class Padre
{
    public string Quien ()
    {
        return GetType ().Name;
    }

    public string Entero ()
    {
        return this.GetType ().FullName;
    }
}

class Hija : Padre
{
}

class Nieta : Hija
{
}

class Sola
{
}

abstract class Abstracta
{
}

sealed class Sellada
{
}

static class Estatica
{
}

interface IQ
{
}

class ConInterfaz : IQ
{
}

struct Vacio
{
}

enum Color
{
    Rojo
}

namespace N
{
    class Dentro
    {
        public class Mas
        {
        }
    }
}

class Prueba
{
    static int llamadas = 0;

    static Sola Dame ()
    {
        llamadas++;
        return new Sola ();
    }

    static object Cuenta ()
    {
        llamadas++;
        return new Hija ();
    }

    public static int Main ()
    {
        // lo sabe el traductor: un valor, una cadena, una clase sin hijas
        int i = 5;
        if (i.GetType () != typeof (int) || i.GetType () == typeof (long))
            return 1;
        string s = "a";
        if (s.GetType () != typeof (string))
            return 2;
        Sola sola = new Sola ();
        if (sola.GetType () != typeof (Sola) || sola.GetType () == typeof (Padre))
            return 3;
        if (typeof (int) != typeof (int) || typeof (int) == typeof (long))
            return 4;
        if (new Hija ().GetType () != typeof (Hija))
            return 5;

        // `is`, y si tiene hijas, que no sea ninguna
        Padre p = new Hija ();
        if (p.GetType () == typeof (Padre) || p.GetType () != typeof (Hija))
            return 6;
        Padre n = new Nieta ();
        if (n.GetType () == typeof (Hija) || n.GetType () != typeof (Nieta))
            return 7;
        IQ q = new ConInterfaz ();
        if (q.GetType () != typeof (ConInterfaz) || q.GetType () == typeof (IQ))
            return 8;

        // dos que dependen de lo que llevan dentro: la VM
        object a = 5;
        object b = 1.5f;
        object c = 7;
        object o = new Hija ();
        if (a.GetType () == b.GetType () || a.GetType () != c.GetType ())
            return 9;
        if (o.GetType () != p.GetType () || o.GetType () == n.GetType ())
            return 10;

        // un object con un tipo escrito
        if (a.GetType () != typeof (int) || b.GetType () != typeof (float) || a.GetType () == typeof (long))
            return 11;
        if (o.GetType () != typeof (Hija) || o.GetType () == typeof (Padre))
            return 12;

        // los nombres
        if (typeof (Sola).Name != "Sola" || typeof (Sola).FullName != "Sola")
            return 13;
        if (typeof (N.Dentro.Mas).FullName != "N.Dentro+Mas" || typeof (N.Dentro.Mas).Name != "Mas")
            return 14;
        if (typeof (int).FullName != "System.Int32" || typeof (string).Name != "String" || typeof (int[]).FullName != "System.Int32[]")
            return 15;
        if (i.GetType ().FullName != "System.Int32" || sola.GetType ().ToString () != "Sola")
            return 16;
        if (p.GetType ().Name != "Hija" || p.Quien () != "Hija" || new Padre ().Quien () != "Padre" || n.Entero () != "Nieta")
            return 17;
        if (a.GetType ().FullName != "System.Int32" || o.GetType ().Name != "Hija")
            return 18;
        if ("es " + typeof (Sola) != "es Sola" || "es " + p.GetType () != "es Hija")
            return 19;
        Console.WriteLine (o.GetType ());
        Console.WriteLine (typeof (Vacio));

        // lo que dice la declaración
        if (!typeof (Abstracta).IsAbstract || typeof (Sola).IsAbstract || !typeof (Sellada).IsSealed || typeof (Sola).IsSealed)
            return 20;
        if (!typeof (Estatica).IsAbstract || !typeof (Estatica).IsSealed || !typeof (IQ).IsInterface || !typeof (Color).IsEnum)
            return 21;
        if (!typeof (Vacio).IsValueType || !typeof (Vacio).IsSealed || !typeof (Sola).IsClass)
            return 22;

        // GetType de un null lanza
        Sola nadie = null;
        try {
            if (nadie.GetType () == typeof (Sola))
                return 23;
            return 24;
        } catch (Exception) {
        }
        Padre nadie2 = null;
        try {
            if (nadie2.GetType ().Name == "x")
                return 25;
            return 26;
        } catch (Exception) {
        }

        // lo que calcula, una vez
        if (Dame ().GetType () != typeof (Sola) || llamadas != 1)
            return 27;
        bool es = Cuenta ().GetType () == typeof (Hija);
        if (!es || llamadas != 2)
            return 28;

        // sin usar, no hace nada
        var t = typeof (Sola);

        return 0;
    }
}
