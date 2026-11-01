// Caso del traductor C# -> JC (V8/H7.3, #756): las interfaces de verdad. Una clase con su padre y dos interfaces, un
// diamante (IC hereda de IA y de IB, que heredan de IR), la implementación heredada del padre (Mono test-80), una
// implementación explícita, una clase abstracta que implementa, una propiedad de interfaz, `is`, conversiones y `==`
// entre una interfaz y una clase. Se comprueba solo: Main devuelve 0.
using System;

interface IR { int Raiz(); }
interface IA : IR { int A(); }
interface IB : IR { int B(); }
interface IC : IA, IB { string Nombre { get; } }
interface ISuma { int Suma(int a, int b); }

class Base
{
    public int Suma(int a, int b) { return a + b; }    // la implementación de ISuma la pone el padre
}

class Todo : Base, IC, ISuma
{
    public int Raiz() { return 1; }
    public int A() { return 2; }
    int IB.B() { return 3; }                            // explícita: en JC, un método público
    public string Nombre { get { return "todo"; } }
}

abstract class Figura : IA
{
    public int Raiz() { return 10; }
    public abstract int A();
}

class Cuadro : Figura
{
    public override int A() { return 20; }
}

class Programa
{
    static int Usa(IR r) { return r.Raiz(); }

    static int Main()
    {
        Todo t = new Todo();
        IC c = t;
        if (c.Raiz() + c.A() + ((IB) c).B() != 6) return 1;
        if (c.Nombre != "todo") return 2;
        ISuma s = t;
        if (s.Suma(2, 3) != 5) return 3;
        if (Usa(t) != 1 || Usa(new Cuadro()) != 10) return 4;
        IA a = new Cuadro();
        if (a.A() != 20) return 5;
        object o = t;
        if (!(o is IB) || o is Figura || !(a is IR)) return 6;
        IB b = (IB) o;
        if (b.B() != 3) return 7;
        if (b != c || (object) a == (object) t) return 8;
        Console.WriteLine("interfaces " + c.Nombre + " " + s.Suma(c.A(), b.B()));
        return 0;
    }
}
