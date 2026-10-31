// Caso del traductor C# -> JC (V8/H6): un inicializador de objeto en una declaración, los métodos de extensión,
// `using` (con su Dispose, también si salta una excepción), `lock`, `default`, las cadenas `$"…"` y `unchecked`. Se
// comprueba solo: Main devuelve 0.
using System;

class Punto
{
    public int X;
    public int Y { get; set; }
}

static class Extensiones
{
    public static int Doble(this int x) { return x * 2; }
    public static string Saluda(this Punto p, string quien) { return "hola " + quien + " en " + p.X; }
}

class Recurso : IDisposable
{
    public static int Cerrados = 0;
    public void Dispose() { Cerrados++; }
}

class Programa
{
    static int Main()
    {
        Punto p = new Punto { X = 3, Y = 4 };
        if (p.X != 3 || p.Y != 4) return 1;
        if (5.Doble() != 10) return 2;
        if (Extensiones.Doble(6) != 12) return 3;
        if (p.Saluda("ana") != "hola ana en 3") return 4;

        using (Recurso r = new Recurso())
        {
            p.X = 7;
        }
        if (Recurso.Cerrados != 1) return 5;
        try
        {
            using (new Recurso())
            {
                throw new Exception("dentro");
            }
        }
        catch (Exception)
        {
        }
        if (Recurso.Cerrados != 2) return 6;

        int dentro = 0;
        lock (p)
        {
            dentro = 1;
        }
        if (dentro != 1) return 7;

        int d = default(int);
        bool b = default;
        if (d != 0 || b) return 8;
        Punto nada = default(Punto);
        if (nada != null) return 9;

        string s = $"x={p.X} y={p.Y}";
        if (s != "x=7 y=4") return 10;
        string t = $"{p.X + 1}{"!"}";
        if (t != "8!") return 11;

        int grande = unchecked(2147483647 + d + 1);
        if (grande != -2147483648) return 12;
        Console.WriteLine(s + " " + t);
        return 0;
    }
}
