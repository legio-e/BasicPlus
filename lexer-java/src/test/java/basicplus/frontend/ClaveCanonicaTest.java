package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import basicplus.frontend.Symbol.ClassSymbol;
import basicplus.frontend.Symbol.ClaveCanonica;
import basicplus.frontend.Symbol.EnumSymbol;
import basicplus.frontend.Symbol.EventSymbol;
import basicplus.frontend.Symbol.FunctionSymbol;
import basicplus.frontend.Symbol.ParamSymbol;
import basicplus.frontend.BpType.PrimitiveType;

/**
 * V7/H2 (#523, regla R3) — LA CLAVE CANÓNICA es lo que un miembro exporta y lo que
 * el consumidor pide: si el dueño y el consumidor la calculan distinto, el enlace
 * tardío rechaza lo que Java aceptaría (o peor, casa lo que no debería). Esta tabla
 * fija la codificación de una vez, con los ejemplos escritos en V7_IDEAS §H2.7:
 * {@code toString()s}, {@code compareTo(LObject;)i}, {@code m(LLib.Base;)i},
 * {@code getP()i}, {@code setP(i)V}, {@code __cls_new_Box(i)}, {@code n:i}, {@code cambio(i)}.
 *
 * Lo que la distingue de {@code overloadMangle} y por lo que existe: la firma va
 * SIEMPRE completa (también en la cabeza de una cadena de sobrecargas), lleva el
 * RETORNO, cualifica las clases declaradas por su módulo (y deja simples las del
 * lenguaje), cualifica los enums, y un tipo sin resolver es un ERROR, nunca «?».
 */
class ClaveCanonicaTest {

    private static FunctionSymbol fn(String nombre, ClassSymbol dueno, BpType ret, BpType... params) {
        FunctionSymbol f = new FunctionSymbol(nombre, true, false, false, dueno, null);
        f.returnType = ret;
        int i = 0;
        for (BpType t : params) { ParamSymbol p = new ParamSymbol("p" + (i++), 0, 0); p.type = t; f.params.add(p); }
        return f;
    }
    private static ClassSymbol clase(String nombre) { return new ClassSymbol(nombre, true, null, null, 0, 0); }
    private static ClassSymbol externa(String lib, String mod, String nombre) {
        ClassSymbol c = clase(nombre); c.isExternal = true; c.externalLibrary = lib; c.externalModule = mod; return c;
    }
    private static ClassSymbol delLenguaje(String nombre) { ClassSymbol c = clase(nombre); c.isBuiltin = true; return c; }

    @Test @DisplayName("Los ejemplos del diseño, tal cual: retorno siempre, V para void, ctor sin retorno")
    void ejemplosDelDiseno() {
        ClassSymbol object = delLenguaje("Object");
        assertEquals("toString()s", ClaveCanonica.metodo(fn("toString", object, PrimitiveType.STRING), "Core"));
        assertEquals("compareTo(LObject;)i",
                ClaveCanonica.metodo(fn("compareTo", object, PrimitiveType.INTEGER, new BpType.ClassType(object)), "Core"));
        assertEquals("getP()i", ClaveCanonica.metodo(fn("getP", clase("Base"), PrimitiveType.INTEGER), "Lib"));
        assertEquals("setP(i)V", ClaveCanonica.metodo(fn("setP", clase("Base"), null, PrimitiveType.INTEGER), "Lib"));
        FunctionSymbol ctor = fn("__cls_new_Box", clase("Box"), null, PrimitiveType.INTEGER);
        ctor.isConstructor = true;
        assertEquals("__cls_new_Box(i)", ClaveCanonica.metodo(ctor, "Lib"));
        assertEquals("n:i", ClaveCanonica.campo("n", PrimitiveType.INTEGER, "Lib"));
        EventSymbol ev = new EventSymbol("cambio", clase("Base"), 0, 0);
        ParamSymbol p = new ParamSymbol("v", 0, 0); p.type = PrimitiveType.INTEGER; ev.params.add(p);
        assertEquals("cambio(i)", ClaveCanonica.evento(ev, "Lib"));
    }

    @Test @DisplayName("Una clase DECLARADA va cualificada por su módulo; una del lenguaje, simple; una externa, con su library")
    void clasesCualificadas() {
        ClassSymbol base = clase("Base");                       // declarada en el módulo que compila
        assertEquals("m(LLib.Base;)i", ClaveCanonica.metodo(fn("m", base, PrimitiveType.INTEGER, new BpType.ClassType(base)), "Lib"));
        ClassSymbol lista = externa("", "Core", "List");          // importada de Core
        assertEquals("addAll(LCore.List;)V", ClaveCanonica.metodo(fn("addAll", base, null, new BpType.ClassType(lista)), "Collections"));
        ClassSymbol dao = externa("com.acme", "Orm", "Dao");      // importada con library
        assertEquals("Lcom.acme.Orm.Dao;", ClaveCanonica.tipo(new BpType.ClassType(dao), "App"));
        assertEquals("LThread;", ClaveCanonica.tipo(new BpType.ClassType(delLenguaje("Thread")), "App"));
        // el mismo nombre en dos módulos son DOS claves distintas (D2)
        assertEquals("LA.Punto;", ClaveCanonica.tipo(new BpType.ClassType(externa("", "A", "Punto")), "App"));
        assertEquals("LB.Punto;", ClaveCanonica.tipo(new BpType.ClassType(externa("", "B", "Punto")), "App"));
    }

    @Test @DisplayName("El mismo tipo da la MISMA clave visto desde el dueño y desde el consumidor")
    void duenoYConsumidorCoinciden() {
        // En Core, List es una clase declarada: se cualifica con el módulo local.
        ClassSymbol listaEnCore = clase("List");
        String enDueno = ClaveCanonica.tipo(new BpType.ClassType(listaEnCore), "Core");
        // En Collections, List es un stub importado de Core.
        String enConsumidor = ClaveCanonica.tipo(new BpType.ClassType(externa("", "Core", "List")), "Collections");
        assertEquals(enDueno, enConsumidor);
        assertEquals("LCore.List;", enDueno);
    }

    @Test @DisplayName("Enums, arrays, any y tuplas")
    void enumsArraysTuplas() {
        EnumSymbol color = new EnumSymbol("Color", true, 0, 0);            // declarado aquí
        assertEquals("ELib.Color;", ClaveCanonica.tipo(new BpType.EnumType(color), "Lib"));
        EnumSymbol ajeno = new EnumSymbol("Color", true, 0, 0); ajeno.externalModule = "Lib";   // importado
        assertEquals("ELib.Color;", ClaveCanonica.tipo(new BpType.EnumType(ajeno), "App"));
        assertEquals("[i", ClaveCanonica.tipo(new BpType.ArrayType(PrimitiveType.INTEGER), "X"));
        assertEquals("[LCore.List;", ClaveCanonica.tipo(new BpType.ArrayType(new BpType.ClassType(externa("", "Core", "List"))), "X"));
        assertEquals("a", ClaveCanonica.tipo(BpType.AnyType.INSTANCE, "X"));
        assertEquals("Tis;", ClaveCanonica.tipo(new BpType.TupleType(Arrays.asList(PrimitiveType.INTEGER, PrimitiveType.STRING)), "X"));
        assertEquals("f(Ei.Color;)V", ClaveCanonica.metodo(fn("f", null, null, new BpType.EnumType(color)), "i"));
    }

    @Test @DisplayName("Un tipo SIN RESOLVER es un error con nombre, nunca un «?» (la trampa L/E de #514)")
    void sinResolverEsError() {
        ClaveCanonica.TipoSinResolver e = assertThrows(ClaveCanonica.TipoSinResolver.class,
                () -> ClaveCanonica.tipo(new BpType.UnresolvedClassRef("Lib.Base"), "App"));
        assertEquals("clave canonica: tipo sin resolver: Lib.Base", e.getMessage());
        FunctionSymbol f = fn("m", clase("C"), PrimitiveType.INTEGER, (BpType) null);   // parámetro sin tipo
        assertThrows(ClaveCanonica.TipoSinResolver.class, () -> ClaveCanonica.metodo(f, "Lib"));
        // una clase local sin módulo local tampoco: no hay con qué cualificarla
        assertThrows(ClaveCanonica.TipoSinResolver.class, () -> ClaveCanonica.tipo(new BpType.ClassType(clase("C")), ""));
    }

    @Test @DisplayName("La cabeza de una sobrecarga lleva la firma igual que las demás (donde overloadMangle la dejaba pelada)")
    void cabezaConFirma() {
        ClassSymbol base = clase("Base");
        FunctionSymbol cabeza = fn("m", base, PrimitiveType.INTEGER, PrimitiveType.INTEGER);
        FunctionSymbol segunda = fn("m", base, PrimitiveType.STRING, PrimitiveType.STRING);
        assertEquals("m(i)i", ClaveCanonica.metodo(cabeza, "Lib"));
        assertEquals("m(s)s", ClaveCanonica.metodo(segunda, "Lib"));
        // y cambiar el retorno cambia la clave (Java rechaza; hoy pasaba en silencio: mezcla.py «retorno»)
        FunctionSymbol otroRetorno = fn("k", base, PrimitiveType.INTEGER);
        assertEquals("k()i", ClaveCanonica.metodo(otroRetorno, "Lib"));
        assertEquals("k()s", ClaveCanonica.metodo(fn("k", base, PrimitiveType.STRING), "Lib"));
    }
}
