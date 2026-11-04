// Caso del traductor C# -> JC (V8/H6): lo que no se traduce y salió al pasar las pruebas de Mono (31-oct): lo que le
// falta a JC, y los fallos de JC, con su ficha (la traducción sería buena y JC la haría mal).
using System;

class Foo
{
    public static int X = 1;
}

class Padre
{
    public virtual int P { get { return 1; } }
}

class Hija : Padre
{
    public override int P { get { return base.P + 1; } }      // era #744 (cerrada el 1-nov): ya no se rechaza
}

class Antes
{
    public Despues Crea() { return new Despues(); }           // era #751 (cerrada el 1-nov): ya no se rechaza
}

class Despues
{
    public Despues() : base() { }                              // era #753 (cerrada el 1-nov): ya no se rechaza
}

class Programa
{
    static sbyte Uno() { return 1; }                           // ya se traduce (H8): ((sbyte) 1)
    static int Doble(short s) { return s * 2; }

    static int Main()
    {
        bool a = true, b = false;
        bool c = a & b;                                        // le falta a JC: & sobre bool
        c |= b;                                                // le falta a JC: |= sobre bool
        string u = Une(Nombre(), Nombre() ?? "x");             // ?? que calcula, detrás de otra llamada: no se puede adelantar
        Foo Foo = null;
        int x = Foo.X;                                         // ya se traduce (H8): la clase, Foo_
        int y = Doble(3);                                      // ya se traduce (H8): ((short) 3)
        string s = null;                                       // ya se traduce (#774): un string puede valer null
        int z = 1__000;                                        // ya se traduce (H8): 1000
        return 0;
    }

    static int Tono(Tonos t = Tonos.Alto) { return 0; }       // le falta a JC: un enum como valor por defecto
    static string Nombre() { return null; }
    static string Une(string a, string b) { return a + b; }
}

enum Tonos { Bajo, Alto }
