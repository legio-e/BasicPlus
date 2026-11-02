// Caso del traductor C# -> JC (V8/H8, tema 1; 2-nov): `++`, `--` y `=` dentro de una expresión, partida en dos. El
// cambio sale a su sentencia —delante, o detrás para un `x++`— y en su sitio queda su valor, o una temporal: en una
// declaración, una asignación, una llamada, un `return`, un `if` (también un else if), un `switch` y la condición de un
// bucle (en cada vuelta, también la última). Con una variable, un campo, un elemento, un int? y un uint. Se comprueba
// solo: Main devuelve 0.
using System;

class Cuenta
{
    int n;
    static int veces;
    public int Siguiente() { return n++; }
    public int Otra() { return ++n; }
    public static int Vez() { return veces++; }
}

class Programa
{
    static object Lee(int i) { return i > 0 ? new object() : null; }
    static int Doble(int x) { return x * 2; }

    static int Main()
    {
        int i = 5;
        int j = i++;
        if (j != 5 || i != 6) return 1;
        int k = ++i;
        if (k != 7 || i != 7) return 2;
        int d = Doble(i++);
        if (d != 14 || i != 8) return 3;
        object o;
        if ((o = Lee(1)) == null) return 4;
        int a, b;
        a = b = 3;
        if (a != 3 || b != 3) return 5;
        int c = b += 2;
        if (c != 5 || b != 5) return 6;

        // la condición de un bucle: lo sacado, en cada vuelta, también en la última
        int n = 3, vueltas = 0;
        while (n-- > 0) vueltas++;
        if (vueltas != 3 || n != -1) return 7;
        int m = 2, hechas = 0;
        do { hechas++; } while (m-- > 0);
        if (hechas != 3 || m != -1) return 8;
        int total = 0;
        for (int q = 0; q++ < 3; ) total += q;
        if (total != 6) return 9;
        int p = 5, contadas = 0;
        do
        {
            if (p == 3) continue;                     // en C#, a la condición: --p también se hace
            contadas++;
        } while (--p > 0);
        if (contadas != 4 || p != 0) return 19;

        // un campo (en un return: con una temporal), un elemento, un else if, un switch
        var cuenta = new Cuenta();
        if (cuenta.Siguiente() != 0 || cuenta.Otra() != 2) return 10;
        if (Cuenta.Vez() != 0 || Cuenta.Vez() != 1) return 11;
        int[] arr = { 10, 20 };
        int v = arr[1]++;
        if (v != 20 || arr[1] != 21) return 12;
        int r = 0, s = 3;
        if (s > 9) return 13;
        else if ((r = s--) == 3) { if (s != 2) return 14; }
        else return 15;
        int sw = 1, salida = 0;
        switch (sw++)
        {
            case 1: salida = 10; break;
            default: salida = 20; break;
        }
        if (salida != 10 || sw != 2) return 16;

        // un int? y un uint
        int? nul = 4;
        int? viejo = nul++;
        if (viejo != 4 || nul != 5) return 17;
        uint u = 4294967295;
        uint previo = u++;
        if (previo != 4294967295 || u != 0) return 18;
        return 0;
    }
}
