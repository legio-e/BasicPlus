// Caso del traductor C# -> JC (V8, #761): el ternario con dos clases que sólo comparten una interfaz. C# le da el tipo
// de la rama a la que se convierte la otra: con `(IA)` en una rama, IA. El traductor quita las conversiones hacia arriba
// (no hacen falta), pero ésa la conserva: en una rama del ternario, a una interfaz o a una clase (#797), da el tipo (test-865
// de Mono). Se comprueba solo: Main devuelve 0.
using System;

interface IA { int Valor(); }

class B1 : IA { public int Valor() { return 1; } }
class B2 : IA { public int Valor() { return 2; } }

class P { public virtual int Valor() { return 0; } }
class H1 : P { public override int Valor() { return 1; } }
class H2 : P { public override int Valor() { return 2; } }

class Programa
{
    static IA Elige(bool arg)
    {
        IA r = arg ? new B2() : (IA) new B1();
        return r;
    }

    static int Main()
    {
        if (Elige(true).Valor() != 2) return 1;
        if (Elige(false).Valor() != 1) return 2;
        IA otra = !true ? (IA) new B2() : new B1();
        if (otra.Valor() != 1) return 3;
        P padre = !true ? new H1() : (P) new H2();
        if (padre.Valor() != 2) return 4;
        Console.WriteLine("ternario bien");
        return 0;
    }
}
