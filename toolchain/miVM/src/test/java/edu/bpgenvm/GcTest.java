package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/** Cubre MainGC.java: reuso de hueco (GC manual), disparo automático y preservación del payload. */
class GcTest {

    @Test
    void markSweepFreelistReuseYStress() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainGC.main(new String[0]));

        // FASE 1: lengths de a/c/d y b=0; FASE 3: a[0]=sentinel1, a[99]=sentinel2.
        assertEquals(
            Arrays.asList(100, 100, 100, 0, 1010101, 2020202),
            r.prints,
            r.rawOutput
        );

        // FASE 1 emite 3 GCs manuales. FASE 2 dispara varios automáticos + 1 manual final.
        assertTrue(r.gcCount >= 5, "esperaba al menos 5 eventos de GC, vi " + r.gcCount);
    }
}
