package edu.bpgenvm;

import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.bytecode.TablasMod9;
import edu.bpgenvm.generador.ModWriter;
import edu.bpgenvm.tools.Disasm;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523) — EL FORMATO MOD9 (enlace tardío), escrito por ModWriter y leído por
 * TablasMod9, sobre un módulo construido a mano: una clase cerrada con padre local
 * (Object sintetizado), otra cerrada que la extiende, y una ABIERTA cuyo padre vive en
 * otro módulo (Lib.Base, dep0) — con campos, un override, un evento y las cinco clases
 * de referencia. Lo que se afirma es lo que el cargador va a necesitar: que la ref i
 * viva en ext[count3.1 + i], que la abierta no tenga descriptor, que lo propio de cada
 * clase (y sólo lo propio) viaje con su clave canónica y su ranura, y que el
 * desensamblador lo cuente.
 */
class ModV9EnlaceTardioTest {

    private static void cuerpoVacio(ModWriter w) throws IOException { w.emitPushInt(0); w.emitRet(); }

    private static Path escribe(Path dir) throws IOException {
        ModWriter w = new ModWriter();
        w.setFormato(9);
        w.addModulo("App");
        w.setGeneracion(7);
        w.declareDependency("Lib", "", 7, 0x1234abcd);      // dep0

        // Object: la raíz sintetizada de este módulo (cerrada, sin padre)
        w.addClass("Object", null);
        w.addMethod("toString"); w.setMethodMeta("toString", "toString()s", false, false); cuerpoVacio(w);
        w.addMethod("compareTo"); w.setMethodMeta("compareTo", "compareTo(LObject;)i", false, false); cuerpoVacio(w);
        w.endClass();
        w.exportDataSymbol("Object");

        // Base: cerrada, padre local Object; un campo, un método propio y un override
        w.addClass("Base", "Object");
        w.declareField("n", false, false, false); w.setFieldMeta("n", "n:i", false);
        w.declareField("s", true, true, false);  w.setFieldMeta("s", "s:s", true);
        w.addMethod("k"); w.setMethodMeta("k", "k()s", false, false); cuerpoVacio(w);
        w.addMethod("toString"); w.setMethodMeta("toString", "toString()s", true, false); cuerpoVacio(w);
        w.endClass();
        w.exportDataSymbol("Base");

        // Hija: ABIERTA, padre Lib.Base (dep0); campos propios desde 0, override k, evento
        w.addClassAbierta("Hija", true, "Lib.Base", 0);
        w.declareField("q", false, false, false);  w.setFieldMeta("q", "q:i", false);
        w.declareField("g", false, false, true);   w.setFieldMeta("g", "g:j", false);   // long: 2 ranuras
        w.declareField("cambio$recv", true, false, true);     // el evento: recv (8) + dest (4)
        w.declareField("cambio$dest", false, false, false);
        w.declareEventMeta("cambio(i)", "cambio$recv");
        w.addMethod("propio"); w.setMethodMeta("propio", "propio()i", false, false); cuerpoVacio(w);
        w.addMethod("k");      w.setMethodMeta("k", "k()s", true, false);            cuerpoVacio(w);
        w.endClass();
        w.exportDataSymbol("Hija");   // no-op en MOD9: una abierta no tiene descriptor

        // main: un CALL_EXT (import 3.1) y las refs
        w.addFunction("main", true);
        w.emitCallExt("Lib.f()i");
        int rK   = w.requireRef(ModWriter.REF_MET,   0, "Lib.Base", "k()s");
        int rK2  = w.requireRef(ModWriter.REF_MET,   0, "Lib.Base", "k()s");           // dedupe
        int rQ   = w.requireRef(ModWriter.REF_CAMPO, ModWriter.DEP_LOCAL, "Hija", "q:i");
        int rNew = w.requireRef(ModWriter.REF_CLASE, 0, "Lib.Base", "");
        int rSub = w.requireRef(ModWriter.REF_SUBTIPO, 0, "Lib.Derived", "Lib.Base");
        int rEv  = w.requireRef(ModWriter.REF_EV_RECV, ModWriter.DEP_LOCAL, "Hija", "cambio(i)");
        assertEquals(rK, rK2, "la misma ref se da de alta una vez");
        w.emitNewObjectRef(rNew);
        w.emitInvokeVirtualRef(rK, 0);
        w.emitGetFieldRef(rQ, false);
        w.emitInstanceOfRef(rSub);
        w.emitPushRef(rEv);
        cuerpoVacio(w);
        w.setMainEntry("main");
        Path f = dir.resolve("App.mod");
        w.writeToFile(f.toString());
        return f;
    }

    /** Las 7 secciones tal como las corta la cabecera. */
    private static byte[][] secciones(byte[] mod) {
        ByteBuffer b = ByteBuffer.wrap(mod);
        int[] h = new int[11];
        for (int i = 0; i < 11; i++) h[i] = b.getInt();
        int off = ModFormat.HEADER_SIZE_V9;
        int[] tam = { h[6], h[3], h[4], h[7], h[8], h[1], h[5] };   // library, imports, exports, interface, native, data, code
        byte[][] out = new byte[7][];
        for (int i = 0; i < 7; i++) { out[i] = new byte[tam[i]]; System.arraycopy(mod, off, out[i], 0, tam[i]); off += tam[i]; }
        return out;
    }

    @Test @DisplayName("MOD9: la cabecera es la de v8 con el magic nuevo, y las refs viven detras de los imports (ext[count3.1 + i])")
    void refsEnLaExtTable(@TempDir Path dir) throws IOException {
        byte[] mod = Files.readAllBytes(escribe(dir));
        assertEquals(ModFormat.MAGIC_NUMBER_V9, ByteBuffer.wrap(mod).getInt());
        byte[][] sec = secciones(mod);
        assertEquals(1, TablasMod9.contarImports(sec[1]), "un solo import de funcion (Lib.f()i)");
        List<TablasMod9.Ref> refs = TablasMod9.leerRefs(sec[1]);
        assertEquals(5, refs.size(), "cinco refs distintas (la repetida se dedupe)");
        assertEquals("MET Lib.Base.k()s [dep0]", refs.get(0).toString());
        assertEquals("CAMPO Hija.q:i [local]", refs.get(1).toString());
        assertEquals("CLASE Lib.Base [dep0]", refs.get(2).toString());
        assertEquals("SUBTIPO Lib.Derived.Lib.Base [dep0]", refs.get(3).toString());
        assertEquals("EV_RECV Hija.cambio(i) [local]", refs.get(4).toString());
        // los operandos idx u16 del codigo apuntan a ext[1 + i] — se leen del bytecode de main
        byte[] code = sec[6];
        int pos = indexOf(code, (byte) OpCode.NEW_OBJECT_REF.code);
        assertTrue(pos >= 0, "hay un NEW_OBJECT_REF en el codigo");
        assertEquals(1 + 2, u16(code, pos + 1), "NEW_OBJECT_REF apunta a ext[count3.1 + refIdx] = ext[1 + 2]");
        assertEquals(OpCode.INVOKE_VIRTUAL_REF.code, code[pos + 3]);
        assertEquals(1 + 0, u16(code, pos + 4), "INVOKE_VIRTUAL_REF → ext[1 + 0]");
        assertEquals(0, code[pos + 6], "numArgs");
        assertEquals(OpCode.GET_FIELD_REF.code, code[pos + 7]);
        assertEquals(1 + 1, u16(code, pos + 8));
        assertEquals(OpCode.INSTANCEOF_REF.code, code[pos + 10]);
        assertEquals(1 + 3, u16(code, pos + 11));
        assertEquals(OpCode.PUSH_REF.code, code[pos + 13]);
        assertEquals(1 + 4, u16(code, pos + 14));
    }

    @Test @DisplayName("MOD9: la subtabla CLASES lleva lo PROPIO de cada clase con su clave, y la abierta no tiene descriptor")
    void subtablaClases(@TempDir Path dir) throws IOException {
        byte[] mod = Files.readAllBytes(escribe(dir));
        byte[][] sec = secciones(mod);
        List<TablasMod9.Clase> clases = TablasMod9.leerClases(sec[2]);
        assertEquals(3, clases.size());

        TablasMod9.Clase object = clases.get(0);
        assertEquals("Object", object.name);
        assertFalse(object.abierta); assertEquals(TablasMod9.DEP_RAIZ, object.parentDepIdx);
        assertNotEquals(0, object.csOff, "una cerrada tiene descriptor");
        assertEquals(2, object.metodos.size());
        assertEquals("toString()s", object.metodos.get(0).clave); assertEquals(0, object.metodos.get(0).slot);
        assertEquals("compareTo(LObject;)i", object.metodos.get(1).clave); assertEquals(1, object.metodos.get(1).slot);

        TablasMod9.Clase base = clases.get(1);
        assertEquals("Base", base.name);
        assertFalse(base.abierta); assertEquals(TablasMod9.DEP_LOCAL, base.parentDepIdx); assertEquals("Object", base.parentName);
        assertEquals(3, base.nFieldsOwn, "n (1) y s (una ref = 2 ranuras): tres ranuras propias");
        assertEquals(2, base.metodos.size(), "solo lo PROPIO: k y el override de toString, no compareTo");
        // en orden de RANURA: el override de toString (0) delante de k (2, detras de toString y compareTo)
        assertEquals("toString()s", base.metodos.get(0).clave); assertEquals(0, base.metodos.get(0).slot); assertTrue(base.metodos.get(0).override);
        assertEquals("k()s", base.metodos.get(1).clave); assertEquals(2, base.metodos.get(1).slot, "detras de toString y compareTo");
        assertFalse(base.metodos.get(1).override);
        assertEquals(2, base.campos.size());
        assertEquals("n:i", base.campos.get(0).clave); assertEquals(0, base.campos.get(0).slot); assertFalse(base.campos.get(0).privado);
        assertEquals("s:s", base.campos.get(1).clave); assertTrue(base.campos.get(1).ref); assertTrue(base.campos.get(1).owner); assertTrue(base.campos.get(1).privado);
        assertEquals(2, base.campos.get(1).ancho, "una ref ocupa 2 ranuras");
        assertEquals(1, base.ownFieldBitmap.length); assertEquals(0b10, base.ownFieldBitmap[0], "s es ref: bit 1 relativo a la primera propia");

        TablasMod9.Clase hija = clases.get(2);
        assertEquals("Hija", hija.name);
        assertTrue(hija.abierta); assertTrue(hija.publica);
        assertEquals(0, hija.csOff, "una ABIERTA no lleva descriptor: lo construye el cargador");
        assertEquals(0, hija.parentDepIdx); assertEquals("Lib.Base", hija.parentName);
        assertEquals(1 + 2 + 2 + 1, hija.nFieldsOwn, "q(1) + g(2) + recv(2) + dest(1) ranuras propias, relativas");
        assertEquals(2, hija.metodos.size());
        assertEquals("propio()i", hija.metodos.get(0).clave); assertEquals(0, hija.metodos.get(0).slot, "relativa: sin placeholders del padre");
        assertEquals("k()s", hija.metodos.get(1).clave); assertTrue(hija.metodos.get(1).override, "el cargador la casa por clave con la del padre real");
        assertEquals(2, hija.campos.size(), "q y g con clave; los $recv/$dest van con el evento");
        assertEquals("q:i", hija.campos.get(0).clave); assertEquals(0, hija.campos.get(0).slot); assertEquals(1, hija.campos.get(0).ancho);
        assertEquals("g:j", hija.campos.get(1).clave); assertEquals(1, hija.campos.get(1).slot); assertEquals(2, hija.campos.get(1).ancho);
        assertEquals(1, hija.eventos.size());
        assertEquals("cambio(i)", hija.eventos.get(0).clave); assertEquals(3, hija.eventos.get(0).recvSlot);
        assertEquals(0b1000, hija.ownFieldBitmap[0], "solo el recv es ref (ranura 3)");
    }

    @Test @DisplayName("MOD9: sin marcadores Cls#clave#slot, sin fixups 4.3/4.4, y el desensamblador lo cuenta")
    void sinMarcadoresYDisasm(@TempDir Path dir) throws IOException {
        Path f = escribe(dir);
        String bytes = new String(Files.readAllBytes(f), StandardCharsets.ISO_8859_1);
        assertFalse(bytes.matches("(?s).*[A-Za-z_]+#[^#\\s]+#[0-9]+.*"), "un MOD9 no lleva marcadores de vtable");
        ByteArrayOutputStream bo = new ByteArrayOutputStream();
        Disasm.dump(f.toString(), new PrintStream(bo, true, "UTF-8"));
        String d = bo.toString("UTF-8");
        assertTrue(d.contains("=== REFS (5)"), d);
        assertTrue(d.contains("ext[1] = MET Lib.Base.k()s [dep0]"), d);
        assertTrue(d.contains("class Hija public ABIERTA  padre=dep0 Lib.Base  sin descriptor"), d);
        assertTrue(d.contains("method k()s") && d.contains("override"), d);
        assertTrue(d.contains("INVOKE_VIRTUAL_REF") && d.contains("ext[1] args=0"), d);
    }

    @Test @DisplayName("en formato 8 (el de hoy) nada de esto se escribe, y pedir una ref es un error del emisor")
    void formato8Intacto(@TempDir Path dir) throws IOException {
        ModWriter w = new ModWriter();
        w.addModulo("Viejo");
        w.addFunction("main", true); cuerpoVacio(w);
        assertThrows(RuntimeException.class, () -> w.requireRef(ModWriter.REF_MET, 0, "X", "m()V"));
        Path f = dir.resolve("Viejo.mod");
        w.writeToFile(f.toString());
        assertEquals(ModFormat.MAGIC_NUMBER_V8, ByteBuffer.wrap(Files.readAllBytes(f)).getInt());
    }

    private static int indexOf(byte[] a, byte b) { for (int i = 0; i < a.length; i++) if (a[i] == b) return i; return -1; }
    private static int u16(byte[] a, int i) { return ((a[i] & 0xFF) << 8) | (a[i + 1] & 0xFF); }
}
