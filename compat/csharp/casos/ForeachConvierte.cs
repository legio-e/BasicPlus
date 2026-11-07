// El foreach con otro tipo que el de los elementos (H8, tema 11): cada elemento, convertido al de la variable, como C#
// (de una caja a otra, a una clase hija, a un número más ancho); y ReferenceEquals y el Equals de un array, la identidad.
// Se comprueba solo: Main devuelve 0.
using System;

class Animal
{
}

class Perro : Animal
{
}

class ForeachConvierte
{
    public static int Main ()
    {
        int?[] valores = new int?[] { null, 3, 4 };
        long suma = 0;
        int nulos = 0;
        foreach (long? x in valores) {
            if (x == null)
                nulos++;
            else
                suma += x.Value;
        }
        if (suma != 7 || nulos != 1)
            return 1;

        Animal[] zoo = new Animal[] { new Perro (), new Perro () };
        int perros = 0;
        foreach (Perro p in zoo)
            if (p != null)
                perros++;
        if (perros != 2)
            return 2;

        int[] enteros = { 1, 2, 3 };
        long total = 0;
        foreach (long l in enteros)
            total += l;
        if (total != 6)
            return 3;

        byte[] a = new byte[0];
        byte[] b = new byte[0];
        byte[] c = a;
        if (a.Equals (b) || ReferenceEquals (a, b) || !ReferenceEquals (a, c) || !a.Equals (c))
            return 4;

        Console.WriteLine ("ok");
        return 0;
    }
}
