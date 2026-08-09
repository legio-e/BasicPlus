// ============================================================
// MdnPack.java
// Tool standalone que toma un .o de arm-none-eabi-gcc -fpic y
// produce un .mdn listo para subir al Pico (H3 #158).
//
// Uso:
//   java basicplus.frontend.MdnPack <input.o> <output.mdn> <ModuleName>
//
//   input.o:    ELF generado por arm-none-eabi-gcc.
//   output.mdn: archivo .mdn resultante.
//   ModuleName: prefijo de los símbolos del BP (e.g., "Bench").
//
// Estrategia:
//   1. Parsea el ELF (mínimo: section headers, .text, .symtab, .strtab).
//   2. Encuentra .text → bytes del code section.
//   3. Encuentra .symtab → símbolos cuyo nombre empieza por "thunk_<Module>_".
//      Para cada uno extrae "<Module>.<func>" como qualified name del BP.
//   4. Escribe .mdn: header + symbol table + code.
//
// Endianness: el código Thumb-2 ya está en little-endian (que es el
// del Cortex-M). El header lo escribimos en little-endian también
// para que el loader del firmware no tenga que swappear.
// ============================================================
package basicplus.frontend;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public final class MdnPack {

    /* Estos constantes deben coincidir con mdn_format.h del firmware.
     *
     * ⚠️ SI CAMBIA `aot_helpers`, ESTE NÚMERO SUBE. Es la mitad del contrato que
     * el otro lado comprueba, y se quedó atrás una vez: el 18-jul (`bf42bed`,
     * #302 paso 2) los helpers pasaron a v2 (refs = handles de 64 bits) y se
     * actualizaron `bpvm_aot_helpers.*`, `AotCEmitter` (que emite el código) y
     * `mdn_format.h` (que lo espera)... pero no ESTA línea, que es quien lo
     * ESTAMPA. Resultado: ocho días generando .mdn con código v2 y etiqueta v1.
     * El loader los aceptaba (su gate usaba `>` en vez de `!=`) y ejecutaba
     * código nativo contra helpers que ya no coincidían → corrupción y reset
     * mudo en placa. Se destapó el 26-jul al arreglar el gate. */
    private static final byte[] MAGIC = {'M', 'D', 'N', 0};
    private static final int MDN_VERSION = 1;
    private static final int MDN_ABI_VERSION = 2;   /* #302 paso 2 — aot_helpers v2 */
    private static final int MDN_NAME_MAX = 32;

    public static void main(String[] args) throws IOException {
        if (args.length != 3) {
            System.err.println("Uso: MdnPack <input.o> <output.mdn> <ModuleName>");
            System.exit(2);
        }
        try {
            PackResult r = pack(Paths.get(args[0]), Paths.get(args[1]), args[2]);
            System.out.println("emitido: " + args[1]);
            System.out.println("  code:    " + r.codeBytes + " bytes");
            System.out.println("  symbols: " + r.symbols);
        } catch (PackException ex) {
            System.err.println(ex.getMessage());
            System.exit(3);
        }
    }

    /** Resultado del empaquetado (para logging). */
    public static final class PackResult {
        public final int codeBytes;
        public final int symbols;
        PackResult(int codeBytes, int symbols) { this.codeBytes = codeBytes; this.symbols = symbols; }
    }

    /** El `.o` no es empaquetable (sin sección .text o sin thunks del módulo). */
    public static final class PackException extends Exception {
        PackException(String m) { super(m); }
    }

    /**
     * Empaqueta un `.o` (arm-none-eabi-gcc -fpic) en un `.mdn`. REUTILIZABLE desde
     * el CLI (main) y desde el IDE (compilación AOT automática, H12) — sin
     * System.exit; lanza PackException con un mensaje claro en caso de error.
     */
    public static PackResult pack(Path inPath, Path outPath, String moduleName)
            throws IOException, PackException {
        byte[] elf = Files.readAllBytes(inPath);

        ElfFile f = ElfFile.parse(elf);
        byte[] code = f.getSectionBytes(".text");
        if (code == null) {
            throw new PackException("ELF sin sección .text — ¿gcc -ffunction-sections? "
                    + "Disponibles: " + f.sectionNames());
        }

        /* ── EL GUARDIÁN: .text TIENE QUE BASTARSE SOLA (V5/H4, 9-ago) ──────
         *
         * Esta herramienta empaqueta `.text` Y NADA MÁS: ni copia `.rodata` ni
         * aplica una sola reubicación. Eso quiere decir que si el código
         * referencia algo de fuera —un literal de cadena, una variable global,
         * `strlen`— en el `.mdn` queda una palabra sin rellenar, y en la placa
         * eso es un puntero a ninguna parte. NO da error al empaquetar, ni al
         * cargar, ni al llamar: da basura, o un reset mudo.
         *
         * Se descubrió midiendo, al escribir el puente a los packs: un literal
         * de C acaba en `.rodata.str1.1` con un `R_ARM_REL32` en `.text`. Y de
         * propina, gcc reconoce un bucle `while (p[k]) k++` y lo convierte en
         * una llamada a `strlen`, que es otra reloc — la trampa no estaba sólo
         * en lo que uno escribe, sino en lo que el compilador escribe por uno.
         *
         * Así que a partir de aquí el desfase GRITA en el build, en vez de
         * viajar a la placa. Si esto salta, el código generado usa algo que el
         * `.mdn` no puede llevarse: hay que quitarlo del código (materializar la
         * constante, acotar el bucle) o enseñar a esta herramienta a llevar
         * `.rodata` y aplicar relocs — que es la solución de verdad y está
         * apuntada como tarea aparte. */
        List<String> pendientes = f.relocSymbols(".text");
        if (!pendientes.isEmpty()) {
            StringBuilder sb = new StringBuilder();
            sb.append("el .o tiene ").append(pendientes.size())
              .append(" referencia(s) a cosas FUERA de .text, y un .mdn solo se lleva .text:\n");
            for (String r : pendientes) sb.append("    - ").append(r).append("\n");
            sb.append("  En la placa cada una de esas seria un puntero a ninguna parte, en silencio.\n")
              .append("  Causas tipicas en codigo generado:\n")
              .append("    * un literal de cadena  -> materializarlo byte a byte en la pila\n")
              .append("    * while (p[k]) k++      -> gcc lo convierte en strlen; acotar el bucle\n")
              .append("    * una variable static   -> no hay .data/.bss en un .mdn");
            throw new PackException(sb.toString());
        }

        // Símbolos exportados: nombre que empieza por "thunk_<Module>_". En Thumb-2
        // el bit 0 del valor indica "modo Thumb" (no es parte del offset): lo
        // limpiamos; el loader del firmware re-añade `| 1u` al construir la dirección.
        String prefix = "thunk_" + moduleName + "_";
        int textIdx = f.findSectionIndex(".text");
        List<Symbol> exports = new ArrayList<>();
        for (Symbol s : f.symbols()) {
            if (s.shndx != textIdx) continue;
            if (s.name.contains(".")) continue;  // skip gcc localalias
            if (!s.name.startsWith(prefix)) continue;
            String funcName = s.name.substring(prefix.length());
            String qualified = moduleName + "." + funcName;
            if (qualified.length() >= MDN_NAME_MAX) {
                throw new PackException("Nombre muy largo: '" + qualified + "'");
            }
            int byteOff = ((int) s.value) & ~1;
            exports.add(new Symbol(qualified, byteOff, s.shndx));
        }

        if (exports.isEmpty()) {
            StringBuilder sb = new StringBuilder(
                    "No se encontraron símbolos thunk_" + moduleName + "_* — disponibles:");
            for (Symbol s : f.symbols()) sb.append("\n  ").append(s.name);
            throw new PackException(sb.toString());
        }

        // Construir .mdn — header LE + symbol table + code bytes.
        ByteBuffer hdr = ByteBuffer.allocate(20).order(ByteOrder.LITTLE_ENDIAN);
        hdr.put(MAGIC);
        hdr.putShort((short) MDN_VERSION);
        hdr.putShort((short) MDN_ABI_VERSION);
        hdr.putInt(code.length);
        hdr.putInt(exports.size());
        hdr.putInt(f.machine);  // arch = e_machine del ELF (H4 — gate del loader)

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        out.write(hdr.array());
        for (Symbol s : exports) {
            byte[] nameBuf = new byte[MDN_NAME_MAX];
            byte[] nameBytes = s.name.getBytes(StandardCharsets.UTF_8);
            System.arraycopy(nameBytes, 0, nameBuf, 0, nameBytes.length);
            out.write(nameBuf);
            ByteBuffer off = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN);
            off.putInt(s.value);
            out.write(off.array());
        }
        out.write(code);

        Files.write(outPath, out.toByteArray());
        return new PackResult(code.length, exports.size());
    }

    /* ====================================================
     * Parser ELF mínimo (ELF32 little-endian, suficiente
     * para .o producidos por arm-none-eabi-gcc).
     * ==================================================== */

    static final class Symbol {
        final String name;
        final int    value;     /* offset dentro de la sección referida */
        final int    shndx;     /* índice de sección que la contiene */
        Symbol(String name, int value, int shndx) {
            this.name = name; this.value = value; this.shndx = shndx;
        }
    }

    static final class ElfFile {
        private final byte[] data;
        private final List<Section> sections = new ArrayList<>();
        private final List<Symbol> syms = new ArrayList<>();
        private int machine;   /* e_machine (offset 18): 40=EM_ARM, 243=EM_RISCV */

        private ElfFile(byte[] data) { this.data = data; }

        static ElfFile parse(byte[] data) {
            ElfFile f = new ElfFile(data);
            f.parseSelf();
            return f;
        }

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
                if (name.isEmpty()) continue;
                syms.add(new Symbol(name, value, shndx));
            }
        }

        private String readCStr(int off) {
            int end = off;
            while (end < data.length && data[end] != 0) end++;
            return new String(data, off, end - off, StandardCharsets.UTF_8);
        }

        byte[] getSectionBytes(String name) {
            for (Section s : sections) {
                if (name.equals(s.name)) {
                    byte[] b = new byte[s.size];
                    System.arraycopy(data, s.offset, b, 0, s.size);
                    return b;
                }
            }
            return null;
        }

        int findSectionIndex(String name) {
            for (int i = 0; i < sections.size(); i++) {
                if (name.equals(sections.get(i).name)) return i;
            }
            return -1;
        }

        List<Symbol> symbols() { return syms; }

        /**
         * Nombres de los símbolos a los que apunta cada reubicación de la sección
         * dada. Lista vacía = esa sección se basta sola.
         *
         * Mira las dos formas que puede tomar (`.rel.X` con entradas de 8 bytes y
         * `.rela.X` de 12): ARM usa REL y RISC-V usa RELA, y las dos familias
         * pasan por aquí. Leer sólo una habría dejado media flota sin guardián,
         * que es peor que no tenerlo — daría una sensación falsa de cobertura.
         */
        List<String> relocSymbols(String seccion) {
            List<String> fuera = new ArrayList<>();
            for (Section rs : sections) {
                boolean rela = rs.name.equals(".rela" + seccion);
                if (!rela && !rs.name.equals(".rel" + seccion)) continue;
                int paso = rela ? 12 : 8;
                Section symtab = null;
                if (rs.link >= 0 && rs.link < sections.size()) symtab = sections.get(rs.link);
                for (int off = 0; off + paso <= rs.size; off += paso) {
                    int info = u32(rs.offset + off + 4);
                    int simIdx = info >>> 8;
                    String nom = nombreDeSimbolo(symtab, simIdx);
                    if (!fuera.contains(nom)) fuera.add(nom);
                }
            }
            return fuera;
        }

        /** Nombre del símbolo `idx` del symtab dado, o algo legible si no se puede. */
        private String nombreDeSimbolo(Section symtab, int idx) {
            if (symtab == null) return "(simbolo #" + idx + ")";
            int ent = symtab.offset + idx * 16;   /* el parser ya asume entradas de 16 */
            if (ent + 4 > data.length) return "(simbolo #" + idx + ")";
            int nameOff = u32(ent);
            if (symtab.link < 0 || symtab.link >= sections.size()) return "(simbolo #" + idx + ")";
            Section str = sections.get(symtab.link);
            String n = readCStr(str.offset + nameOff);
            /* Un símbolo de SECCIÓN no tiene nombre propio; el interesante es a qué
             * sección apunta, que es justo lo que hay que enseñar (".rodata"). */
            if (n == null || n.isEmpty()) {
                int shndx = (ent + 14 + 2 <= data.length) ? u16(ent + 14) : -1;
                if (shndx >= 0 && shndx < sections.size()) return sections.get(shndx).name;
                return "(simbolo #" + idx + ")";
            }
            return n;
        }


        List<String> sectionNames() {
            List<String> ns = new ArrayList<>();
            for (Section s : sections) ns.add(s.name);
            return ns;
        }
    }

    static final class Section {
        String name;
        int name_off, type, flags, addr, offset, size, link, info, entsize;
    }
}
