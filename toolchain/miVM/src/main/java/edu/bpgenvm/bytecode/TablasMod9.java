package edu.bpgenvm.bytecode;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * V7/H2 (#523) — LAS DOS SUBTABLAS DEL ENLACE TARDÍO (MOD9), leídas en UN sitio para
 * sus tres lectores: el cargador (ModuleManager), el desensamblador (Disasm) y los
 * tests del formato. Lo que escribe {@code ModWriter.writeToFile} (3.3) y
 * {@code ModWriter.escribirSubtablaClases} (4.5); el layout campo a campo está en
 * docs/V7_IDEAS.md §H2.7 y, cuando cierre, en MOD_FORMAT.md.
 *
 * <p><b>3.3 refs</b> (sección imports, tras 3.1 entradas y 3.2 deps): {@code count u16}
 * + por ref {@code (kind u8, depIdx i16, clase UTF, clave UTF)}. La ref i vive en
 * {@code ext[count3.1 + i]}.
 *
 * <p><b>4.5 CLASES</b> (sección exports, tras 4.1 funcs y 4.2 data — en MOD9 no hay
 * 4.3/4.4): {@code ver u16, nClases u16} y por clase {@code name UTF, flags u8, csOff
 * i32, parentDepIdx i16, parentName UTF, nFieldsOwn u16, nMethodsOwn u16, bwOwn u16,
 * ownFieldBitmap[bw] i32, ownOwnerBitmap[bw] i32, métodos (n u16 + (clave UTF, slot
 * u16, codeOff i32, flags u8)*), campos (n u16 + (clave UTF, slot u16, ancho u8, flags
 * u8)*), eventos (n u16 + (clave UTF, recvSlot u16)*)}.
 */
public final class TablasMod9 {
    private TablasMod9() {}

    public static final int REF_CLASE = 0, REF_MET = 1, REF_CAMPO = 2, REF_EV_RECV = 3, REF_EV_DEST = 4, REF_SUBTIPO = 5, REF_SUPER = 6;
    /** V8 H7.3 (#756) — un método de una interfaz: (la interfaz, su clave) → su posición en la lista aplanada. */
    public static final int REF_IFAZ_MET = 7;
    public static final int DEP_RAIZ = -1, DEP_LOCAL = -2;

    public static final class Ref {
        public final int kind; public final int depIdx; public final String clase; public final String clave;
        Ref(int kind, int depIdx, String clase, String clave) { this.kind = kind; this.depIdx = depIdx; this.clase = clase; this.clave = clave; }
        public String kindNombre() {
            switch (kind) {
                case REF_CLASE: return "CLASE"; case REF_MET: return "MET"; case REF_CAMPO: return "CAMPO";
                case REF_EV_RECV: return "EV_RECV"; case REF_EV_DEST: return "EV_DEST"; case REF_SUBTIPO: return "SUBTIPO"; case REF_SUPER: return "SUPER";
                case REF_IFAZ_MET: return "IFAZ_MET";
                default: return "?" + kind;
            }
        }
        @Override public String toString() {
            String dep = depIdx == DEP_LOCAL ? "local" : ("dep" + depIdx);
            return kindNombre() + " " + clase + (clave.isEmpty() ? "" : "." + clave) + " [" + dep + "]";
        }
    }
    public static final class Metodo {
        public final String clave; public final int slot; public final int codeOff; public final boolean override, privado;
        /** V8 H7.3 (#756) — el bit 2: implementa uno de una interfaz de su clase (E12 si ya no está). */
        public final boolean implementa;
        Metodo(String clave, int slot, int codeOff, int flags) { this.clave = clave; this.slot = slot; this.codeOff = codeOff; this.override = (flags & 1) != 0; this.privado = (flags & 2) != 0; this.implementa = (flags & 4) != 0; }
    }
    public static final class Campo {
        public final String clave; public final int slot; public final int ancho; public final boolean ref, owner, privado;
        Campo(String clave, int slot, int ancho, int flags) { this.clave = clave; this.slot = slot; this.ancho = ancho; this.ref = (flags & 1) != 0; this.owner = (flags & 2) != 0; this.privado = (flags & 4) != 0; }
    }
    public static final class Evento {
        public final String clave; public final int recvSlot;
        Evento(String clave, int recvSlot) { this.clave = clave; this.recvSlot = recvSlot; }
    }
    public static final class Clase {
        public final String name; public final boolean abierta, publica; public final int csOff;
        public final int parentDepIdx; public final String parentName;
        public final int nFieldsOwn, nMethodsOwn; public final int[] ownFieldBitmap, ownOwnerBitmap;
        public final List<Metodo> metodos = new ArrayList<>();
        public final List<Campo> campos = new ArrayList<>();
        public final List<Evento> eventos = new ArrayList<>();
        /** V8 H7.3 (#756) — una INTERFAZ (el bit 2 de sus flags): sin descriptor ni código, sus métodos por clave. */
        public final boolean interfaz;
        /** V8 H7.3 — (versión 2 de la subtabla) las interfaces de la fila: las de la cabecera de una clase, o de las
         *  que hereda una interfaz; su depIdx (DEP_LOCAL o el de la subtabla deps) y su nombre. */
        public final List<Integer> interfacesDep = new ArrayList<>();
        public final List<String> interfacesNombre = new ArrayList<>();
        Clase(String name, int flags, int csOff, int parentDepIdx, String parentName, int nF, int nM, int[] fb, int[] ob) {
            this.name = name; this.abierta = (flags & 1) != 0; this.publica = (flags & 2) != 0; this.csOff = csOff;
            this.interfaz = (flags & 4) != 0;
            this.parentDepIdx = parentDepIdx; this.parentName = parentName; this.nFieldsOwn = nF; this.nMethodsOwn = nM;
            this.ownFieldBitmap = fb; this.ownOwnerBitmap = ob;
        }
    }

    /** Salta 3.1 y 3.2 y lee 3.3. Devuelve la lista (vacía si no hay subtabla). */
    public static List<Ref> leerRefs(byte[] imports) throws IOException {
        List<Ref> out = new ArrayList<>();
        if (imports.length == 0) return out;
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(imports));
        int n = in.readInt();
        for (int i = 0; i < n; i++) { in.readUTF(); in.readUTF(); }
        if (in.available() < 4) return out;
        int nd = in.readInt();
        for (int i = 0; i < nd; i++) { in.readUTF(); in.readUTF(); in.readInt(); in.readInt(); in.readInt(); }
        if (in.available() < 2) return out;
        int nr = in.readUnsignedShort();
        for (int i = 0; i < nr; i++) {
            int kind = in.readUnsignedByte(); int dep = in.readShort();
            String clase = in.readUTF(); String clave = in.readUTF();
            out.add(new Ref(kind, dep, clase, clave));
        }
        return out;
    }

    /** Cuántas entradas tiene 3.1 (para situar la ref i en ext[count3.1 + i]). */
    public static int contarImports(byte[] imports) throws IOException {
        if (imports.length == 0) return 0;
        return new DataInputStream(new ByteArrayInputStream(imports)).readInt();
    }

    /** Salta 4.1 y 4.2 y lee 4.5 (MOD9: no hay 4.3/4.4). Vacía si no hay subtabla. */
    public static List<Clase> leerClases(byte[] exports) throws IOException {
        List<Clase> out = new ArrayList<>();
        if (exports.length == 0) return out;
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(exports));
        int n = in.readInt();
        for (int i = 0; i < n; i++) { in.readUTF(); in.readInt(); }
        if (in.available() < 4) return out;
        int nd = in.readInt();
        for (int i = 0; i < nd; i++) { in.readUTF(); in.readInt(); }
        if (in.available() < 4) return out;
        int ver = in.readUnsignedShort();
        // V8 H7.3 (#756) — la versión 2 añade a cada fila sus interfaces; la llevan sólo los módulos que las usan
        if (ver != 1 && ver != 2) throw new IOException("subtabla CLASES: version " + ver + " desconocida (se esperaba 1 o 2)");
        int nc = in.readUnsignedShort();
        for (int i = 0; i < nc; i++) {
            String name = in.readUTF(); int flags = in.readUnsignedByte(); int csOff = in.readInt();
            int pdep = in.readShort(); String pname = in.readUTF();
            int nF = in.readUnsignedShort(), nM = in.readUnsignedShort(), bw = in.readUnsignedShort();
            int[] fb = new int[bw], ob = new int[bw];
            for (int w = 0; w < bw; w++) fb[w] = in.readInt();
            for (int w = 0; w < bw; w++) ob[w] = in.readInt();
            Clase c = new Clase(name, flags, csOff, pdep, pname, nF, nM, fb, ob);
            int nm = in.readUnsignedShort();
            for (int k = 0; k < nm; k++) c.metodos.add(new Metodo(in.readUTF(), in.readUnsignedShort(), in.readInt(), in.readUnsignedByte()));
            int ncam = in.readUnsignedShort();
            for (int k = 0; k < ncam; k++) c.campos.add(new Campo(in.readUTF(), in.readUnsignedShort(), in.readUnsignedByte(), in.readUnsignedByte()));
            int ne = in.readUnsignedShort();
            for (int k = 0; k < ne; k++) c.eventos.add(new Evento(in.readUTF(), in.readUnsignedShort()));
            if (ver >= 2) {
                int ni = in.readUnsignedShort();
                for (int k = 0; k < ni; k++) { c.interfacesDep.add((int) in.readShort()); c.interfacesNombre.add(in.readUTF()); }
            }
            out.add(c);
        }
        return out;
    }
}
