// Caso del traductor C# -> JC (V8/H6, el escalón 1): los nombres que chocan. Dos clases que se llaman igual en dos
// namespace llevan el suyo delante (N1.X es N1_X), y sus anidadas también (N1.X.Dentro es N1_X_Dentro); una del espacio
// global, como estaba; las que no chocan, como estaban. Se comprueba solo: Main devuelve 0.
using System;

class X
{
    public static int Valor() { return 0; }
}

namespace N1
{
    class X
    {
        public static int Valor() { return 1; }
        public class Dentro { public int V = 10; }
    }
}

namespace N2.Sub
{
    class X
    {
        public static int Valor() { return 2; }
        public class Dentro { public int V = 20; }
    }

    class Sola
    {
        public static int Valor() { return 3; }
    }
}

class Programa
{
    static int Main()
    {
        if (X.Valor() != 0) return 1;
        if (N1.X.Valor() != 1) return 2;
        if (N2.Sub.X.Valor() != 2) return 3;
        if (N2.Sub.Sola.Valor() != 3) return 4;
        if (new N1.X.Dentro().V + new N2.Sub.X.Dentro().V != 30) return 5;
        Console.WriteLine("nombres bien");
        return 0;
    }
}
