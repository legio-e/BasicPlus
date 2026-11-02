// Caso del traductor C# -> JC (V8/H8; #774, 2-nov): los null de las cadenas. En JC una cadena nunca es null, pero una
// variable string sí puede no referenciar ninguna, como en C#. Lo que difiere: un campo o una propiedad string sin valor
// nace null en C# y "" en JC, así que el traductor le escribe el null de C#. Lo demás, igual en los dos: concatenar un
// null, como ""; == sólo igual a null; un switch sobre él, al default; string.IsNullOrEmpty; y medirlo lanza (en C#,
// NullReferenceException; en JC, «referencia nula»). Y un null escrito en una suma de cadenas, que JC no deja a
// propósito: el traductor escribe "", lo que vale en C#. Se comprueba solo: Main devuelve 0.
using System;

class Ficha
{
    string nombre;                                   // sin valor: en C#, null
    public string Titulo { get; set; }               // también
    public static string Global;                     // también (en JC, una propiedad estática)
    string vacio = "";
    public bool NombreNulo() { return nombre == null; }
    public bool VacioNoNulo() { return vacio != null && vacio == ""; }
    public void Pon(string n) { nombre = n; }
    public string Nombre() { return nombre; }
}

class Programa
{
    static string Nada() { return null; }

    static int Cual(string s)
    {
        switch (s)
        {
            case "a": return 1;
            case "": return 2;
            default: return 3;
        }
    }

    static int Main()
    {
        var f = new Ficha();
        if (!f.NombreNulo()) return 1;
        if (f.Titulo != null) return 2;
        if (Ficha.Global != null) return 3;
        if (!f.VacioNoNulo()) return 4;
        string s = null;
        if (s != null) return 5;
        if (s == "") return 6;
        string t = default(string);
        if (t != null || s != t) return 7;
        string u = (string) null;
        if (u != null) return 8;
        if ("x" + s + "y" != "xy") return 9;
        if ("x" + null + (string) null + (object) null != "x") return 16;   // el null escrito, en JC, ""
        if (Cual(s) != 3 || Cual("") != 2 || Cual("a") != 1) return 10;
        if (!string.IsNullOrEmpty(s) || !string.IsNullOrEmpty("") || string.IsNullOrEmpty("a")) return 11;
        f.Pon("pepe");
        if (f.NombreNulo() || f.Nombre() != "pepe") return 12;
        f.Pon(Nada());
        if (!f.NombreNulo()) return 13;
        bool lanzo = false;
        try
        {
            if (s.Length > 0) return 14;
        }
        catch (Exception)
        {
            lanzo = true;
        }
        if (!lanzo) return 15;
        return 0;
    }
}
