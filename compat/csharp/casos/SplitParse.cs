// bool.Parse y string.Split, de la librería (CsSystem; H8, tema 9): «True» y «false» sin mirar mayúsculas ni los espacios
// de los lados, y si no, FormatException; los trozos entre cada separador, vacíos incluidos, y sin separadores, por los
// espacios, como .NET. Se comprueba solo: Main devuelve 0.
using System;

class SplitParse
{
    public static int Main ()
    {
        if (!bool.Parse ("True") || bool.Parse (" false ") || !bool.Parse ("tRUE"))
            return 1;
        try {
            bool.Parse ("si");
            return 2;
        } catch (FormatException) {
        }
        string[] a = "uno,dos,,tres".Split (',');
        if (a.Length != 4 || a[0] != "uno" || a[2] != "" || a[3] != "tres")
            return 3;
        string[] b = "a-b c".Split ('-', ' ');
        if (b.Length != 3 || b[1] != "b" || b[2] != "c")
            return 4;
        string[] c = "uno dos\ttres".Split ();
        if (c.Length != 3 || c[2] != "tres")
            return 5;
        string[] d = "".Split ('a');
        if (d.Length != 1 || d[0] != "")
            return 6;
        Console.WriteLine ("ok");
        return 0;
    }
}
