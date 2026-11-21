package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/** Cubre MainFloats.java: f32 constantes, FADD/FSUB/FMUL/FDIV, FLT, I2F/F2I, array de floats. */
class FloatsTest {

    @Test
    void operacionesEnFloat() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainFloats.main(new String[0]));

        // Prints enteros (en orden de emisión)
        assertEquals(Arrays.asList(42, 7, 1, 0, 4), r.prints, r.rawOutput);

        // Prints float — comparamos con tolerancia
        assertEquals(7, r.fprints.size(), "esperaba 7 FPRINTs: " + r.fprints);
        assertEquals(3.14159f,   r.fprints.get(0), 1e-4f);
        assertEquals(2.71828f,   r.fprints.get(1), 1e-4f);
        assertEquals(3.14159f * 2.71828f, r.fprints.get(2), 1e-3f);
        assertEquals(3.25f,      r.fprints.get(3), 1e-6f);
        assertEquals(42.0f,      r.fprints.get(4), 0.0f);
        assertEquals(3.5f,       r.fprints.get(5), 0.0f);
        assertTrue(Float.isInfinite(r.fprints.get(6)) && r.fprints.get(6) > 0,
                "esperaba +Infinity, vi " + r.fprints.get(6));
    }
}
