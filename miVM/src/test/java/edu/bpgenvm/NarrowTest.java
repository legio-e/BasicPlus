package edu.bpgenvm;

import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;

/** Cubre MainNarrow.java: tipos enteros estrechos i8/u8/i16/u16 (data block, arrays y SET/GET globales). */
class NarrowTest {

    @Test
    void tiposEstrechos() {
        VmTestSupport.VmResult r = VmTestSupport.runMain(() -> MainNarrow.main(new String[0]));
        assertEquals(
            Arrays.asList(
                -1, 255,                       // byteMinus1 i8 / u8
                -1, 65535,                     // shortMinus1 i16 / u16
                5,                             // length de bytes
                10, -5, -56, 0, 127,           // bytes[] ALOAD_I8
                200,                           // bytes[2] ALOAD_U8
                3,                             // length de shorts
                1000, -32000, 32767,           // shorts[] ALOAD_I16
                42,                            // counterB tras SET 42
                -1, 255,                       // counterB tras SET 0x1FF (truncado al byte bajo) leído i8/u8
                100                            // length del NEWARRAY_I8 en heap
            ),
            r.prints,
            r.rawOutput
        );
    }
}
