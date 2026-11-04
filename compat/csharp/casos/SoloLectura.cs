// Una propiedad de sólo lectura que redefine una hija (H8, tema 3): en C#, darle valor en el constructor escribe su campo, sin pasar por
// la hija; leerla sí va por su `get`, que es virtual. Se comprueba solo.
using System;

class A
{
    public int X { get; }
    public virtual int Y { get; }
    public virtual string Z { get; } = "z";

    public A ()
    {
        X = 4;
        X++;
        Y = 2;
        Y++;
        Y += 2;
        --Y;
    }
}

class B : A
{
    public int veces;

    public override int Y {
        get {
            ++veces;
            return base.Y;
        }
    }

    public override string Z {
        get {
            return base.Z + "!";
        }
    }
}

class Prueba
{
    public static int Main ()
    {
        A a = new A ();
        if (a.X != 5 || a.Y != 4 || a.Z != "z")
            return 1;

        // tres lecturas en el constructor de A: Y++, Y += 2 y --Y
        B b = new B ();
        if (b.veces != 3)
            return 2;
        if (b.Y != 4 || b.veces != 4 || b.X != 5 || b.Z != "z!")
            return 3;
        return 0;
    }
}
