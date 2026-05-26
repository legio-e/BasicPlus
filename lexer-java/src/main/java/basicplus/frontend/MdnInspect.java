// ============================================================
// MdnInspect.java
// Validador standalone del formato .mdn (H3 #158).
//
// Replica EXACTAMENTE las validaciones de bpvm_load_mdn() en el
// firmware Pico, pero en JVM puro — sin cross-compile, sin flash,
// sin hardware. Si esto pasa, el .mdn está bien formado y el bug
// del firmware ha de estar en algo hardware-específico (memcpy a
// SRAM, register con thunk addr, o ejecutar desde RAM).
//
// Uso:
//   java basicplus.frontend.MdnInspect <archivo.mdn>
//
// Comprueba:
//   1. Tamaño mínimo (≥ sizeof(mdn_header_t))
//   2. Magic = "MDN\0"
//   3. version == 1, abi_version ≤ 1
//   4. Tamaño total = header + N×sym_entry + code_size
//   5. code_size > 0 y razonable
//   6. Cada thunk_offset cae dentro del code section
//   7. Cada thunk_offset es par (Thumb-2 requiere alineación 2)
//      — el bit Thumb se aplica SOLO al cargar el address, no al
//      offset crudo del .mdn (que es byte offset puro)
//   8. Nombres terminan en NUL antes de los 32 bytes
//
// Reporta cualquier anomalía. Salida 0 = todo OK, !=0 = error.
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public final class MdnInspect {

    /* Constantes idénticas a mdn_format.h. */
    private static final int    MDN_MAGIC_0 = 'M';
    private static final int    MDN_MAGIC_1 = 'D';
    private static final int    MDN_MAGIC_2 = 'N';
    private static final int    MDN_MAGIC_3 = 0;
    private static final int    MDN_VERSION     = 1;
    private static final int    MDN_ABI_VERSION = 1;
    private static final int    MDN_NAME_MAX    = 32;

    private static final int    HEADER_SIZE = 20;
    private static final int    SYM_SIZE    = 36;

    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("Uso: MdnInspect <archivo.mdn>");
            System.exit(2);
        }
        Path p = Paths.get(args[0]);
        byte[] data = Files.readAllBytes(p);
        int rc = inspect(data, p.toString());
        System.exit(rc);
    }

    /** Validación completa. Devuelve 0 si todo OK, !=0 si hay
     *  alguna anomalía. Imprime diagnóstico legible. */
    public static int inspect(byte[] data, String label) {
        System.out.println("== MdnInspect: " + label + " ==");
        System.out.println("file size: " + data.length + " bytes");

        // 1. Tamaño mínimo
        if (data.length < HEADER_SIZE) {
            System.err.println("FAIL: tamaño < header (" + data.length
                + " < " + HEADER_SIZE + ")");
            return 10;
        }

        ByteBuffer bb = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);

        // 2. Magic
        byte m0 = bb.get(0), m1 = bb.get(1), m2 = bb.get(2), m3 = bb.get(3);
        if ((m0 & 0xff) != MDN_MAGIC_0 || (m1 & 0xff) != MDN_MAGIC_1
                || (m2 & 0xff) != MDN_MAGIC_2 || (m3 & 0xff) != MDN_MAGIC_3) {
            System.err.printf("FAIL: magic incorrecto: %02x %02x %02x %02x (esperaba 4D 44 4E 00)%n",
                m0 & 0xff, m1 & 0xff, m2 & 0xff, m3 & 0xff);
            return 11;
        }
        System.out.println("magic OK: \"MDN\\0\"");

        // 3. Version + abi
        int version = bb.getShort(4) & 0xffff;
        int abi     = bb.getShort(6) & 0xffff;
        if (version != MDN_VERSION) {
            System.err.println("FAIL: version=" + version + " (esperaba " + MDN_VERSION + ")");
            return 12;
        }
        if (abi > MDN_ABI_VERSION) {
            System.err.println("FAIL: abi_version=" + abi + " > soportado=" + MDN_ABI_VERSION);
            return 13;
        }
        System.out.println("version=" + version + " abi_version=" + abi + " (OK)");

        // 4. code_size + sym_count + _reserved
        long codeSize  = bb.getInt(8)  & 0xffffffffL;
        long symCount  = bb.getInt(12) & 0xffffffffL;
        long reserved  = bb.getInt(16) & 0xffffffffL;
        System.out.println("code_size=" + codeSize + " sym_count=" + symCount
            + " _reserved=" + reserved);
        if (codeSize == 0) {
            System.err.println("FAIL: code_size=0 — .mdn sin código");
            return 14;
        }
        if (codeSize > 64 * 1024) {
            System.err.println("WARN: code_size=" + codeSize + " > 64 KB (sospechoso)");
        }
        if (symCount == 0) {
            System.err.println("FAIL: sym_count=0 — .mdn sin símbolos exportados");
            return 15;
        }
        if (symCount > 1024) {
            System.err.println("WARN: sym_count=" + symCount + " > 1024 (sospechoso)");
        }
        if (reserved != 0) {
            System.err.println("WARN: _reserved=" + reserved + " (debería ser 0)");
        }

        // 5. Tamaño total
        long hdrTotal = HEADER_SIZE + symCount * SYM_SIZE;
        long expectedSize = hdrTotal + codeSize;
        if (data.length < expectedSize) {
            System.err.println("FAIL: archivo truncado. esperaba >=" + expectedSize
                + " bytes, tengo " + data.length);
            return 16;
        }
        if (data.length > expectedSize) {
            System.err.println("WARN: archivo con " + (data.length - expectedSize)
                + " bytes de trailing — posible contaminación");
        }
        System.out.println("size layout OK: header(" + HEADER_SIZE + ") + syms("
            + (symCount * SYM_SIZE) + ") + code(" + codeSize + ") = " + expectedSize);

        // 6. Iterar símbolos
        System.out.println("symbols:");
        for (long i = 0; i < symCount; i++) {
            int symOff = HEADER_SIZE + (int) i * SYM_SIZE;
            // Name: hasta MDN_NAME_MAX bytes, terminado en NUL.
            int nameLen = 0;
            while (nameLen < MDN_NAME_MAX && data[symOff + nameLen] != 0) nameLen++;
            if (nameLen == MDN_NAME_MAX) {
                System.err.println("FAIL: sym[" + i + "] nombre sin NUL en " + MDN_NAME_MAX + " bytes");
                return 17;
            }
            String name = new String(data, symOff, nameLen, StandardCharsets.UTF_8);
            long thunkOff = bb.getInt(symOff + MDN_NAME_MAX) & 0xffffffffL;
            if (thunkOff >= codeSize) {
                System.err.println("FAIL: sym[" + i + "] '" + name
                    + "' thunk_offset=" + thunkOff
                    + " fuera del code section (size=" + codeSize + ")");
                return 18;
            }
            if ((thunkOff & 1) != 0) {
                System.err.println("FAIL: sym[" + i + "] '" + name
                    + "' thunk_offset=" + thunkOff
                    + " IMPAR — el bit Thumb se añade al cargar, no aquí. MdnPack debería haberlo strippeado.");
                return 19;
            }
            System.out.println("  [" + i + "] '" + name + "' @ offset 0x"
                + Long.toHexString(thunkOff)
                + " (Thumb addr en runtime = base+0x"
                + Long.toHexString(thunkOff) + " | 1)");
        }

        // 7. Hex dump primeros 16 bytes de código + últimos 4 bytes
        int codeStart = (int) hdrTotal;
        System.out.print("code first 16 bytes:");
        for (int i = 0; i < Math.min(16, codeSize); i++) {
            System.out.printf(" %02x", data[codeStart + i] & 0xff);
        }
        System.out.println();
        if (codeSize > 4) {
            System.out.print("code last  4 bytes:");
            for (int i = (int) (codeSize - 4); i < codeSize; i++) {
                System.out.printf(" %02x", data[codeStart + i] & 0xff);
            }
            System.out.println();
        }

        System.out.println("== INSPECT OK ==");
        return 0;
    }
}
