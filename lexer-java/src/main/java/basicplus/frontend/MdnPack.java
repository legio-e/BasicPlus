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
    static final int MDN_ABI_VERSION = 4;   /* 11-ago — el .mdn tambien depende
                                                    * del OFFSET de `memory` y
                                                    * `aot_helpers` dentro de
                                                    * `struct bpvm`, no solo de la
                                                    * tabla. Ver mdn_format.h. */
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

        Elf32 f = Elf32.parse(elf);
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
        List<Elf32.Symbol> exports = new ArrayList<>();
        for (Elf32.Symbol s : f.symbols()) {
            if (s.shndx != textIdx) continue;
            if (s.name.contains(".")) continue;  // skip gcc localalias
            if (!s.name.startsWith(prefix)) continue;
            String funcName = s.name.substring(prefix.length());
            String qualified = moduleName + "." + funcName;
            if (qualified.length() >= MDN_NAME_MAX) {
                throw new PackException("Nombre muy largo: '" + qualified + "'");
            }
            int byteOff = ((int) s.value) & ~1;
            exports.add(new Elf32.Symbol(qualified, byteOff, s.shndx));
        }

        if (exports.isEmpty()) {
            StringBuilder sb = new StringBuilder(
                    "No se encontraron símbolos thunk_" + moduleName + "_* — disponibles:");
            for (Elf32.Symbol s : f.symbols()) sb.append("\n  ").append(s.name);
            throw new PackException(sb.toString());
        }

        // Construir .mdn — header LE + symbol table + code bytes.
        ByteBuffer hdr = ByteBuffer.allocate(20).order(ByteOrder.LITTLE_ENDIAN);
        hdr.put(MAGIC);
        hdr.putShort((short) MDN_VERSION);
        hdr.putShort((short) MDN_ABI_VERSION);
        hdr.putInt(code.length);
        hdr.putInt(exports.size());
        hdr.putInt(f.machine());  // arch = e_machine del ELF (H4 — gate del loader)

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        out.write(hdr.array());
        for (Elf32.Symbol s : exports) {
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

    /* El lector de ELF32 vive en Elf32.java: lo comparte con el relocalizador
     * de packs nativos (V5/H8). Dos lectores serian dos sitios donde arreglar
     * el mismo fallo. */
}
