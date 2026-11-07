// Los errores de la VM con su tipo (H7.5): NullReferenceException, IndexOutOfRangeException, DivideByZeroException e
// InvalidCastException son las clases de Core (NullReferenceError, IndexError, ArithmeticError, CastError); una lanzada a
// mano lleva el mensaje de .NET; y el filtro `catch … when (c)`, que relanza si no se cumple. Se comprueba solo: Main
// devuelve 0.
using System;

class Animal
{
    public string Nombre = "animal";
}

class Perro : Animal
{
}

class ErroresVm
{
    static int cero = 0;

    static string Divide ()
    {
        try {
            int r = 10 / cero;
            return "nada " + r;
        } catch (IndexOutOfRangeException) {
            return "índice";
        } catch (DivideByZeroException) {
            return "división";
        }
    }

    static string Nulo ()
    {
        Animal a = null;
        try {
            return a.Nombre;
        } catch (NullReferenceException) {
            return "null";
        }
    }

    static string Indice ()
    {
        int[] v = new int[2];
        try {
            return "nada " + v[cero + 5];
        } catch (IndexOutOfRangeException) {
            return "índice";
        }
    }

    static string Conversion ()
    {
        object o = new Animal ();
        try {
            Perro p = (Perro) o;
            return "nada " + p.Nombre;
        } catch (InvalidCastException) {
            return "conversión";
        }
    }

    static string AMano ()
    {
        try {
            throw new NullReferenceException ();
        } catch (NullReferenceException e) {
            return e.Message;
        }
    }

    static string Filtro (int x)
    {
        try {
            int r = 10 / cero;
            return "nada " + r;
        } catch (ArithmeticException) when (x > 0) {
            return "positivo";
        }
    }

    public static int Main ()
    {
        if (Divide () != "división")
            return 1;
        if (Nulo () != "null")
            return 2;
        if (Indice () != "índice")
            return 3;
        if (Conversion () != "conversión")
            return 4;
        if (AMano () != "Object reference not set to an instance of an object.")
            return 5;
        if (Filtro (1) != "positivo")
            return 6;
        try {
            Filtro (-1);
            return 7;
        } catch (DivideByZeroException) {
        }
        Console.WriteLine ("ok");
        return 0;
    }
}
