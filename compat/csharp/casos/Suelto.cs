// Lo suelto de la biblioteca (H8, tema 1): el ToString de un double, `is` con una cadena delante, sizeof de un tipo de C#,
// escribir en un TextWriter y string.Concat. Se comprueba solo.
using System;
using System.IO;

class Prueba
{
    static int llamadas = 0;

    static string Cuenta ()
    {
        llamadas++;
        return "c";
    }

    public static int Main ()
    {
        double d = 1.5;
        float f = 2.5f;
        if (d.ToString () != "1.5" || d.ToString ("R") != "1.5")
            return 1;
        if (f.ToString () != "2.5")
            return 2;

        string s = "b";
        string n = null;
        if (!("a" is String) || !("a" is Object))
            return 3;
        if (!(s is string) || n is string || n is object)
            return 4;

        const int tam = sizeof (int);
        if (tam != 4 || sizeof (long) != 8 || sizeof (byte) != 1 || sizeof (char) != 2)
            return 5;

        // no escribe en ningún sitio, pero lo que se le pasa se calcula
        StreamWriter.Null.WriteLine (Cuenta ());
        TextWriter.Null.Write (5);
        if (llamadas != 1)
            return 6;

        string texto;
        using (StringWriter w = new StringWriter ()) {
            w.Write ("a");
            w.Write (1);
            w.Write ('c');
            w.WriteLine ("b{0}", 2);
            w.WriteLine ();
            texto = w.ToString ();
        }
        if (texto != "a1cb2" + Environment.NewLine + Environment.NewLine)
            return 7;

        if (String.Concat ("a", "b") != "ab" || String.Concat ("a", "b", "c") != "abc")
            return 8;
        if (string.Concat ("a", n) != "a" || string.Concat (n, s, n) != "b")
            return 9;
        if (string.Concat (1, "b") != "1b" || string.Concat ("a", Cuenta (), "d", "e", "f") != "acdef")
            return 10;

        return 0;
    }
}
