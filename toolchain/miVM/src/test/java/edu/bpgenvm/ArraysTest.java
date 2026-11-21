package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/** Cubre MainArrays.java: arrays en data block, locales y heap (NEWARRAY). */
class ArraysTest {

    @Test
    void arraysDataLocalYHeap() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainArrays.main(new String[0]));
        assertEquals(
            Arrays.asList(
                5, 2, 11,                       // primos: length, [0], [4]
                1, 4, 9, 16, 25,                // squares (i+1)^2
                3, 100, 300,                    // buffer local: length, [0], [2]
                4, 1000, 2000, 3000, 4000       // array dinámico en heap: length + contenido
            ),
            r.prints,
            r.rawOutput
        );
        assertTrue(r.stringPrints.contains("Demo de arrays:"), "stringPrints=" + r.stringPrints);
    }
}
