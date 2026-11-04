// Ocultar un miembro del padre (`new`, H8, tema 3): en JC todo método es virtual, y lo que se llama igual en una hija sobrescribe. El que
// oculta lleva otro nombre, y Roslyn dice a cuál va cada sitio. Se comprueba solo.
using System;

class Base
{
    public int campo = 1;
    public const int Tres = 3;
    public static int Est = 7;

    public int Prop {
        get { return campo; }
        set { campo = value; }
    }

    public string Id ()
    {
        return "Base";
    }

    public virtual string Vir ()
    {
        return "BaseV";
    }

    public string Llama ()
    {
        return Id () + Vir ();
    }
}

class Hija : Base
{
    public new int campo = 10;
    public new const int Tres = 4;
    public new static int Est = 8;

    public new int Prop {
        get { return campo + 1; }
        set { campo = value; }
    }

    public new string Id ()
    {
        return "Hija";
    }

    // otra cadena de virtuales: no sobrescribe el del padre
    public new virtual string Vir ()
    {
        return "HijaV";
    }

    public string DeArriba ()
    {
        return base.Id () + base.Prop + base.campo + Base.Tres + Id () + Prop + campo + Tres;
    }
}

class Nieta : Hija
{
    public override string Vir ()
    {
        return "NietaV";
    }
}

// sin `new`, C# sólo avisa
class SinNew : Base
{
    public string Id ()
    {
        return "SinNew";
    }
}

interface IQ
{
    string Id ();
}

// lo que oculta implementa la interfaz
class ConInterfaz : Base, IQ
{
    public new string Id ()
    {
        return "ConInterfaz";
    }
}

// privado: desde fuera se ve el del padre
class Privada : Base
{
    new string Id ()
    {
        return "Privada";
    }

    public string Mio ()
    {
        return Id ();
    }
}

// un campo que oculta una propiedad, y un tipo anidado que no se oculta
class Padre2
{
    public int Valor = 3;

    public class Anidada
    {
    }
}

class Hija2 : Padre2
{
    public new int Valor {
        get { return 9; }
    }

    public new int Anidada { get; set; }
}

// dos propiedades que sólo se diferencian en la primera letra: en JC, los dos `getP`
class Letra
{
    public int p = 1;

    public int P {
        get { return p + 1; }
    }
}

class Prueba
{
    public static int Main ()
    {
        Letra l = new Letra ();
        if (l.p != 1 || l.P != 2)
            return 12;

        Hija h = new Hija ();
        Base b = h;
        if (h.Id () != "Hija" || b.Id () != "Base")
            return 1;
        if (h.Vir () != "HijaV" || b.Vir () != "BaseV" || b.Llama () != "BaseBaseV")
            return 2;
        if (h.campo != 10 || b.campo != 1 || h.Prop != 11 || b.Prop != 1)
            return 3;
        if (Hija.Tres != 4 || Base.Tres != 3 || Hija.Est != 8 || Base.Est != 7)
            return 4;
        if (h.DeArriba () != "Base113Hija11104")
            return 5;

        Nieta n = new Nieta ();
        Hija hn = n;
        Base bn = n;
        if (hn.Vir () != "NietaV" || bn.Vir () != "BaseV")
            return 6;

        SinNew s = new SinNew ();
        Base bs = s;
        if (s.Id () != "SinNew" || bs.Id () != "Base")
            return 7;

        ConInterfaz c = new ConInterfaz ();
        Base bc = c;
        IQ q = c;
        if (c.Id () != "ConInterfaz" || bc.Id () != "Base" || q.Id () != "ConInterfaz")
            return 8;

        Privada p = new Privada ();
        if (p.Id () != "Base" || p.Mio () != "Privada")
            return 9;

        Hija2 h2 = new Hija2 ();
        Padre2 p2 = h2;
        if (h2.Valor != 9 || p2.Valor != 3)
            return 10;
        h2.Anidada = 5;
        if (h2.Anidada != 5)
            return 11;
        return 0;
    }
}
