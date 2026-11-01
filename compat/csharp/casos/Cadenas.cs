// Caso del traductor C# -> JC (V8/H6, el escalón 1): las cadenas de .NET. El ToString de un entero y de un bool (True,
// como .NET), string.Format (la suma de sus piezas), string.Join, Substring (con la longitud de .NET), lo que Core ya
// tiene (StartsWith, EndsWith, Contains, IndexOf, Replace, ToUpper, ToLower, Trim), Equals, Console.Out.WriteLine y
// new object(). Se comprueba solo: Main devuelve 0.
using System;

class Programa
{
    static int Main()
    {
        int n = -42;
        long grande = 9000000000L;
        if (n.ToString() != "-42" || grande.ToString() != "9000000000") return 1;
        bool si = true;
        if (si.ToString() != "True" || (!si).ToString() != "False") return 2;
        if (string.Format("{0}{1}", 1, 2) != "12") return 3;
        if (string.Format("x={0}, y={1} {{z}}", 3, "b") != "x=3, y=b {z}") return 4;
        if (string.Join(", ", new string[] { "a", "b", "c" }) != "a, b, c") return 5;
        string s = "  Hola mundo  ";
        string t = s.Trim();
        if (t != "Hola mundo") return 6;
        if (!t.StartsWith("Hola") || !t.EndsWith("mundo") || !t.Contains("a m") || t.Contains("xyz")) return 7;
        if (t.IndexOf("mundo") != 5 || t.IndexOf("zz") != -1) return 8;
        if (t.Replace("mundo", "BP") != "Hola BP" || t.ToUpper() != "HOLA MUNDO" || t.ToLower() != "hola mundo") return 9;
        if (t.Substring(5) != "mundo" || t.Substring(1, 3) != "ola") return 10;
        if (!string.Equals(t, "Hola mundo") || !t.Equals("Hola mundo") || t.Equals("hola")) return 11;
        if (t.ToString() != t) return 12;
        object o = new object();
        if (o == null) return 13;
        Console.Out.WriteLine("cadenas bien");
        return 0;
    }
}
