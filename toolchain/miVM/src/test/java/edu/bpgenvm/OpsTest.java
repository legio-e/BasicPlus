package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;

/** Cubre MainOps.java: SUB, MUL, DIV, MOD, NEG, AND, OR, NOT, GT, GE, LE, NEQ. */
class OpsTest {

    @Test
    void opcodesAritmeticosLogicosYComparacion() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainOps.main(new String[0]));
        assertEquals(
            Arrays.asList(
                7,        // 10 - 3
                42,       // 6 * 7
                5,        // 20 / 4
                3,        // 23 % 5
                -42,      // -(42)
                1, 0,     // AND
                1, 0,     // OR
                1, 0,     // NOT
                1, 0,     // GT
                1, 0,     // GE
                1, 0,     // LE
                1, 0      // NEQ
            ),
            r.prints,
            r.rawOutput
        );
    }
}
