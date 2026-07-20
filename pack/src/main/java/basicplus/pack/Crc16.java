package basicplus.pack;

/**
 * CRC-16/CCITT-FALSE — poly {@code 0x1021}, init {@code 0xFFFF}, SIN reflexión
 * de entrada ni salida, SIN XOR final.
 *
 * <p>Es EXACTAMENTE el CRC que el lector del micro tendrá que igualar: un CRC
 * distinto = el micro RECHAZA TODOS los packs (ESPECIFICACION-PACKS-V4 §2.1).
 * Valor de comprobación estándar: {@code crc("123456789") == 0x29B1}.
 */
public final class Crc16 {

    /** Valor inicial del registro (init). */
    public static final int INIT = 0xFFFF;

    private Crc16() {}

    /**
     * Continúa el CRC sobre un rango. Permite CRCs NO contiguos (p.ej. la
     * cabecera del pack se cubre saltándose el campo {@code flags}).
     */
    public static int update(int crc, byte[] data, int off, int len) {
        for (int i = 0; i < len; i++) {
            crc ^= (data[off + i] & 0xFF) << 8;
            for (int b = 0; b < 8; b++) {
                if ((crc & 0x8000) != 0) crc = (crc << 1) ^ 0x1021;
                else                     crc = (crc << 1);
                crc &= 0xFFFF;
            }
        }
        return crc & 0xFFFF;
    }

    public static int compute(byte[] data, int off, int len) {
        return update(INIT, data, off, len);
    }

    public static int compute(byte[] data) {
        return compute(data, 0, data.length);
    }
}
