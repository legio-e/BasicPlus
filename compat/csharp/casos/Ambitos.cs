// Caso del traductor C# -> JC (V8/H6, el escalón 1): dos reescrituras de C#. Una local declarada en un case y usada en
// otro (en C#, su ámbito es el switch: test-152 de Mono), y una constante de un case usada en la etiqueta de otro
// (test-285); y la llamada a un método partial sin cuerpo, que C# quita con sus argumentos sin calcular (test-partial-22).
// Se comprueba solo: Main devuelve 0.
using System;

partial class Programa
{
    static int llamadas = 0;

    static partial void Avisa(int x);

    static int Cuenta()
    {
        llamadas++;
        return llamadas;
    }

    static int Elige(int v)
    {
        int a;
        switch (v)
        {
            case 0:
                int i = v + 1;
                a = i;
                break;
            default:
                i = 5;
                a = i * 2;
                break;
        }
        return a;
    }

    static int Etiqueta(string s)
    {
        switch (s)
        {
            case "HI":
                const string x = "Nice";
                return 1;
            case x:
                return 2;
        }
        return 3;
    }

    static int Main()
    {
        if (Elige(0) != 1 || Elige(7) != 10) return 1;
        if (Etiqueta("Nice") != 2 || Etiqueta("HI") != 1 || Etiqueta("otra") != 3) return 2;
        Avisa(Cuenta());
        if (llamadas != 0) return 3;
        Console.WriteLine("ambitos bien");
        return 0;
    }
}
