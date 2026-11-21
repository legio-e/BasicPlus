package edu.bpgenvm;

import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8 #768 (3-nov) — el bloque de datos de un módulo, hasta 64 KB (Eduardo: «si utilizas un word pasamos a 64K»).
 * El desplazamiento de un dato (una global, un literal, una constante, el descriptor de una clase) va en los 16 bits
 * de su operando y se lee como un word menos 65 536 (OpCode.desplazamientoDeDato; en la VM-C, bpvm_read_csoff_be):
 * lo de antes (de -32 768 a -1) se lee igual, lo nuevo llega a -65 532 y el 0 sigue siendo el centinela. Y el
 * ModWriter dice, con un error del programa, cuándo un módulo no cabe.
 */
class DatosDe64KTest {

    @Test @DisplayName("el word se lee menos 65 536; lo de antes, igual; el 0, centinela")
    void laRegla() {
        assertEquals(0, OpCode.desplazamientoDeDato(0), "el centinela de CHECKCAST y TRY_BEGIN");
        assertEquals(-1, OpCode.desplazamientoDeDato(0xFFFF));
        assertEquals(-32768, OpCode.desplazamientoDeDato(0x8000), "el tope de antes, igual que con signo");
        assertEquals(-32769, OpCode.desplazamientoDeDato(0x7FFF), "lo primero que antes no se podía");
        assertEquals(-OpCode.DATOS_MAX, OpCode.desplazamientoDeDato(4), "el dato más bajo");
        // el operando llega como lo lee readI16 de la VM-Java: con el signo extendido
        assertEquals(-4, OpCode.desplazamientoDeDato((short) -4));
        // ida y vuelta: lo que escribe el ModWriter (los 16 bits bajos) es lo que se lee
        for (int off = -4; off >= -OpCode.DATOS_MAX; off -= 4) {
            int word = ((short) off) & 0xFFFF;
            assertEquals(off, OpCode.desplazamientoDeDato(word), "desplazamiento " + off);
            if (off >= -32768) assertEquals(off, (short) word, "lo de antes se lee igual que con signo");
        }
    }

    @Test @DisplayName("65 532 bytes de datos caben; uno más, no, y lo dice")
    void elTope() {
        ModWriter w = new ModWriter();
        w.addModulo("Lleno");
        assertEquals(-OpCode.DATOS_MAX, w.addGlobalArray("a", (OpCode.DATOS_MAX - 4) / 4), "justo en el tope");
        ModWriter.DatosLlenos e = assertThrows(ModWriter.DatosLlenos.class, () -> w.addConstantInt("x", 1));
        assertTrue(e.getMessage().contains("el módulo 'Lleno' no cabe"), e.getMessage());
        assertTrue(e.getMessage().contains("pasan de 64 KB, el tope (65532 bytes); ya ocupan 65532 y 'x' (4 bytes) no cabe"),
                e.getMessage());
    }

    @Test @DisplayName("un literal que no cabe se nombra por su principio")
    void unLiteral() {
        ModWriter w = new ModWriter();
        w.addModulo("Texto");
        w.addGlobalArray("a", 10000);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 30000; i++) sb.append((char) ('A' + i % 26));
        ModWriter.DatosLlenos e = assertThrows(ModWriter.DatosLlenos.class,
                () -> w.addConstantString("__str_0", sb.toString()));
        assertTrue(e.getMessage().contains("ya ocupan 40004 y el literal \"ABCDEFGHIJKLMNOPQRSTUVWX…\" (30000 bytes) no cabe"),
                e.getMessage());
    }
}
