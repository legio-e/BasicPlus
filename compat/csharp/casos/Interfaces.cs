// Caso del traductor C# -> JC (V8/H6): las interfaces simples (#745, decidido por Eduardo: una interfaz que sólo
// implementan clases sin otra base es su clase base, cuyos métodos lanzan, como un abstract), las clases abstractas, los
// enums (con `(int)`; Eduardo: «(int)Enum.mivalor») y Main(string[] args). Se comprueba solo: Main devuelve 0.
using System;

interface IForma
{
    int Area();
    string Nombre { get; }
}

interface IConLados : IForma                    // una interfaz que hereda de otra: una cadena de clases
{
    int Lados();
}

class Cuadrado : IConLados
{
    int lado;
    public Cuadrado(int lado) { this.lado = lado; }
    public int Area() { return lado * lado; }
    public string Nombre { get { return "cuadrado"; } }
    int IConLados.Lados() { return 4; }          // una implementación explícita
}

abstract class Animal
{
    public abstract string Habla();
    public abstract int Patas { get; }
    public string Presenta() { return Habla() + " con " + Patas + " patas"; }
}

class Perro : Animal
{
    public override string Habla() { return "guau"; }
    public override int Patas { get { return 4; } }
}

enum Dia { Lunes, Martes, Miercoles = 10, Jueves }

class Programa
{
    static int Numero(Dia d) { return (int) d; }

    static int Main(string[] args)
    {
        if (args == null) return 1;
        IForma f = new Cuadrado(3);
        if (f.Area() != 9) return 2;
        if (f.Nombre != "cuadrado") return 3;
        IConLados c = new Cuadrado(2);
        if (c.Lados() != 4) return 4;
        if (!(f is IConLados)) return 5;
        Animal a = new Perro();
        if (a.Presenta() != "guau con 4 patas") return 6;
        Dia d = Dia.Jueves;
        if ((int) d != 11) return 7;
        if (Numero(Dia.Martes) != 1) return 8;
        if (d == Dia.Lunes || d != Dia.Jueves) return 9;
        if (!(Dia.Martes < d)) return 10;
        Console.WriteLine("dia " + (int) d);
        return 0;
    }
}
