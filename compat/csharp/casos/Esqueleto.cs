// Caso del traductor C# -> JC (V8/H6): el esqueleto. Lo que se copia (las clases, la herencia, los métodos virtuales,
// las propiedades, if, while, do, el for de JC, try, throw), Console, static y this. Se comprueba solo: Main devuelve 0.
using System;

namespace Casos
{
    class Animal
    {
        public string Nombre;                     // un campo público: en JC, una propiedad
        protected int patas = 4;
        private static int cuantos = 0;
        public static int Total { get { return cuantos; } }

        public Animal(string nombre)
        {
            Nombre = nombre;
            cuantos++;
        }

        public virtual string Habla() { return "..."; }
        public int Patas() => patas;
        public override string ToString() { return Nombre + "(" + Patas() + ")"; }
    }

    class Perro : Animal
    {
        public Perro(string nombre) : base(nombre) { }
        public Perro() : this("sin nombre") { }
        public override string Habla() { return "guau"; }
    }

    class Pajaro : Animal
    {
        public Pajaro(string n) : base(n) { patas = 2; }
        public override string Habla() => "pio";
    }

    class MiError : Exception { }

    class ConMensaje : Exception
    {
        public ConMensaje(string m) : base(m) { }
    }

    static class Cuentas
    {
        public const int Base = 10;
        public static int Doble(int x) { return x * 2; }
        public static int Suma(int a, int b = 10) { return a + b; }
    }

    class Programa
    {
        static int Comprueba(int dado, int esperado, int codigo)
        {
            if (dado != esperado)
            {
                Console.WriteLine("FALLO {0}: {1} en vez de {2}", codigo, dado, esperado);
                return codigo;
            }
            return 0;
        }

        static int Main()
        {
            Animal a = new Perro("Rex");
            Animal b = new Pajaro("Piolin");
            Animal c = new Perro();
            Console.WriteLine(a.Habla() + " " + b.Habla() + " " + c.Nombre);
            Console.WriteLine("{0} y {1}", a, b);
            Console.Write("sin salto, ");
            Console.WriteLine();
            int r = Comprueba(Animal.Total, 3, 1);
            if (r != 0) return r;
            r = Comprueba(b.Patas(), 2, 2);
            if (r != 0) return r;
            r = Comprueba(Cuentas.Doble(Cuentas.Suma(1)) + Cuentas.Base, 32, 3);
            if (r != 0) return r;

            int n = 0;
            for (int i = 1; i <= 10; i++) n += i;
            for (int i = 10; i >= 1; i -= 3) n -= 1;      // 10, 7, 4 y 1: cuatro vueltas
            r = Comprueba(n, 51, 4);
            if (r != 0) return r;

            int k = 0;
            while (k < 5) k++;
            do { k += 10; } while (k < 30);
            r = Comprueba(k, 35, 5);
            if (r != 0) return r;

            {
                int dentro = 7;                           // un bloque suelto
                k += dentro;
            }
            r = Comprueba(k, 42, 6);
            if (r != 0) return r;

            string traza = "";
            try
            {
                try
                {
                    throw new MiError();
                }
                catch (MiError)
                {
                    traza += "a";
                    throw;
                }
                finally
                {
                    traza += "b";
                }
            }
            catch (Exception e)
            {
                traza += "c:" + e.Message;
            }
            Console.WriteLine(traza);
            if (traza != "abc:Exception of type 'Casos.MiError' was thrown.") return 7;

            try
            {
                throw new ConMensaje("hola");
            }
            catch (ConMensaje e)
            {
                if (e.Message != "hola") return 8;
            }

            int x = 6;
            x *= 7;
            x = x > 40 ? x - 1 : 0;
            x = ~x + 1;                                   // -41
            r = Comprueba(x, -41, 9);
            if (r != 0) return r;
            return 0;
        }
    }
}
