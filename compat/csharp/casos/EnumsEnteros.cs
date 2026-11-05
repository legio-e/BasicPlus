// Un enum que se opera (H8, tema 5): el enum se queda como enum, y lo que se opera, un entero. `A | B`, `~e`, `e + 1`, `e++`, `(E) n`, `E e = 0`; un switch, un
// valor por defecto, una constante y un array; con valores que un enum de JC no lleva, y de tipos de debajo distintos. Su nombre en una cadena, como en C#. Uno que no
// se opera sigue siendo un enum de JC. Se comprueba solo: Main devuelve 0.
using System;

[Flags]
enum Permiso
{
    Nada = 0,
    Leer = 1,
    Escribir = 2,
    Borrar = 4
}

enum Dia
{
    Lunes,
    Martes,
    Miercoles
}

enum Signo
{
    Menos = -1,
    Cero,
    Mas
}

enum Grande : long
{
    Uno = 1,
    Mucho = 5000000000
}

enum Corto : byte
{
    A = 1,
    B = 200
}

enum Quieto
{
    Uno,
    Dos
}

class M
{
    static Permiso campo = Permiso.Leer | Permiso.Escribir;
    const Dia Fijo = Dia.Martes;
    const Corto Junto = Corto.A | Corto.B;
    Dia sinValor;

    static int Cual (int a)
    {
        return 1;
    }

    static int Cual (Dia a)
    {
        return 2;
    }

    static int Largo (long a)
    {
        return 3;
    }

    static int Largo (uint a)
    {
        return 4;
    }

    static string Que (Dia d)
    {
        switch (d) {
        case Dia.Lunes:
            return "l";
        case Dia.Martes:
            return "m";
        default:
            return "?";
        }
    }

    static Dia Siguiente (Dia d, Dia paso = Dia.Martes)
    {
        return d + (int) paso;
    }

    public static int Main ()
    {
        Permiso p = Permiso.Leer | Permiso.Escribir;
        if (p != campo)
            return 1;
        if ((p & Permiso.Escribir) == 0 || (p & Permiso.Borrar) != 0)
            return 2;
        p &= ~Permiso.Leer;
        if (p != Permiso.Escribir)
            return 3;
        p |= Permiso.Borrar;
        if ((int) p != 6)
            return 4;

        Dia d = Dia.Lunes;
        d++;
        if (d != Dia.Martes || d != Fijo)
            return 5;
        d = (Dia) 2;
        if (Que (d) != "?" || Que (Dia.Lunes) != "l" || Que (d - 1) != "m")
            return 6;
        Dia cero = 0;
        if (cero != Dia.Lunes || (int) d != 2 || Siguiente (Dia.Lunes) != Dia.Martes)
            return 7;
        if (new M ().sinValor != Dia.Lunes)
            return 8;

        Signo s = Signo.Menos;
        if ((int) s != -1 || s + 2 != Signo.Mas)
            return 9;

        Grande g = Grande.Mucho;
        if ((long) g != 5000000000 || g - 4999999999 != Grande.Uno)
            return 10;

        Corto c = Corto.A | Corto.B;
        if ((byte) c != 201 || (byte) Corto.B != 200)
            return 11;

        Dia[] v = new Dia [2];
        v [1] = Dia.Miercoles;
        if (v [0] != Dia.Lunes || (int) v [1] != 2)
            return 12;

        // su nombre, como en C#
        if (Dia.Martes.ToString () != "Martes" || "" + d != "Miercoles" || ((Dia) 7).ToString () != "7")
            return 13;
        if ((Permiso.Leer | Permiso.Borrar).ToString () != "Leer, Borrar" || Permiso.Nada.ToString () != "Nada")
            return 14;
        if (Signo.Menos.ToString () != "Menos")
            return 15;

        const Dia dentro = Dia.Miercoles;
        if ((int) dentro != 2 || (byte) Junto != 201)
            return 17;
        if (Cual (1) != 1 || Cual (d - 1) != 2 || Largo (5L) != 3 || Largo (5u) != 4)
            return 18;

        Quieto q = Quieto.Dos;
        if ("" + q != "Dos")
            return 16;
        return 0;
    }
}
