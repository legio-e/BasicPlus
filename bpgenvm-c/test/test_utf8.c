/* test_utf8.c — V8/H5 (#711): la rutina de VALIDAR Y CORREGIR UTF-8 (bpvm_internal.h: utf8_paso, utf8_primer_malo,
 * utf8_largo_corregido, utf8_corrige, utf8_es_caracter) contra un ORÁCULO escrito de otra forma: la tabla 3-7 del
 * estándar de Unicode («Well-Formed UTF-8 Byte Sequences»), fila a fila, en vez de la máquina de estados de la rutina.
 * Un trozo malo es la «subparte máxima» (§3.9): el principio más largo de una fila que casa, o un byte si no casa
 * ninguna; cada uno pasa a U+FFFD (EF BF BD).
 *
 * Todas las secuencias de 1 a 3 bytes (16,8 millones) y un millón al azar de 4 a 12, como Utf8Test de miVM: las dos
 * rutinas, cada una contra su oráculo; y la paridad, con el corpus (Utf8Descodifica).
 *
 *   make test-utf8
 */
#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;

/* Tabla 3-7: el rango de cada byte de cada forma bien formada de 2 a 4 bytes (la de 1 es 00..7F). */
typedef struct { int n; unsigned char r[4][2]; } fila_t;
static const fila_t FILAS[] = {
    {2, {{0xC2, 0xDF}, {0x80, 0xBF}}},
    {3, {{0xE0, 0xE0}, {0xA0, 0xBF}, {0x80, 0xBF}}},
    {3, {{0xE1, 0xEC}, {0x80, 0xBF}, {0x80, 0xBF}}},
    {3, {{0xED, 0xED}, {0x80, 0x9F}, {0x80, 0xBF}}},
    {3, {{0xEE, 0xEF}, {0x80, 0xBF}, {0x80, 0xBF}}},
    {4, {{0xF0, 0xF0}, {0x90, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}},
    {4, {{0xF1, 0xF3}, {0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}},
    {4, {{0xF4, 0xF4}, {0x80, 0x8F}, {0x80, 0xBF}, {0x80, 0xBF}}},
};

/* El oráculo, un paso: >0 los bytes de un carácter válido; <0, -k: un trozo malo de k bytes. */
static int oraculo_paso(const uint8_t* b, uint32_t i, uint32_t fin) {
    uint8_t c = b[i];
    if (c <= 0x7F) return 1;
    for (size_t f = 0; f < sizeof FILAS / sizeof FILAS[0]; f++) {
        const fila_t* fl = &FILAS[f];
        if (c < fl->r[0][0] || c > fl->r[0][1]) continue;
        int casan = 1;
        while (casan < fl->n && i + (uint32_t) casan < fin) {
            uint8_t d = b[i + (uint32_t) casan];
            if (d < fl->r[casan][0] || d > fl->r[casan][1]) break;
            casan++;
        }
        return (casan == fl->n) ? casan : -casan;
    }
    return -1;   /* no empieza ninguna fila */
}

/* El oráculo entero: el primer trozo malo (-1 si no hay) y los bytes corregidos (*largo). */
static int32_t oraculo(const uint8_t* b, uint32_t n, uint8_t* out, uint32_t* largo) {
    uint32_t i = 0, o = 0;
    int32_t primero = -1;
    while (i < n) {
        int p = oraculo_paso(b, i, n);
        if (p > 0) {
            for (int k = 0; k < p; k++) out[o++] = b[i + (uint32_t) k];
            i += (uint32_t) p;
        } else {
            if (primero < 0) primero = (int32_t) i;
            out[o++] = 0xEF; out[o++] = 0xBF; out[o++] = 0xBD;
            i += (uint32_t) -p;
        }
    }
    *largo = o;
    return primero;
}

static void hex(const uint8_t* b, uint32_t n, char* s, size_t cap) {
    size_t k = 0;
    s[0] = '\0';
    for (uint32_t i = 0; i < n && k + 4 < cap; i++) k += (size_t) snprintf(s + k, cap - k, i ? " %02X" : "%02X", b[i]);
}

static int comprueba(const uint8_t* b, uint32_t n) {
    uint8_t esperado[64], obtenido[64];
    uint32_t m;
    int32_t primero = oraculo(b, n, esperado, &m);
    int32_t r1 = utf8_primer_malo(b, n);
    uint32_t r2 = utf8_largo_corregido(b, n);
    uint32_t r3 = utf8_corrige(b, n, obtenido);
    if (r1 != primero || r2 != m || r3 != m || memcmp(esperado, obtenido, m) != 0) {
        char s[200], e[200], g[200];
        hex(b, n, s, sizeof s); hex(esperado, m, e, sizeof e); hex(obtenido, r3, g, sizeof g);
        if (fallos < 10)
            printf("  FAIL: [%s]: primer malo %d (el oráculo, %d), largo %u/%u (%u), [%s] (el oráculo, [%s])\n",
                   s, (int) r1, (int) primero, (unsigned) r2, (unsigned) r3, (unsigned) m, g, e);
        fallos++;
        return 0;
    }
    return 1;
}

int main(void) {
    uint8_t b[12];

    /* todas las de 1, 2 y 3 bytes */
    unsigned long buenas = 0;
    for (int n = 1; n <= 3; n++) {
        uint32_t total = 1u << (8 * n);
        for (uint32_t v = 0; v < total; v++) {
            for (int k = 0; k < n; k++) b[k] = (uint8_t) (v >> (8 * (n - 1 - k)));
            buenas += (unsigned long) comprueba(b, (uint32_t) n);
        }
    }
    printf("  %s: todas las de 1 a 3 bytes (%lu de 16843008), como el oráculo\n",
           buenas == 16843008ul ? "ok  " : "FAIL", buenas);

    /* un millón al azar de 4 a 12, con un alfabeto con lo que importa (y bytes cualesquiera) */
    static const uint8_t alfabeto[] = {0x00, 0x41, 0x7F, 0x80, 0x8F, 0x90, 0x9F, 0xA0, 0xBF, 0xC0, 0xC1, 0xC2, 0xDF,
                                       0xE0, 0xE1, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF3, 0xF4, 0xF5, 0xFF};
    uint32_t x = 711u;
    unsigned long azar = 0;
    for (int t = 0; t < 1000000; t++) {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        uint32_t n = 4u + x % 9u;
        for (uint32_t k = 0; k < n; k++) {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            b[k] = (x & 1u) ? alfabeto[(x >> 1) % sizeof alfabeto] : (uint8_t) (x >> 8);
        }
        azar += (unsigned long) comprueba(b, n);
    }
    printf("  %s: un millón al azar de 4 a 12 bytes (%lu bien)\n", azar == 1000000ul ? "ok  " : "FAIL", azar);

    /* el ejemplo del estándar (tabla 3-8) y un sustituto en tres bytes (tres «�»; Java 8 pone uno) */
    {
        static const uint8_t t38[] = {0x61, 0xF1, 0x80, 0x80, 0xE1, 0x80, 0xC2, 0x62, 0x80, 0x63, 0x80, 0xBF, 0x64};
        uint8_t out[64]; char s[200];
        uint32_t m = utf8_corrige(t38, sizeof t38, out);
        hex(out, m, s, sizeof s);
        int ok = strcmp(s, "61 EF BF BD EF BF BD EF BF BD 62 EF BF BD 63 EF BF BD EF BF BD 64") == 0
                 && utf8_primer_malo(t38, sizeof t38) == 1;
        printf("  %s: la tabla 3-8 del estándar: %s\n", ok ? "ok  " : "FAIL", s);
        if (!ok) fallos++;
        static const uint8_t sus[] = {0xED, 0xA0, 0x80};
        ok = utf8_largo_corregido(sus, 3) == 9;
        printf("  %s: un sustituto en tres bytes, tres U+FFFD\n", ok ? "ok  " : "FAIL");
        if (!ok) fallos++;
    }

    /* los caracteres */
    {
        int ok = utf8_es_caracter(0) && utf8_es_caracter(0xD7FF) && !utf8_es_caracter(0xD800) && !utf8_es_caracter(0xDFFF)
                 && utf8_es_caracter(0xE000) && utf8_es_caracter(0x10FFFF) && !utf8_es_caracter(0x110000)
                 && !utf8_es_caracter(-1);
        printf("  %s: los caracteres (sin sustitutos, de 0 a 10FFFF)\n", ok ? "ok  " : "FAIL");
        if (!ok) fallos++;
    }

    printf("[status=%s] %d fallo(s)\n", fallos ? "FAIL" : "OK", fallos);
    return fallos ? 1 : 0;
}
