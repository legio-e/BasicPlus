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
    public override int P { get { return base.P + 1; } }      // un fallo de JC: #744
}

class Antes
{
    public Despues Crea() { return new Despues(); }           // un fallo de JC: #751
}

class Despues
{
    public Despues() : base() { }                              // un fallo de JC: #753
}

class Programa
{
    static sbyte Uno() { return 1; }                           // le falta a JC: una constante en un return estrecho
    static int Doble(short s) { return s * 2; }

    static int Main()
    {
        bool a = true, b = false;
        bool c = a & b;                                        // le falta a JC: & sobre bool
        c |= b;                                                // le falta a JC: |= sobre bool
        Foo Foo = null;
        int x = Foo.X;                                         // le falta a JC: el Color Color
        int y = Doble(3);                                      // le falta a JC: una constante en un argumento estrecho
        string s = null;                                       // le falta a JC: null en una cadena
        int z = 1__000;                                        // le falta a JC: los _ de C#
        return 0;
    }

    static int Tono(Tonos t = Tonos.Alto) { return 0; }       // le falta a JC: un enum como valor por defecto
}

enum Tonos { Bajo, Alto }
