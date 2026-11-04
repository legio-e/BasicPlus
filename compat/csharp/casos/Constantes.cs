// Una constante de clase con una cuenta (H8, tema 2): JC pide un literal, y C# deja cualquier cuenta constante: el valor que
// calcula. La que no se escribe con un literal (NaN, el mínimo de un entero), una variable que nadie cambia. Se comprueba solo.
using System;

enum Dia { Lunes, Martes }

class Prueba
{
    const int X = 1 + 2;
    const int Y = X * 2;
    const int Negativa = -X;
    const bool B = !true;
    const bool C = X > 2;
    const bool D = !B & C | false;
    const string S = "a" + "b";
    const double Doble = 1.5 * 2;
    const byte Pequena = 2 + 3;
    const char Letra = 'a';
    const long Largo = 1L << 40;
    const Dia Hoy = Dia.Martes;
    public const int Publica = X + 1;
    const double Nada = double.NaN;
    const int Minimo = int.MinValue;

    public static int Main ()
    {
        const int L = X + 1;
        if (X != 3 || Y != 6 || Negativa != -3 || L != 4 || Publica != 4)
            return 1;
        if (B || !C || !D)
            return 2;
        if (S != "ab" || Doble != 3.0 || Pequena != 5 || Letra != 97 || Largo != 1099511627776L)
            return 3;
        if (Hoy != Dia.Martes || Nada == Nada || Minimo != -2147483647 - 1)
            return 4;
        return 0;
    }
}
