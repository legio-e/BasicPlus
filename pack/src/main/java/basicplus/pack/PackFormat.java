package basicplus.pack;

import java.nio.charset.StandardCharsets;

/**
 * Layout binario del pack V4 — la fuente de la verdad del formato, compartida
 * por {@link PackWriter} y {@link PackReader} (y la referencia para el lector
 * del micro). TODO big-endian; alineación interna de datos a 4 B; relleno de
 * slack y campos {@code reservado} a {@code 0xFF} (estado borrado de la NOR);
 * relleno de campos-string a {@code 0x00} (NUL).
 *
 * <p>Ver {@code bp-analisis/temas/packs/especificacion/ESPECIFICACION-PACKS-V4.md} §2.
 */
public final class PackFormat {

    private PackFormat() {}

    /** 'B''P''A''K' — identifica el pack. {@code 0xFFFFFFFF} (borrado) = fin de cadena. */
    public static final int  MAGIC           = 0x4250414B;
    /** Versión del layout binario (la usa el runtime para evolucionar el formato). */
    public static final int  VERSION_FORMATO = 1;

    // ── Cabecera de pack (128 B) ──
    //  0 magic(4)  4 version_formato(2)  6 flags(2)  8 size_total(4)
    // 12 nombre(32)  44 fecha(4)  48 version_contenido(16)  64 crc_contenido(2)
    // 66 reservado(60, 0xFF)  126 crc_cab(2)
    public static final int  HEADER_SIZE    = 128;
    public static final int  OFF_MAGIC      = 0;
    public static final int  OFF_VERFMT     = 4;
    public static final int  OFF_FLAGS      = 6;
    public static final int  OFF_SIZE_TOTAL = 8;
    public static final int  OFF_NOMBRE     = 12;
    public static final int  OFF_FECHA      = 44;
    public static final int  OFF_VERCONT    = 48;
    public static final int  OFF_CRC_CONT   = 64;
    public static final int  OFF_RESERVADO  = 66;
    public static final int  OFF_CRC_CAB    = 126;

    // ── Cabecera de fichero (48 B) ──
    //  0 tipo(4)  4 nombre(32)  36 longitud(4)  40 reservado(8, 0xFF)
    public static final int  ENTRY_HEADER_SIZE = 48;
    public static final int  EOFF_TIPO      = 0;
    public static final int  EOFF_NOMBRE    = 4;
    public static final int  EOFF_LONGITUD  = 36;
    public static final int  EOFF_RESERVADO = 40;

    public static final int  NAME_LEN    = 32;   // campo nombre (pack y fichero), NUL-padded
    public static final int  TYPE_LEN    = 4;    // FourCC = la extensión, minúsculas, NUL-padded
    public static final int  VERCONT_LEN = 16;   // version_contenido (string libre, informativo)
    public static final int  ALIGN       = 4;    // alineación de los datos de fichero
    public static final byte PAD         = (byte) 0xFF;  // slack / reservado (borrado NOR)

    public static final int  DEFAULT_BLOCK = 4096;        // bloque de borrado por defecto (Pico/ESP)
    public static final int  FLAGS_ACTIVE  = 0xFFFF;      // pack activo (todos los bits a 1)
    public static final int  ALIVE_BIT     = 0x0001;      // tombstone = bit 1->0 (escribible en NOR)
    public static final long TIPO_END      = 0xFFFFFFFFL; // tipo == 0xFF..FF (slack) = fin de entradas

    public static final String TYPE_MANIFEST = "mft";     // entrada manifest (pack ejecutable)
    public static final String MANIFEST_NAME = "manifest";

    // ── helpers ──

    /** Redondea al alto múltiplo de {@link #ALIGN} (4 B). */
    public static int align4(int n) { return (n + (ALIGN - 1)) & ~(ALIGN - 1); }

    /** Redondea {@code n} al alto múltiplo de {@code blk}. */
    public static int alignUp(int n, int blk) { return ((n + blk - 1) / blk) * blk; }

    public static void putU16(byte[] a, int off, int v) {
        a[off]     = (byte) (v >>> 8);
        a[off + 1] = (byte) v;
    }

    public static void putU32(byte[] a, int off, long v) {
        a[off]     = (byte) (v >>> 24);
        a[off + 1] = (byte) (v >>> 16);
        a[off + 2] = (byte) (v >>> 8);
        a[off + 3] = (byte) v;
    }

    public static int getU16(byte[] a, int off) {
        return ((a[off] & 0xFF) << 8) | (a[off + 1] & 0xFF);
    }

    public static long getU32(byte[] a, int off) {
        return ((long) (a[off] & 0xFF) << 24) | ((long) (a[off + 1] & 0xFF) << 16)
             | ((long) (a[off + 2] & 0xFF) << 8) | (a[off + 3] & 0xFF);
    }

    /** Escribe una string en un campo fijo: bytes UTF-8 + relleno {@code 0x00} (NUL). */
    public static void writeFixed(byte[] a, int off, byte[] s, int fieldLen) {
        System.arraycopy(s, 0, a, off, s.length);
        for (int i = s.length; i < fieldLen; i++) a[off + i] = 0;
    }

    /** Lee una string de un campo fijo: hasta el 1er NUL (o el campo entero). */
    public static String readFixed(byte[] a, int off, int fieldLen) {
        int n = 0;
        while (n < fieldLen && a[off + n] != 0) n++;
        return new String(a, off, n, StandardCharsets.UTF_8);
    }

    public static byte[] utf8(String s) { return s.getBytes(StandardCharsets.UTF_8); }
}
