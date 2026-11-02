#define CON_TRAZA
// Caso del traductor C# -> JC (V8/H8, tema 1; 2-nov): los atributos. Una clase de atributo (`: Attribute`) es una clase
// más, y sus usos (`[Marca(...)]`, `[Obsolete]`, `[AttributeUsage]`…) se quitan. Un método `[Conditional("S")]` se
// escribe, y sus llamadas, si S no está definido, se quitan como en C#, sin calcular sus argumentos; con `#define S`, se
// hacen. Se comprueba solo: Main devuelve 0.
using System;
using System.Diagnostics;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Method)]
class MarcaAttribute : Attribute
{
    public string Nombre;
    public MarcaAttribute(string nombre) { Nombre = nombre; }
    public int Peso { get; set; }
    int orden;
    public int Orden { set { orden = value * 2; } get { return orden; } }   // el set delante, como en test-272
}

[Marca("clase", Peso = 2)]
[Obsolete]
class Programa
{
    static int llamadas = 0;
    static int calculos = 0;

    static int Efecto() { calculos++; return calculos; }

    [Conditional("SIN_DEFINIR")]
    static void Depura(int x) { llamadas += 100; }

    [Conditional("CON_TRAZA")]
    static void Traza(int x) { llamadas += x; }

    [Conditional("SIN_DEFINIR")]
    static void DepuraCorto() => Depura(1);

    [Marca("método")]
    static int Main()
    {
        Depura(Efecto());                         // se quita, y Efecto() no se calcula
        DepuraCorto();
        if (calculos != 0 || llamadas != 0) return 1;
        Traza(Efecto());                          // CON_TRAZA está definido: se hace
        if (calculos != 1 || llamadas != 1) return 2;
        var m = new MarcaAttribute("a mano");     // una clase de atributo, como cualquier otra
        m.Peso = 3;
        m.Orden = 5;
        if (m.Nombre != "a mano" || m.Peso != 3 || m.Orden != 10) return 3;
        return 0;
    }
}
