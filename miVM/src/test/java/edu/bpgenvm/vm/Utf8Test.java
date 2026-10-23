package edu.bpgenvm.vm;

import org.junit.jupiter.api.Test;

import java.util.Random;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8/H5 (#711) — la rutina de validar y corregir UTF-8 (`Utf8`) contra un ORÁCULO escrito de otra forma: la tabla 3-7
 * del estándar de Unicode («Well-Formed UTF-8 Byte Sequences»), fila a fila, en vez de la máquina de estados de la
 * rutina. Un trozo malo es la «subparte máxima» (§3.9): el principio más largo de una fila que casa, o un byte si no
 * casa ninguna. Todas las secuencias de 1 a 3 bytes, y un millón al azar de 4 a 12.
 */
class Utf8Test {

    /** Tabla 3-7: el rango de cada byte de cada forma bien formada (de 2 a 4 bytes; la de 1 es 00..7F). */
    private static final int[][][] FILAS = {
        {{0xC2, 0xDF}, {0x80, 0xBF}},
        {{0xE0, 0xE0}, {0xA0, 0xBF}, {0x80, 0xBF}},
        {{0xE1, 0xEC}, {0x80, 0xBF}, {0x80, 0xBF}},
        {{0xED, 0xED}, {0x80, 0x9F}, {0x80, 0xBF}},
        {{0xEE, 0xEF}, {0x80, 0xBF}, {0x80, 0xBF}},
        {{0xF0, 0xF0}, {0x90, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}},
        {{0xF1, 0xF3}, {0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}},
        {{0xF4, 0xF4}, {0x80, 0x8F}, {0x80, 0xBF}, {0x80, 0xBF}},
    };

    /** El oráculo, un paso: >0 los bytes de un carácter válido; <0, -k: un trozo malo de k bytes. */
    private static int oraculoPaso(byte[] b, int i, int fin) {
        int c = b[i] & 0xFF;
        if (c <= 0x7F) return 1;
        for (int[][] fila : FILAS) {
            if (c < fila[0][0] || c > fila[0][1]) continue;
            int casan = 1;
            while (casan < fila.length && i + casan < fin) {
                int d = b[i + casan] & 0xFF;
                if (d < fila[casan][0] || d > fila[casan][1]) break;
                casan++;
            }
            return (casan == fila.length) ? casan : -casan;
        }
        return -1;   // no empieza ninguna fila
    }

    /** El oráculo entero: el primer trozo malo (-1 si no hay), y los bytes corregidos. */
    private static int oraculo(byte[] b, int n, byte[] out, int[] largo) {
        int i = 0, o = 0, primero = -1;
        while (i < n) {
            int p = oraculoPaso(b, i, n);
            if (p > 0) {
                for (int k = 0; k < p; k++) out[o++] = b[i + k];
                i += p;
            } else {
                if (primero < 0) primero = i;
                out[o++] = (byte) 0xEF; out[o++] = (byte) 0xBF; out[o++] = (byte) 0xBD;
                i -= p;
            }
        }
        largo[0] = o;
        return primero;
    }

    private static void comprueba(byte[] b, int n, byte[] esperado, byte[] obtenido, int[] largo) {
        int primero = oraculo(b, n, esperado, largo);
        int m = largo[0];
        assertEquals(primero, Utf8.primerMalo(b, 0, n), () -> "primerMalo de " + hex(b, n));
        assertEquals(m, Utf8.largoCorregido(b, 0, n), () -> "largoCorregido de " + hex(b, n));
        assertEquals(m, Utf8.corrige(b, 0, n, obtenido, 0), () -> "corrige (largo) de " + hex(b, n));
        for (int k = 0; k < m; k++) {
            if (esperado[k] != obtenido[k]) {
                assertEquals(hex(esperado, m), hex(obtenido, m), "corrige de " + hex(b, n));
            }
        }
    }

    @Test
    void todasLasDeUnoADosYTresBytes() {
        byte[] b = new byte[3], esperado = new byte[9], obtenido = new byte[9];
        int[] largo = new int[1];
        for (int n = 1; n <= 3; n++) {
            int total = 1 << (8 * n);
            for (int v = 0; v < total; v++) {
                for (int k = 0; k < n; k++) b[k] = (byte) (v >>> (8 * (n - 1 - k)));
                comprueba(b, n, esperado, obtenido, largo);
            }
        }
    }

    @Test
    void unMillonAlAzarDeCuatroADoce() {
        // un alfabeto con lo que importa: ASCII, continuaciones, cada principio y los bytes que no empiezan nada
        int[] alfabeto = {0x00, 0x41, 0x7F, 0x80, 0x8F, 0x90, 0x9F, 0xA0, 0xBF, 0xC0, 0xC1, 0xC2, 0xDF,
                          0xE0, 0xE1, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF3, 0xF4, 0xF5, 0xFF};
        Random r = new Random(711);
        byte[] b = new byte[12], esperado = new byte[36], obtenido = new byte[36];
        int[] largo = new int[1];
        for (int t = 0; t < 1_000_000; t++) {
            int n = 4 + r.nextInt(9);
            for (int k = 0; k < n; k++) b[k] = (byte) (r.nextBoolean() ? alfabeto[r.nextInt(alfabeto.length)] : r.nextInt(256));
            comprueba(b, n, esperado, obtenido, largo);
        }
    }

    @Test
    void casosDelEstandar() {
        // Unicode §3.9, tabla 3-8: «61 F1 80 80 E1 80 C2 62 80 63 80 BF 64» → a � � � b � c � � d
        byte[] b = bytes(0x61, 0xF1, 0x80, 0x80, 0xE1, 0x80, 0xC2, 0x62, 0x80, 0x63, 0x80, 0xBF, 0x64);
        byte[] out = new byte[64];
        int m = Utf8.corrige(b, 0, b.length, out, 0);
        assertEquals("61 EF BF BD EF BF BD EF BF BD 62 EF BF BD 63 EF BF BD EF BF BD 64", hex(out, m));
        assertEquals(1, Utf8.primerMalo(b, 0, b.length));
        // un sustituto en tres bytes: tres «�» (Java 8 pone uno)
        assertEquals(9, Utf8.largoCorregido(bytes(0xED, 0xA0, 0x80), 0, 3));
        // Latin-1: «año» = 61 F1 6F → a � o
        assertEquals(-1, Utf8.primerMalo(bytes(0xC3, 0xB1), 0, 2));   // ñ en UTF-8: válida
        assertEquals(1, Utf8.primerMalo(bytes(0x61, 0xF1, 0x6F), 0, 3));
        assertEquals("fromBytes: UTF-8 mal formado en el byte 1 (0xF1)", Utf8.mensaje("fromBytes", 1, 0xF1));
    }

    @Test
    void caracteres() {
        assertTrue(Utf8.esCaracter(0));
        assertTrue(Utf8.esCaracter(0xD7FF));
        assertFalse(Utf8.esCaracter(0xD800));
        assertFalse(Utf8.esCaracter(0xDFFF));
        assertTrue(Utf8.esCaracter(0xE000));
        assertTrue(Utf8.esCaracter(0x10FFFF));
        assertFalse(Utf8.esCaracter(0x110000));
        assertFalse(Utf8.esCaracter(-1));
    }

    private static byte[] bytes(int... v) {
        byte[] b = new byte[v.length];
        for (int i = 0; i < v.length; i++) b[i] = (byte) v[i];
        return b;
    }

    private static String hex(byte[] b, int n) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < n; i++) s.append(i > 0 ? " " : "").append(String.format("%02X", b[i] & 0xFF));
        return s.toString();
    }
}
