package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/** Cubre MainOOP.java: clases, herencia, dispatch dinámico (vtable) y GC preciso por bitmap. */
class OopTest {

    @Test
    void polimorfismoYGcPreciso() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainOOP.main(new String[0]));

        // Polimorfismo: Animal.hablar imprime peso, Perro.hablar override imprime peso*10.
        // arr = [a1=Animal(100), p1=Perro(15), p2=Perro(30)]
        assertEquals(Arrays.asList(100, 150, 300), r.prints, r.rawOutput);

        // GC preciso: a1.nombre = ref a array "OOPS" en heap; tras GC sigue accesible.
        assertTrue(r.stringPrints.contains("OOPS"),
                "esperaba 'OOPS' en stringPrints, vi " + r.stringPrints);

        // Al menos un GC manual en FASE 3 del test.
        assertTrue(r.gcCount >= 1, "esperaba >=1 GC, vi " + r.gcCount);
    }
}
