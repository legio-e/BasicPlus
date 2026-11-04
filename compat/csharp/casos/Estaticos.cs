// Un constructor estático que sólo da valores (H8, tema 2): en JC corre al cargar el módulo, detrás de los valores de los
// campos, como en C#. Vacío, nada. Si llama a algo, se nota cuándo corre, y no se traduce: el caso Rechazos. Se comprueba solo.
using System;

class Vacio
{
    static Vacio ()
    {
    }
}

class Otro
{
}

class Valores
{
    // declarado antes que los campos: corre detrás de sus valores
    static Valores ()
    {
        v = 10;
        doble = w * 2 + Fijo;
        s = "x";
        a = new int [] { 1, 2 };
        o = new Otro ();
        Prop = 8;
    }

    const int Fijo = 1;
    public static int v;
    public static int w = 3;
    public static int doble;
    public static string s;
    public static int [] a;
    public static Otro o;
    public static int Prop { get; set; }
}

struct ConStruct
{
    public static ConStruct Vacio;
    public static int n;

    static ConStruct ()
    {
        Vacio = new ConStruct ();
        n = 2;
    }
}

class Prueba
{
    public static int Main ()
    {
        if (Valores.v != 10 || Valores.w != 3 || Valores.doble != 7)
            return 1;
        if (Valores.s != "x" || Valores.a [1] != 2 || Valores.o == null || Valores.Prop != 8)
            return 2;
        if (ConStruct.n != 2)
            return 3;
        return 0;
    }
}
