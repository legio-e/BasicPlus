// Caso del traductor C# -> JC (V8/H6): lo que JC escribe de otra forma y el traductor reescribe sin cambiar lo que se
// calcula: la clase padre delante de la hija, una clase que se llama como una de Core, el valor por defecto que calcula
// C#, y `throw;` cuando el catch ha cambiado su variable. Se comprueba solo: Main devuelve 0.
using System;

class Hija : Padre                    // la hija antes que el padre: en JC, detrás
{
    public override int Valor() { return base.Valor() + 1; }
}

class Padre
{
    public virtual int Valor() { return 1; }
}

class List                            // se llama como una de Core: en JC, List_
{
    public int Cuantos = 3;
}

class MiError : Exception
{
    public MiError(string m) : base(m) { }
}

class Programa
{
    const int Diez = 10;

    static int ConDefecto(int a = Diez * 2 + 1, bool b = 1 > 2, double d = 1 / 4.0)
    {
        return a + (b ? 100 : 0) + (int) (d * 4);
    }

    static int Relanza()
    {
        try
        {
            try
            {
                throw new MiError("primera");
            }
            catch (MiError e)
            {
                e = new MiError("segunda");
                throw;                        // relanza la primera, no la que se guardó en e
            }
        }
        catch (MiError e)
        {
            return e.Message == "primera" ? 0 : 1;
        }
    }

    static int Main()
    {
        if (new Hija().Valor() != 2) return 1;
        if (new List().Cuantos != 3) return 2;
        if (ConDefecto() != 22) return 3;
        if (ConDefecto(5, true) != 106) return 4;
        if (Relanza() != 0) return 5;
        Console.WriteLine("bien");
        return 0;
    }
}
