// Caso del traductor C# -> JC (V8/H8, tema 1; 2-nov): cuatro cosas de C# que JC, como Java, escribe de otra forma.
// `x as T`, `(x is T ? (T) x : null)` (subir, el mismo objeto; un T? de una caja, su caja); una constante estrecha en
// un argumento o un return, con su conversión escrita; un tipo que se llama como una variable (el «Color Color»), con
// un `_` detrás; y `1__000`, `1000`. Se comprueba solo: Main devuelve 0.
using System;

enum Color { Rojo, Verde }

class Animal { public virtual string Nombre() { return "animal"; } }
class Perro : Animal { public override string Nombre() { return "perro"; } }

class Pintor
{
    public Color Color;                               // el «Color Color»: un campo que se llama como su tipo
    public bool EsRojo() { return Color == Color.Rojo; }
}

class Programa
{
    static int Cuenta(byte b) { return b * 2; }
    static short Corto() { return 7; }

    static int Main()
    {
        object o = new Perro();
        Perro p = o as Perro;
        if (p == null || p.Nombre() != "perro") return 1;
        object nada = new object();
        if (nada as Animal != null) return 2;
        Animal a = p as Animal;                       // subir: el mismo objeto
        if (a.Nombre() != "perro") return 3;
        object caja = 5;
        int? ent = caja as int?;
        if (ent != 5) return 4;
        if (Cuenta(5) != 10 || Corto() != 7) return 5;  // una constante estrecha, en un argumento y en un return
        var pintor = new Pintor();
        pintor.Color = Color.Rojo;
        if (!pintor.EsRojo()) return 6;
        int mil = 1__000;
        if (mil != 1000) return 7;
        return 0;
    }
}
