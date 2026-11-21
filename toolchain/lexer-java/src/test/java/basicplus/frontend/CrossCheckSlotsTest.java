package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * #393 — el guardián del IMPORTADOR: «lo que calculé es lo que sembré».
 *
 * <h3>Qué protege</h3>
 *
 * El dueño de una clase ya tenía su guardián (#299, «no se publica un layout que
 * miente»). El importador reconstruía los slots por su cuenta y NADIE comprobaba
 * el resultado: #392 fue exactamente eso, y no se supo hasta ENLAZAR, con un
 * mensaje que mandaba a buscar un {@code .mod} rancio que no existía.
 *
 * <h3>Por qué se prueba la comprobación y no un programa que falle</h3>
 *
 * Porque el bug que la motivó ya está arreglado: un programa BP no puede
 * disparar hoy la incoherencia. Lo que hay que asegurar es que la comprobación
 * SIGUE SABIENDO detectarla el día que algo vuelva a torcerse — así que se le da
 * un mapa torcido a mano y se mira si lo dice. Verificado además contra el
 * código real de #392 (poniendo el reparto viejo, salta con sus dos claves).
 *
 * <h3>Lo que este guardián NO puede ver</h3>
 *
 * Que el NÚMERO de ranura coincida con el del dueño. Comparar contra
 * {@code binaryNumMethods} sería lo natural y está MAL: un {@code function event}
 * privado ocupa ranura pero no se exporta, así que el importador cuenta
 * legítimamente menos (medido: publica 4, reconstruye 3). Con esa comparación el
 * guardián saltaría en toda clase que maneje eventos.
 */
class CrossCheckSlotsTest {

    private static Symbol.ClassSymbol claseVacia() {
        return new Symbol.ClassSymbol("Caja", true, null, null, 0, 0);
    }

    @Test
    @DisplayName("una clave calculada que no llegó al mapa se detecta")
    void claveQueNoSeSembro() {
        Symbol.ClassSymbol c = claseVacia();
        c.externalMethodSlots.put("toString", 0);
        c.externalMethodSlots.put("buscar",   1);
        // 'buscar$s' se calculó (está en la lista) pero NO se sembró: es la
        // forma EXACTA de #392, donde el reparto numeraba por el nombre pelado
        // y las sobrecargas mangleadas se perdían.
        List<String> calculadas = Arrays.asList("buscar", "buscar$s");
        assertEquals(1, Main.cruzarSlots(c, calculadas, "Lib.Caja"),
                "una clave calculada que no está en el mapa tiene que salir");
    }

    @Test
    @DisplayName("dos métodos en la misma ranura se detectan")
    void dosEnLaMismaRanura() {
        Symbol.ClassSymbol c = claseVacia();
        c.externalMethodSlots.put("uno", 2);
        c.externalMethodSlots.put("dos", 2);          // uno taparía al otro
        assertEquals(1, Main.cruzarSlots(c, Arrays.asList("uno", "dos"), "Lib.Caja"),
                "dos claves en la misma ranura tienen que salir");
    }

    @Test
    @DisplayName("un layout coherente no dispara nada")
    void layoutCoherenteNoDisparaNada() {
        // El CONTROL. Sin él, un guardián que dijera «error» siempre pasaría los
        // dos tests de arriba y sería inservible.
        Symbol.ClassSymbol c = claseVacia();
        c.externalMethodSlots.put("toString", 0);
        c.externalMethodSlots.put("compareTo", 1);
        c.externalMethodSlots.put("uno", 2);
        c.externalMethodSlots.put("dos", 3);
        assertEquals(0, Main.cruzarSlots(c, Arrays.asList("uno", "dos"), "Lib.Caja"),
                "un layout sano no puede dar ninguna queja");
    }
}
