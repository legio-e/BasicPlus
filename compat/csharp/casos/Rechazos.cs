// Caso del traductor C# -> JC (V8/H6): lo que no se traduce, y por qué. Una construcción de cada motivo: le falta a JC
// (una mejora de H7), fuera, el traductor todavía no, y la biblioteca. Todas se dicen, no sólo la primera.
using System;

struct Punto { public int X; }                        // le falta a JC: struct

interface IForma { int Area(); }                      // el traductor todavía no: interfaces

enum Color { Rojo, Verde }                            // el traductor todavía no: enum

class Base
{
    public void Saluda() { }
}

class Hija : Base
{
    public void Saluda() { }                          // le falta a JC: métodos no virtuales (oculta el de Base)
}

class Programa
{
    static T Primero<T>(T a) { return a; }           // le falta a JC: genéricos

    static void Doble(ref int x) { x *= 2; }          // le falta a JC: ref / out

    static int Main()
    {
        object o = "hola";
        string s = o as string;                       // le falta a JC: as
        ulong u = 5;                                  // fuera: ulong (el uint ya se traduce: SinSigno)
        Func<int, int> f = x => x + 1;                // fuera: lambdas
        int i = 0;
        int j = i++ + i;                              // le falta a JC: ++ dentro de una expresión (i, leída otra vez)
        int[,] mm = new int[2, 2];                    // le falta a JC: int[,] (char ya se traduce: Caracteres)
        string t = string.Join(",", "a", "b");        // la biblioteca
        try
        {
            s = null;                                 // ya se traduce (#774): un string puede valer null
        }
        catch (NullReferenceException)                // le falta a JC: errores de la VM con su tipo
        {
        }
        goto fin;                                     // fuera: goto
    fin:
        return 0;
    }
}

class ConLlamada
{
    static ConLlamada() { Console.WriteLine("hola"); }  // le falta a JC: un constructor estático que hace algo (Estaticos)
}
