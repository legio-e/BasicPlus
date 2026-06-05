package edu.bpgenvm;

import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.debug.DbgFile;
import edu.bpgenvm.vm.debug.DeviceFrameResolver;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

/**
 * H6.b.3.a — verifica la resolución de símbolos del DEVICE en el host: dado el
 * `.dbg` + un relPc + bp + un lector de memoria (= los READ_INT/READ_STRING del
 * wire), {@link DeviceFrameResolver} produce locales POR NOMBRE Y TIPO + la
 * línea de origen. Es lo que hará el IDE al depurar la VM-C/Pico (rol device).
 *
 * <p>Simula el device con un fake reader: aprende los offsets de cada var del
 * propio `.dbg`, mete valores canónicos por nombre y comprueba el render por
 * tipo (string→texto, double→1.5, long→100, boolean→true, ...).</p>
 */
class DeviceResolveTest {

    @Test
    void resuelveFrameDeSumaPorNombreYTipo() {
        File dbg = new File("../samples/LocalsDbg.dbg");
        assumeTrue(dbg.isFile(),
                "samples/LocalsDbg.dbg no compilado (--backend=mivm); test omitido");

        DbgFile d = DbgFile.load(dbg.getPath());
        assertNotNull(d, "DbgFile.load devolvió null");

        // Busca un relPc cuya línea-origen sea 10 (`var msg := \"hola\"`, dentro
        // de suma) — la misma mecánica que usará el IDE con relPc = pc - cs.
        int relPc = -1;
        for (int p = 0; p < 300_000; p++) {
            if (d.lineForRelPc(p) == 10) { relPc = p; break; }
        }
        assertTrue(relPc >= 0, "no se encontró relPc para la línea 10");

        ModuleManager.FunctionVars fv = d.functionForRelPc(relPc);
        assertNotNull(fv, "functionForRelPc null para un relPc dentro de suma");
        assertEquals("suma", fv.name);

        // Prepara el fake device: valores canónicos por NOMBRE, en bp+offset.
        final int bp = 10_000;
        final Map<Integer, Integer> mem = new HashMap<>();
        long ratioBits = Double.doubleToLongBits(1.5);
        for (ModuleManager.LocalVarDescriptor v : fv.vars) {
            int a = bp + v.offset;
            switch (v.name) {
                case "n":     mem.put(a, 5);  break;
                case "base":  mem.put(a, 10); break;
                case "total": mem.put(a, 15); break;
                case "i":     mem.put(a, 3);  break;
                case "ok":    mem.put(a, 1);  break;          // boolean
                case "msg":   mem.put(a, 777); break;          // string ref
                case "big":   mem.put(a, 0); mem.put(a + 4, 100); break;  // long 100
                case "ratio": mem.put(a, (int) (ratioBits >>> 32));
                              mem.put(a + 4, (int) ratioBits); break;      // double 1.5
                default: break;
            }
        }
        DeviceFrameResolver.MemReader reader = new DeviceFrameResolver.MemReader() {
            @Override public int readI32(int addr) { return mem.getOrDefault(addr, 0); }
            @Override public String readString(int ref) { return (ref == 777) ? "hola" : null; }
        };

        DeviceFrameResolver.Frame f = DeviceFrameResolver.resolve(d, relPc, bp, reader);
        assertEquals("suma", f.function);
        assertEquals(10, f.line, "la línea resuelta debe ser 10");

        Map<String, DeviceFrameResolver.Local> by = new HashMap<>();
        for (DeviceFrameResolver.Local l : f.locals) by.put(l.name, l);
        assertTrue(by.keySet().containsAll(java.util.Arrays.asList(
                "n", "base", "total", "big", "ratio", "msg", "ok", "i")),
                "faltan vars; vi " + by.keySet());

        // Render por tipo (lo que el IDE mostraría en el panel de Variables).
        assertEquals("integer", by.get("n").type);   assertEquals("5",   by.get("n").display);
        assertEquals("integer", by.get("base").type);assertEquals("10",  by.get("base").display);
        assertEquals("integer", by.get("total").type);assertEquals("15", by.get("total").display);
        assertEquals("long",    by.get("big").type);  assertEquals("100", by.get("big").display);
        assertEquals("double",  by.get("ratio").type);assertEquals("1.5", by.get("ratio").display);
        assertEquals("string",  by.get("msg").type);  assertEquals("\"hola\"", by.get("msg").display);
        assertEquals("boolean", by.get("ok").type);   assertEquals("true",  by.get("ok").display);
        assertEquals("integer", by.get("i").type);    assertEquals("3",   by.get("i").display);

        // params en offset negativo, locales en offset>=0 (sanity del frame crudo).
        assertTrue(by.get("n").offset < 0 && by.get("base").offset < 0);
        assertTrue(by.get("total").offset >= 0 && by.get("i").offset >= 0);
    }
}
