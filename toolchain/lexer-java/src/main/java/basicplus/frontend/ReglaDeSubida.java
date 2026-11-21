package basicplus.frontend;

/**
 * V7/H9 #624 (2-oct) — ¿hay que subir a la placa esta LIBRERÍA, con lo que la placa dice que tiene? La regla de
 * Eduardo, la de Java (2-oct): <i>«podemos exigir una versión concreta pero generalmente se pide una versión mínima. Si
 * es igual o superior a la mínima es válido. Por supuesto si el formato del fichero es diferente es incompatible, así
 * que los criterios iniciales deben ser: 1º nombre correcto, 2º formato compatible»</i>; y la mínima que pide un módulo
 * es <i>«en principio la versión con la que se compiló»</i>.
 *
 * <p>El nombre ya lo resolvió la placa ({@code STAT} por nombre, con el orden del RUN: #466); aquí van el formato (el
 * MAGIC) y la versión ({@code gen}, la «version» del proyecto, de la cabecera desde MOD8). El CRC sólo distingue
 * «idéntico» de «otra compilación que vale». Antes decidía él: la placa sólo decía el formato (hoy todo es MOD9) y el
 * CRC, así que cada recompilación de la misma versión se subía —y a {@code /lib}, que se carga en RAM: en la C6 la GUI
 * se quedaba sin memoria (#623)—. Es la misma regla con la que el cargador acepta una dependencia MOD9 (#523: sólo la
 * versión mínima; lo que use y no esté, lo dice el enlace con su nombre).
 *
 * <p>Sólo para LIBRERÍAS (la stdlib, lo instalado): un módulo DEL PROGRAMA se sube si cambia —el que se acaba de
 * recompilar tiene la misma versión y otro CRC—.
 */
public final class ReglaDeSubida {

    private ReglaDeSubida() {}

    /** Lo que se decide, y por qué (el motivo va al usuario tal cual). */
    public static final class Decision {
        public final boolean subir;
        public final String motivo;

        Decision(boolean subir, String motivo) {
            this.subir = subir;
            this.motivo = motivo;
        }

        @Override
        public String toString() {
            return (subir ? "subir: " : "no subir: ") + motivo;
        }
    }

    /** El formato de un {@code .mod} —su MAGIC: "MOD9"—, o null si no lo parece. */
    public static String formatoDe(byte[] mod) {
        if (mod == null || mod.length < 4 || mod[0] != 'M' || mod[1] != 'O' || mod[2] != 'D') return null;
        return new String(mod, 0, 4, java.nio.charset.StandardCharsets.US_ASCII);
    }

    /** La versión ({@code gen}, +36, big-endian) de un {@code .mod} MOD8/MOD9; -1 si su formato no la lleva. */
    public static long versionDe(byte[] mod) {
        String f = formatoDe(mod);
        if (f == null || mod.length < 44 || (f.charAt(3) != '8' && f.charAt(3) != '9')) return -1;
        return java.nio.ByteBuffer.wrap(mod, 36, 4).getInt() & 0xFFFFFFFFL;
    }

    /**
     * @param formatoPlaca el MAGIC que dice la placa; null = no lo tiene, o no es un módulo
     * @param versionPlaca su {@code gen}; -1 = no la dice (firmware anterior a #624, o un formato sin versión)
     * @param crcPlaca     su CRC; -1 = no lo dio
     * @param formatoLocal el del {@code .mod} contra el que se compiló el programa (el que se subiría)
     * @param versionLocal su {@code gen}: la versión MÍNIMA que pide el programa
     * @param crcLocal     su CRC
     */
    public static Decision decidir(String formatoPlaca, long versionPlaca, long crcPlaca,
                                   String formatoLocal, long versionLocal, long crcLocal) {
        if (formatoPlaca == null) {
            return new Decision(true, "la placa no lo tiene como módulo");
        }
        if (crcPlaca >= 0 && crcPlaca == crcLocal) {
            return new Decision(false, "ya en la placa, idéntico");
        }
        if (formatoLocal == null || !formatoLocal.equals(formatoPlaca)) {
            return new Decision(true, "formato distinto (la placa " + formatoPlaca + ", aquí " + formatoLocal
                    + "): incompatible");
        }
        if (versionPlaca < 0 || versionLocal < 0) {
            return new Decision(true, "la placa no dice su versión (un firmware anterior) y el CRC es otro");
        }
        if (versionPlaca < versionLocal) {
            return new Decision(true, "la placa tiene una versión anterior a la mínima (gen " + versionPlaca
                    + " < " + versionLocal + ")");
        }
        return new Decision(false, "versión " + versionPlaca + (versionPlaca > versionLocal ? " > " : " = ")
                + "mínima " + versionLocal + ", otra compilación (CRC distinto): vale");
    }
}
