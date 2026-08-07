/*
 * test_bios.c — V5/I: la tabla que se le presta al pack nativo.
 *
 * Lo que se verifica es el CHIVATO, no la tabla: que un hueco se detecte ANTES
 * de ejecutar nada y que diga QUÉ falta, no sólo que algo falta. Un NULL en una
 * ranura es un cuelgue esperando a ocurrir; el valor de esto es convertirlo en
 * "la BIOS tiene huecos: memcpy" en vez de un hard fault mudo.
 *
 *   make test-bios
 */
#include "bpvm_bios.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Implementaciones de mentira: sólo hacen falta las DIRECCIONES. */
static void  f_log(const char* m) { (void) m; }
static void* f_mem3(void* a, const void* b, size_t n) { (void)b;(void)n; return a; }
static void* f_memset(void* a, int c, size_t n) { (void)c;(void)n; return a; }
static int   f_cmp3(const void* a, const void* b, size_t n) { (void)a;(void)b;(void)n; return 0; }
static void* f_memchr(const void* a, int c, size_t n) { (void)c;(void)n; return (void*) a; }
static size_t f_strlen(const char* s) { (void) s; return 0; }
static int   f_strcmp(const char* a, const char* b) { (void)a;(void)b; return 0; }
static int   f_strncmp(const char* a, const char* b, size_t n) { (void)a;(void)b;(void)n; return 0; }
static char* f_strchr(const char* s, int c) { (void)c; return (char*) s; }
static size_t f_span(const char* s, const char* a) { (void)s;(void)a; return 0; }
static void* f_malloc(size_t n) { (void) n; return 0; }
static void  f_free(void* p) { (void) p; }
static void* f_realloc(void* p, size_t n) { (void)n; return p; }
static struct tm* f_localtime(const void* t) { (void) t; return 0; }

/* Devuelve una tabla COMPLETA y bien formada. Cada caso la estropea de una
 * manera — un solo cambio por caso, que es como se sabe qué causó qué. */
static bpvm_bios_t buena(void) {
    bpvm_bios_t b;
    memset(&b, 0, sizeof b);
    b.magic = BPVM_BIOS_MAGIC;  b.version = BPVM_BIOS_VERSION;
    b.log = f_log;
    b.memcpy = f_mem3; b.memmove = f_mem3; b.memset = f_memset;
    b.memcmp = f_cmp3; b.memchr = f_memchr;
    b.strlen = f_strlen; b.strcmp = f_strcmp; b.strncmp = f_strncmp;
    b.strchr = f_strchr; b.strrchr = f_strchr;
    b.strspn = f_span;   b.strcspn = f_span;
    b.malloc = f_malloc; b.free = f_free; b.realloc = f_realloc;
    b.localtime = f_localtime;
    return b;
}

int main(void) {
    printf("== tabla BIOS del pack nativo (V5/I) ==\n");

    /* ── 1. Completa ── */
    {
        bpvm_bios_t b = buena();
        CHECK(bpvm_bios_verify(&b) == NULL, "tabla completa -> NULL (sin quejas)");
    }

    /* ── 2. Un hueco se detecta Y SE NOMBRA ──
     * Lo que importa no es que falle: es que diga CUÁL. "algo falta" obliga a
     * mirar 17 campos a mano; "memcpy" te lleva al sitio. */
    {
        bpvm_bios_t b = buena(); b.memcpy = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "memcpy") == 0, "hueco en memcpy -> lo NOMBRA");
    }
    {
        bpvm_bios_t b = buena(); b.realloc = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "realloc") == 0, "hueco en realloc -> lo NOMBRA");
    }
    {
        bpvm_bios_t b = buena(); b.localtime = NULL;   /* la ULTIMA de la lista */
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "localtime") == 0, "hueco en la ULTIMA ranura -> tambien");
    }

    /* ── 3. `log` es la voz del pack: si falta, se dice ANTES que nada ──
     * Sin log el pack es mudo, así que ese hueco es cualitativamente peor: no
     * sólo rompe una función, deja ciego todo lo demás. */
    {
        bpvm_bios_t b = buena(); b.log = NULL; b.memcpy = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "log") == 0,
              "faltan log Y memcpy -> se reporta LOG (sin voz no hay diagnostico)");
    }

    /* ── 4. Magic y version ANTES que las ranuras ──
     * Si la tabla no es lo que creemos, leer punteros de ella es leer basura:
     * la comprobacion barata va primero. */
    {
        bpvm_bios_t b = buena(); b.magic = 0xDEADBEEF;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "magic") == 0, "magic malo -> 'magic', no se leen punteros");
    }
    {
        bpvm_bios_t b = buena(); b.version = BPVM_BIOS_VERSION + 1; b.log = NULL;
        const char* r = bpvm_bios_verify(&b);
        CHECK(r && strcmp(r, "version") == 0,
              "version distinta -> 'version' AUNQUE tambien falte log");
    }

    /* ── 5. Tabla NULL: no debe explotar ── */
    {
        const char* r = bpvm_bios_verify(NULL);
        CHECK(r && strcmp(r, "tabla") == 0, "tabla NULL -> 'tabla', sin reventar");
    }

    /* ── 6. Tabla a ceros (el caso del que se olvida de rellenarla) ── */
    {
        bpvm_bios_t b; memset(&b, 0, sizeof b);
        CHECK(bpvm_bios_verify(&b) != NULL, "tabla a CEROS -> se queja (no pasa por buena)");
    }

    /* ── 7. El contador de ranuras cuadra con la struct ──
     * Si alguien añade un campo y olvida ponerlo en RANURAS, el hueco vuelve a
     * ser mudo. Esto no lo detecta del todo, pero deja el numero a la vista. */
    CHECK(bpvm_bios_slot_count() == 17,
          "17 ranuras (las MEDIDAS en la prueba A: 16 de libc + log)");

    printf("\n[status=%s]\n", g_fail == 0 ? "OK" : "FAIL");
    return g_fail != 0;
}
