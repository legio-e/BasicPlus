package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;

/** Cubre MainCuadrado.java: módulo Cliente llama a Aritmetica.cuadrado(i) en un bucle 1..3. */
class AritmeticaClienteTest {

    @Test
    void cuadradoEnBucle() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainCuadrado.main(new String[0]));
        // cuadrado(x) = x + x; el bucle imprime para i = 1, 2, 3
        assertEquals(Arrays.asList(2, 4, 6), r.prints, r.rawOutput);
    }
}
