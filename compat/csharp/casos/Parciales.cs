// Caso del traductor C# -> JC (V8/H6, el escalón 1): los métodos partial. El valor por defecto de un parámetro es el de
// la declaración sin cuerpo, la que define el método en C# (test-partial-25 de Mono). Se comprueba solo: Main devuelve 0.
using System;

partial class C
{
    static int visto;

    static partial void Anota(int i = 8);

    static partial void Anota(int i)
    {
        visto = i;
    }

    static int Main()
    {
        Anota();
        if (visto != 8) return 1;
        Anota(3);
        if (visto != 3) return 2;
        Console.WriteLine("parciales bien");
        return 0;
    }
}
