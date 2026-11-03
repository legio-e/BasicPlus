/*
 * banco.c — lo que tarda cada función de bpstr (V8 #730), en ns por byte: frente a su pareja de la libc donde la hay
 * (memchr, memcmp, memmem, memcpy, memset) y frente al bucle de la VM de antes (oraculo.h, «hoy»), con 16, 256, 2048
 * y 16 384 bytes. El criterio de Eduardo (3-nov): no apartarse más de un ±25 % de la libc de cada familia; lo que no
 * tiene pareja, más deprisa que hoy y cerca del ritmo de leer la memoria una vez (memchr de un byte que no está).
 *
 * Cada medida: las vueltas que hagan falta para unos 20 ms, la mejor de tres. En el PC, el reloj de Windows o el de
 * POSIX; en una placa (BPSTR_PLACA: una imagen de prueba de la VM, con placa.c), los de placa.c: el reloj, la salida
 * (/app/bpstr.txt) y la PSRAM. En una placa los datos van, primero, en la RAM interna, del tamaño que quepa (hasta
 * 16 KB), y si hay PSRAM, otra vuelta con ellos allí, que es donde viven las cadenas de la VM. Se entra por
 * bpstr_banco_main.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* memmem, donde la libc lo tiene */
#endif
#include "bpstr.h"
#include "oraculo.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(BPSTR_PLACA)
uint64_t bpstr_banco_ns(void);           /* placa.c */
void bpstr_banco_linea(const char* s);   /* placa.c */
void* bpstr_placa_psram(size_t n);       /* placa.c: NULL si la placa no tiene */
int bpstr_banco_main(void);
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
static uint8_t *texto, *acentos, *otra, *salida;   /* MAXN + 64 los tres primeros; la salida, 3 MAXN + 64 */
static uint32_t maxn = MAXN;                       /* el tamaño más grande que se mide: el que cupo */
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
static void f_copia_d(uint32_t n)       { bpstr_copia(salida + 1, texto, n); sumidero = salida[n]; }   /* desalineada */
static void l_copia_d(uint32_t n)       { memcpy(salida + 1, texto, n); sumidero = salida[n]; }
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

/* Un número con tres decimales, con enteros: el printf de una placa puede no tener %f (las ESP llevan picolibc). */
static void con3(char* b, size_t cap, double v) {
    unsigned long m = (unsigned long) (v * 1000.0 + 0.5);
    snprintf(b, cap, "%3lu.%03lu", m / 1000ul, m % 1000ul);
}

static void fila(const char* que, fn_t f, fn_t libc, fn_t hoy) {
    static const uint32_t tam[4] = {16u, 256u, 2048u, 16384u};
    for (int i = 0; i < 4; i++) {
        uint32_t n = tam[i];
        if (n > maxn) break;
        double b = mide(f, n), l = libc ? mide(libc, n) : 0.0, h = hoy ? mide(hoy, n) : 0.0;
        char cb[16], cl[40] = "", ch[40] = "", x[16];
        con3(cb, sizeof cb, b);
        if (libc) {
            double p = 100.0 * (b - l) / l;
            con3(x, sizeof x, l);
            snprintf(cl, sizeof cl, "%7s  %+5ld %%", x, (long) (p < 0 ? p - 0.5 : p + 0.5));
        }
        if (hoy) {
            unsigned long r = (unsigned long) (h / b * 10.0 + 0.5);
            con3(x, sizeof x, h);
            snprintf(ch, sizeof ch, "%7s  x%3lu.%lu", x, r / 10ul, r % 10ul);
        }
        di("%-26s %6u  %7s   %-16s   %-16s", que, (unsigned) n, cb, cl, ch);
    }
}

/* Los datos, en la memoria que se le da: el texto (palabras con espacios y comas) y, con acentos, una de cada diez
 * letras de dos bytes. */
static void prepara(void) {
    uint32_t i, n, semilla = 0x2545F491u;
    for (i = 0; i < maxn + 64; i++) {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        texto[i] = (uint8_t) ((semilla % 7u == 0) ? ' ' : (semilla % 23u == 0) ? ',' : 'a' + (semilla >> 8) % 26u);
    }
    for (i = 0, n = 0; n < maxn + 60; i++) {
        semilla ^= semilla << 13; semilla ^= semilla >> 17; semilla ^= semilla << 5;
        if (semilla % 10u == 0) n += bpstr_codifica(0xE1u + (semilla >> 8) % 26u, acentos + n);   /* á, é, ñ… */
        else acentos[n++] = texto[i % maxn];
    }
    memcpy(otra, texto, maxn + 64);
    memcpy(aguja_larga, texto + maxn / 3u, 31);
    aguja_larga[31] = '@';                             /* casi un trozo del texto: no está */
}

static void tabla(void);
#ifdef BPSTR_CAMINOS
static void caminos(void);
#endif

int bpstr_banco_main(void) {
    uint8_t* bloque = NULL;
    /* la RAM interna: lo que quepa, de 16 KB para abajo (en el PC, siempre los 16) */
    for (maxn = MAXN; maxn >= 2048u; maxn /= 2u) {
        bloque = (uint8_t*) malloc(6u * maxn + 4u * 64u);
        if (bloque) break;
    }
    if (!bloque) { di("bpstr, el banco: no hay ni 12 KB de RAM"); return 1; }
    texto = bloque; acentos = texto + maxn + 64; otra = acentos + maxn + 64; salida = otra + maxn + 64;
    prepara();
    di("bpstr, el banco, con los datos en la RAM interna (hasta %u bytes)", (unsigned) maxn);
    tabla();
#ifdef BPSTR_CAMINOS
    caminos();
#endif
    free(bloque);
#if defined(BPSTR_PLACA)
    /* y en la PSRAM, si la hay */
    maxn = MAXN;
    bloque = (uint8_t*) bpstr_placa_psram(6u * maxn + 4u * 64u);
    if (bloque) {
        texto = bloque; acentos = texto + maxn + 64; otra = acentos + maxn + 64; salida = otra + maxn + 64;
        prepara();
        di("bpstr, el banco, con los datos en la PSRAM (hasta %u bytes)", (unsigned) maxn);
        tabla();
        free(bloque);
    }
#endif
    di("fin del banco");
    return 0;
}

static void tabla(void) {
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
    fila("copia (desalineada)", f_copia_d, l_copia_d, NULL);
    fila("rellena", f_rellena, l_rellena, NULL);
}

#ifdef BPSTR_CAMINOS
/* ── buscar, camino a camino: para elegir los topes por la medida ──
 * Con agujas de 2 a 300 bytes que no están (acaban en @) y de tres clases, por su primer byte, que es lo que hace ir
 * deprisa o despacio a la corta: el espacio (uno de cada siete bytes del texto), una letra (uno de cada 33) y uno que no
 * está (@). Cada camino por separado (bpstr.c, BPSTR_CAMINOS), la mezcla (bpstr_busca, con sus topes) y la libc. */
int32_t bpstr_camino_corta(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn);
int32_t bpstr_camino_parejas(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn);
int32_t bpstr_camino_dos(const uint8_t* p, uint32_t n, const uint8_t* q, uint32_t qn);
static uint8_t camino_q[600];
static uint32_t camino_qn;
static void k_corta(uint32_t n)   { sumidero = (uint32_t) bpstr_camino_corta(texto, n, camino_q, camino_qn); }
static void k_parejas(uint32_t n) { sumidero = (uint32_t) bpstr_camino_parejas(texto, n, camino_q, camino_qn); }
static void k_dos(uint32_t n)     { sumidero = (uint32_t) bpstr_camino_dos(texto, n, camino_q, camino_qn); }
static void k_mezcla(uint32_t n)  { sumidero = (uint32_t) bpstr_busca(texto, n, camino_q, camino_qn, 0u); }
#if CON_MEMMEM
static void k_libc(uint32_t n)    { sumidero = (uint32_t) (uintptr_t) memmem(texto, n, camino_q, camino_qn); }
#endif

static void caminos(void) {
    static const uint32_t largos[15] = {2u, 3u, 4u, 5u, 6u, 8u, 12u, 16u, 24u, 32u, 64u, 128u, 256u, 300u, 600u};
    static const uint8_t primeros[3] = {' ', 'e', '@'};
    uint32_t tams[2] = {2048u, maxn}, nt = (maxn > 2048u) ? 2u : 1u;
    di("bpstr, buscar camino a camino (ns por byte; la aguja no esta: empieza por el byte de la columna y acaba en @)");
    di("%-9s %6s %5s  %8s %8s %8s %8s %8s", "empieza", "bytes", "aguja", "corta", "parejas", "dos", "mezcla", "libc");
    for (uint32_t t = 0; t < nt; t++)
        for (uint32_t k = 0; k < 3u; k++)
            for (uint32_t l = 0; l < 15u; l++) {
                const uint32_t n = tams[t], qn = largos[l];
                char c[5][16];
                camino_q[0] = primeros[k];
                memcpy(camino_q + 1, texto + maxn / 5u, qn - 2u);
                camino_q[qn - 1u] = '@';
                camino_qn = qn;
                con3(c[0], sizeof c[0], mide(k_corta, n));
                if (qn >= 3u && qn <= 256u) con3(c[1], sizeof c[1], mide(k_parejas, n)); else snprintf(c[1], 16, "-");
                con3(c[2], sizeof c[2], mide(k_dos, n));
                con3(c[3], sizeof c[3], mide(k_mezcla, n));
#if CON_MEMMEM
                con3(c[4], sizeof c[4], mide(k_libc, n));
#else
                snprintf(c[4], 16, "-");
#endif
                di("%-9s %6u %5u  %8s %8s %8s %8s %8s", k == 0u ? "espacio" : k == 1u ? "letra" : "ninguno",
                   (unsigned) n, (unsigned) qn, c[0], c[1], c[2], c[3], c[4]);
            }
}
#endif

#ifndef BPSTR_PLACA
int main(void) { return bpstr_banco_main(); }
#endif
