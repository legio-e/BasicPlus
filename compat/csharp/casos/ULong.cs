// ulong (H8, 5-nov): en JC, un long con sus 64 bits tal cual. Se declara, se pasa, se lee y se escribe; y lo que da los mismos
// bits en un long sale sin nada, también de 2^63 para arriba y dando la vuelta: +, -, *, &, |, ^, ~, <<, ++, --, ==, != y un
// switch. Las conversiones: de un int, con su signo; a un long, los mismos bits; a uno estrecho, sus bits de abajo. Lo que no
// da lo mismo no se traduce: RechazosULong. Se comprueba solo: Main devuelve 0.
using System;

class Caja
{
    public ulong valor;
    public static ulong compartido = 7;
    public const ulong Max = ulong.MaxValue;
    public const ulong Medio = 9223372036854775808;
    public const ulong Cinco = 5;

    public ulong Doble (ulong x)
    {
        return x * 2;
    }

    public ulong Valor {
        get { return valor; }
        set { valor = value; }
    }
}

class M
{
    static ulong Suma (ulong x, ulong y = 3)
    {
        return x + y;
    }

    static ulong Mucho (ulong y = ulong.MaxValue)
    {
        return y;
    }

    static int Grande (ulong x)
    {
        switch (x) {
        case 0:
            return 1;
        case long.MaxValue:
            return 2;
        case long.MaxValue + (ulong) 1:
            return 3;
        case ulong.MaxValue:
            return 4;
        default:
            return 5;
        }
    }

    static int Main ()
    {
        ulong a = 5;
        ulong b = a;
        if (b != 5)
            return 1;
        a = ulong.MaxValue;
        if (a != 18446744073709551615)
            return 2;
        a++;
        if (a != 0)
            return 3;
        a--;
        if (a != ulong.MaxValue)
            return 4;

        // de 2^63 para arriba, y dando la vuelta
        ulong c = 9223372036854775808;
        ulong d = c + c;
        if (d != 0)
            return 5;
        if (c * 2 != 0)
            return 6;
        if (c - 1 != 9223372036854775807)
            return 7;
        if (a + 2 != 1)
            return 8;
        if (a * a != 1)
            return 9;

        // los bits
        if ((a & 255) != 255)
            return 10;
        if ((c | 1) != 9223372036854775809)
            return 11;
        if ((a ^ 1) != 18446744073709551614)
            return 12;
        if (~a != 0)
            return 13;
        if (1UL << 63 != c)
            return 14;
        ulong uno = 1;
        int cuenta = 64;
        if (uno << cuenta != 1)
            return 15;
        if (uno << 63 != c)
            return 16;

        // las asignaciones compuestas
        ulong e = 3;
        e += 4;
        e -= 1;
        e *= 3;
        e &= 31;
        e |= 64;
        e ^= 1;
        e <<= 1;
        if (e != 166)
            return 17;

        // las conversiones
        long l = (long) a;
        if (l != -1)
            return 18;
        ulong f = (ulong) l;
        if (f != ulong.MaxValue)
            return 19;
        int i = -1;
        ulong g = (ulong) i;
        if (g != ulong.MaxValue)
            return 20;
        if ((int) g != -1)
            return 21;
        if ((byte) g != 255)
            return 22;
        if ((sbyte) g != -1)
            return 23;
        if ((short) g != -1)
            return 24;
        if ((ushort) g != 65535)
            return 25;
        if ((uint) g != 4294967295)
            return 26;
        ulong h = 65;
        if ((char) h != 'A')
            return 27;
        uint pequeno = 4000000000;
        ulong grande = pequeno;
        if (grande != 4000000000)
            return 28;
        char letra = 'c';
        ulong res = 0;
        res = res * 16 + letra;
        if (res != 99)
            return 29;

        // en un array, y como índice y tamaño
        ulong[] v = new ulong [2];
        if (v [0] != 0)
            return 30;
        v [1] = ulong.MaxValue;
        if (v [1] + 1 != 0)
            return 31;
        ulong[] w = new ulong [] { 7, 8 };
        ulong uno2 = 1;
        if (w [uno2] != 8)
            return 32;
        int[] t = new int [uno2 + 1];
        if (t.Length != 2)
            return 33;

        // una clase, una llamada, un valor por defecto, un ternario
        var caja = new Caja ();
        if (caja.valor != 0)
            return 34;
        caja.Valor = Caja.Max;
        if (caja.Valor != ulong.MaxValue || caja.Doble (Caja.Medio) != 0)
            return 35;
        Caja.compartido += Caja.Cinco;
        if (Caja.compartido != 12)
            return 36;
        if (Suma (1) != 4 || Suma (1, 2) != 3 || Mucho () != ulong.MaxValue)
            return 37;
        ulong q = i == -1 ? ulong.MaxValue : 5;
        if (q + 1 != 0)
            return 38;

        // un switch, con lo que no cabe en un long
        if (Grande (0) != 1 || Grande (long.MaxValue) != 2 || Grande (long.MaxValue + (ulong) 1) != 3
            || Grande (ulong.MaxValue) != 4 || Grande (7) != 5)
            return 39;

        // lo que C# ya calcula: una comparación entre constantes es un valor
        const bool mayor = ulong.MaxValue > 5;
        if (!mayor)
            return 40;
        double real = 5UL;
        if (real != 5.0)
            return 41;
        return 0;
    }
}
