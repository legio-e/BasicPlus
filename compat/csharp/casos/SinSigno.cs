// Caso del traductor C# -> JC (V8/H6, 2-nov): uint, un long con su valor (0 … 4294967295). La máscara donde C# da la
// vuelta (+, -, *, << y ~ entre uint, y ++, --, +=…), la cuenta de un desplazamiento con sus 5 bits, y lo demás tal
// cual: comparar, /, %, >>, y mezclarlo con un int (C# lo sube a long). Las conversiones (de un int, sus 32 bits; a un
// int, los de abajo; a uno estrecho, sus bits de abajo), un array, un índice, su caja (un Long), un switch, un uint? y
// escribirlo. Se comprueba solo: Main devuelve 0.
using System;

class Programa
{
    static uint Doble(uint x)
    {
        return x * 2;
    }

    static int Main()
    {
        uint a = uint.MaxValue;
        a++;
        if (a != 0) return 1;
        uint b = 0;
        b--;
        if (b != 4294967295) return 2;
        uint c = 3000000000;
        uint d = c + c;
        if (d != 1705032704) return 3;
        uint e = 100000;
        if (e * e != 1410065408) return 4;
        if (Doble(c) != 1705032704) return 5;

        // los desplazamientos: la cuenta con sus 5 bits, como C#
        uint g = 1;
        g = g << 31;
        if (g != 2147483648) return 6;
        int n = 32;
        uint h = 1;
        h = h << n;
        if (h != 1) return 7;
        uint m = 4294967295;
        if ((m >> 28) != 15 || (m >> n) != m) return 8;
        uint z = 0;
        if (~z != uint.MaxValue) return 9;

        // /, % y las comparaciones, tal cual; con un int, C# lo sube a long
        if (m / 2 != 2147483647 || m % 10 != 5) return 10;
        int menos = -1;
        if (m < menos || m == menos) return 11;
        long suma = m + menos;
        if (suma != 4294967294L) return 12;

        // las conversiones
        int comoInt = (int) m;
        if (comoInt != -1) return 13;
        uint deInt = (uint) menos;
        if (deInt != 4294967295) return 14;
        long comoLong = m;
        double comoDouble = m;
        if (comoLong != 4294967295L || comoDouble != 4294967295.0) return 15;
        byte by = (byte) m;
        short sh = (short) m;
        uint setenta = 70000;
        ushort us = (ushort) setenta;
        if (by != 255 || sh != -1 || us != 4464) return 16;
        uint cte = (uint) int.MaxValue + 1;
        if (cte != 2147483648) return 17;

        // las asignaciones compuestas, recortadas
        uint s = 4294967295;
        s += 2;
        if (s != 1) return 18;
        s -= 2;
        if (s != 4294967295) return 19;
        s *= 2;
        if (s != 4294967294) return 20;
        s <<= 33;
        if (s != 4294967292) return 21;

        // un array, un índice uint, su caja, un switch y un uint?
        uint[] varios = { 1, 2, 4294967295 };
        uint i = 1;
        if (varios[i] != 2 || varios[2] != uint.MaxValue) return 22;
        uint[] vacio = new uint[3];
        vacio[2] = 7;
        if (vacio[2] != 7 || vacio[0] != 0) return 23;
        object caja = m;
        if ((uint) caja != 4294967295) return 24;
        switch (m)
        {
            case 4294967295:
                break;
            default:
                return 25;
        }
        uint? nu = 5;
        uint? nada = null;
        if (nu != 5 || nada.HasValue || nu > 7) return 26;
        uint total = 0;
        for (uint q = 0; q < 4; q++) total += q;
        if (total != 6) return 27;
        if (m.ToString() != "4294967295") return 28;
        Console.WriteLine("m = " + m);
        Console.WriteLine(m);
        return 0;
    }
}
