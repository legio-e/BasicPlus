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
    /* [V6/N1.5] 24-ago: 6 — slot `newarray_i64` nuevo, y tres newarray_* que
     * dejaron de ser stubs. Histórico: 5 = 11-ago, el .mdn depende también del
     * OFFSET de `memory` y `aot_helpers` dentro de `struct bpvm`, no sólo de la
     * tabla. La autoridad del número es mdn_format.h; esto lo copia. */
    static final int MDN_ABI_VERSION = 6;
    private static final int MDN_NAME_MAX = 32;

    /** Sufijos que gcc le pega a un simbolo cuando lo clona o especializa. Son
     *  copias de una funcion nuestra y NO deben entrar en el `.mdn`: registrar
     *  dos direcciones para el mismo nombre BP es pedir que gane la que no es. */
    private static final String[] CLON_GCC = {
        ".localalias", ".constprop", ".isra", ".part", ".cold", ".lto_priv"
    };
    private static boolean esClonDeGcc(String nombre) {
        for (String c : CLON_GCC) if (nombre.contains(c)) return true;
        return false;
    }

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
        Empaquetado e = empaquetar(inPath, moduleName);
        Files.write(outPath, e.bytes);
        return e.resultado;
    }

    /** El `.mdn` construido, EN MEMORIA. */
    public static final class Empaquetado {
        public final byte[] bytes;
        public final PackResult resultado;
        Empaquetado(byte[] b, PackResult r) { this.bytes = b; this.resultado = r; }
    }

    /**
     * [V6/N1.4] Igual que {@link #pack}, pero DEVUELVE los bytes en vez de
     * escribirlos.
     *
     * <p>Idea de Eduardo: si el `.mdn` va a acabar DENTRO del `.mod` (sección
     * `native`, formato v7), escribirlo a un fichero para leerlo justo después y
     * volver a escribir el `.mod` son dos vueltas de disco que no hacen falta —
     * el empaquetado ya se construía entero en memoria y sólo al final se
     * volcaba. Con esto el llamante junta módulo y nativo en memoria y **escribe
     * el fichero final una sola vez**.
     *
     * <p>{@link #pack} se queda como está —el `.mdn` suelto sigue existiendo
     * mientras la fusión no esté completa— y ahora delega aquí, así que no hay
     * dos implementaciones que puedan separarse.
     */
    public static Empaquetado empaquetar(Path inPath, String moduleName)
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
            /* #381 — y la causa que trajo `long`: la ARITMETICA QUE EL MICRO NO
             * TIENE. El Cortex-M no divide enteros de 64 bits en hardware, asi
             * que gcc emite una llamada a una rutina de libgcc; con `double`
             * pasa lo mismo con casi todas las operaciones. El sintoma es este
             * mismo error, pero las tres causas de arriba no lo explican y uno
             * se queda mirando un `__aeabi_ldivmod` sin saber de donde sale. */
            if (pendientes.stream().anyMatch(MdnPack::esRutinaAritmetica)) {
                sb.append("\n  * UNA RUTINA DE ARITMETICA de la libreria del compilador\n")
                  .append("    (__aeabi_ldivmod, __aeabi_dadd, __divdi3, __adddf3...).\n")
                  .append("    El micro no sabe hacer esa operacion por hardware y gcc la\n")
                  .append("    delega. Sale con: dividir o hacer el modulo de un 'long', y\n")
                  .append("    casi cualquier cosa con 'double'.\n")
                  .append("    Que hacer HOY: sacar esa operacion de la funcion 'native' (el\n")
                  .append("    resto del modulo sigue yendo a nativo), o quitarle el 'native'\n")
                  .append("    a esa funcion para que corra interpretada.\n")
                  .append("    Que lo arreglaria de raiz: enlazar libgcc dentro del .mdn.\n")
                  .append("    Esta analizado en docs/AOT_ABI8_IDEAS.md (#381 F3).");
            }
            throw new PackException(sb.toString());
        }

        /* ── [25-ago-2026] NADA CON CONTENIDO PUEDE QUEDARSE FUERA DEL `.text` ──
         *
         * Un `.mdn` se lleva `.text` y NADA MAS. El guardian de relocalizaciones
         * de mas abajo mira el `.o`; pero el pipeline empaqueta el `.elf` YA
         * ENLAZADO, y ahi las relocalizaciones estan resueltas — o sea que una
         * seccion que se quede fuera ya no deja rastro que aquel pueda ver. Se
         * cae por el agujero EN SILENCIO.
         *
         * Paso: el guion de enlace fusionaba `.rodata*` dentro de `.text` pero no
         * `.srodata*`, que es donde RISC-V pone las CONSTANTES DE COMA FLOTANTE.
         * El `auipc` las apuntaba mas alla del final del blob y la P4 leia lo que
         * hubiera detras: `(3.0+5.0)/2.0` devolvio Infinity y un `float[]` dio
         * 1.219193E25. Sin un solo error. En ARM no pasaba porque alli van a
         * `.rodata.cst8`.
         *
         * La comprobacion es la que deberia haber existido desde el principio:
         * si queda ALGUNA seccion asignable con contenido que no sea la que se
         * empaqueta, se aborta. Vale para lo que venga — `.sdata`, `.srodata`,
         * `.data`, o lo que invente el siguiente toolchain. */
        {
            StringBuilder fuera = new StringBuilder();
            for (Elf32.Section sec : f.sections()) {
                if ((sec.flags & 0x2) == 0) continue;          /* no SHF_ALLOC */
                if (sec.size == 0) continue;
                if (".text".equals(sec.name)) continue;
                fuera.append(fuera.length() == 0 ? "" : ", ")
                     .append(sec.name).append(" (").append(sec.size).append(" B)");
            }
            if (fuera.length() > 0) {
                throw new PackException(
                    "hay secciones CON CONTENIDO fuera de `.text`, y un .mdn solo se lleva\n"
                    + "  `.text`: " + fuera + "\n"
                    + "  En la placa eso se lee como basura, sin dar error — ahi cayeron las\n"
                    + "  constantes de coma flotante de RISC-V (.srodata.cst8) el 25-ago.\n"
                    + "  Si es un `.o` SIN ENLAZAR, ese es el problema: el pipeline enlaza\n"
                    + "  con bpgenvm-c/aot/mdn.ld, que las fusiona dentro de `.text`.");
            }
        }

        /* ── [25-ago-2026] EL `.mdn` TIENE QUE HABLAR LA MISMA ABI QUE EL FIRMWARE ──
         *
         * No lo comprobaba nadie, y el dia que dejo de casar la placa NO dio un
         * error: se COLGO. `AotBuild` no fijaba `-march`/`-mabi` para RISC-V
         * —decia que «casan por construccion»— y el defecto del toolchain trae la
         * extension `d` (coma flotante de DOBLE en hardware) y ABI soft-float,
         * mientras el ESP32-P4 va sin `d` y con single-float. Con doubles vivos,
         * gcc los derrama con `fld`/`fsd`: instruccion ilegal en el P4.
         *
         * Se caza aqui porque aqui esta el `.o`, que es quien lo sabe. Dos
         * preguntas, las dos baratas:
         *   1. la ABI de coma flotante de `e_flags` (bits 1-2);
         *   2. si el repertorio declarado lleva `d`.
         *
         * ⚠️ Hoy RISC-V solo significa ESP32-P4. El dia que haya otra placa
         * RISC-V con otra ABI, esto tiene que preguntarselo a la familia en vez
         * de darlo por sabido — pero un valor fijo y comprobado es infinitamente
         * mejor que ninguna comprobacion. */
        if (f.machine() == 243) {   /* EM_RISCV */
            int fabi = f.flags() & 0x6;
            if (fabi != 0x2) {      /* 0=soft, 2=single, 4=double */
                String cual = (fabi == 0) ? "soft-float" : (fabi == 4 ? "double-float" : "?");
                throw new PackException(
                    "el .o es " + cual + " y el ESP32-P4 va con SINGLE-FLOAT (ilp32f).\n"
                    + "  Los `float` viajarian por registros distintos que en el firmware.\n"
                    + "  Compila con:  -march=rv32imafc_zicsr_zifencei -mabi=ilp32f");
            }
            String arch = f.riscvArch();
            if (arch.matches(".*_d[0-9].*") || arch.matches(".*[0-9]d[0-9].*")) {
                throw new PackException(
                    "el .o declara la extension `d` (coma flotante de DOBLE en hardware)\n"
                    + "  y el ESP32-P4 NO la tiene: gcc derrama los double con fld/fsd y la\n"
                    + "  placa se CUELGA (instruccion ilegal), sin mensaje.\n"
                    + "  arch del .o: " + arch + "\n"
                    + "  Compila con:  -march=rv32imafc_zicsr_zifencei -mabi=ilp32f");
            }
        }

        // Símbolos exportados: nombre que empieza por "thunk_<Module>_". En Thumb-2
        // el bit 0 del valor indica "modo Thumb" (no es parte del offset): lo
        // limpiamos; el loader del firmware re-añade `| 1u` al construir la dirección.
        String prefix = "thunk_" + moduleName + "_";
        int textIdx = f.findSectionIndex(".text");
        List<Elf32.Symbol> exports = new ArrayList<>();
        for (Elf32.Symbol s : f.symbols()) {
            if (s.shndx != textIdx) continue;
            /* [24-ago-2026] Antes esto era `if (s.name.contains(".")) continue`,
             * para saltarse los alias que fabrica gcc. Pero el nombre BP de un
             * METODO LLEVA PUNTO (`Caja.doble`) — desde que el emisor lo pone
             * explicito con `__asm__`, un punto ya no es senal de alias. Se
             * saltan los sufijos de clonado de gcc, que es lo que se queria
             * saltar; el punto del nombre BP pasa. */
            if (esClonDeGcc(s.name)) continue;
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

        return new Empaquetado(out.toByteArray(),
                               new PackResult(code.length, exports.size()));
    }

    /**
     * #381 — ¿este símbolo indefinido es una rutina de aritmética de la
     * librería del compilador?
     *
     * <p>Se reconocen las dos familias que emiten los toolchains que usamos:
     * `__aeabi_*` (ARM EABI: `__aeabi_ldivmod`, `__aeabi_dadd`…) y los nombres
     * genéricos de libgcc (`__divdi3`, `__adddf3`, `__muldf3`…), que son los
     * que salen en RISC-V.
     *
     * <p>Sólo sirve para AFINAR UN MENSAJE. Si algún nombre se escapa del
     * patrón, el error salta igual —la lista de símbolos se imprime entera—:
     * lo único que se pierde es la explicación de dónde viene. Por eso puede
     * ser una heurística sin que eso apague nada.
     */
    private static boolean esRutinaAritmetica(String sym) {
        if (sym == null) return false;
        if (sym.startsWith("__aeabi_")) return true;
        return sym.matches("__[a-z]{2,8}(di3|df3|sf3|si3)");
    }

    /* El lector de ELF32 vive en Elf32.java: lo comparte con el relocalizador
     * de packs nativos (V5/H8). Dos lectores serian dos sitios donde arreglar
     * el mismo fallo. */
}
