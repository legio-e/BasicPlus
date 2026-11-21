package edu.bpgenvm;

import edu.bpgenvm.bytecode.TablasMod9;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523, V3) — EL CARGADOR, SIN EJECUTAR NADA: fixtures escritas a mano con ModWriter
 * (Lib v1/v2/v3 × App con la abierta Hija) y la vtable, los campos y los bitmaps que el
 * cargador TIENE que construir, ESCRITOS DE ANTEMANO. El test no compara la miVM consigo
 * misma: compara con lo que un humano calculó sobre el papel a partir del diseño (§H2.7):
 * la forma del padre real + lo propio detrás; el override casa por clave; un privado del
 * padre no se sobreescribe; los campos propios se rebasan; los bitmaps se desplazan y se
 * OR-ean; el evento son recv (2 ranuras, ref) + dest (1, no ref); parentOff CS-relativo al
 * módulo del hijo y el sello +6 = índice del módulo dueño + 1.
 *
 * <p>Lib.Base (cerrada, padre Object local: toString/compareTo en 0/1):
 *   v1: campos n:i(1) s:s(2 ref) · métodos k()s, m(i)i                 → nF 3, nM 4
 *   v2: campo extra:i DELANTE de n; métodos nuevo()i y sec()s (PRIVADO) DELANTE de k → nF 4, nM 6
 *   v3: como v1 más un campo ref delante (r:LLib/Base;) y un evento propio (ev(i))  → nF 5+3, nM 4
 * App.Hija (abierta, extends Lib.Base): campos q:i, g:j(2), cambio$recv(2 ref)+dest(1) · métodos propio()i, k()s (override),
 *   sec()s (SIN marca de override: en v2 el padre tiene un sec() privado, que no se sobreescribe → ranura nueva)
 */
class EnlaceTardioFixturesTest {

    private static void cuerpo(ModWriter w) throws IOException { w.emitPushInt(0); w.emitRet(); }

    /** Object sintetizado (como hace el emisor en cada módulo). */
    private static void object(ModWriter w) throws IOException {
        w.addClass("Object", null);
        w.addMethod("toString"); w.setMethodMeta("toString", "toString()s", false, false); cuerpo(w);
        w.addMethod("compareTo"); w.setMethodMeta("compareTo", "compareTo(LObject;)i", false, false); cuerpo(w);
        w.endClass();
        w.exportDataSymbol("Object");
    }

    private static void lib(Path dir, int version, int gen) throws IOException {
        ModWriter w = new ModWriter();
        w.setFormato(9); w.addModulo("Lib"); w.setGeneracion(gen);
        object(w);
        w.addClass("Base", "Object");
        if (version == 2) { w.declareField("extra", false, false, false); w.setFieldMeta("extra", "extra:i", false); }
        if (version == 3) { w.declareField("r", true, false, true);       w.setFieldMeta("r", "r:LLib/Base;", false); }
        if (version == 4) for (int i = 0; i < 30; i++) { w.declareField("f" + i, false, false, false); w.setFieldMeta("f" + i, "f" + i + ":i", false); }
        w.declareField("n", false, false, false); w.setFieldMeta("n", "n:i", false);
        w.declareField("s", true, true, false);  w.setFieldMeta("s", "s:s", true);
        if (version == 3) {
            w.declareField("ev$recv", true, false, true); w.declareField("ev$dest", false, false, false);
            w.declareEventMeta("ev(i)", "ev$recv");
        }
        if (version == 2) {
            w.addMethod("nuevo"); w.setMethodMeta("nuevo", "nuevo()i", false, false); cuerpo(w);
            w.addMethod("sec");   w.setMethodMeta("sec", "sec()s", false, true);     cuerpo(w);   // PRIVADO con ranura
        }
        w.addMethod("k"); w.setMethodMeta("k", "k()s", false, false); cuerpo(w);
        w.addMethod("m"); w.setMethodMeta("m", "m(i)i", false, false); cuerpo(w);
        w.endClass();
        w.exportDataSymbol("Base");
        w.addFunction("__init", true); cuerpo(w);
        w.addFunction("main", true); cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("Lib.mod").toString());
    }

    private static void app(Path dir, int genSellado) throws IOException {
        ModWriter w = new ModWriter();
        w.setFormato(9); w.addModulo("App"); w.setGeneracion(genSellado);
        w.declareDependency("Lib", "", genSellado, 0);   // dep0
        object(w);
        w.addClassAbierta("Hija", true, "Lib.Base", 0);
        w.declareField("q", false, false, false);  w.setFieldMeta("q", "q:i", false);
        w.declareField("g", false, false, true);   w.setFieldMeta("g", "g:j", false);
        w.declareField("cambio$recv", true, false, true);
        w.declareField("cambio$dest", false, false, false);
        w.declareEventMeta("cambio(i)", "cambio$recv");
        w.addMethod("propio"); w.setMethodMeta("propio", "propio()i", false, false); cuerpo(w);
        w.addMethod("k");      w.setMethodMeta("k", "k()s", true, false);            cuerpo(w);
        w.addMethod("sec");    w.setMethodMeta("sec", "sec()s", false, false);       cuerpo(w);   // homónimo del privado del padre (v2)
        w.endClass();
        w.exportDataSymbol("Hija");
        w.addFunction("main", true);
        w.emitCallExt("Lib.__init");   // el import 3.1 por el que el cargador descubre la dep (como el __init real)
        w.emitNewObjectRef(w.requireRef(ModWriter.REF_CLASE, ModWriter.DEP_LOCAL, "Hija", ""));
        w.emitInvokeVirtualRef(w.requireRef(ModWriter.REF_MET, 0, "Lib.Base", "m(i)i"), 1);
        cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("App.mod").toString());
    }

    /** Carga y enlaza (sin ejecutar); devuelve la VM con todo construido. */
    private static Object[] carga(Path dir) throws IOException {
        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        vm.setModuleManager(mm);
        mm.executeRootModule(dir.resolve("App.mod").toString(), "App");
        return new Object[]{ vm, mm };
    }

    /** codeOff de cada método propio de una clase, leído de la 4.5 del .mod (para saber QUÉ hay en cada ranura). */
    private static java.util.Map<String, Integer> codeOffs(Path mod, String clase) throws IOException {
        byte[] b = Files.readAllBytes(mod);
        ByteBuffer bb = ByteBuffer.wrap(b);
        int[] h = new int[11]; for (int i = 0; i < 11; i++) h[i] = bb.getInt();
        int off = 44 + h[6] + h[3];   // lib, imports → exports (con la 4.5)
        byte[] exp = new byte[h[4]]; System.arraycopy(b, off, exp, 0, h[4]);
        java.util.Map<String, Integer> m = new java.util.HashMap<>();
        for (TablasMod9.Clase c : TablasMod9.leerClases(exp)) if (c.name.equals(clase)) for (TablasMod9.Metodo x : c.metodos) m.put(x.clave, x.codeOff);
        return m;
    }

    /** La vtable construida como lista «clave@Mod» por ranura (o «-» si heredada sin dueño conocido). */
    private static List<String> vtable(VirtualMachine vm, int desc, java.util.Map<String, Integer> libOffs, java.util.Map<String, Integer> appOffs, int libCs, int appCs) {
        int nM = vm.readInt16(desc + 2), bw = vm.readInt16(desc + 4);
        int vt = desc + 12 + 8 * bw;
        List<String> out = new ArrayList<>();
        for (int s = 0; s < nM; s++) {
            int v = vm.readInt32(vt + s * 4);
            String quien = "-1";
            for (java.util.Map.Entry<String, Integer> e : appOffs.entrySet()) if (e.getValue() == v) quien = e.getKey() + "@App";
            if (quien.equals("-1")) for (java.util.Map.Entry<String, Integer> e : libOffs.entrySet()) if (e.getValue() == v) quien = e.getKey() + "@Lib?";
            out.add(quien);
        }
        return out;
    }

    @Test @DisplayName("v1: la forma del padre + lo propio detrás; k() de la hija reutiliza la ranura del padre; campos rebasados; bitmaps y evento en su sitio")
    void v1(@TempDir Path dir) throws IOException {
        lib(dir, 1, 7); app(dir, 7);
        Object[] r = carga(dir); VirtualMachine vm = (VirtualMachine) r[0]; ModuleManager mm = (ModuleManager) r[1];
        Integer desc = mm.resolveGlobal("App.Hija");
        assertNotNull(desc, "la abierta construida se registra como data-symbol App.Hija");
        assertEquals(3 + 1 + 2 + 3, vm.readInt16(desc), "nF = 3 del padre + q(1) + g(2) + evento(3)");
        assertEquals(4 + 2, vm.readInt16(desc + 2), "nM = 4 del padre + propio + sec (k reutiliza)");
        assertEquals(1, vm.readInt16(desc + 4), "un word de bitmap");
        int selloHija = vm.readInt16(desc + 6), selloBase = vm.readInt16(mm.resolveGlobal("Lib.Base") + 6);
        assertTrue(selloHija >= 1 && selloHija <= 2 && selloBase >= 1 && selloBase <= 2 && selloHija != selloBase,
            "sello +6 = índice del módulo dueño + 1 (dos módulos, dos sellos distintos): Hija " + selloHija + ", Base " + selloBase);
        // ranuras propias construidas: la vtable en orden con QUIÉN vive en cada una
        java.util.Map<String, Integer> appOffs = codeOffs(dir.resolve("App.mod"), "Hija");
        List<String> vt = vtable(vm, desc, codeOffs(dir.resolve("Lib.mod"), "Base"), appOffs, 0, 0);
        assertEquals("k()s@App", vt.get(2), "k()s del padre está en la 2 (tras toString/compareTo) y la hija la SOBREESCRIBE ahí: " + vt);
        assertEquals("propio()i@App", vt.get(4), "propio, lo propio, detrás de todo: " + vt);
        assertEquals("sec()s@App", vt.get(5), vt.toString());
        assertEquals("-1", vt.get(3), "m(i)i heredada: -1 en la vtable de la hija (el despacho sube al padre): " + vt);
        // bitmaps: ref en s (ranura 1-2 del padre → bit 1) y en cambio$recv (3+1+2 = ranura 6 → bit 6); owner: s
        int fb = vm.readInt32(desc + 12), ob = vm.readInt32(desc + 16);
        assertEquals((1 << 1) | (1 << 6), fb, "bits de ref: s (1) y el receptor del evento (6): " + Integer.toBinaryString(fb));
        assertEquals(1 << 1, ob, "owner: solo s");
    }

    @Test @DisplayName("v2: campo y métodos nuevos DELANTE en el padre: la hija se recoloca; su k() sigue sobreescribiendo; su sec() NO sobreescribe el privado del padre (ranura nueva)")
    void v2(@TempDir Path dir) throws IOException {
        lib(dir, 2, 8); app(dir, 7);   // App sellada contra gen 7, Lib gen 8: regla 1 no aplica (más nueva)
        Object[] r = carga(dir); VirtualMachine vm = (VirtualMachine) r[0]; ModuleManager mm = (ModuleManager) r[1];
        int desc = mm.resolveGlobal("App.Hija");
        assertEquals(4 + 6, vm.readInt16(desc), "nF = 4 del padre (extra delante) + 6 propios");
        assertEquals(6 + 2, vm.readInt16(desc + 2), "nM = 6 del padre (nuevo y sec delante) + propio + sec (el privado del padre no se sobreescribe: ranura NUEVA); k reutiliza");
        List<String> vt = vtable(vm, desc, codeOffs(dir.resolve("Lib.mod"), "Base"), codeOffs(dir.resolve("App.mod"), "Hija"), 0, 0);
        assertEquals("-1", vt.get(2), "nuevo()i del padre en la 2: " + vt);
        assertEquals("-1", vt.get(3), "el sec()s PRIVADO del padre sigue en la 3, intacto: " + vt);
        assertEquals("k()s@App", vt.get(4), "k()s se ha corrido a la 4 y la hija la sobreescribe AHÍ: " + vt);
        assertEquals("-1", vt.get(5), "m(i)i heredada: " + vt);
        assertEquals("propio()i@App", vt.get(6), vt.toString());
        assertEquals("sec()s@App", vt.get(7), "el sec()s de la hija, en ranura nueva: " + vt);
        int fb = vm.readInt32(desc + 12), ob = vm.readInt32(desc + 16);
        assertEquals((1 << 2) | (1 << 7), fb, "s se ha corrido a la 2 (extra delante) y el receptor del evento a la 7: " + Integer.toBinaryString(fb));
        assertEquals(1 << 2, ob, "owner s corrido a la 2");
    }

    @Test @DisplayName("v3: un campo ref y un evento nuevos en el padre: los bitmaps del padre se copian y los propios se desplazan (OR)")
    void v3(@TempDir Path dir) throws IOException {
        lib(dir, 3, 8); app(dir, 7);
        Object[] r = carga(dir); VirtualMachine vm = (VirtualMachine) r[0]; ModuleManager mm = (ModuleManager) r[1];
        int desc = mm.resolveGlobal("App.Hija");
        // padre: r(2 ref) n(1) s(2 ref) ev$recv(2 ref) ev$dest(1) = 8 → propios q(8) g(9-10) cambio$recv(11-12) dest(13) → nF 14
        assertEquals(14, vm.readInt16(desc));
        int fb = vm.readInt32(desc + 12);
        assertEquals((1 << 0) | (1 << 3) | (1 << 5) | (1 << 11), fb,
            "refs: r(0) s(3) ev$recv(5) del padre + cambio$recv(11) de la hija: " + Integer.toBinaryString(fb));
        assertEquals(4 + 2, vm.readInt16(desc + 2), "los métodos no cambian en v3");
    }

    @Test @DisplayName("v4: el padre crece a 32 ranuras: el bitmap de la hija cruza la palabra (bw=2) y los bits propios caen en la segunda")
    void v4(@TempDir Path dir) throws IOException {
        lib(dir, 4, 8); app(dir, 7);
        Object[] r = carga(dir); VirtualMachine vm = (VirtualMachine) r[0]; ModuleManager mm = (ModuleManager) r[1];
        int desc = mm.resolveGlobal("App.Hija");
        // padre: f0..f29 (30) + n (30) + s (31-32) = 33 → propios q(33) g(34-35) cambio$recv(36-37) dest(38) → nF 39, bw 2
        assertEquals(39, vm.readInt16(desc)); assertEquals(2, vm.readInt16(desc + 4));
        int fb0 = vm.readInt32(desc + 12), fb1 = vm.readInt32(desc + 16), ob0 = vm.readInt32(desc + 20), ob1 = vm.readInt32(desc + 24);
        assertEquals(1 << 31, fb0, "s en la 31 (palabra 0, bit 31): " + Integer.toBinaryString(fb0));
        assertEquals(1 << (36 - 32), fb1, "el receptor del evento en la 36 → palabra 1, bit 4: " + Integer.toBinaryString(fb1));
        assertEquals(1 << 31, ob0); assertEquals(0, ob1);
        // y la vtable detras de DOS palabras de bitmaps
        List<String> vt = vtable(vm, desc, codeOffs(dir.resolve("Lib.mod"), "Base"), codeOffs(dir.resolve("App.mod"), "Hija"), 0, 0);
        assertEquals("k()s@App", vt.get(2)); assertEquals("propio()i@App", vt.get(4)); assertEquals("sec()s@App", vt.get(5));
    }

    @Test @DisplayName("abuelo en un tercer módulo: Lib.Base ← Mid.Media (abierta) ← App.Hija (abierta): dos construcciones encadenadas, con Mid recompilada en medio")
    void abuelo(@TempDir Path dir) throws IOException {
        lib(dir, 1, 7);
        // Mid.Media extends Lib.Base: un campo ref y dos métodos (uno override de m(i)i, uno propio)
        ModWriter w = new ModWriter();
        w.setFormato(9); w.addModulo("Mid"); w.setGeneracion(8);
        w.declareDependency("Lib", "", 7, 0);
        object(w);
        w.addClassAbierta("Media", true, "Lib.Base", 0);
        w.declareField("t", true, false, true);   w.setFieldMeta("t", "t:s", false);
        w.declareField("extraMid", false, false, false); w.setFieldMeta("extraMid", "extraMid:i", false);   // «recompilada en medio»: campo que la hija no vio
        w.addMethod("m");     w.setMethodMeta("m", "m(i)i", true, false);     cuerpo(w);
        w.addMethod("medio"); w.setMethodMeta("medio", "medio()i", false, false); cuerpo(w);
        w.endClass(); w.exportDataSymbol("Media");
        w.addFunction("__init", true); w.emitCallExt("Lib.__init"); cuerpo(w);
        w.addFunction("main", true); cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("Mid.mod").toString());
        // App.Hija extends Mid.Media (sellada contra un Mid SIN extraMid: sus ranuras propias son relativas y se rebasan igual)
        w = new ModWriter();
        w.setFormato(9); w.addModulo("App"); w.setGeneracion(7);
        w.declareDependency("Mid", "", 7, 0);   // dep0 = Mid
        object(w);
        w.addClassAbierta("Hija", true, "Mid.Media", 0);
        w.declareField("q", false, false, false);  w.setFieldMeta("q", "q:i", false);
        w.addMethod("propio"); w.setMethodMeta("propio", "propio()i", false, false); cuerpo(w);
        w.addMethod("k");      w.setMethodMeta("k", "k()s", true, false);            cuerpo(w);   // override del ABUELO
        w.addMethod("medio");  w.setMethodMeta("medio", "medio()i", true, false);    cuerpo(w);   // override del padre
        w.endClass(); w.exportDataSymbol("Hija");
        w.addFunction("main", true); w.emitCallExt("Mid.__init"); cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("App.mod").toString());

        Object[] r = carga(dir); VirtualMachine vm = (VirtualMachine) r[0]; ModuleManager mm = (ModuleManager) r[1];
        int media = mm.resolveGlobal("Mid.Media"), hija = mm.resolveGlobal("App.Hija");
        // Media: Base(3 campos: n, s) + t(2 ref) + extraMid(1) = 6; métodos 4 + medio = 5 (m sobreescribe la 3)
        assertEquals(6, vm.readInt16(media)); assertEquals(5, vm.readInt16(media + 2));
        List<String> vtM = vtable(vm, media, codeOffs(dir.resolve("Lib.mod"), "Base"), codeOffs(dir.resolve("Mid.mod"), "Media"), 0, 0);
        assertEquals("m(i)i@App", vtM.get(3), "(«@App» = codeOff casado en la tabla que se pasó como propia) m(i)i de Media en la ranura de Lib: " + vtM);
        assertEquals("medio()i@App", vtM.get(4), vtM.toString());
        // Hija: Media(6) + q = 7; métodos 5 + propio = 6; k en la 2 (abuelo), medio en la 4 (padre)
        assertEquals(7, vm.readInt16(hija)); assertEquals(6, vm.readInt16(hija + 2));
        List<String> vtH = vtable(vm, hija, codeOffs(dir.resolve("Mid.mod"), "Media"), codeOffs(dir.resolve("App.mod"), "Hija"), 0, 0);
        assertEquals("k()s@App", vtH.get(2), "override del abuelo en su ranura: " + vtH);
        assertEquals("medio()i@App", vtH.get(4), "override del padre en su ranura: " + vtH);
        assertEquals("propio()i@App", vtH.get(5), vtH.toString());
        assertEquals("-1", vtH.get(3), "m(i)i: la de Media, heredada: " + vtH);
        int fb = vm.readInt32(hija + 12);
        assertEquals((1 << 1) | (1 << 3), fb, "refs: s (1) del abuelo y t (3) del padre: " + Integer.toBinaryString(fb));
        // parentOff de la hija apunta al descriptor construido de Media (CS-relativo al módulo de la hija)
        assertEquals(media, mm.resolveGlobal("Mid.Media").intValue());
        assertEquals(vm.readInt16(media + 6) != vm.readInt16(hija + 6), true, "sellos de módulo distintos (Mid vs App)");
    }

    @Test @DisplayName("E2 al enlazar: la hija sobreescribe k()s y el padre nuevo ya no la tiene → override huérfano, nombrando la clave")
    void e2(@TempDir Path dir) throws IOException {
        ModWriter w = new ModWriter();
        w.setFormato(9); w.addModulo("Lib"); w.setGeneracion(8);
        object(w);
        w.addClass("Base", "Object");
        w.declareField("n", false, false, false); w.setFieldMeta("n", "n:i", false);
        w.declareField("s", true, true, false);  w.setFieldMeta("s", "s:s", true);
        w.addMethod("m"); w.setMethodMeta("m", "m(i)i", false, false); cuerpo(w);   // sin k()s
        w.endClass(); w.exportDataSymbol("Base");
        w.addFunction("__init", true); cuerpo(w);
        w.addFunction("main", true); cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("Lib.mod").toString());
        app(dir, 7);
        ModuleManager.LinkException ex = assertThrows(ModuleManager.LinkException.class, () -> carga(dir));
        assertEquals("'App.Hija' redefine 'k()s' y 'Lib' (gen 8) ya no lo tiene: recompila 'App'", ex.getMessage());
    }

    @Test @DisplayName("E3 al enlazar: la app pide m(i)i y el padre nuevo ya no lo tiene → rechazo nombrando la clave, sin ejecutar nada")
    void e3(@TempDir Path dir) throws IOException {
        // Lib sin m(i)i (pero con m(j)i, para que cite la candidata)
        ModWriter w = new ModWriter();
        w.setFormato(9); w.addModulo("Lib"); w.setGeneracion(8);
        object(w);
        w.addClass("Base", "Object");
        w.declareField("n", false, false, false); w.setFieldMeta("n", "n:i", false);
        w.declareField("s", true, true, false);  w.setFieldMeta("s", "s:s", true);
        w.addMethod("k"); w.setMethodMeta("k", "k()s", false, false); cuerpo(w);
        w.addMethod("m"); w.setMethodMeta("m", "m(j)i", false, false); cuerpo(w);
        w.endClass(); w.exportDataSymbol("Base");
        w.addFunction("__init", true); cuerpo(w);
        w.addFunction("main", true); cuerpo(w); w.setMainEntry("main");
        w.writeToFile(dir.resolve("Lib.mod").toString());
        app(dir, 7);
        ModuleManager.LinkException ex = assertThrows(ModuleManager.LinkException.class, () -> carga(dir));
        assertEquals("'App' necesita 'Lib.Base.m(i)i' y 'Lib' (gen 8) no lo tiene (tiene 'm(j)i'): recompila 'App'", ex.getMessage());
    }
}
