// Caso del traductor C# -> JC (V8/H6, el escalón 1): las excepciones de .NET, con la librería CsSystem
// (compat/csharp/lib). Las de la librería, con y sin mensaje (el de .NET); una del programa que hereda de una de ellas,
// con constructor y sin él; `catch` por su tipo, por su padre y por SystemException (el RuntimeError de BP, que atrapa
// también lo que lanza la VM); `is`; `Message` y `ParamName`. Se comprueba solo: Main devuelve 0.
using System;

class MiError : ApplicationException
{
}

class ConDetalle : ArgumentException
{
    public ConDetalle(string m) : base(m, "detalle") { }
}

class Programa
{
    static int Lanza(int que)
    {
        switch (que)
        {
            case 1: throw new ApplicationException();
            case 2: throw new ArgumentNullException("x");
            case 3: throw new InvalidOperationException("estado");
            case 4: throw new MiError();
            case 5: throw new ConDetalle("malo");
            case 6: throw new NotSupportedException();
            case 7: throw new SystemException();
        }
        return 0;
    }

    static int Main()
    {
        try { Lanza(1); return 1; }
        catch (ApplicationException e)
        {
            if (e.Message != "Error in the application.") return 2;
        }
        try { Lanza(2); return 3; }
        catch (ArgumentException e)
        {
            if (!(e is ArgumentNullException)) return 4;
            if (e.ParamName != "x") return 5;
            if (e.Message != "Value cannot be null. (Parameter 'x')") return 6;
        }
        try { Lanza(3); return 7; }
        catch (SystemException e)
        {
            if (e.Message != "estado") return 8;
        }
        try { Lanza(4); return 9; }
        catch (Exception e)
        {
            if (!(e is MiError) || e.Message != "Error in the application.") return 10;
        }
        try { Lanza(5); return 11; }
        catch (ArgumentException e)
        {
            if (e.Message != "malo (Parameter 'detalle')") return 12;
        }
        try { Lanza(6); return 13; }
        catch (NotSupportedException e)
        {
            if (e.Message != "Specified method is not supported.") return 14;
        }
        try { Lanza(7); return 15; }
        catch (SystemException e)
        {
            if (e.Message != "System error.") return 16;
        }
        // lo que lanza la VM es un SystemException, como en .NET
        try
        {
            object o = null;
            string s = o.ToString();
            return 17;
        }
        catch (SystemException)
        {
        }
        Console.WriteLine("excepciones bien");
        return 0;
    }
}
