package edu.bpgenvm.vm;

import edu.bpgenvm.bytecode.TablasMod9;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * V7/H2 (#523) — EL ENLACE TARDÍO en la VM de referencia. El algoritmo ÚNICO que después
 * repite `link.c` en C (V7_IDEAS §H2.7, §6): con TODOS los módulos cargados,
 *
 *   1. registra cada clase de cada módulo MOD9 (su subtabla CLASES, 4.5), y sella en cada
 *      descriptor CERRADO el módulo dueño (+6 = índice + 1) para que el despacho encuentre
 *      su CS en O(1) — un descriptor CONSTRUIDO vive fuera de todo módulo y no tiene otra
 *      forma de decir de quién es;
 *   2. CONSTRUYE las clases ABIERTAS (algún `extends` cruza módulo), memoizado padre → hijo,
 *      contra el padre REAL que hay cargado: copia su forma (nF, nM, bitmaps), casa los
 *      overrides por CLAVE, añade lo propio detrás, rebasa los campos, y escribe el
 *      descriptor en la arena (entre el último módulo y el heap);
 *   3. resuelve cada referencia simbólica (3.3) del módulo a su ranura/dirección en la
 *      ext-table, ext[count3.1 + i] — lo que leen los opcodes _REF;
 *   4. verifica lo construido (sin huecos, sin claves dobles, bitmaps coherentes, ninguna
 *      ref sin resolver) y rechaza NOMBRANDO lo que falla (E1-E10).
 *
 * Lo que Java haría en su lugar es el criterio: donde Java corre, esto corre; donde Java
 * lanza (miembro quitado, firma o retorno cambiados, clase quitada, override huérfano,
 * subtipo que ya no lo es), esto rechaza AL ENLAZAR, nunca al llamar. Los textos de rechazo
 * son byte-idénticos a los de la VM-C (los compara compat/mezcla.py).
 */
final class EnlaceTardio {

    /** Layout del descriptor de clase (el de siempre; +6 era _pad y ahora es el módulo dueño). */
    static final int CLS_NUM_FIELDS = 0, CLS_NUM_METHODS = 2, CLS_BITMAP_WORDS = 4, CLS_MODULO = 6,
                     CLS_PARENT_OFF = 8, CLS_FIELD_BITMAP = 12;
    static final int REF_SIN_RESOLVER = 0xFFFFFFFF;

    /** Una clase cargada: su módulo, su fila de 4.5 y su descriptor efectivo (0 = pendiente). */
    static final class Clase {
        final ModuleManager.ModuleMetadata mod;
        final TablasMod9.Clase c;
        int descAbs;                      // dirección absoluta del descriptor (cerrada: en data; abierta: en la arena)
        int nF, nM;                       // efectivos (con lo heredado)
        /** clave → ranura de vtable, EFECTIVA (heredadas incluidas); memoizada. */
        Map<String, Integer> metodos;
        /** clave → ranura de campo, EFECTIVA; memoizada. */
        Map<String, Integer> campos;
        /** clave del evento → ranura del recv, EFECTIVA. */
        Map<String, Integer> eventos;
        /** privadas con ranura (clave): NO se sobreescriben ni se piden desde fuera. */
        java.util.Set<String> privados;
        Clase padre;                      // null = raíz
        boolean construyendo;             // guardia de ciclo
        Clase(ModuleManager.ModuleMetadata mod, TablasMod9.Clase c) { this.mod = mod; this.c = c; }
        String nombreCualificado() { return mod.exportPrefix + c.name; }
    }

    private final ModuleManager mm;
    private final VirtualMachine vm;
    /** `[lib.]Mod.Cls` → clase, de TODOS los módulos MOD9 cargados. */
    private final Map<String, Clase> registro = new LinkedHashMap<>();
    private int arena;                    // siguiente dirección libre para descriptores construidos
    int clasesConstruidas, refsResueltas, bytesArena;

    EnlaceTardio(ModuleManager mm, VirtualMachine vm, int arenaInicio) {
        this.mm = mm; this.vm = vm; this.arena = arenaInicio;
    }
    int arenaFin() { return arena; }

    // ------------------------------------------------------------------ 1. registro
    void registrar(List<ModuleManager.ModuleMetadata> modulos) {
        for (int i = 0; i < modulos.size(); i++) {
            ModuleManager.ModuleMetadata m = modulos.get(i);
            if (m.clases == null) continue;                    // MOD7/MOD8: sin 4.5
            for (TablasMod9.Clase c : m.clases) {
                Clase k = new Clase(m, c);
                if (!c.abierta) {
                    k.descAbs = m.codeStart + c.csOff;
                    // el sello del dueño: +6 = índice del módulo + 1 (0 = sin sello → por rango)
                    vm.writeInt16(k.descAbs + CLS_MODULO, i + 1);
                    k.nF = vm.readInt16(k.descAbs + CLS_NUM_FIELDS);
                    k.nM = vm.readInt16(k.descAbs + CLS_NUM_METHODS);
                }
                registro.put(k.nombreCualificado(), k);
            }
        }
    }

    /** La clase `[lib.]Mod.Cls`, o null. */
    Clase buscar(String cualificado) { return registro.get(cualificado); }

    /** La clase que una fila de 4.5 nombra como padre. */
    private Clase padreDe(Clase k) {
        TablasMod9.Clase c = k.c;
        if (c.parentDepIdx == TablasMod9.DEP_RAIZ) return null;
        String q;
        if (c.parentDepIdx == TablasMod9.DEP_LOCAL) q = k.mod.exportPrefix + c.parentName;
        else {
            if (k.mod.depNames == null || c.parentDepIdx >= k.mod.depNames.size())
                throw new ModuleManager.LinkException("'" + k.mod.name + "' declara la clase '" + c.name + "' con un padre fuera de su tabla de dependencias (dep" + c.parentDepIdx + "): .mod corrupto");
            q = c.parentName;   // ya cualificado por su dueño: [lib.]Mod.Cls
        }
        Clase p = registro.get(q);
        if (p == null) {
            // E1 (padre): la clase no está — o su módulo es MOD7/8 (E9)
            String modPadre = q.contains(".") ? q.substring(0, q.lastIndexOf('.')) : q;
            ModuleManager.ModuleMetadata mp = mm.moduloPorNombreCanonico(modPadre);
            if (mp != null && mp.clases == null)
                throw new ModuleManager.LinkException("'" + mp.name + "' es MOD" + versionDe(mp.magic) + " y '" + k.mod.name + "' necesita su tabla de clases: recompila '" + mp.name + "'");
            throw new ModuleManager.LinkException("'" + k.mod.name + "' extiende la clase '" + q + "' y " + quien(modPadre) + " no la tiene: recompila '" + k.mod.name + "'");
        }
        return p;
    }

    private String quien(String modCanon) {
        ModuleManager.ModuleMetadata m = mm.moduloPorNombreCanonico(modCanon);
        return (m == null) ? "'" + modCanon + "' (no cargado)" : "'" + m.name + "' (gen " + m.gen + ")";
    }

    // ------------------------------------------------------------------ 2. construcción
    /** Deja `k` con descriptor y tablas efectivas (cerrada: las lee; abierta: la construye). */
    void asegurar(Clase k) {
        if (k.metodos != null) return;                 // ya hecha
        if (k.construyendo)
            throw new ModuleManager.LinkException("ciclo de herencia en '" + k.nombreCualificado() + "'");
        k.construyendo = true;
        Clase p = padreDe(k);
        k.padre = p;
        if (p != null) asegurar(p);
        // tablas efectivas: las del padre + lo propio
        k.metodos  = new HashMap<>(); k.campos = new HashMap<>(); k.eventos = new HashMap<>(); k.privados = new java.util.HashSet<>();
        if (p != null) { k.metodos.putAll(p.metodos); k.campos.putAll(p.campos); k.eventos.putAll(p.eventos); k.privados.addAll(p.privados); }
        if (!k.c.abierta) {
            // CERRADA: ranuras absolutas ya en su fila; sólo se indexa
            // V10 (16-sep, F1/REF-2): manda la declaración MÁS CERCANA — si esta clase publica una
            // clave que un ancestro tenía privada, deja de ser privada (la VM-C ya lo hacía así).
            for (TablasMod9.Metodo m : k.c.metodos) { k.metodos.put(m.clave, m.slot); if (m.privado) k.privados.add(m.clave); else k.privados.remove(m.clave); }
            for (TablasMod9.Campo f : k.c.campos)   k.campos.put(f.clave, f.slot);
            for (TablasMod9.Evento e : k.c.eventos) k.eventos.put(e.clave, e.recvSlot);
            k.construyendo = false;
            return;
        }
        // ABIERTA: construir contra el padre real
        if (p == null) throw new ModuleManager.LinkException("la clase abierta '" + k.nombreCualificado() + "' no tiene padre: .mod corrupto");
        int baseF = p.nF, baseM = p.nM;
        int nF = baseF + k.c.nFieldsOwn;
        int nM = baseM;
        int[] ranuraDe = new int[k.c.metodos.size()];
        for (int i = 0; i < k.c.metodos.size(); i++) {
            TablasMod9.Metodo m = k.c.metodos.get(i);
            Integer heredada = (p.privados.contains(m.clave)) ? null : p.metodos.get(m.clave);   // A2: lo privado no se sobreescribe
            if (heredada != null) {
                ranuraDe[i] = heredada;                       // override (marcado o no: criterio JVM)
            } else {
                if (m.override)                               // E2: override huérfano
                    throw new ModuleManager.LinkException("'" + k.mod.name + "." + k.c.name + "' redefine '" + m.clave + "' y " + quien(canonDe(p.mod)) + " ya no lo tiene: recompila '" + k.mod.name + "'");
                ranuraDe[i] = nM++;
            }
            k.metodos.put(m.clave, ranuraDe[i]);
            if (m.privado) k.privados.add(m.clave); else k.privados.remove(m.clave);   // V10 (F1): la más cercana manda
        }
        if (nF > 0xFFFF || nM > 0xFFFF)
            throw new ModuleManager.LinkException("'" + k.mod.name + "." + k.c.name + "' tendria " + Math.max(nF, nM) + " ranuras (maximo 65535)");
        for (TablasMod9.Campo f : k.c.campos)   k.campos.put(f.clave, baseF + f.slot);
        for (TablasMod9.Evento e : k.c.eventos) k.eventos.put(e.clave, baseF + e.recvSlot);
        // el descriptor: cabecera, bitmaps (padre OR propios desplazados), vtable (-1 heredadas, propias con codeOff)
        int bw = (nF + 31) >>> 5;
        int tam = 12 + 8 * bw + 4 * nM;
        int desc = arena; arena += (tam + 3) & ~3; bytesArena += (tam + 3) & ~3;
        vm.reservarArena(arena);
        vm.writeInt16(desc + CLS_NUM_FIELDS, nF);
        vm.writeInt16(desc + CLS_NUM_METHODS, nM);
        vm.writeInt16(desc + CLS_BITMAP_WORDS, bw);
        vm.writeInt16(desc + CLS_MODULO, mm.indiceDe(k.mod) + 1);
        vm.writeInt32(desc + CLS_PARENT_OFF, p.descAbs - k.mod.codeStart);   // CS-relativo al módulo del HIJO
        int[] fb = new int[bw], ob = new int[bw];
        int pbw = vm.readInt16(p.descAbs + CLS_BITMAP_WORDS);
        for (int w = 0; w < pbw && w < bw; w++) {
            fb[w] = vm.readInt32(p.descAbs + CLS_FIELD_BITMAP + w * 4);
            ob[w] = vm.readInt32(p.descAbs + CLS_FIELD_BITMAP + pbw * 4 + w * 4);
        }
        for (int bit = 0; bit < k.c.nFieldsOwn; bit++) {
            int w = bit >>> 5, mk = 1 << (bit & 31);
            boolean ref = w < k.c.ownFieldBitmap.length && (k.c.ownFieldBitmap[w] & mk) != 0;
            boolean own = w < k.c.ownOwnerBitmap.length && (k.c.ownOwnerBitmap[w] & mk) != 0;
            int abs = baseF + bit;
            if (ref) fb[abs >>> 5] |= 1 << (abs & 31);
            if (own) ob[abs >>> 5] |= 1 << (abs & 31);
        }
        for (int w = 0; w < bw; w++) { vm.writeInt32(desc + CLS_FIELD_BITMAP + w * 4, fb[w]); vm.writeInt32(desc + CLS_FIELD_BITMAP + bw * 4 + w * 4, ob[w]); }
        int vt = desc + CLS_FIELD_BITMAP + 8 * bw;
        for (int s = 0; s < nM; s++) vm.writeInt32(vt + s * 4, -1);
        for (int i = 0; i < k.c.metodos.size(); i++) vm.writeInt32(vt + ranuraDe[i] * 4, k.c.metodos.get(i).codeOff);
        k.descAbs = desc; k.nF = nF; k.nM = nM;
        clasesConstruidas++;
        // el data-symbol para quien la pida por nombre (nietos, refs CLASE, la reflexion)
        mm.registrarDataSymbol(k.mod, k.c.name, desc);
        k.construyendo = false;
    }

    private static String canonDe(ModuleManager.ModuleMetadata m) {
        return m.exportPrefix.substring(0, m.exportPrefix.length() - 1);
    }

    // ------------------------------------------------------------------ 0. E9 antes que nada
    // V4 de #523 (17-sep) — medido en mezcla.py: si el dueño es MOD7/8, el import 3.1 del constructor con
    // firma («no exporta 'Lib.__cls_new_Base(i)' ... version vieja?») caía ANTES de que ninguna ref llegara
    // a resolverse, y el mensaje que dice lo que hay que hacer («recompila 'Lib'») no se veía nunca. Así que
    // se comprueba lo primero, antes de los imports 3.1: un consumidor MOD9 con una ref a un módulo que no
    // tiene tabla de clases es E9, diga lo que diga la 3.1. Mismo orden en la VM-C.
    void exigirDuenosConClases(List<ModuleManager.ModuleMetadata> mods) {
        for (ModuleManager.ModuleMetadata m : mods) {
            if (m.refs == null) continue;
            for (TablasMod9.Ref r : m.refs) {
                if (r.depIdx == TablasMod9.DEP_LOCAL) continue;
                String modClase = r.clase.contains(".") ? r.clase.substring(0, r.clase.lastIndexOf('.')) : r.clase;
                ModuleManager.ModuleMetadata mp = mm.moduloPorNombreCanonico(modClase);
                if (mp != null && mp.clases == null)
                    throw new ModuleManager.LinkException("'" + mp.name + "' es MOD" + versionDe(mp.magic) + " y '" + m.name + "' necesita su tabla de clases: recompila '" + mp.name + "'");
            }
        }
    }

    // ------------------------------------------------------------------ 3. las refs
    void resolverRefs(ModuleManager.ModuleMetadata m) {
        if (m.refs == null) return;
        for (int i = 0; i < m.refs.size(); i++) {
            TablasMod9.Ref r = m.refs.get(i);
            int valor = resolver(m, r);
            vm.writeInt32(m.extTableAddress + (m.extCount31 + i) * 4, valor);
            refsResueltas++;
        }
    }

    private int resolver(ModuleManager.ModuleMetadata m, TablasMod9.Ref r) {
        String q = (r.depIdx == TablasMod9.DEP_LOCAL) ? m.exportPrefix + r.clase : r.clase;
        String modClase = q.contains(".") ? q.substring(0, q.lastIndexOf('.')) : q;
        Clase k = registro.get(q);
        if (k == null) {
            ModuleManager.ModuleMetadata mp = mm.moduloPorNombreCanonico(modClase);
            if (mp != null && mp.clases == null)
                throw new ModuleManager.LinkException("'" + mp.name + "' es MOD" + versionDe(mp.magic) + " y '" + m.name + "' necesita su tabla de clases: recompila '" + mp.name + "'");
            throw new ModuleManager.LinkException("'" + m.name + "' necesita la clase '" + q + "' y " + quien(modClase) + " no la tiene: recompila '" + m.name + "'");
        }
        asegurar(k);
        String duenoTxt = quien(canonDe(k.mod));
        switch (r.kind) {
            case TablasMod9.REF_CLASE:
                return k.descAbs;
            case TablasMod9.REF_MET: {
                Integer s = k.metodos.get(r.clave);
                if (s == null || (k.privados.contains(r.clave) && k.mod != m))
                    throw new ModuleManager.LinkException("'" + m.name + "' necesita '" + q + "." + r.clave + "' y " + duenoTxt + " no lo tiene" + tiene(k, r.clave) + ": recompila '" + m.name + "'");
                return s;
            }
            case TablasMod9.REF_CAMPO: {
                Integer s = "__syncMutex:LMutex;".equals(r.clave) ? mutexDesdeLaRaiz(k) : k.campos.get(r.clave);
                if (s == null)
                    throw new ModuleManager.LinkException("'" + m.name + "' necesita el campo '" + q + "." + r.clave + "' y " + duenoTxt + " no lo tiene" + tieneCampo(k, r.clave) + ": recompila '" + m.name + "'");
                return s;
            }
            case TablasMod9.REF_EV_RECV:
            case TablasMod9.REF_EV_DEST: {
                Integer s = k.eventos.get(r.clave);
                if (s == null)
                    throw new ModuleManager.LinkException("'" + m.name + "' necesita el evento '" + q + "." + r.clave + "' y " + duenoTxt + " no lo tiene: recompila '" + m.name + "'");
                return r.kind == TablasMod9.REF_EV_DEST ? s + 2 : s;
            }
            case TablasMod9.REF_SUPER: {
                // V10/J-3: el método EFECTIVO de la clase (el padre estático del que llama): la primera
                // fila con esa clave subiendo por la cadena real → su dirección absoluta (para CALL_EXT).
                // Un privado de otro módulo no cuenta (como en MET).
                for (Clase a = k; a != null; a = a.padre)
                    for (TablasMod9.Metodo mt : a.c.metodos)
                        if (mt.clave.equals(r.clave) && !(mt.privado && a.mod != m)) return a.mod.codeStart + mt.codeOff;
                throw new ModuleManager.LinkException("'" + m.name + "' necesita '" + q + "." + r.clave + "' y " + duenoTxt + " no lo tiene" + tiene(k, r.clave) + ": recompila '" + m.name + "'");
            }
            case TablasMod9.REF_SUBTIPO: {
                // E10: la clase asumida como supertipo tiene que estar en la cadena efectiva
                String qs = r.clave.contains(".") ? r.clave : m.exportPrefix + r.clave;
                for (Clase a = k; a != null; a = a.padre) if (a.nombreCualificado().equals(qs)) return 1;
                throw new ModuleManager.LinkException("'" + m.name + "' asume que '" + q + "' extiende '" + qs + "' y " + duenoTxt + " ya no: recompila '" + m.name + "'");
            }
            default:
                throw new ModuleManager.LinkException("'" + m.name + "' pide una referencia de tipo desconocido (" + r.kind + "): .mod corrupto");
        }
    }

    /** D4: el cerrojo `sync` es UNO por objeto, el del PRIMER eslabón desde la raíz que lo declara. */
    private Integer mutexDesdeLaRaiz(Clase k) {
        List<Clase> cadena = new ArrayList<>();
        for (Clase a = k; a != null; a = a.padre) cadena.add(a);
        for (int i = cadena.size() - 1; i >= 0; i--) {
            Clase a = cadena.get(i);
            for (TablasMod9.Campo f : a.c.campos) if ("__syncMutex:LMutex;".equals(f.clave)) return a.campos.get(f.clave);
        }
        return null;
    }

    /** «(tiene 'k()i')»: si existe otra clave con el mismo nombre, se cita — es el caso de la firma o el retorno cambiados. */
    private static String tiene(Clase k, String clave) { return citar(k.metodos.keySet(), clave, '('); }
    private static String tieneCampo(Clase k, String clave) { return citar(k.campos.keySet(), clave, ':'); }
    /** V10 (16-sep, E-01/E-03/F3/F5): las otras claves con el mismo nombre, ORDENADAS y como mucho
     *  {@link #MAX_CANDIDATAS} — el mismo texto en métodos y campos y el mismo que la VM-C. */
    static final int MAX_CANDIDATAS = 8;
    private static String citar(java.util.Collection<String> claves, String clave, char sep) {
        int corte = clave.indexOf(sep);
        if (corte < 0) return "";   // una clave sin separador no es de este tipo: nada que citar (R10)
        String nombre = clave.substring(0, corte);
        java.util.TreeSet<String> otras = new java.util.TreeSet<>();
        for (String otra : claves)
            if (otra.startsWith(nombre + sep) && !otra.equals(clave)) otras.add(otra);
        if (otras.isEmpty()) return "";
        StringBuilder sb = new StringBuilder(" (tiene ");
        int i = 0;
        for (String o : otras) { if (i == MAX_CANDIDATAS) break; if (i++ > 0) sb.append(", "); sb.append("'").append(o).append("'"); }
        return sb.append(')').toString();
    }
    private static int versionDe(int magic) { return (magic & 0xFF) - 0x30; }

    // ------------------------------------------------------------------ 4. verificación
    void verificar(List<ModuleManager.ModuleMetadata> modulos) {
        for (Clase k : registro.values()) {
            if (!k.c.abierta || k.metodos == null) continue;
            int bw = vm.readInt16(k.descAbs + CLS_BITMAP_WORDS);
            if (bw != ((k.nF + 31) >>> 5)) throw inconsistente(k, "bitmapWords");
            int vt = k.descAbs + CLS_FIELD_BITMAP + 8 * bw;
            for (int s = 0; s < k.nM; s++) {
                if (vm.readInt32(vt + s * 4) == -1 && (k.padre == null || s >= k.padre.nM)) throw inconsistente(k, "ranura " + s + " sin dueno");   // ASCII, como la VM-C (V10, E-08)
            }
        }
        for (ModuleManager.ModuleMetadata m : modulos) {
            if (m.refs == null) continue;
            for (int i = 0; i < m.refs.size(); i++)
                if (vm.readInt32(m.extTableAddress + (m.extCount31 + i) * 4) == REF_SIN_RESOLVER)
                    throw new ModuleManager.LinkException("la referencia " + m.refs.get(i) + " de '" + m.name + "' quedo sin resolver: bug del cargador");
        }
    }
    private static ModuleManager.LinkException inconsistente(Clase k, String detalle) {
        return new ModuleManager.LinkException("la clase '" + k.mod.name + "." + k.c.name + "' construida es inconsistente (" + detalle + "): .mod corrupto o bug del cargador");
    }

    // ------------------------------------------------------------------ reflexión (Forms)
    /** #324 con MOD9: el slot de un método por su nombre simple, buscando la clase por su descriptor. */
    int slotPorNombre(int descAbs, String simple) {
        for (Clase k : registro.values()) {
            if (k.descAbs != descAbs || k.metodos == null) continue;
            Integer mejor = null;
            for (Map.Entry<String, Integer> e : k.metodos.entrySet())
                if (e.getKey().startsWith(simple + "(") && (mejor == null || e.getValue() < mejor)) mejor = e.getValue();
            return mejor == null ? -1 : mejor;
        }
        return -1;
    }
}
