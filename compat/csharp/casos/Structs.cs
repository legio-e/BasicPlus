// Un struct sin estado, sin campos de instancia (H8, tema 2): en JC, una clase. Copiarlo no se nota; lo que hay que
// escribir es su valor por defecto, que en C# existe: una local sin valor, un campo, una propiedad, default(S). Se comprueba
// solo. El que tiene campos sigue sin traducirse: el caso Rechazos.
using System;

interface IHabla
{
    string Di ();
}

struct Solo
{
    public static int Cuenta = 5;
    public const int Fijo = 7;

    public static int Dame ()
    {
        return Cuenta + Fijo;
    }
}

struct ConMetodos : IHabla
{
    public int Doble (int x)
    {
        return x * 2;
    }

    public string Di ()
    {
        return "hola";
    }

    public int Siempre {
        get {
            return 3;
        }
    }
}

struct ConConstructor
{
    public static int Hechos = 0;

    public ConConstructor (int a) : this ()
    {
        Hechos += a;
    }
}

struct Compartido
{
    static Compartido uno = new Compartido ();
    public static int Pedidos = 0;

    static Compartido Pide ()
    {
        Pedidos++;
        return uno;
    }

    public Compartido (int a)
    {
        this = uno;
    }

    public Compartido (string s)
    {
        this = Pide ();
    }
}

class Prueba
{
    static ConMetodos campo;
    ConMetodos otro;
    public ConMetodos Propiedad { get; set; }

    static ConMetodos Copia (ConMetodos c)
    {
        return c;
    }

    int Usa ()
    {
        return otro.Doble (2) + Propiedad.Doble (3);
    }

    public static int Main ()
    {
        if (Solo.Dame () != 12)
            return 1;

        ConMetodos a = new ConMetodos ();
        ConMetodos b = a;
        if (b.Doble (4) != 8 || b.Siempre != 3)
            return 2;

        ConMetodos sinValor;
        sinValor = default (ConMetodos);
        if (sinValor.Doble (1) != 2)
            return 3;

        if (campo.Doble (5) != 10)
            return 4;
        if (new Prueba ().Usa () != 10)
            return 5;
        if (Copia (a).Di () != "hola")
            return 6;

        // una caja: el mismo objeto
        object o = a;
        if (!(o is ConMetodos))
            return 7;
        ConMetodos c = (ConMetodos) o;
        if (c.Doble (6) != 12)
            return 8;
        IHabla h = a;
        if (h.Di () != "hola")
            return 9;

        ConConstructor x = new ConConstructor (2);
        ConConstructor y = new ConConstructor ();
        if (ConConstructor.Hechos != 2)
            return 10;

        // `this = …`: no hay nada que copiar, pero lo de la derecha se calcula
        Compartido p = new Compartido (1);
        Compartido q = new Compartido ("x");
        if (Compartido.Pedidos != 1)
            return 11;

        return 0;
    }
}
