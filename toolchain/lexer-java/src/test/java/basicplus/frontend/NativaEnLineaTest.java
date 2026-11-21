package basicplus.frontend;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/** V7 #583 — la línea dentro de una native (para el aviso del IDE al poner un breakpoint). */
class NativaEnLineaTest {

    private static final String FUENTE = String.join("\n",
            "module Prueba",                                          // 1
            "  import Core",                                          // 2
            "",                                                       // 3
            "  native function suma(n: integer): integer",            // 4
            "    var s: integer := 0",                                // 5
            "    return s",                                           // 6
            "  end suma",                                             // 7
            "",                                                       // 8
            "  function normal(): integer",                           // 9
            "    return 1",                                           // 10
            "  end normal",                                           // 11
            "",                                                       // 12
            "  public class Cosa",                                    // 13
            "    public property n: integer := 0",                    // 14
            "    public native function doble(): integer",            // 15
            "      return this.n * 2",                                // 16
            "    end doble",                                          // 17
            "    public function ~Cosa()",                            // 18
            "      this.n := 0",                                      // 19
            "    end ~Cosa",                                          // 20
            "  end Cosa",                                             // 21
            "",                                                       // 22
            "  native function Cosa.crear(): Cosa",                   // 23
            "    return Cosa()",                                      // 24
            "  end Cosa.crear",                                       // 25
            "",                                                       // 26
            "  native function sinFin(): integer",                    // 27
            "    return 0",                                           // 28
            "end Prueba");                                            // 29

    private static String en(int linea) { return NativaEnLinea.nativaQueContiene(FUENTE, linea); }

    @Test void dentroDeUnaNativeDeModulo() {
        assertEquals("suma", en(4), "la línea de la declaración cuenta");
        assertEquals("suma", en(5));
        assertEquals("suma", en(6));
        assertEquals("suma", en(7), "la del end también");
    }

    @Test void fueraDeLasNatives() {
        assertNull(en(1));
        assertNull(en(3));
        assertNull(en(8), "entre dos funciones, tras el end de la native");
        assertNull(en(10), "una función normal");
        assertNull(en(14), "una property de una clase, tras ninguna función");
        assertNull(en(19), "el destructor, que no es native");
        assertNull(en(22));
    }

    @Test void unMetodoNativeYUnaEstaticaConNombreCompuesto() {
        assertEquals("doble", en(16), "con modificadores delante: public native function");
        assertEquals("Cosa.crear", en(24), "el end lleva el nombre entero");
    }

    @Test void aMedioEscribirNoAfirmaNada() {
        assertNull(en(28), "una native sin su end: no se dice nada");
        assertNull(NativaEnLinea.nativaQueContiene(null, 3));
        assertNull(en(0));
        assertNull(en(100));
    }

    @Test void conFinalesDeLineaDeWindows() {
        String crlf = FUENTE.replace("\n", "\r\n");
        assertEquals("suma", NativaEnLinea.nativaQueContiene(crlf, 5));
        assertNull(NativaEnLinea.nativaQueContiene(crlf, 10));
    }
}
