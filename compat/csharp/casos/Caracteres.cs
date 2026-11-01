// Caso del traductor C# -> JC (V8/H6, el escalón 1): char, que en JC es un entero (Eduardo, 1-nov). El literal y las
// cuentas (es un número); como texto, su carácter: al sumarlo a una cadena, al escribirlo, en un formato, en $"…" y en
// ToString(); s[i], su código; (char) n, los 16 bits de abajo; el switch, los arrays, char.IsDigit y los suyos, y un
// valor por defecto. Se comprueba solo: Main devuelve 0.
using System;

class Programa
{
    static int Siguiente(char c = 'a')
    {
        return c + 1;
    }

    static int Main()
    {
        char a = 'A';
        if (a + 1 != 66 || (int) a != 65) return 1;
        char b = (char) (a + 1);
        if (b != 'B') return 2;
        if ("x" + a + b != "xAB" || a.ToString() != "A" || char.ToString(b) != "B") return 3;
        if (string.Format("{0}-{1}", a, 7) != "A-7" || $"<{b}>" != "<B>") return 4;
        string s = "hola";
        if (s[0] != 'h' || s[3] - 'a' != 0) return 5;
        int n = 0;
        for (int i = 0; i < s.Length; i++)
        {
            switch (s[i])
            {
                case 'o':
                case 'a': n++; break;
                default: break;
            }
        }
        if (n != 2) return 6;
        int grande = 65601;
        if ((char) grande != 'A') return 7;
        char[] letras = new char[] { 'x', 'y' };
        char[] vacio = new char[3];
        if (letras[1] != 'y' || vacio[0] != '\0' || vacio.Length != 3) return 8;
        if (!char.IsDigit('7') || char.IsDigit('x') || !char.IsLetter('q') || !char.IsWhiteSpace(' ')) return 9;
        if (char.ToUpper('q') != 'Q' || char.ToLower('Q') != 'q') return 10;
        if (Siguiente() != 'b' || Siguiente('y') != 'z') return 11;
        Console.WriteLine(a);
        Console.WriteLine("caracteres bien");
        return 0;
    }
}
