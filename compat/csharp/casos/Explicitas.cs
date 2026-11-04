// Las implementaciones explícitas que chocan (H8, tema 2): lleva otro nombre el miembro de la interfaz, y una clase que lo
// implementa con un miembro normal le reenvía. Se comprueba solo.
using System;

interface IFoo
{
    int Hello (int x);
}

// una explícita y un método normal con su nombre
class Dos : IFoo
{
    int IFoo.Hello (int x)
    {
        return x + 1;
    }

    public int Hello (int x)
    {
        return x + 2;
    }

    // otra sobrecarga no choca
    public int Hello (string s)
    {
        return 9;
    }
}

// la misma interfaz, implementada con un método normal: se llama por la interfaz
class Uno : IFoo
{
    public virtual int Hello (int x)
    {
        return x + 3;
    }
}

class Hija : Uno
{
    public override int Hello (int x)
    {
        return x + 4;
    }
}

interface IA
{
    string Stop ();
}

interface IB
{
    string Stop ();
}

// dos interfaces con el mismo método, cada una con su cuerpo, y además uno normal
class Coche : IA, IB
{
    string IA.Stop ()
    {
        return "a";
    }

    string IB.Stop ()
    {
        return "b";
    }

    public string Stop ()
    {
        IA self = (IA) this;
        return "c" + self.Stop ();
    }
}

interface IP
{
    string Spec { get; }
}

interface IQ : IP
{
    new string Spec { get; }
}

// propiedades: una interfaz que tapa a la de su padre
class Concreta : IQ
{
    string IP.Spec { get { return "P"; } }
    string IQ.Spec { get { return "Q"; } }
    public string Spec { get { return "C"; } }
}

class ConProp : IP
{
    string valor = "v";
    public string Spec { get { return valor; } }
}

// una explícita, y en la hija un método que se llama igual: no la sobrescribe
class Base : IFoo
{
    int IFoo.Hello (int x)
    {
        return 100;
    }
}

class Derivada : Base
{
    public int Hello (int x)
    {
        return 200;
    }
}

// una clase anidada con su nombre no choca
class ConAnidada : IFoo
{
    int IFoo.Hello (int x)
    {
        return 5;
    }

    public class Hello2 { }
}

class Prueba
{
    public static int Main ()
    {
        Dos d = new Dos ();
        IFoo fd = d;
        if (d.Hello (1) != 3 || fd.Hello (1) != 2 || d.Hello ("s") != 9)
            return 1;

        Uno u = new Uno ();
        IFoo fu = u;
        if (fu.Hello (1) != 4 || u.Hello (1) != 4)
            return 2;
        IFoo fh = new Hija ();
        if (fh.Hello (1) != 5)
            return 3;

        Coche c = new Coche ();
        IA a = c;
        IB b = c;
        if (a.Stop () != "a" || b.Stop () != "b" || c.Stop () != "ca")
            return 4;

        Concreta k = new Concreta ();
        if (((IP) k).Spec != "P" || ((IQ) k).Spec != "Q" || k.Spec != "C")
            return 5;
        IP p = new ConProp ();
        if (p.Spec != "v")
            return 6;

        Derivada de = new Derivada ();
        IFoo fb = de;
        if (de.Hello (1) != 200 || fb.Hello (1) != 100)
            return 7;

        IFoo fa = new ConAnidada ();
        if (fa.Hello (1) != 5)
            return 8;

        return 0;
    }
}
