package edu.bpgenvm.vm;

/**
 * V8/H5 (#711) — VALIDAR Y CORREGIR UTF-8, por el estándar de Unicode (§3.9, «U+FFFD Substitution of Maximal
 * Subparts»; el descodificador UTF-8 del WHATWG Encoding Standard hace exactamente lo mismo).
 *
 * La regla de Eduardo (22-oct): un string es un invariante; lo que viene de fuera se DESCODIFICA al nacer la cadena
 * (`fromBytes`, `readFile`, `readLine`): por defecto, `EncodingError`; si se ignora el error, cada trozo malo pasa a
 * U+FFFD («�», `EF BF BD`). Un trozo malo es una «subparte máxima»: el principio más largo de un carácter que se corta
 * (`E2 82` y una `x`: uno, y la `x` sigue), o un byte que no puede empezar ninguno (un `80`, un `C0`, un `FF`: uno cada
 * byte). Fuera quedan los sobrelargos, los sustitutos (`ED A0 80`: tres) y lo que pasa de 10FFFF.
 *
 * No se usa el descodificador de Java: Java 8 se aparta del estándar en un caso (un sustituto escrito en tres bytes da
 * un «�», no tres), y la VM-C tiene que dar lo mismo byte a byte. El espejo, en `bpgenvm-c/src/builtins.c`
 * (`utf8_paso`, `utf8_primer_malo`, `utf8_largo_corregido`, `utf8_corrige`).
 */
final class Utf8 {
    private Utf8() {}

    /**
     * Un paso desde `i` (con `i < fin` y `b[i]` NO ASCII): los bytes del carácter que empieza ahí (2 a 4) si es
     * válido, o `-k` si los `k` bytes desde `i` son un trozo malo (k ≥ 1).
     */
    static int paso(byte[] b, int i, int fin) {
        int c = b[i] & 0xFF;
        int necesita, lo = 0x80, hi = 0xBF;
        if (c >= 0xC2 && c <= 0xDF) {
            necesita = 1;
        } else if (c >= 0xE0 && c <= 0xEF) {
            necesita = 2;
            if (c == 0xE0) lo = 0xA0;          // sin sobrelargos de tres bytes
            else if (c == 0xED) hi = 0x9F;     // sin sustitutos (D800..DFFF)
        } else if (c >= 0xF0 && c <= 0xF4) {
            necesita = 3;
            if (c == 0xF0) lo = 0x90;          // sin sobrelargos de cuatro bytes
            else if (c == 0xF4) hi = 0x8F;     // nada por encima de 10FFFF
        } else {
            return -1;                          // 80..C1 y F5..FF no empiezan ningún carácter
        }
        for (int k = 1; k <= necesita; k++) {
            if (i + k >= fin) return -k;        // cortado por el final: el trozo son los k que hay
            int d = b[i + k] & 0xFF;
            if (d < lo || d > hi) return -k;    // el byte k no sigue: el trozo son los k de antes, y él empieza otro
            lo = 0x80;
            hi = 0xBF;
        }
        return necesita + 1;
    }

    /** Dónde empieza el primer trozo malo de `b[off, off+n)`, contado desde `off`; -1 si todo es UTF-8 válido. */
    static int primerMalo(byte[] b, int off, int n) {
        int fin = off + n, i = off;
        while (i < fin) {
            if (b[i] >= 0) { i++; continue; }   // el atajo: un byte ASCII
            int p = paso(b, i, fin);
            if (p < 0) return i - off;
            i += p;
        }
        return -1;
    }

    /** Los bytes que salen al cambiar cada trozo malo por U+FFFD (tres bytes). */
    static int largoCorregido(byte[] b, int off, int n) {
        int fin = off + n, i = off, largo = 0;
        while (i < fin) {
            int p = (b[i] >= 0) ? 1 : paso(b, i, fin);
            if (p > 0) { largo += p; i += p; }
            else { largo += 3; i -= p; }
        }
        return largo;
    }

    /** Escribe en `out`, desde `outOff`, los bytes con cada trozo malo cambiado por U+FFFD; devuelve cuántos. */
    static int corrige(byte[] b, int off, int n, byte[] out, int outOff) {
        int fin = off + n, i = off, o = outOff;
        while (i < fin) {
            int p = (b[i] >= 0) ? 1 : paso(b, i, fin);
            if (p > 0) {
                System.arraycopy(b, i, out, o, p);
                o += p;
                i += p;
            } else {
                out[o++] = (byte) 0xEF;
                out[o++] = (byte) 0xBF;
                out[o++] = (byte) 0xBD;
                i -= p;
            }
        }
        return o - outOff;
    }

    /** ¿Es `cp` un carácter? Un punto de código de Unicode (0..10FFFF) que no es un sustituto (D800..DFFF). */
    static boolean esCaracter(int cp) {
        return cp >= 0 && cp <= 0x10FFFF && (cp < 0xD800 || cp > 0xDFFF);
    }

    /** El texto del error, el mismo en las dos VMs: `<que>: UTF-8 mal formado en el byte 5 (0xF1)`. */
    static String mensaje(String que, int pos, int byteMalo) {
        return que + ": UTF-8 mal formado en el byte " + pos + " (0x" + hex2(byteMalo) + ")";
    }

    private static String hex2(int v) {
        final String d = "0123456789ABCDEF";
        return "" + d.charAt((v >> 4) & 0xF) + d.charAt(v & 0xF);
    }
}
