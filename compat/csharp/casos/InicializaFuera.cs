// Un inicializador de objeto fuera de una declaración (H8, tema 3): `new T () { A = 1 }` en un return, una asignación, un argumento, una
// condición, una cadena de asignaciones o delante de un método. El objeto, a una temporal delante de la sentencia, y no se ve hasta que está
// hecho: en `f = new Foo () { P = f.P }`, `f` es el de antes. Se comprueba solo: Main devuelve 0.
using System;

class Datos
{
    public int Valor;
    public string Nombre { get; set; }

    public bool Es (string n)
    {
        return Nombre == n;
    }
}

class Par
{
    public Datos Uno;
    public int Dos;
}

class M
{
    static Datos propiedad;
    static int orden = 0;

    static Datos Hecho ()
    {
        return new Datos () { Valor = 3, Nombre = "tres" };
    }

    static int Lee (Datos d)
    {
        return d.Valor;
    }

    static int Paso (int n)
    {
        orden = orden * 10 + n;
        return n;
    }

    public static int Main ()
    {
        if (Hecho ().Valor != 3 || Hecho ().Nombre != "tres")
            return 1;

        propiedad = new Datos () { Valor = 4 };
        if (propiedad.Valor != 4)
            return 2;

        if (Lee (new Datos () { Valor = 5 }) != 5)
            return 3;

        Datos a, b;
        a = b = new Datos () { Valor = 6 };
        if (a != b || b.Valor != 6)
            return 4;

        if (!new Datos () { Nombre = "x" }.Es ("x"))
            return 5;

        // el objeto de antes, mientras se hace el nuevo
        Datos f = new Datos () { Valor = 7 };
        f = new Datos () { Valor = f.Valor + 1 };
        if (f.Valor != 8)
            return 6;

        // anidado, y en orden
        Par p = null;
        p = new Par () { Uno = new Datos () { Valor = Paso (1) }, Dos = Paso (2) };
        if (p.Uno.Valor != 1 || p.Dos != 2 || orden != 12)
            return 7;

        Datos d;
        int r = Lee (d = new Datos () { Valor = 9 });
        if (r != 9 || d.Valor != 9)
            return 8;

        // como sentencia: nadie lo guarda, pero lo que calcula se calcula
        orden = 0;
        new Datos () { Valor = Paso (3) };
        if (orden != 3)
            return 9;

        return 0;
    }
}
