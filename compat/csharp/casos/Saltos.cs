// `goto` (H8): JC no tiene etiquetas. El bloque se parte por ellas, y un bucle con nombre y una variable dicen por dónde sigue; hacia
// delante, hacia atrás y cuando se cruzan; desde un bucle, desde un try; dentro de un switch, y goto case. Se comprueba solo.
using System;

class Prueba
{
    static int Adelante (int n)
    {
        int r = 1;
        if (n > 0)
            goto fin;
        r = 2;
    fin:
        return r;
    }

    static int Atras (int n)
    {
        int i = 0, t = 0;
    otra:
        t += i;
        i++;
        if (i < n)
            goto otra;
        return t;
    }

    // se cruzan: `goto a` salta por encima de `b`, y `goto b` vuelve
    static int Cruzan ()
    {
        int i = 0;
        goto a;
    b:
        if (++ i > 1)
            throw new Exception ("infloop");
        return i + 10;
    a:
        goto b;
    }

    // una local sin valor, declarada en un trozo y usada en otro
    static int Declarada ()
    {
        goto x;
        int a;
    x:
        a = 3;
        return a;
    }

    static int DesdeBucles ()
    {
        int t = 0;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                t++;
                if (i == 1 && j == 1)
                    goto fuera;
            }
        t = 100;
    fuera:
        return t;
    }

    // su finally corre, y el de un catch también sale
    static int ConFinally ()
    {
        int r = 0;
        try {
            goto hecho;
        } finally {
            r = 7;
        }
    hecho:
        return r;
    }

    static int DesdeCatch ()
    {
        int r = 1;
        try {
            throw new Exception ("x");
        } catch (Exception) {
            goto salta;
        }
        r = 2;
    salta:
        return r;
    }

    // un bloque con etiquetas dentro de un bucle: su break y su continue son del bucle de fuera
    static int EnUnBucle ()
    {
        int i = 0;
        while (true) {
            {
                goto a;
            a:
                i += 3;
                break;
            }
        }
        return i;
    }

    static int ConContinue ()
    {
        int t = 0;
        for (int i = 0; i < 4; i++) {
            {
                if (i == 1)
                    goto b;
                t += 1;
                continue;
            b:
                t += 10;
            }
            t += 100;
        }
        return t;
    }

    // dos bloques, cada uno con su etiqueta, y el goto de dentro a una de fuera
    static int Anidados ()
    {
        int r = 0;
        {
            goto a;
        a:
            r += 1;
            {
                goto b;
            b:
                r += 10;
                goto fuera;
            }
        }
    fuera:
        return r + 100;
    }

    // en una sección de un switch: una etiqueta, y un goto desde otra sección
    static int Etiqueta (int a)
    {
        int res = 0;
        switch (a) {
        case 1:
            res += 1;
        label:
            res += 3;
            break;
        case 2:
            res += 2;
            goto label;
        }
        return res;
    }

    static int GotoCase (int x)
    {
        int r = 0;
        switch (x) {
        case 1:
            r += 1;
            goto case 3;
        case 2:
            r += 2;
            goto default;
        case 3:
            r += 4;
            break;
        default:
            r += 8;
            break;
        }
        return r;
    }

    static string PorCadena (string s)
    {
        switch (s) {
        case "a":
            goto case "b";
        case "b":
            return "B";
        default:
            return "?";
        }
    }

    // un bucle dentro de una sección, un continue del bucle de fuera, y el break del switch
    static int Mezcla (int x)
    {
        int r = 0;
        while (r < 100) {
            switch (x) {
            case 1:
                for (int i = 0; i < 3; i++) {
                    if (i == 1)
                        break;
                    r += 1;
                }
                x = 2;
                goto case 2;
            case 2:
                r += 10;
                if (r < 30)
                    continue;
                break;
            default:
                r = 1000;
                break;
            }
            r += 100;
        }
        return r;
    }

    public static int Main ()
    {
        if (Adelante (1) != 1 || Adelante (0) != 2)
            return 1;
        if (Atras (4) != 6)
            return 2;
        if (Cruzan () != 11)
            return 3;
        if (Declarada () != 3)
            return 4;
        if (DesdeBucles () != 5)
            return 5;
        if (ConFinally () != 7 || DesdeCatch () != 1)
            return 6;
        if (EnUnBucle () != 3)
            return 7;
        if (ConContinue () != 113)
            return 8;
        if (Anidados () != 111)
            return 9;
        if (Etiqueta (1) != 4 || Etiqueta (2) != 5 || Etiqueta (3) != 0)
            return 10;
        if (GotoCase (1) != 5 || GotoCase (2) != 10 || GotoCase (3) != 4 || GotoCase (9) != 8)
            return 11;
        if (PorCadena ("a") != "B" || PorCadena ("b") != "B" || PorCadena ("c") != "?")
            return 12;
        if (Mezcla (1) != 131 || Mezcla (5) != 1100)
            return 13;
        return 0;
    }
}
