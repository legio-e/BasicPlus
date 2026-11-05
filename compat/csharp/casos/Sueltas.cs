// Las sueltas (H8, temas 1 y 2): un [Conditional] sin definir no se escribe, con su cuerpo; `checked (M) (x)` es la llamada;
// un `$"…"` que se pasa como FormattableString es su texto; una local sin valor que nadie usa no se escribe, sea del tipo que sea; y un campo que se da a
// sí mismo el valor, con lo de la derecha. Se comprueba solo.
using System;
using System.Diagnostics;

class Prueba
{
    static bool asi = asi = Cierto ();

    [Conditional ("NO_DEFINIDO")]
    static void NoEsta ()
    {
        Environment.Exit (1);
    }

    static int Cero (object o)
    {
        return 0;
    }

    static void Escribe (FormattableString texto)
    {
        Console.WriteLine (texto);
    }

    static bool Cierto ()
    {
        return true;
    }

    public static int Main ()
    {
        NoEsta ();
        if (checked (Cero) (null) + unchecked (Cero) (null) != 0)
            return 9;
        Escribe ($"caso {1}");
        System.Net.IPAddress[] direcciones;
        int a, b;
        int usada;
        usada = 2;
        if (!asi || usada != 2)
            return 1;
        return 0;
    }
}
