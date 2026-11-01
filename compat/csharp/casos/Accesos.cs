// Caso del traductor C# -> JC (V8/H6, el escalón 1): lo privado que usa una clase anidada (en JC son clases aparte, A_B:
// se escribe public; test-122 y test-190 de Mono), y lock como C#: lo que se cierra se calcula una vez, y si es null,
// ArgumentNullException (test-524 y test-899). Las anidadas las construye Programa, declarada detrás (cuando se escribió,
// que las construyera la de fuera, declarada antes en JC, era #751, cerrada el 1-nov). Se comprueba solo: Main
// devuelve 0.
using System;

class A
{
    private static int X = 0;
    private int foo = 0;

    public static int Equis() { return X; }
    public int Foo() { return foo; }

    public class B
    {
        public void Sube() { ++X; }
    }

    public class C
    {
        public void Pon(A a) { a.foo = 4; }
    }
}

class Programa
{
    static int Main()
    {
        new A.B().Sube();
        if (A.Equis() != 1) return 1;
        A a = new A();
        new A.C().Pon(a);
        if (a.Foo() != 4) return 2;
        try
        {
            lock (null)
            {
                return 3;
            }
        }
        catch (ArgumentNullException)
        {
        }
        object o;
        lock (o = new object())
        {
            if (o == null) return 4;
        }
        if (o == null) return 5;
        object t = new object();
        lock (t)
        {
            t = null;
        }
        if (t != null) return 6;
        Console.WriteLine("accesos bien");
        return 0;
    }
}
