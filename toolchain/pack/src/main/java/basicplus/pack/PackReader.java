package basicplus.pack;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static basicplus.pack.PackFormat.*;

/**
 * Parsea y VERIFICA una imagen de pack ({@code crc_cab} de la cabecera +
 * {@code crc_contenido} de los datos). Entrada malformada → {@link PackException}
 * con mensaje claro, nunca un crash (ESPECIFICACION-PACKS-V4 §7.8).
 */
public final class PackReader {

    private PackReader() {}

    /** Resultado de leer un pack: cabecera + lista de entradas. */
    public static final class Pack {
        public String  nombre;
        public String  versionContenido;
        public int     versionFormato;
        public int     flags;
        public boolean active;      // (flags & ALIVE_BIT) != 0
        public long    fechaUnix;
        public long    sizeTotal;
        public List<PackEntry> entries;
    }

    public static Pack read(byte[] img) throws PackException {
        if (img.length < HEADER_SIZE)
            throw new PackException("imagen (" + img.length + " B) menor que la cabecera (" + HEADER_SIZE + " B)");

        long magic = getU32(img, OFF_MAGIC);
        if (magic != (MAGIC & 0xFFFFFFFFL))
            throw new PackException(String.format("magic inválido: 0x%08X (esperado 0x%08X)", magic, MAGIC));

        int verFmt = getU16(img, OFF_VERFMT);
        if (verFmt != VERSION_FORMATO)
            throw new PackException("version_formato no soportada: " + verFmt + " (esta impl: " + VERSION_FORMATO + ")");

        // crc_cab ANTES de fiarnos de ningún otro campo de la cabecera.
        int crcCabStored = getU16(img, OFF_CRC_CAB);
        int crcCabCalc = Crc16.update(Crc16.update(Crc16.INIT, img, 0, OFF_FLAGS),
                                      img, OFF_SIZE_TOTAL, OFF_CRC_CAB - OFF_SIZE_TOTAL);
        if (crcCabStored != crcCabCalc)
            throw new PackException(String.format("crc_cab no cuadra: 0x%04X != 0x%04X (cabecera corrupta)",
                                                  crcCabStored, crcCabCalc));

        int  flags     = getU16(img, OFF_FLAGS);
        long sizeTotal = getU32(img, OFF_SIZE_TOTAL);
        if (sizeTotal < HEADER_SIZE || sizeTotal > img.length)
            throw new PackException("size_total (" + sizeTotal + ") fuera de rango (imagen " + img.length + " B)");

        Pack p = new Pack();
        p.versionFormato   = verFmt;
        p.flags            = flags;
        p.active           = (flags & ALIVE_BIT) != 0;
        p.sizeTotal        = sizeTotal;
        p.nombre           = readFixed(img, OFF_NOMBRE, NAME_LEN);
        p.fechaUnix        = getU32(img, OFF_FECHA);
        p.versionContenido = readFixed(img, OFF_VERCONT, VERCONT_LEN);
        int crcContentStored = getU16(img, OFF_CRC_CONT);

        // recorrido secuencial de entradas: paramos al llegar al slack (tipo
        // 0xFF..FF) o al fin del pack.
        List<PackEntry> entries = new ArrayList<>();
        int off = HEADER_SIZE;
        while (off + 4 <= sizeTotal) {
            long tipo = getU32(img, off);
            if (tipo == TIPO_END) break;
            if (off + ENTRY_HEADER_SIZE > sizeTotal)
                throw new PackException("cabecera de fichero truncada en offset " + off);
            String etipo   = readFixed(img, off + EOFF_TIPO,   TYPE_LEN);
            String enombre = readFixed(img, off + EOFF_NOMBRE, NAME_LEN);
            long   elen    = getU32(img, off + EOFF_LONGITUD);
            int    dataOff = off + ENTRY_HEADER_SIZE;
            if (dataOff + elen > sizeTotal)
                throw new PackException("datos de '" + enombre + "' (" + elen + " B) exceden el pack");
            byte[] data = Arrays.copyOfRange(img, dataOff, (int) (dataOff + elen));
            entries.add(new PackEntry(etipo, enombre, data, dataOff));
            off = dataOff + align4((int) elen);
        }
        int contentEnd = off;
        int crcContentCalc = Crc16.compute(img, HEADER_SIZE, contentEnd - HEADER_SIZE);
        if (crcContentStored != crcContentCalc)
            throw new PackException(String.format("crc_contenido no cuadra: 0x%04X != 0x%04X (datos corruptos)",
                                                  crcContentStored, crcContentCalc));
        p.entries = entries;
        return p;
    }
}
