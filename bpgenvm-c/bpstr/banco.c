/*
 * banco.c — lo que tarda cada función de bpstr (V8 #730), en ns por byte: frente a su pareja de la libc donde la hay
 * (memchr, memcmp, memmem, memcpy, memset) y frente al bucle de la VM de antes (oraculo.h, «hoy»), con 16, 256, 2048
 * y 16 384 bytes. El criterio de Eduardo (3-nov): no apartarse más de un ±25 % de la libc de cada familia; lo que no
 * tiene pareja, más deprisa que hoy y cerca del ritmo de leer la memoria una vez (memchr de un byte que no está).
 *
 * Cada medida: las vueltas que hagan falta para unos 20 ms, la mejor de tres. En el PC, el reloj de Windows o el de
 * POSIX; en una placa (BPSTR_BANCO_PLACA), los dos de su imagen de prueba: bpstr_banco_ns y bpstr_banco_linea.
 */
#define _GNU_SOURCE   /* memmem, donde la libc lo tiene */
#include "bpstr.h"
#include "oraculo.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(BPSTR_BANCO_PLACA)
uint64_t bpstr_banco_ns(void);
void bpstr_banco_linea(const char* s);
#define CON_MEMMEM 1
#elif defined(_WIN32)
#include <windows.h>
static uint64_t bpstr_banco_ns(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (uint64_t) ((double) c.QuadPart * 1e9 / (double) f.QuadPart);
}
static void bpstr_banco_linea(const char* s) { fputs(s, stdout); fputc('\n', stdout); fflush(stdout); }
#define CON_MEMMEM 0   /* la libc de MinGW no lo tiene */
#else
#include <time.h>
static uint64_t bpstr_banco_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t) t.tv_sec * 1000000000u + (uint64_t) t.tv_nsec;
}
static void bpstr_banco_linea(const char* s) { fputs(s, stdout); fputc('\n', stdout); fflush(stdout); }
#define CON_MEMMEM 1
#endif

static void di(const char* fmt, ...) {
    char l[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l, sizeof l, fmt, ap);
    va_end(ap);
    bpstr_banco_linea(l);
}

#define MAXN 16384u
static uint8_t texto[MAXN + 64], acentos[MAXN + 64], otra[MAXN + 64], salida[3u * MAXN + 64];
static const uint8_t* aguja_corta = (const uint8_t*) "ab@";
static uint8_t aguja_larga[32];
static volatile uint32_t sumidero;

/* ── los candidatos: cada uno hace UNA vez su trabajo con n bytes ── */
typedef void (*fn_t)(uint32_t n);

static void f_es_ascii(uint32_t n)      { sumidero = (uint32_t) bpstr_es_ascii(texto, n); }
static void h_es_ascii(uint32_t n)      { sumidero = (uint32_t) bpvm_bytes_ascii(texto, n); }
static void l_lee(uint32_t n)           { sumidero = (uint32_t) (uintptr_t) memchr(texto, '@', n); }
static void f_cuenta(uint32_t n)        { sumidero = bpstr_cuenta(acentos, n); }
static void h_cuenta(uint32_t n)        { sumidero = utf8_cp_count(acentos, n); }
static void f_avanza(uint32_t n)        { uint32_t h; sumidero = bpstr_avanza(acentos, n, 0, n, &h); }
static void h_avanza(uint32_t n)        { uint32_t h; sumidero = ref_avanza(acentos, n, 0, n, &h); }
static void f_retrocede(uint32_t n)     { sumidero = bpstr_retrocede(acentos, n, n); }
static void h_retrocede(uint32_t n)     { sumidero = ref_retrocede(acentos, n, n); }
static void f_valida(uint32_t n)        { sumidero = (uint32_t) bpstr_primer_malo(acentos, n); }
static void h_valida(uint32_t n)        { sumidero = (uint32_t) utf8_primer_malo(acentos, n); }
static void f_corrige(uint32_t n)       { sumidero = bpstr_corrige(acentos, n, salida); }
static void h_corrige(uint32_t n)       { sumidero = utf8_corrige(acentos, n, salida); }
static void l_copia_ac(uint32_t n)      { memcpy(salida, acentos, n); sumidero = salida[0]; }
static void f_busca_byte(uint32_t n)    { sumidero = (uint32_t) bpstr_busca_byte(texto, n, '@'); }
static void h_busca_byte(uint32_t n)    { sumidero = (uint32_t) bp_find_bytes(texto, n, (const uint8_t*) "@", 1, 0); }
static void l_busca_byte(uint32_t n)    { sumidero = (uint32_t) (uintptr_t) memchr(texto, '@', n); }
static void f_busca_corta(uint32_t n)   { sumidero = (uint32_t) bpstr_busca(texto, n, aguja_corta, 3, 0); }
static void h_busca_corta(uint32_t n)   { sumidero = (uint32_t) bp_find_bytes(texto, n, aguja_corta, 3, 0); }
static void f_busca_larga(uint32_t n)   { sumidero = (uint32_t) bpstr_busca(texto, n, aguja_larga, 32, 0); }
static void h_busca_larga(uint32_t n)   { sumidero = (uint32_t) bp_find_bytes(texto, n, aguja_larga, 32, 0); }
#if CON_MEMMEM
static void l_busca_corta(uint32_t n)   { sumidero = (uint32_t) (uintptr_t) memmem(texto, n, aguja_corta, 3); }
static void l_busca_larga(uint32_t n)   { sumidero = (uint32_t) (uintptr_t) memmem(texto, n, aguja_larga, 32); }
#endif
static void f_compara(uint32_t n)       { sumidero = (uint32_t) bpstr_compara(texto, n, otra, n); }
static void l_compara(uint32_t n)       { sumidero = (uint32_t) ref_compara(texto, n, otra, n); }
static void f_mayus(uint32_t n)         { sumidero = bpstr_mayusculas(texto, n, salida); }
static void h_mayus(uint32_t n)         { sumidero = ref_cambia(texto, n, salida, 1); }
static void f_mayus_ac(uint32_t n)      { sumidero = bpstr_mayusculas(acentos, n, salida); }
static void h_mayus_ac(uint32_t n)      { sumidero = ref_cambia(acentos, n, salida, 1); }
static void f_copia(uint32_t n)         { bpstr_copia(salida, texto, n); sumidero = salida[n - 1u]; }
static void l_copia(uint32_t n)         { memcpy(salida, texto, n); sumidero = salida[n - 1u]; }
static void f_rellena(uint32_t n)       { bpstr_rellena(salida, (uint8_t) n, n); sumidero = salida[n - 1u]; }
static void l_rellena(uint32_t n)       { memset(salida, (int) (uint8_t) n, n); sumidero = salida[n - 1u]; }

/* ns por byte de f con n bytes: la mejor de tres tandas de unos 20 ms */
static double mide(fn_t f, uint32_t n) {
    uint64_t t0, t;
    uint32_t vueltas = 1, v, r;
    double mejor = 1e30;
    for (;;) {                                         /* cuántas vueltas dan unos 2 ms */
        t0 = bpstr_banco_ns();
        for (v = 0; v < vueltas; v++) f(n);
        t = bpstr_banco_ns() - t0;
        if (t >= 2000000u || vueltas >= (1u << 28)) break;
        vueltas *= 2u;
    }
    vueltas = (uint32_t) ((double) vueltas * 20e6 / (double) (t ? t : 1u)) + 1u;
    for (r = 0; r < 3; r++) {
        double ns;
        t0 = bpstr_banco_ns();
        for (v = 0; v < vueltas; v++) f(n);
        ns = (double) (bpstr_banco_ns() - t0) / (double) vueltas / (double) n;
        if (ns < mejor) mejor = ns;
    }
    return mejor;
}

static void fila(const char* que, fn_t f, fn_t libc, fn_t hoy) {
    static const uint32_t tam[4] = {16u, 256u, 2048u, 16384u};
    for (int i = 0; i < 4; i++) {
        uint32_t n = tam[i];
        double b = mide(f, n), l = libc ? mide(libc, n) : 0.0, h = hoy ? mide(hoy, n) : 0.0;
        char cl[40] = "", ch[40] = "";
        if (libc) snprintf(cl, sizeof cl, "%7.3f  %+5.0f %%", l, 100.0 * (b - l) / l);
        if (hoy) snprintf(ch, sizeof ch, "%7.3f  x%5.1f", h, h / b);
        di("%-26s %6u  %7.3f   %-16s   %-16s", que, (unsigned) n, b, cl, ch);
    }
}

int main(void) {
    uint32_t i, n;
    uint32_t semilla = 0x2545F491u;
    /* el texto: palabras de letras con espacios y comas; con acentos, una de cada diez letras es una de dos bytes */
    for (i = 0; i < MAXN + 64; i++) {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        texto[i] = (uint8_t) ((semilla % 7u == 0) ? ' ' : (semilla % 23u == 0) ? ',' : 'a' + (semilla >> 8) % 26u);
    }
    for (i = 0, n = 0; n < MAXN + 60; i++) {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        if (semilla % 10u == 0) n += bpstr_codifica(0xE1u + (semilla >> 8) % 26u, acentos + n);   /* á, é, ñ… */
        else acentos[n++] = texto[i % MAXN];
    }
    memcpy(otra, texto, sizeof otra);
    memcpy(aguja_larga, texto + 5000, 31);
    aguja_larga[31] = '@';                             /* casi un trozo del texto: no está */

    di("bpstr, el banco (ns por byte; libc: la pareja de la libc y lo que se aparta; hoy: el bucle de antes y lo que");
    di("gana bpstr). Punteros de %u bits.", (unsigned) (8u * sizeof(void*)));
    di("%-26s %6s  %7s   %-16s   %-16s", "funcion", "bytes", "bpstr", "libc", "hoy");
    fila("es_ascii", f_es_ascii, l_lee, h_es_ascii);
    fila("cuenta (con acentos)", f_cuenta, l_lee, h_cuenta);
    fila("avanza hasta el final", f_avanza, NULL, h_avanza);
    fila("retrocede hasta el principio", f_retrocede, NULL, h_retrocede);
    fila("primer_malo (valida)", f_valida, l_lee, h_valida);
    fila("corrige (sin nada malo)", f_corrige, l_copia_ac, h_corrige);
    fila("busca_byte (no esta)", f_busca_byte, l_busca_byte, h_busca_byte);
#if CON_MEMMEM
    fila("busca 3 B (no esta)", f_busca_corta, l_busca_corta, h_busca_corta);
    fila("busca 32 B (no esta)", f_busca_larga, l_busca_larga, h_busca_larga);
#else
    fila("busca 3 B (no esta)", f_busca_corta, NULL, h_busca_corta);
    fila("busca 32 B (no esta)", f_busca_larga, NULL, h_busca_larga);
#endif
    fila("compara (iguales)", f_compara, l_compara, NULL);
    fila("mayusculas (ASCII)", f_mayus, NULL, h_mayus);
    fila("mayusculas (con acentos)", f_mayus_ac, NULL, h_mayus_ac);
    fila("copia", f_copia, l_copia, NULL);
    fila("rellena", f_rellena, l_rellena, NULL);
    di("fin del banco");
    return 0;
}
