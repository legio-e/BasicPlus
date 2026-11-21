package edu.bpgenvm.vm;

/**
 * V8 #716 — QUÉ ES UN NÚMERO para parseInt y parseFloat, lo mismo en las dos VMs (Eduardo: «Me valen las propuestas»).
 * Antes cada VM heredaba lo de su biblioteca: la VM-C, strtol y strtod, que leen lo que pueden y callan ("12abc" → 12);
 * ésta, Long.parseLong y Double.parseDouble, que mataban el hilo sin que el try de BP lo viera. La regla, sobre los
 * BYTES de la cadena (un dígito es 0-9 de ASCII; ni los de otras escrituras ni nada de la configuración regional):
 *   - alrededor, espacios: los bytes de 0 a 32, como el trim de Java; dentro, nada;
 *   - entero:  [+-] dígitos, y que quepa en 32 bits;
 *   - decimal: [+-] (dígitos [. dígitos] | . dígitos) [(e|E) [+-] dígitos] (sin hexadecimal, inf ni nan), y que quepa
 *     en un float.
 * Lo que no, un RuntimeError atrapable con el texto que llegó (40 bytes como mucho, cortado por caracteres). El espejo,
 * en bpgenvm-c/src/builtins.c (num_ini, num_fin, num_entero, num_es_decimal, num_muestra).
 */
final class Numeros {
    private Numeros() {}

    static final int VALE = 0, NO_ES = 1, NO_CABE = 2;

    /** Dónde empieza el número en m[off, off+n), saltando los espacios de delante. */
    static int ini(byte[] m, int off, int n) {
        int i = off, f = off + n;
        while (i < f && (m[i] & 0xFF) <= 32) i++;
        return i;
    }

    /** Dónde acaba, sin los espacios de detrás (desde a, el principio que dio ini). */
    static int fin(byte[] m, int a, int off, int n) {
        int f = off + n;
        while (f > a && (m[f - 1] & 0xFF) <= 32) f--;
        return f;
    }

    private static boolean digito(byte c) { return c >= '0' && c <= '9'; }

    /** El entero de m[a, b): VALE (con el valor en v[0]), NO_ES o NO_CABE. Mira todos los bytes antes de decir
     *  NO_CABE: "99999999999x" no es un número. */
    static int entero(byte[] m, int a, int b, int[] v) {
        int i = a;
        boolean neg = false;
        if (i < b && (m[i] == '+' || m[i] == '-')) { neg = m[i] == '-'; i++; }
        if (i >= b) return NO_ES;
        long acc = 0;
        boolean grande = false;
        for (; i < b; i++) {
            if (!digito(m[i])) return NO_ES;
            if (!grande) {
                acc = acc * 10 + (m[i] - '0');
                if (acc > 2147483648L) grande = true;
            }
        }
        if (grande || (!neg && acc > 2147483647L)) return NO_CABE;
        v[0] = (int) (neg ? -acc : acc);
        return VALE;
    }

    /** ¿Es m[a, b) un decimal? [+-] (dígitos [. dígitos] | . dígitos) [(e|E) [+-] dígitos]. */
    static boolean esDecimal(byte[] m, int a, int b) {
        int i = a;
        if (i < b && (m[i] == '+' || m[i] == '-')) i++;
        int ent = 0, frac = 0;
        while (i < b && digito(m[i])) { i++; ent++; }
        if (i < b && m[i] == '.') {
            i++;
            while (i < b && digito(m[i])) { i++; frac++; }
        }
        if (ent == 0 && frac == 0) return false;
        if (i < b && (m[i] == 'e' || m[i] == 'E')) {
            i++;
            if (i < b && (m[i] == '+' || m[i] == '-')) i++;
            int exp = 0;
            while (i < b && digito(m[i])) { i++; exp++; }
            if (exp == 0) return false;
        }
        return i == b;
    }

    /** Lo que llegó, entre comillas, para el texto del error: 40 bytes como mucho, cortado por caracteres y con «...». */
    static String muestra(byte[] m, int off, int n) {
        int k = (n <= 40) ? n : 40;
        if (n > 40) while (k > 0 && (m[off + k] & 0xC0) == 0x80) k--;
        return "'" + new String(m, off, k, java.nio.charset.StandardCharsets.UTF_8) + (n > 40 ? "..." : "") + "'";
    }
}
