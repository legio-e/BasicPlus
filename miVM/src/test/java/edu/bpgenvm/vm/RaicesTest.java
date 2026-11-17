package edu.bpgenvm.vm;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import java.lang.reflect.Field;
import java.util.Collections;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #833 (17-nov) — EN LA MEMORIA DE BP, SÓLO RETIENE UNA REFERENCIA VIVA. El GC recorre palabra a palabra las pilas de
 * los hilos BP y los bloques de datos de los módulos; una palabra cuenta si lleva la marca de handle, su número está en
 * la tabla y los 4 bytes de delante son el contador del slot (esReferencia; la regla de Eduardo: «antes de dar por
 * bueno que es una referencia ha de verificar que el contador coincide»). Antes, en los bloques de datos contaba
 * cualquier palabra: un número que valía justo la dirección de un objeto lo retenía (en la prueba de resistencia,
 * {@code minLibre} en la C6 y la constante {@code TOPE_LOG} en la Pico 2), y una referencia caducada retenía al nuevo
 * ocupante de su slot. Aquí, con un módulo de mentira cuyo bloque de datos son 64 bytes por debajo del heap:
 * un número con la dirección de un objeto no lo retiene; la referencia entera, sí; una caducada, no. Y con el sabotaje
 * (el recorrido de antes) los dos primeros casos sí retienen: si no, la prueba no vale. Espejo de test_raices.c (VM-C).
 */
class RaicesTest {

    private static final int DATOS = 1024;          // el bloque de datos de mentira, por debajo del heap
    private static final int TYPE_ARRAY_I32 = 2;
    private static final int HANDLE_TAG = 0x40000000;

    private VirtualMachine vm;
    private byte[] memoria;
    private Field handleGen, handleAddr;

    @AfterEach
    void sinSabotaje() {
        VirtualMachine.sabotajeRaices = false;
    }

    /** Un objeto nuevo que nada referencia: un array de 64 enteros. */
    private long nuevo() {
        return vm.heapNuevo(TYPE_ARRAY_I32, 64, 64 * 4);
    }

    private static int slot(long h) {
        return ((int) h) & ~HANDLE_TAG;
    }

    /** ¿Sigue vivo el objeto de este handle? Su slot, con su contador y con una dirección. */
    private boolean vivo(long h) throws Exception {
        int idx = slot(h);
        int[] gen = (int[]) handleGen.get(vm);
        int[] addr = (int[]) handleAddr.get(vm);
        return gen[idx] == (int) (h >>> 32) && addr[idx] != 0;
    }

    private int dir(long h) throws Exception {
        return ((int[]) handleAddr.get(vm))[slot(h)];
    }

    private void escribe32(int at, int v) {
        memoria[at] = (byte) (v >>> 24);
        memoria[at + 1] = (byte) (v >>> 16);
        memoria[at + 2] = (byte) (v >>> 8);
        memoria[at + 3] = (byte) v;
    }

    private void escribeRef(int at, long h) {
        escribe32(at, (int) (h >>> 32));   // el contador, delante
        escribe32(at + 4, (int) h);        // el número con la marca, detrás
    }

    private void limpiaDatos() {
        java.util.Arrays.fill(memoria, DATOS, DATOS + 64, (byte) 0);
    }

    @Test
    @Timeout(30)
    void soloRetieneUnaReferenciaViva() throws Exception {
        vm = new VirtualMachine();
        vm.setHeapStart(4096);   // un heap, como lo deja el cargador (StrMalNacidaTest)
        vm.setModuleManager(new ModuleManager(vm) {
            @Override
            public List<int[]> getDataRegions() {
                return Collections.singletonList(new int[]{DATOS, 64});
            }
        });
        Field memory = VirtualMachine.class.getDeclaredField("memory");
        memory.setAccessible(true);
        memoria = (byte[]) memory.get(vm);
        handleGen = VirtualMachine.class.getDeclaredField("handleGen");
        handleAddr = VirtualMachine.class.getDeclaredField("handleAddr");
        handleGen.setAccessible(true);
        handleAddr.setAccessible(true);
        limpiaDatos();

        // 1. Un número no retiene.
        long a = nuevo();
        assertTrue(vivo(a), "el objeto nace");
        escribe32(DATOS + 8, dir(a));   // una global entera que vale su dirección (la de la tabla: cabecera + 4)
        vm.gc();
        assertFalse(vivo(a), "un número con la dirección de un objeto, en una global: no lo retiene");
        VirtualMachine.sabotajeRaices = true;
        long a2 = nuevo();
        limpiaDatos();
        escribe32(DATOS + 8, dir(a2));
        vm.gc();
        assertTrue(vivo(a2), "con el sabotaje (el recorrido de antes), sí lo retiene: la prueba vale");
        VirtualMachine.sabotajeRaices = false;
        limpiaDatos();
        vm.gc();
        assertFalse(vivo(a2), "y sin el número, se suelta");

        // 2. La referencia entera, sí.
        long b = nuevo();
        escribeRef(DATOS + 16, b);
        vm.gc();
        assertTrue(vivo(b), "la referencia entera, con su contador, en una global: lo retiene");

        // 3. Una referencia caducada no retiene al nuevo ocupante de su slot.
        limpiaDatos();
        vm.gc();
        assertFalse(vivo(b), "sin la global, b se suelta");
        long c = nuevo();
        assertEquals(slot(b), slot(c), "c ocupa el slot de b (si no, la prueba no vale)");
        escribeRef(DATOS + 16, b);   // la global se quedó con la referencia de antes: caducada
        vm.gc();
        assertFalse(vivo(c), "una referencia caducada en una global no retiene al nuevo ocupante de su slot");
        VirtualMachine.sabotajeRaices = true;
        long c2 = nuevo();
        assertEquals(slot(b), slot(c2), "c2 ocupa el mismo slot");
        vm.gc();
        assertTrue(vivo(c2), "con el sabotaje (el recorrido de antes), sí lo retiene: la prueba vale");
    }
}
