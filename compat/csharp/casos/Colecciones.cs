// `ArrayList` es la `List` de Core, y `Hashtable`, la de CsSystem (H8, tema 3): lo que usan los programas. Una lista guarda de todo, en cajas; una tabla busca sus llaves por
// su valor, si son cadenas o cajas. `foreach` sobre una lista, y sobre las `Keys` de una tabla, también por `IDictionary`. Se comprueba solo: Main devuelve 0.
using System;
using System.Collections;

class M
{
    Hashtable tabla;

    M ()
    {
        tabla = new Hashtable ();
    }

    static int Total (ArrayList l)
    {
        int t = 0;
        foreach (int x in l)
            t += x;
        return t;
    }

    public static int Main ()
    {
        ArrayList l = new ArrayList ();
        if (l.Add ("uno") != 0)
            return 1;
        l.Add (5);
        M objeto = new M ();
        l.Add (objeto);
        if (l.Count != 3)
            return 2;
        if ((string) l [0] != "uno" || (int) l [1] != 5 || (M) l [2] != objeto)
            return 3;
        l [1] = 7;
        if ((int) l [1] != 7)
            return 4;

        int n = 0;
        foreach (object o in l)
            n++;
        if (n != 3)
            return 5;

        ArrayList numeros = new ArrayList (2);
        for (int i = 0; i < 20; i++)
            numeros.Add (i);
        if (Total (numeros) != 190)
            return 6;
        numeros.Clear ();
        if (numeros.Count != 0 || Total (numeros) != 0)
            return 7;

        Hashtable h = new Hashtable ();
        h.Add ("A", 10);
        h ["B"] = 20;
        h ["B"] = 21;
        h.Add (5, "cinco");
        if (h.Count != 3 || (int) h ["A"] != 10 || (int) h ["B"] != 21 || (string) h [5] != "cinco")
            return 8;
        if (!h.Contains ("B") || !h.ContainsKey (5) || h.Contains ("Z") || h ["Z"] != null)
            return 9;
        string calculada = "B";
        calculada += "";
        if ((int) h [calculada] != 21)
            return 13;

        Hashtable sinNumeros = new Hashtable ();
        sinNumeros ["A"] = 1;
        sinNumeros ["B"] = 2;
        IDictionary d = sinNumeros;
        string llaves = "";
        foreach (string k in d.Keys)
            llaves += k;
        if (llaves != "AB")
            return 10;
        if (d.Count != 2)
            return 11;

        objeto.tabla.Add ("x", objeto);
        if ((M) objeto.tabla ["x"] != objeto)
            return 12;
        return 0;
    }
}
