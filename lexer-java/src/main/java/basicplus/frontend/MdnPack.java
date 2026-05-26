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

    /* Estos constantes deben coincidir con mdn_format.h del firmware. */
    private static final byte[] MAGIC = {'M', 'D', 'N', 0};
    private static final int MDN_VERSION = 1;
    private static final int MDN_ABI_VERSION = 1;
    private static final int MDN_NAME_MAX = 32;

    public static void main(String[] args) throws IOException {
        if (args.length != 3) {
            System.err.println("Uso: MdnPack <input.o> <output.mdn> <ModuleName>");
            System.exit(2);
        }
        Path inPath  = Paths.get(args[0]);
        Path outPath = Paths.get(args[1]);
        String moduleName = args[2];
        byte[] elf = Files.readAllBytes(inPath);

        ElfFile f = ElfFile.parse(elf);
        byte[] code = f.getSectionBytes(".text");
        if (code == null) {
            System.err.println("ELF sin sección .text — ¿gcc -ffunction-sections?");
            System.err.println("Disponibles: " + f.sectionNames());
            System.exit(3);
        }

        // Símbolos exportados: aquellos cuyo nombre empieza por "thunk_<Module>_".
        // En Thumb-2 el bit 0 del valor del símbolo indica "modo Thumb"
        // (no es parte del offset). Lo limpiamos aquí — el loader del
        // firmware re-añade `| 1u` cuando construye la dirección final
        // del thunk para llamarlo.
        String prefix = "thunk_" + moduleName + "_";
        List<Symbol> exports = new ArrayList<>();
        for (Symbol s : f.symbols()) {
            if (s.name.startsWith(prefix) && s.shndx == f.findSectionIndex(".text")) {
                String funcName = s.name.substring(prefix.length());
                String qualified = moduleName + "." + funcName;
                if (qualified.length() >= MDN_NAME_MAX) {
                    System.err.println("Nombre muy largo: '" + qualified + "'");
                    System.exit(4);
                }
                int byteOff = ((int) s.value) & ~1;
                exports.add(new Symbol(qualified, byteOff, s.shndx));
            }
        }

        if (exports.isEmpty()) {
            System.err.println("No se encontraron símbolos thunk_" + moduleName + "_*");
            System.err.println("Disponibles: ");
            for (Symbol s : f.symbols()) System.err.println("  " + s.name);
            System.exit(5);
        }

        // Construir .mdn — header LE + symbol table + code bytes.
        ByteBuffer hdr = ByteBuffer.allocate(20).order(ByteOrder.LITTLE_ENDIAN);
        hdr.put(MAGIC);
        hdr.putShort((short) MDN_VERSION);
        hdr.putShort((short) MDN_ABI_VERSION);
        hdr.putInt(code.length);
        hdr.putInt(exports.size());
        hdr.putInt(0);  // _reserved

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
        System.out.println("emitido: " + outPath);
        System.out.println("  code:    " + code.length + " bytes");
        System.out.println("  symbols: " + exports.size());
        for (Symbol s : exports) {
            System.out.println("    " + s.name + " @ offset 0x"
                + Integer.toHexString(s.value));
        }
        System.out.println("  total:   " + out.size() + " bytes");
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
