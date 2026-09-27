/*
 * test_ejecutor.c — V7/H5 P2b.3.a: el EJECUTOR de una native y el TESTIGO, las dos piezas de la plataforma
 * (bpvm_platform.h), probadas solas.
 *
 * Idea de Eduardo (27-sep): la native corre en un hilo de la plataforma con su propia pila, y ella y el intérprete
 * se pasan el testigo (dos semáforos binarios): sólo corre uno de los dos. Cuatro cosas, cada una con lo que la
 * rompería:
 *
 *  1. IDA Y VUELTA con estado vivo a los dos lados: 100 000 pasos del testigo, cada uno con un dato que viaja, y
 *     el resultado IGUAL al mismo cálculo hecho sin hilos. Un testigo que dejara correr a los dos a la vez, o que
 *     perdiera un aviso (el hilo esperaría para siempre), lo estropea o lo cuelga. Y dice LO QUE CUESTA un paso:
 *     la primera medida de P2b.3.
 *  2. TERMINAR un ejecutor PARADO a mitad, con diez marcos de por medio: se despierta con la orden, salta a su
 *     fondo (longjmp) y vuelve, sin ejecutar nada de lo que había detrás del punto donde esperaba. Es como
 *     acaba un programa con una native suspendida.
 *  3. Un marco de 40 KB en el ejecutor: el tamaño de pila pedido se respeta.
 *  4. La marca de agua de su pila (en el PC, -1: el SO no la lleva).
 */
#include "bpvm_platform.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); \
                         else { printf("  FAIL: %s\n", (m)); g_fail = 1; } } while (0)

#define VUELTAS  100000
#define PILA     (256u * 1024u)

/* Las cuentas de cada lado: las mismas macros en la ejecución de verdad y en la de referencia, para que la
 * aritmética sea idéntica y en el mismo orden (las de test_ctx, P2b.1.a). */
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

/* El testigo: el ejecutor espera en `sem_nativa`, el principal en `sem_worker`. Pasarlo es dar el del otro y
 * esperar el propio: exactamente lo que hará bpvm_nativa.c. */
static bpvm_platform_sem_handle_t sem_nativa, sem_worker;
static volatile unsigned long long g_buzon;   /* el dato que viaja con el testigo */
static volatile int g_parar;

static void al_ejecutor(void)  { bpvm_platform_sem_give(&sem_nativa); bpvm_platform_sem_take(&sem_worker); }
static void al_principal(void) { bpvm_platform_sem_give(&sem_worker); bpvm_platform_sem_take(&sem_nativa); }

/* ── 1. ida y vuelta ── */
static unsigned long long g_fin_c;
static void ejecutor_ida_y_vuelta(void* arg) {
    ESTADO(c);
    (void) arg;
    bpvm_platform_sem_take(&sem_nativa);          /* espera el primer testigo */
    for (;;) {
        unsigned long long v = g_buzon;
        if (g_parar) break;
        PASO(c, v, 7u);
        g_buzon = SALIDA(c);
        al_principal();
    }
    g_fin_c = SALIDA(c) ^ 0x5A5Au;
    bpvm_platform_sem_give(&sem_worker);          /* el último testigo, y se acaba */
}

static unsigned long long de_verdad(int64_t* us) {
    ESTADO(m);
    unsigned long long acum = 0;
    bpvm_platform_thread_handle_t h = NULL;
    int i;
    int64_t t0;
    g_parar = 0;
    if (bpvm_platform_thread_create_nativa(&h, ejecutor_ida_y_vuelta, NULL, PILA) != 0) {
        printf("  FAIL: no se pudo crear el ejecutor\n");
        g_fail = 1;
        return 0;
    }
    t0 = bpvm_platform_now_us();
    for (i = 0; i < VUELTAS; i++) {
        g_buzon = SALIDA(m);
        al_ejecutor();
        acum = acum * 1099511628211ull + g_buzon;
        PASO(m, g_buzon, 11u);
    }
    *us = bpvm_platform_now_us() - t0;
    g_parar = 1;
    al_ejecutor();
    bpvm_platform_thread_join(&h);
    return acum ^ SALIDA(m);
}

static unsigned long long de_referencia(unsigned long long* fin_c) {
    ESTADO(m);
    ESTADO(c);
    unsigned long long acum = 0;
    int i;
    for (i = 0; i < VUELTAS; i++) {
        unsigned long long v = SALIDA(m), out;
        PASO(c, v, 7u);
        out = SALIDA(c);
        acum = acum * 1099511628211ull + out;
        PASO(m, out, 11u);
    }
    *fin_c = SALIDA(c) ^ 0x5A5Au;
    return acum ^ SALIDA(m);
}

/* ── 2. terminar un ejecutor parado a mitad ── */
static jmp_buf g_fondo;
static volatile int g_terminar;
static volatile int g_paso_de_largo;   /* 1 si algo de detrás del punto de espera llegó a correr */
static volatile int g_hondo;
static volatile int g_volvio;

static int __attribute__((noinline)) bajar_y_esperar(int n) {
    volatile int x = n;
    if (n <= 0) {
        g_hondo = 1;
        al_principal();                      /* se para aquí, con diez marcos debajo */
        if (g_terminar) longjmp(g_fondo, 1); /* la orden: al fondo, abandonando los marcos */
        g_paso_de_largo = 1;
        return x;
    }
    int r = bajar_y_esperar(n - 1) + x;
    g_paso_de_largo = 1;                     /* no debe llegar: el longjmp se lo salta */
    return r;
}

static void ejecutor_parado(void* arg) {
    (void) arg;
    bpvm_platform_sem_take(&sem_nativa);
    if (setjmp(g_fondo) == 0) {
        bajar_y_esperar(10);
        g_paso_de_largo = 1;
    }
    g_volvio = 1;
    bpvm_platform_sem_give(&sem_worker);
}

/* ── 3. un marco grande ── */
static volatile int g_grande;
static int __attribute__((noinline)) grande(void) {
    volatile char buf[40000];
    buf[0] = 1;
    buf[39999] = 2;
    return buf[0] + buf[39999];
}
static int32_t g_libre;
static void ejecutor_grande(void* arg) {
    (void) arg;
    bpvm_platform_sem_take(&sem_nativa);
    g_grande = grande();
    g_libre = bpvm_platform_pila_libre_actual();
    bpvm_platform_sem_give(&sem_worker);
}

int main(void) {
    unsigned long long fin_r, r_v, r_r;
    int64_t us = 0;
    bpvm_platform_thread_handle_t h = NULL;
    char m[200];
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("--- el ejecutor de natives y el testigo (P2b.3.a) ---\n");
    if (bpvm_platform_sem_init(&sem_nativa) != 0 || bpvm_platform_sem_init(&sem_worker) != 0) {
        printf("  FAIL: sin semáforos\n\n[status=FAIL]\n");
        return 1;
    }

    r_v = de_verdad(&us);
    r_r = de_referencia(&fin_r);
    snprintf(m, sizeof m, "%d idas y vueltas con estado vivo a los dos lados: %016llx == %016llx",
             VUELTAS, r_v, r_r);
    CHECK(r_v == r_r, m);
    CHECK(g_fin_c == fin_r, "  y el estado final del ejecutor, igual al de referencia");
    printf("  medida: %.2f us por paso del testigo (%d idas y vueltas en %.1f ms)\n",
           (double) us / (2.0 * VUELTAS), VUELTAS, (double) us / 1000.0);

    g_terminar = 0; g_paso_de_largo = 0; g_hondo = 0; g_volvio = 0;
    CHECK(bpvm_platform_thread_create_nativa(&h, ejecutor_parado, NULL, PILA) == 0, "ejecutor para terminarlo");
    al_ejecutor();                            /* corre hasta pararse a diez marcos de hondo */
    CHECK(g_hondo == 1 && g_volvio == 0, "  se paró a mitad, con diez marcos de por medio");
    g_terminar = 1;
    al_ejecutor();                            /* la orden de terminar */
    bpvm_platform_thread_join(&h);
    CHECK(g_volvio == 1 && g_paso_de_largo == 0,
          "  al terminarlo saltó a su fondo sin ejecutar nada de detrás del punto de espera");

    CHECK(bpvm_platform_thread_create_nativa(&h, ejecutor_grande, NULL, PILA) == 0, "ejecutor con un marco grande");
    al_ejecutor();
    bpvm_platform_thread_join(&h);
    CHECK(g_grande == 3, "  un marco de 40 KB en su pila (el tamaño pedido se respeta)");
    printf("  marca de agua de su pila: %ld B sin tocar (-1: el SO no la lleva)\n", (long) g_libre);

    bpvm_platform_sem_destroy(&sem_nativa);
    bpvm_platform_sem_destroy(&sem_worker);
    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
