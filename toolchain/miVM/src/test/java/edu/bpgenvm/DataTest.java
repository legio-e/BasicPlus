package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;

/** Cubre MainData.java: data block con constante + global, GET/SET globales. */
class DataTest {

    @Test
    void constanteYGlobalEnDataBlock() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainData.main(new String[0]));
        // acumulador imprime 0, 100, 200; luego se lee la constante INCREMENTO (100).
        assertEquals(Arrays.asList(0, 100, 200, 100), r.prints, r.rawOutput);
    }
}
