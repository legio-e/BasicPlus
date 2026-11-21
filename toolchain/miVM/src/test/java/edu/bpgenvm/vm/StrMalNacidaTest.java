package edu.bpgenvm.vm;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #739 (2-nov) — UNA CADENA QUE NACE EMPEZANDO POR UN BYTE DE CONTINUACIÓN (10xxxxxx: UTF-8 roto). No puede nacer
 * —lo de fuera se descodifica al nacer (#711)—, y si naciera, su nacimiento lo dice (strNace). Ningún programa la
 * crea, así que aquí se fuerza a mano, por dentro: el aviso sale con sus bytes; con una buena, una ASCII o la vacía,
 * no; y sólo las tres primeras veces. Espejo de test_malnacida.c (VM-C).
 */
class StrMalNacidaTest {

    private VirtualMachine vm;
    private Method strNace;
    private Method refDeref;
    private byte[] memoria;

    /** Una cadena del heap con estos bytes, nacida como nacen todas (con su bit ASCII, por los bytes que lleva). */
    private void nace(int... bytes) throws Exception {
        long h = vm.heapNuevoParaLlenar(0, bytes.length, bytes.length);   // 0 = TYPE_ARRAY_I8
        int a = (Integer) refDeref.invoke(vm, h);
        boolean ascii = true;
        for (int i = 0; i < bytes.length; i++) {
            memoria[a + 4 + i] = (byte) bytes[i];
            if (bytes[i] >= 0x80) ascii = false;
        }
        strNace.invoke(vm, h, ascii);
    }

    private static int avisos(ByteArrayOutputStream cap) throws Exception {
        String s = cap.toString("UTF-8");
        int n = 0;
        for (int i = s.indexOf("MAL NACIDA"); i >= 0; i = s.indexOf("MAL NACIDA", i + 1)) n++;
        return n;
    }

    @Test
    @Timeout(30)   // si algo esperase (el GC, sin hilos), que falle aquí y no cuelgue la batería
    void avisaConSusBytesYSoloLasTresPrimeras() throws Exception {
        vm = new VirtualMachine();
        vm.setHeapStart(4096);   // un heap, como lo deja el cargador: sin programa mide cero, y reservar llamaría al GC
        strNace = VirtualMachine.class.getDeclaredMethod("strNace", long.class, boolean.class);
        refDeref = VirtualMachine.class.getDeclaredMethod("refDeref", long.class);
        Field memory = VirtualMachine.class.getDeclaredField("memory");
        strNace.setAccessible(true);
        refDeref.setAccessible(true);
        memory.setAccessible(true);
        memoria = (byte[]) memory.get(vm);

        PrintStream antes = System.err;
        ByteArrayOutputStream cap = new ByteArrayOutputStream();
        System.setErr(new PrintStream(cap, true, "UTF-8"));
        try {
            nace(0x61, 0xC3, 0xB1, 0x6F);   // «año»: buena
            nace(0x61, 0x62, 0x63);         // ASCII
            nace();                         // la vacía
            assertEquals(0, avisos(cap), "una buena, una ASCII y la vacía: ningún aviso");
            nace(0xA9, 0x61, 0x62);         // la segunda mitad de una «é», y «ab»
            assertEquals(1, avisos(cap), "la que empieza por A9: un aviso");
            assertTrue(cap.toString("UTF-8").contains("A9 61 62"), "con sus bytes");
            for (int k = 0; k < 4; k++) nace(0x80);
            assertEquals(3, avisos(cap), "cinco rotas: tres avisos");
        } finally {
            System.setErr(antes);
        }
    }
}
