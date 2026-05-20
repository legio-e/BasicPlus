package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;

/** Cubre MainBits.java: bitwise, shifts y conversiones I32_TO_{I8,U8,I16,U16}. */
class BitsTest {

    @Test
    void bitwiseShiftsYConversiones() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainBits.main(new String[0]));
        assertEquals(
            Arrays.asList(
                0,           // 0xF0 & 0x0F
                255,         // 0xF0 | 0x0F
                170,         // 0xFF ^ 0x55
                -16,         // ~15
                16,          // 1 << 4
                -4,          // -16 SHR_S 2
                1073741820,  // -16 SHR_U 2
                1,           // 1 << 32 (con mask &31 → 1 << 0)
                222,         // (0xDEADBEEF >>> 24) & 0xFF
                173,         // (0xDEADBEEF >>> 16) & 0xFF
                190,         // (0xDEADBEEF >>>  8) & 0xFF
                239,         //  0xDEADBEEF        & 0xFF
                100,         // I32_TO_I8(100)
                -100,        // I32_TO_I8(-100)
                200,         // I32_TO_U8(200)
                30000,       // I32_TO_I16(30000)
                60000        // I32_TO_U16(60000)
            ),
            r.prints,
            r.rawOutput
        );
    }
}
