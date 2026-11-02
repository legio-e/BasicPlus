// Caso del traductor C# -> JC (V8/H6, 2-nov): int? y los demás T? (Eduardo: «int? es Integer (la clase)»), la caja de
// BP de su T, o null. Los operadores de C# con ellos, con las funciones Nullable_* de CsSystem: con un null, la
// aritmética da null y <, >, <= y >=, false; == compara los valores, y dos null son iguales; un null se escribe vacío.
// Y `??`, `x.Value`, `HasValue`, `GetValueOrDefault`, `ToString`, `++` y `+=`, el bool? con la lógica de tres valores,
// las conversiones (entre ellos, y con object) y un array. Se comprueba solo: Main devuelve 0.
using System;

class Contador
{
    public int? Cuenta { get; set; }
}

class Programa
{
    static int? Doble(int? x)
    {
        return x * 2;
    }

    static int Main()
    {
        int? a = null;
        int? b = 5;
        if (a != null || b == null) return 1;
        if (a.HasValue || !b.HasValue) return 2;
        if (b.Value != 5 || (int) b != 5) return 3;
        if (a.GetValueOrDefault() != 0 || a.GetValueOrDefault(7) != 7 || b.GetValueOrDefault(7) != 5) return 4;
        try
        {
            int z = (int) a;
            return 5;
        }
        catch (InvalidOperationException)
        {
        }

        // la aritmética: con un null, null
        int? c = a + b;
        if (c != null) return 6;
        c = b + 1;
        if (c != 6) return 7;
        if (Doble(b) != 10 || Doble(null) != null) return 8;
        int? r = b % 3;
        if (r != 2) return 9;
        int? neg = -b;
        if (neg != -5) return 10;

        // == compara los valores, y dos null son iguales; <, >, <= y >=, con un null, false
        int? otro = 5;
        if (b != otro) return 11;
        int? nada = null;
        if (a != nada) return 12;
        if (a < b || a > b || a <= b || a >= b) return 13;
        if (!(b > 4) || b < 5) return 14;

        // ?? y las cuentas mezcladas
        int d = a ?? 3;
        if (d != 3) return 15;
        int e = b ?? 3;
        if (e != 5) return 16;
        long? l = b;
        long? m = b * 2L;
        if (l != 5 || m != 10) return 17;
        double? x = 2.5;
        double? y = x * 2;
        if (y != 5.0) return 18;
        int? grande = 300;
        byte? bb = (byte?) grande;
        if (bb != 44) return 19;

        // ++, += y |= sobre un T?; el bool?, con la lógica de tres valores de C#
        var k = new Contador();
        k.Cuenta++;
        if (k.Cuenta != null) return 20;
        k.Cuenta = 0;
        k.Cuenta++;
        k.Cuenta += 10;
        if (k.Cuenta != 11) return 21;
        bool? t = null;
        t |= true;
        if (t != true) return 22;
        bool? f = null;
        bool? fy = f & false;
        if (fy != false) return 23;
        bool? fo = f | false;
        if (fo != null) return 24;

        // con object, la misma caja; ToString y lo que se escribe: un null, vacío
        object caja = b;
        if ((int) caja != 5) return 25;
        int? vuelta = (int?) caja;
        if (vuelta != 5) return 26;
        object nula = a;
        if (nula != null) return 27;
        if (b.ToString() != "5" || a.ToString() != "") return 28;
        Console.WriteLine("a=[" + a + "] b=[" + b + "]");
        Console.WriteLine(a);
        Console.WriteLine(b);

        int?[] varios = { null, 3, 4 };
        if (varios[0] != null || varios[2] != 4) return 29;
        int i = 7;
        if (i == null) return 30;                     // un int nunca es null: siempre false
        return 0;
    }
}
