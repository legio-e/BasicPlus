package basicplus.pack;

import java.util.Arrays;
import java.util.List;

import static basicplus.pack.PackFormat.*;

/**
 * Construye la imagen binaria de un pack a partir de sus entradas.
 *
 * <p><b>Determinista:</b> mismos inputs → mismos bytes → mismo CRC. La ÚNICA
 * no-determinación intencionada es la {@code fecha}, y por eso es un
 * <em>parámetro</em> (no se lee del reloj aquí). El slack final va a
 * {@code 0xFF}; el micro puede no programarlo (queda borrado = idéntico).
 * Ver ESPECIFICACION-PACKS-V4 §2, §7.
 */
public final class PackWriter {

    private PackWriter() {}

    /**
     * @param packName          nombre del pack (≤ 32 B UTF-8)
     * @param versionContenido  string libre informativo (≤ 16 B UTF-8); {@code null} = vacío
     * @param fechaUnix         timestamp Unix en segundos, sellado por el PC
     * @param entries           entradas en ORDEN (determinista)
     * @param blockSize         bloque de borrado de la flash destino (múltiplo de 4)
     */
    public static byte[] build(String packName, String versionContenido, long fechaUnix,
                               List<PackEntry> entries, int blockSize) throws PackException {
        if (blockSize <= 0 || (blockSize % ALIGN) != 0)
            throw new PackException("blockSize debe ser múltiplo positivo de " + ALIGN + ": " + blockSize);
        byte[] nameB = utf8(packName);
        if (nameB.length > NAME_LEN)
            throw new PackException("nombre de pack > " + NAME_LEN + " B: '" + packName + "'");
        byte[] vcB = utf8(versionContenido == null ? "" : versionContenido);
        if (vcB.length > VERCONT_LEN)
            throw new PackException("version_contenido > " + VERCONT_LEN + " B");

        // 1) validar entradas + medir el contenido
        int content = HEADER_SIZE;
        for (PackEntry e : entries) {
            byte[] t = utf8(e.tipo);
            if (t.length == 0 || t.length > TYPE_LEN)
                throw new PackException("tipo debe ser 1.." + TYPE_LEN + " chars: '" + e.tipo + "'");
            if (!isLowerFourcc(e.tipo))
                throw new PackException("tipo debe ser ASCII [a-z0-9]: '" + e.tipo + "'");
            byte[] n = utf8(e.nombre);
            if (n.length == 0 || n.length > NAME_LEN)
                throw new PackException("nombre de fichero 1.." + NAME_LEN + " B: '" + e.nombre + "'");
            content += ENTRY_HEADER_SIZE + align4(e.data.length);
        }
        int sizeTotal = alignUp(content, blockSize);

        // 2) imagen prellenada a 0xFF → slack, reservados y padding ya quedan bien
        byte[] img = new byte[sizeTotal];
        Arrays.fill(img, PAD);

        // 3) entradas, desde el fin de la cabecera de pack
        int off = HEADER_SIZE;
        for (PackEntry e : entries) {
            writeFixed(img, off + EOFF_TIPO,   utf8(e.tipo),   TYPE_LEN);
            writeFixed(img, off + EOFF_NOMBRE, utf8(e.nombre), NAME_LEN);
            putU32(img, off + EOFF_LONGITUD, e.data.length);
            // reservado del fichero (EOFF_RESERVADO..ENTRY_HEADER_SIZE) queda 0xFF
            System.arraycopy(e.data, 0, img, off + ENTRY_HEADER_SIZE, e.data.length);
            // padding de alineación tras los datos queda 0xFF
            off += ENTRY_HEADER_SIZE + align4(e.data.length);
        }
        int contentEnd = off;
        int crcContent = Crc16.compute(img, HEADER_SIZE, contentEnd - HEADER_SIZE);

        // 4) cabecera de pack
        putU32(img, OFF_MAGIC,      MAGIC & 0xFFFFFFFFL);
        putU16(img, OFF_VERFMT,     VERSION_FORMATO);
        putU16(img, OFF_FLAGS,      FLAGS_ACTIVE);
        putU32(img, OFF_SIZE_TOTAL, sizeTotal);
        writeFixed(img, OFF_NOMBRE, nameB, NAME_LEN);
        putU32(img, OFF_FECHA,      fechaUnix & 0xFFFFFFFFL);
        writeFixed(img, OFF_VERCONT, vcB, VERCONT_LEN);
        putU16(img, OFF_CRC_CONT,   crcContent);
        // reservado de pack (OFF_RESERVADO..OFF_CRC_CAB) queda 0xFF

        // crc_cab: cubre la cabecera EXCEPTO flags (6..8) y el propio crc_cab
        // (126..128), para que el tombstone (que reescribe flags 1->0 sin borrar
        // sector) NO invalide el CRC de la cabecera.
        int crcCab = Crc16.update(Crc16.update(Crc16.INIT, img, 0, OFF_FLAGS),
                                  img, OFF_SIZE_TOTAL, OFF_CRC_CAB - OFF_SIZE_TOTAL);
        putU16(img, OFF_CRC_CAB, crcCab);
        return img;
    }

    /** {@link #build} con el bloque de borrado por defecto (4 KB). */
    public static byte[] build(String packName, String versionContenido, long fechaUnix,
                               List<PackEntry> entries) throws PackException {
        return build(packName, versionContenido, fechaUnix, entries, DEFAULT_BLOCK);
    }

    private static boolean isLowerFourcc(String s) {
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
        }
        return true;
    }
}
