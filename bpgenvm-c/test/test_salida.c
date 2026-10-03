/*
 * test_salida.c — V7/H10, el RUN común, paso 1 (3-oct): LA SALIDA DEL PROGRAMA, COMÚN (wire_v1_salida).
 *
 * Cuatro sinks de OUTPUT fallaban distinto con lo mismo: la STM32 tiraba la línea cuyo escape no cabía, el micro
 * simulado cortaba en el primer NUL, la ESP32 partía un carácter entre dos tramas y el hilo `io` trocea a 512 B sin
 * mirar caracteres. Aquí, sin cable: cada trama que sale se recoge, se comprueba que es una línea JSON entera con
 * UTF-8 válido y sin un escape a medias, que no pasa de WIRE_V1_SALIDA_TRAMA, y que deshaciendo los escapes de todas
 * se vuelve EXACTAMENTE a lo que imprimió el programa (con U+FFFD donde había bytes que no son UTF-8).
 *
 *   make test-salida
 */
#include "bpvm_wire_v1.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); else { printf("  FAIL: %s\n", (m)); g_fail++; } } while (0)

/* Lo que sale por el cable: las tramas, y los datos ya desescapados uno detrás de otro. */
static int    g_tramas, g_mal, g_max;
static char   g_datos[16384];
static size_t g_ndatos;

static int hexv(char c) { return (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1; }

void wire_v1_send_line(const char* l, size_t n) {
    static const char CAB[] = "{\"type\":\"OUTPUT\",\"session\":7,\"stream\":\"stdout\",\"data\":\"";
    g_tramas++;
    if (n < sizeof CAB + 1 || memcmp(l, CAB, sizeof CAB - 1) != 0 || l[n - 2] != '"' || l[n - 1] != '}') { g_mal++; return; }
    size_t i = sizeof CAB - 1, fin = n - 2;
    if ((int) (fin - i) > g_max) g_max = (int) (fin - i);
    while (i < fin) {
        unsigned char c = (unsigned char) l[i];
        if (c == '\\') {
            if (i + 1 >= fin) { g_mal++; return; }          /* un escape partido */
            char e = l[i + 1];
            if (e == 'u') {
                if (i + 6 > fin) { g_mal++; return; }
                int v = 0;
                for (int k = 2; k < 6; k++) { int h = hexv(l[i + k]); if (h < 0) { g_mal++; return; } v = v * 16 + h; }
                if (v == 0xFFFD) { memcpy(g_datos + g_ndatos, "\xEF\xBF\xBD", 3); g_ndatos += 3; }
                else g_datos[g_ndatos++] = (char) v;
                i += 6;
            } else {
                char d = (e == 'n') ? '\n' : (e == 'r') ? '\r' : (e == 't') ? '\t' : (e == '"') ? '"' : (e == '\\') ? '\\' : 0;
                if (!d) { g_mal++; return; }
                g_datos[g_ndatos++] = d; i += 2;
            }
        } else if (c < 0x20) { g_mal++; return; }        /* un control sin escapar */
        else if (c < 0x80) { g_datos[g_ndatos++] = (char) c; i++; }
        else {                                            /* UTF-8 ENTERO y válido, o la trama no vale */
            unsigned want = (c >= 0xC2 && c <= 0xDF) ? 2 : (c >= 0xE0 && c <= 0xEF) ? 3 : (c >= 0xF0 && c <= 0xF4) ? 4 : 0;
            if (want == 0 || i + want > fin) { g_mal++; return; }
            for (unsigned k = 1; k < want; k++) if (((unsigned char) l[i + k] & 0xC0) != 0x80) { g_mal++; return; }
            memcpy(g_datos + g_ndatos, l + i, want); g_ndatos += want; i += want;
        }
    }
}
void wire_v1_send_bulk(const uint8_t* d, size_t n) { (void) d; (void) n; }

static void limpia(void) { g_tramas = g_mal = g_max = 0; g_ndatos = 0; }

static int igual(const char* esperado, size_t n) { return g_ndatos == n && memcmp(g_datos, esperado, n) == 0; }

int main(void) {
    wire_v1_salida_t s;
    char m[200];
    static char big[12000], esp[12000];

    printf("--- el RUN comun, paso 1: la salida del programa, comun ---\n");

    limpia(); wire_v1_salida_inicia(&s, 7);
    wire_v1_salida(&s, "a\0b\n", 4);
    CHECK(g_tramas == 1 && g_mal == 0 && igual("a\0b\n", 4), "un NUL en mitad: sale \\u0000 y lo de detras (el sim cortaba ahi)");

    limpia();
    memset(big, 1, 512); big[512] = '\n';
    wire_v1_salida(&s, big, 513);
    snprintf(m, sizeof m, "512 controles: escapan a 3072 B y salen ENTEROS, en %d tramas de <= %u (la STM32 los tiraba)", g_tramas, WIRE_V1_SALIDA_TRAMA);
    CHECK(g_mal == 0 && g_tramas == 4 && g_max <= (int) WIRE_V1_SALIDA_TRAMA && igual(big, 513), m);

    limpia();
    wire_v1_salida(&s, "aaa\xC3", 4);
    wire_v1_salida(&s, "\xB1z", 2);
    CHECK(g_mal == 0 && igual("aaa\xC3\xB1z", 6), "una ñ partida entre dos llamadas (el corte a 512 de io): sale ENTERA en la segunda");

    limpia();
    memset(big, 'a', 1023); memcpy(big + 1023, "\xE2\x82\xAC", 3);
    wire_v1_salida(&s, big, 1026);
    CHECK(g_mal == 0 && g_tramas == 2 && igual(big, 1026), "un € que no cabe al final de una trama: pasa entero a la siguiente");

    limpia();
    memset(big, 'a', 1023); big[1023] = '"';
    wire_v1_salida(&s, big, 1024);
    CHECK(g_mal == 0 && g_tramas == 2 && igual(big, 1024), "un escape (\\\") que no cabe: pasa entero a la siguiente");

    limpia();
    wire_v1_salida(&s, "x\xFFy", 3);
    CHECK(g_mal == 0 && igual("x\xEF\xBF\xBDy", 5), "un byte que no es UTF-8: U+FFFD (en JSON no cabe otra cosa), y no se pierde lo de alrededor");

    limpia();
    wire_v1_salida(&s, "fin\xE2\x82", 5);
    wire_v1_salida_cierra(&s);
    CHECK(g_mal == 0 && igual("fin\xEF\xBF\xBD", 6), "un carácter a medias al acabar el programa: U+FFFD, no se tira");

    /* 10 KB de todo un poco, en trozos de tamaños raros (como los da el hilo io), y vuelve a ser lo mismo */
    limpia();
    {
        size_t n = 0, k = 0;
        static const char* piezas[] = { "hola ", "\xC3\xB1", "\xE2\x82\xAC", "\xF0\x9F\x99\x82", "\"", "\\", "\n", "\x01", "\t" };
        while (n < 10000) { const char* p = piezas[k++ % 9]; size_t L = strlen(p); if (L == 0) { big[n++] = 1; continue; } memcpy(big + n, p, L); n += L; }
        memcpy(esp, big, n);
        size_t i = 0, paso = 1;
        while (i < n) { size_t t = (i + paso > n) ? n - i : paso; wire_v1_salida(&s, big + i, t); i += t; paso = (paso * 7 + 3) % 517 + 1; }
        wire_v1_salida_cierra(&s);
        snprintf(m, sizeof m, "%zu B de todo un poco en trozos de 1 a 517: %d tramas, ninguna mala, la mayor de %d B, y se reconstruye igual", n, g_tramas, g_max);
        CHECK(g_mal == 0 && g_max <= (int) WIRE_V1_SALIDA_TRAMA && igual(esp, n), m);
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
