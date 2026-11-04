// Dos sueltas (H8, temas 1 y 2): una local sin valor que nadie usa no se escribe, sea del tipo que sea; y un campo que se da a
// sí mismo el valor, con lo de la derecha. Se comprueba solo.
using System;

class Prueba
{
    static bool asi = asi = Cierto ();

    static bool Cierto ()
    {
        return true;
    }

    public static int Main ()
    {
        System.Net.IPAddress[] direcciones;
        int a, b;
        int usada;
        usada = 2;
        if (!asi || usada != 2)
            return 1;
        return 0;
    }
}
