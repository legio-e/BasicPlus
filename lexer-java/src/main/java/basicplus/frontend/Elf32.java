package basicplus.frontend;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Lector de ELF32 little-endian — lo justo para los `.o` y `.elf` que producen
 * `arm-none-eabi-gcc` y `riscv32-esp-elf-gcc`.
 *
 * <h3>Por qué vive aquí y no dentro de quien lo usa</h3>
 * Nació dentro de {@code MdnPack} (V4/H4), que necesitaba leer `.text` y
 * `.symtab` para empaquetar los thunks del `.mdn`. En V5/H8 hace falta el mismo
 * lector para el RELOCALIZADOR de packs nativos, que además necesita las
 * ENTRADAS de relocalización, no sólo sus nombres.
 *
 * <p>Dos lectores de ELF en el mismo producto serían dos sitios donde arreglar
 * el mismo fallo, y uno de los dos se quedaría atrás — es el patrón que ya
 * costó caro en el layout de clase (#299) y en los slots de vtable (#315). Así
 * que se saca a un sitio y los dos tiran de él.
 *
 * <h3>Lo que este lector NO hace</h3>
 * No valida casi nada: da por bueno que el fichero viene de nuestro propio
 * `gcc`. Es deliberado — no es un cargador que reciba binarios ajenos, es un
 * paso de nuestra cadena de construcción, y un ELF corrupto aquí significa que
 * el toolchain está roto, no que haya que defenderse. Lo único que sí
 * comprueba es que sea ELF y que sea de 32 bits, porque confundir eso daría
 * basura silenciosa en vez de un error.
 */
public final class Elf32 {

    /** Una entrada del `.symtab`. */
    public static final class Symbol {
        public final String name;
        public final int    value;     /* offset dentro de la sección referida */
        public final int    shndx;     /* índice de sección que la contiene */
        public Symbol(String name, int value, int shndx) {
            this.name = name; this.value = value; this.shndx = shndx;
        }
    }

    /** Una cabecera de sección. */
    public static final class Section {
        public String name;
        public int name_off, type, flags, addr, offset, size, link, info, entsize;
    }

    /**
     * Una entrada de relocalización: «en `offset` de la sección destino hay que
     * escribir la dirección final del símbolo `symIdx`, de la forma que diga
     * `type`».
     *
     * <h3>REL contra RELA — la primera bifurcación real entre familias</h3>
     * ARM emite `.rel.X` (8 B por entrada) y RISC-V emite `.rela.X` (12 B).
     * La diferencia NO es sólo el tamaño: es dónde vive el <b>addend</b>, el
     * desplazamiento que se suma a la dirección del símbolo.
     *
     * <ul>
     *   <li><b>RELA</b>: el addend viaja EXPLÍCITO en la entrada. Se lee aquí.</li>
     *   <li><b>REL</b>: el addend está DENTRO de la palabra a parchear, en el
     *       propio código. Aquí vale 0 y es el parcheador quien lo saca del
     *       sitio — porque cómo está codificado depende del tipo de
     *       relocalización, no del formato de la tabla.</li>
     * </ul>
     *
     * Por eso {@code addend} es 0 en REL y NO se puede tratar como «no hay
     * addend»: hay, pero está en otro sitio. Confundir las dos cosas daría
     * direcciones desplazadas —no un error, números malos— que es justo el modo
     * de fallo que más cuesta encontrar.
     */
    public static final class Reloc {
        /** Offset DENTRO de la sección destino donde hay que parchear. */
        public final int offset;
        /** `R_ARM_*` / `R_RISCV_*`. Su significado depende de la arquitectura. */
        public final int type;
        /** Índice en el `.symtab` — usar {@link Elf32#symbol(int)}. */
        public final int symIdx;
        /** RELA: el addend explícito. REL: 0, el de verdad está en la palabra. */
        public final int addend;
        /** true = venía de `.rela.X` (addend explícito); false = de `.rel.X`. */
        public final boolean rela;

        Reloc(int offset, int type, int symIdx, int addend, boolean rela) {
            this.offset = offset; this.type = type; this.symIdx = symIdx;
            this.addend = addend; this.rela = rela;
        }
    }

    private final byte[] data;
    private final List<Section> sections = new ArrayList<>();
    /* La tabla COMPLETA, en su orden original: una relocalización nombra su
     * símbolo POR ÍNDICE, así que aquí no se puede saltar ninguno — ni los que
     * no tienen nombre (el 0, los de sección...). Quien quiera la vista
     * filtrada de siempre tiene `symbols()`. */
    private final List<Symbol> symsAll = new ArrayList<>();
    private int machine;   /* e_machine (offset 18): 40=EM_ARM, 243=EM_RISCV */

    private Elf32(byte[] data) { this.data = data; }

    public static Elf32 parse(byte[] data) {
        Elf32 f = new Elf32(data);
        f.parseSelf();
        return f;
    }

    /** `e_machine`: 40 = EM_ARM, 243 = EM_RISCV. Es el tag de arquitectura del .mdn. */
    public int machine() { return machine; }

    private int u32(int off) {
        return ByteBuffer.wrap(data, off, 4)
                .order(ByteOrder.LITTLE_ENDIAN).getInt();
    }
    private int u16(int off) {
        return ByteBuffer.wrap(data, off, 2)
                .order(ByteOrder.LITTLE_ENDIAN).getShort() & 0xFFFF;
    }

    private void parseSelf() {
        // ELF header (32-bit):
        //   0..15 ident
        //   16    e_type     u16
        //   18    e_machine  u16
        //   20    e_version  u32
        //   24    e_entry    u32
        //   28    e_phoff    u32
        //   32    e_shoff    u32  ← section header table offset
        //   36    e_flags    u32
        //   40    e_ehsize   u16
        //   42    e_phentsize u16
        //   44    e_phnum    u16
        //   46    e_shentsize u16
        //   48    e_shnum    u16
        //   50    e_shstrndx u16
        if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
            throw new RuntimeException("no es ELF");
        }
        if (data[4] != 1) {
            throw new RuntimeException("solo ELF32 soportado");
        }
        machine = u16(18);   /* e_machine: 40=EM_ARM, 243=EM_RISCV (H4 arch tag) */
        int shoff = u32(32);
        int shentsize = u16(46);
        int shnum = u16(48);
        int shstrndx = u16(50);

        // Primera pasada: leer section headers (sin nombres).
        for (int i = 0; i < shnum; i++) {
            int off = shoff + i * shentsize;
            Section s = new Section();
            s.name_off = u32(off + 0);
            s.type     = u32(off + 4);
            s.flags    = u32(off + 8);
            s.addr     = u32(off + 12);
            s.offset   = u32(off + 16);
            s.size     = u32(off + 20);
            s.link     = u32(off + 24);
            s.info     = u32(off + 28);
            s.entsize  = u32(off + 36);
            sections.add(s);
        }
        // Asignar nombres usando la section .shstrtab.
        int strtab_off = sections.get(shstrndx).offset;
        for (Section s : sections) {
            s.name = readCStr(strtab_off + s.name_off);
        }

        // Parsear .symtab (si existe).
        for (Section s : sections) {
            if (".symtab".equals(s.name)) {
                parseSymtab(s);
                break;
            }
        }
    }

    private void parseSymtab(Section symtab) {
        // ELF32 Symbol entry: 16 bytes
        //   0: name_off u32
        //   4: value    u32
        //   8: size     u32
        //   12: info    u8
        //   13: other   u8
        //   14: shndx   u16
        int strtab_off = sections.get(symtab.link).offset;
        int n = symtab.size / 16;
        for (int i = 0; i < n; i++) {
            int e = symtab.offset + i * 16;
            int name_off = u32(e + 0);
            int value    = u32(e + 4);
            int shndx    = u16(e + 14);
            String name  = readCStr(strtab_off + name_off);
            /* Se guardan TODOS, incluidos los de nombre vacío: el índice tiene
             * que corresponder con el que citan las relocalizaciones. Filtrar
             * aquí (como se hacía) corría la numeración en silencio. */
            symsAll.add(new Symbol(name, value, shndx));
        }
    }

    private String readCStr(int off) {
        int end = off;
        while (end < data.length && data[end] != 0) end++;
        return new String(data, off, end - off, StandardCharsets.UTF_8);
    }

    public byte[] getSectionBytes(String name) {
        for (Section s : sections) {
            if (name.equals(s.name)) {
                byte[] b = new byte[s.size];
                System.arraycopy(data, s.offset, b, 0, s.size);
                return b;
            }
        }
        return null;
    }

    public int findSectionIndex(String name) {
        for (int i = 0; i < sections.size(); i++) {
            if (name.equals(sections.get(i).name)) return i;
        }
        return -1;
    }

    /**
     * Los símbolos CON NOMBRE. Es la vista que quiere quien busca por nombre
     * (los `thunk_*` del `.mdn`); para relocalizar hace falta
     * {@link #symbol(int)}, que va por índice.
     */
    public List<Symbol> symbols() {
        List<Symbol> v = new ArrayList<>();
        for (Symbol s : symsAll) if (!s.name.isEmpty()) v.add(s);
        return v;
    }

    /** El símbolo `idx` del `.symtab`, tal cual — o null si el índice no existe. */
    public Symbol symbol(int idx) {
        return (idx >= 0 && idx < symsAll.size()) ? symsAll.get(idx) : null;
    }

    /** Las secciones, en su orden. Se devuelve la lista viva: no modificarla. */
    public List<Section> sections() { return sections; }

    /** La sección `idx`, o null. El `shndx` de un símbolo indexa aquí. */
    public Section section(int idx) {
        return (idx >= 0 && idx < sections.size()) ? sections.get(idx) : null;
    }

    /**
     * Las relocalizaciones que apuntan a `seccionDestino` (p.ej. ".text"),
     * vengan de `.rel.X` o de `.rela.X`. Lista vacía = esa sección se basta sola.
     *
     * <p>ESTE es el único sitio que sabe cómo está codificada una entrada; todo
     * lo demás —el guardián del `.mdn`, el relocalizador de packs— se construye
     * encima. Leer el formato en dos sitios sería tener dos versiones de la
     * misma verdad.
     */
    public List<Reloc> relocs(String seccionDestino) {
        List<Reloc> out = new ArrayList<>();
        for (Section rs : sections) {
            boolean rela = rs.name.equals(".rela" + seccionDestino);
            if (!rela && !rs.name.equals(".rel" + seccionDestino)) continue;
            int paso = rela ? 12 : 8;
            for (int off = 0; off + paso <= rs.size; off += paso) {
                int base   = rs.offset + off;
                int rOff   = u32(base);
                int info   = u32(base + 4);
                int addend = rela ? u32(base + 8) : 0;
                /* ELF32: el byte bajo de r_info es el TIPO, los 24 altos el
                 * índice de símbolo. (En ELF64 el reparto es otro — aquí sólo
                 * hay 32 bits, y el lector ya rechazó lo que no lo sea.) */
                out.add(new Reloc(rOff, info & 0xFF, info >>> 8, addend, rela));
            }
        }
        return out;
    }

    /**
     * Nombres, sin repetir, de los símbolos a los que apunta cada reubicación
     * de la sección dada. Lista vacía = esa sección se basta sola.
     *
     * <p>Es la vista LEGIBLE de {@link #relocs(String)} — la usa el guardián del
     * `.mdn` para decir QUÉ falta, no cuántas cosas faltan.
     */
    public List<String> relocSymbols(String seccion) {
        List<String> fuera = new ArrayList<>();
        for (Reloc r : relocs(seccion)) {
            String nom = nombreDeSimbolo(r.symIdx);
            if (!fuera.contains(nom)) fuera.add(nom);
        }
        return fuera;
    }

    /** Nombre del símbolo `idx`, o algo legible si no se puede. */
    private String nombreDeSimbolo(int idx) {
        Symbol s = symbol(idx);
        if (s == null) return "(simbolo #" + idx + ")";
        /* Un símbolo de SECCIÓN no tiene nombre propio; el interesante es a qué
         * sección apunta, que es justo lo que hay que enseñar (".rodata"). */
        if (s.name.isEmpty()) {
            Section sec = section(s.shndx);
            return (sec != null) ? sec.name : "(simbolo #" + idx + ")";
        }
        return s.name;
    }

    public List<String> sectionNames() {
        List<String> ns = new ArrayList<>();
        for (Section s : sections) ns.add(s.name);
        return ns;
    }
}
