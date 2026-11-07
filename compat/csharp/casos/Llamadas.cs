// Lo que C# pone en una llamada y JC no escribe solo (H8, tema 4): los valores por defecto que no son un literal (null, un
// enum, un T? con valor) y los de un método con un params detrás de uno, escritos en cada llamada; un null con su tipo, que
// elige la sobrecarga; un array de valores de otro tipo que el suyo; y los argumentos con nombre que cambian el orden de lo
// que se calcula. Se comprueba solo: Main devuelve 0.
using System;

enum Color { Rojo, Verde, Azul }

enum Talla { Chica, Media, Grande }

class Animal
{
    public virtual string Nombre () { return "animal"; }
}

class Perro : Animal
{
    public override string Nombre () { return "perro"; }
}

class Gato : Animal
{
    public override string Nombre () { return "gato"; }
}

class Base
{
    public Color Tono;

    public Base (Color c = Color.Azul)
    {
        Tono = c;
    }
}

class Hija : Base
{
    public Hija ()
    {
    }
}

class Nieta : Base
{
}

class Llamadas
{
    static string traza = "";

    static int Anota (int n)
    {
        traza += n;
        return n;
    }

    static string Pinta (Color c = Color.Verde, string s = null, int? n = 5)
    {
        return c + ":" + (s == null ? "null" : s) + ":" + (n == null ? "null" : n.ToString ());
    }

    static bool EsMedia (Talla t = Talla.Media)
    {
        return t == Talla.Media;
    }

    static int Junta (int a = 1, params int[] resto)
    {
        int s = a;
        foreach (int x in resto)
            s += x;
        return s;
    }

    static string Elige (string s) { return "string"; }
    static string Elige (Animal a) { return "animal"; }

    static string ConVacio (string[] s, string valor = null) { return "uno"; }
    static string ConVacio (string s, params string[] resto) { return "dos"; }

    static int Cuenta (params object[] xs) { return xs.Length; }

    static int Tres (int a, int b, int c)
    {
        return a * 100 + b * 10 + c;
    }

    public static int Main ()
    {
        // 1. los valores por defecto, en la llamada
        if (Pinta () != "Verde:null:5")
            return 1;
        if (Pinta (Color.Rojo) != "Rojo:null:5")
            return 2;
        if (Pinta (s: "x") != "Verde:x:5")
            return 3;
        if (Pinta (n: null) != "Verde:null:null")
            return 4;
        if (Junta () != 1 || Junta (2) != 2 || Junta (2, 3, 4) != 9)
            return 5;
        if (!EsMedia () || EsMedia (Talla.Grande))
            return 15;
        if (new Hija ().Tono != Color.Azul || new Nieta ().Tono != Color.Azul || new Base (Color.Rojo).Tono != Color.Rojo)
            return 6;
        if (ConVacio (null) != "uno")
            return 7;

        // 2. un null con su tipo
        if (Elige ((string) null) != "string" || Elige ((Animal) null) != "animal")
            return 8;

        // 3. un array de valores de otro tipo
        Animal[] zoo = new Animal[] { new Perro (), new Gato (), new Animal () };
        if (zoo.Length != 3 || zoo[0].Nombre () != "perro" || zoo[1].Nombre () != "gato" || zoo[2].Nombre () != "animal")
            return 9;
        if (Cuenta (8, "a", 2.5) != 3 || Cuenta () != 0)
            return 10;
        string[] nulos = new string[] { null, null };
        if (nulos.Length != 2 || nulos[1] != null)
            return 11;

        // 4. los argumentos con nombre, en el orden en que C# los calcula
        traza = "";
        if (Tres (c: Anota (1), a: Anota (2), b: Anota (3)) != 231 || traza != "123")
            return 12;
        traza = "";
        int r = Tres (b: Anota (4), c: 7, a: Anota (5));
        if (r != 547 || traza != "45")
            return 13;
        int vueltas = 0;
        traza = "";
        while (Tres (c: Anota (vueltas), a: 0, b: Anota (9)) < 93)
            vueltas++;
        if (vueltas != 3 || traza != "09192939")
            return 14;

        Console.WriteLine ("ok");
        return 0;
    }
}
