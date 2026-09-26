/*
 * test_ctx.c — V7/H5 P2b.1.a: el cambio de contexto (bpvm_ctx) en el PC.
 *
 * Tres cosas, cada una con lo que la rompería:
 *
 *  1. IDA Y VUELTA con mucho estado vivo a los dos lados: 8 enteros y 6 double en cada contexto, que gcc -O2
 *     guarda en los registros no volátiles (rbx, rsi, rdi, r12-r15, xmm6-xmm15) o en su pila, a través de
 *     cada salto. 100 000 saltos, y el resultado tiene que ser IGUAL al mismo cálculo hecho sin cambiar de
 *     pila. Un registro que el cambio no conservara lo estropea.
 *  2. setjmp/longjmp DENTRO de la pila prestada: el longjmp de MinGW deshace marcos (RtlUnwindEx) y exige
 *     que caigan entre los límites de pila del TIB. Sin actualizarlos en el cambio, esto revienta — y es lo
 *     que hace el límite de fallos de #186 cuando una native lanza.
 *  3. Un marco de 40 KB en la pila prestada: la sonda de pila de gcc (___chkstk_ms) tiene que ir bien.
 */
#include "bpvm_ctx.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); \
                         else { printf("  FAIL: %s\n", (m)); g_fail = 1; } } while (0)

#if BPVM_CTX_DISPONIBLE

static bpvm_ctx_t g_principal, g_corrutina;
static volatile int g_parar = 0;

#define VUELTAS  100000
#define PILA     (256u * 1024u)

/* Las cuentas de cada lado: las mismas macros en la ejecución de verdad y en la de referencia, para que la
 * aritmética sea idéntica y en el mismo orden. Enteros sin signo (desbordar uno con signo es indefinido) y
 * double que contraen (no crecen sin límite). */
#define ESTADO(p) unsigned long long p##a = 3, p##b = 5, p##c = 7, p##d = 11, \
                                     p##e = 13, p##f = 17, p##g = 19, p##h = 23; \
                  double p##x = 1.5, p##y = 2.5, p##z = 3.5, p##w = 4.5, p##u = 5.5, p##t = 6.5
#define PASO(p, v, K) do { unsigned long long v_ = (unsigned long long) (v); \
        p##a = p##a * 31u + v_;  p##b ^= p##a + (K);  p##c += p##b * 3u;  p##d = p##d * 5u + p##c; \
        p##e ^= p##d;  p##f += p##e * 7u;  p##g = p##g * 9u + p##f;  p##h ^= p##g; \
        p##x = p##x * 0.5 + (double) (v_ & 255u);  p##y = p##y * 0.25 + p##x; \
        p##z = p##z * 0.5 + p##y * 0.5;  p##w = p##w * 0.75 + p##z * 0.1; \
        p##u = p##u * 0.5 + p##w;  p##t = p##t * 0.9 + p##u * 0.01; } while (0)
#define SALIDA(p) (p##a ^ p##b ^ p##c ^ p##d ^ p##e ^ p##f ^ p##g ^ p##h ^ \
                   (unsigned long long) (p##x + p##y + p##z + p##w + p##u + p##t))

/* ── 1. ida y vuelta ── */
static void corrutina(void* dato) {
    ESTADO(c);
    uintptr_t v = (uintptr_t) dato;
    for (;;) {
        PASO(c, v, 7u);
        v = (uintptr_t) bpvm_ctx_saltar(&g_corrutina, g_principal, (void*) (uintptr_t) SALIDA(c));
        if (g_parar) break;
    }
    bpvm_ctx_saltar(&g_corrutina, g_principal, (void*) (uintptr_t) (SALIDA(c) ^ 0x5A5Au));
}

static unsigned long long de_verdad(void* pila, unsigned long long* fin_c) {
    ESTADO(m);
    unsigned long long acum = 0;
    int i;
    g_parar = 0;
    g_corrutina = bpvm_ctx_crear(pila, PILA, corrutina);
    for (i = 0; i < VUELTAS; i++) {
        uintptr_t out = (uintptr_t) bpvm_ctx_saltar(&g_principal, g_corrutina, (void*) (uintptr_t) SALIDA(m));
        acum = acum * 1099511628211ull + out;
        PASO(m, out, 11u);
    }
    g_parar = 1;
    *fin_c = (uintptr_t) bpvm_ctx_saltar(&g_principal, g_corrutina, (void*) 1);
    return acum ^ SALIDA(m);
}

static unsigned long long de_referencia(unsigned long long* fin_c) {
    ESTADO(m);
    ESTADO(c);
    unsigned long long acum = 0;
    int i;
    for (i = 0; i < VUELTAS; i++) {
        uintptr_t v = (uintptr_t) SALIDA(m);
        uintptr_t out;
        PASO(c, v, 7u);
        out = (uintptr_t) SALIDA(c);
        acum = acum * 1099511628211ull + out;
        PASO(m, out, 11u);
    }
    *fin_c = SALIDA(c) ^ 0x5A5Au;
    return acum ^ SALIDA(m);
}

/* ── 2. setjmp/longjmp en la pila prestada, y los límites del TIB ── */
static jmp_buf g_jb;
static void* g_tib_base_dentro;
static void* g_tib_limite_dentro;

static void* tib_base(void)   { void* p; __asm__ volatile ("movq %%gs:0x08, %0" : "=r"(p)); return p; }
static void* tib_limite(void) { void* p; __asm__ volatile ("movq %%gs:0x10, %0" : "=r"(p)); return p; }

static void __attribute__((noinline, noreturn)) lanzar(int n) { longjmp(g_jb, n); }
static volatile int g_lanza = 1;   /* siempre 1: que gcc vea que bajar PODRÍA volver */
static int __attribute__((noinline)) bajar(int n) {
    volatile int x = n;
    if (n <= 0) {
        if (g_lanza) lanzar(42);
        return x;
    }
    return bajar(n - 1) + x;
}
static void corrutina_salto(void* dato) {
    int r, res;
    (void) dato;
    g_tib_base_dentro = tib_base();
    g_tib_limite_dentro = tib_limite();
    r = setjmp(g_jb);
    res = (r == 0) ? bajar(10) : r;
    bpvm_ctx_saltar(&g_corrutina, g_principal, (void*) (intptr_t) res);
}

/* ── 3. un marco grande ── */
static int __attribute__((noinline)) grande(void) {
    volatile char buf[40000];
    buf[0] = 1;
    buf[39999] = 2;
    return buf[0] + buf[39999];
}
static void corrutina_grande(void* dato) {
    (void) dato;
    bpvm_ctx_saltar(&g_corrutina, g_principal, (void*) (intptr_t) grande());
}

int main(void) {
    void* pila = malloc(PILA);
    unsigned long long fin_v, fin_r, r_v, r_r;
    void* base_fuera;
    void* limite_fuera;
    intptr_t r;
    char m[160];
    if (!pila) { printf("sin memoria\n"); return 2; }
    printf("--- bpvm_ctx: x86-64, ABI de Windows ---\n");

    r_v = de_verdad(pila, &fin_v);
    r_r = de_referencia(&fin_r);
    snprintf(m, sizeof m, "%d saltos con 8 enteros y 6 double vivos a cada lado: %016llx == %016llx",
             VUELTAS, r_v, r_r);
    CHECK(r_v == r_r, m);
    CHECK(fin_v == fin_r, "  y el estado final de la corrutina, igual al de referencia");

    base_fuera = tib_base();
    limite_fuera = tib_limite();
    g_corrutina = bpvm_ctx_crear(pila, PILA, corrutina_salto);
    r = (intptr_t) bpvm_ctx_saltar(&g_principal, g_corrutina, NULL);
    CHECK(r == 42, "setjmp/longjmp con 10 marcos de por medio, DENTRO de la pila prestada");
    CHECK((uintptr_t) g_tib_base_dentro == (((uintptr_t) pila + PILA) & ~(uintptr_t) 15u)
          && g_tib_limite_dentro == pila, "  dentro, el TIB dice los límites de la pila prestada");
    CHECK(tib_base() == base_fuera && tib_limite() == limite_fuera, "  y al volver, los de la pila de siempre");

    g_corrutina = bpvm_ctx_crear(pila, PILA, corrutina_grande);
    r = (intptr_t) bpvm_ctx_saltar(&g_principal, g_corrutina, NULL);
    CHECK(r == 3, "un marco de 40 KB en la pila prestada (la sonda de pila)");

    CHECK(bpvm_ctx_crear(pila, BPVM_CTX_PILA_MIN - 1, corrutina) == NULL, "una pila por debajo del mínimo: NULL");

    free(pila);
    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}

#else

int main(void) {
    printf("SALTADO: esta arquitectura no tiene cambio de contexto todavía (P2b.3)\n[status=OK]\n");
    return 0;
}

#endif
