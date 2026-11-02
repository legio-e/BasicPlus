/*
 * test_malnacida.c — V8 #739 (2-nov): UNA CADENA QUE NACE EMPEZANDO POR UN BYTE DE CONTINUACIÓN.
 *
 * Una cadena así (el primer byte, 10xxxxxx: UTF-8 roto) no puede nacer: lo de fuera se descodifica al nacer (#711),
 * una cadena no cambia (#714), el texto de los packs se descodifica (#724) y lo que da el sistema en el PC llega en
 * UTF-8 (#763). Por eso el cursor de las cadenas (builtins.c) ya no tiene un caso para ella, y donde nacen todas
 * (bpvm_str_nace_ascii) lo dice si alguna vez pasa: por el diagnóstico, con sus primeros bytes, las tres primeras
 * veces. Ningún programa la crea, así que aquí se fuerza a mano:
 *   1. el aviso sale al nacer una así, con sus bytes; con una buena, una ASCII o la vacía, no;
 *   2. sólo las tres primeras veces;
 *   3. el cursor, con ella, no se sale de la cadena: la longitud, charCodeAt, charAt, substring y el contar de
 *      indexOf, con todos los índices y en varios órdenes (el cursor sigue de una llamada a otra), dan algo dentro de
 *      la cadena —lo que den: una cadena rota no tiene un valor bueno— sin leer ni escribir fuera.
 *
 *   make test-malnacida
 */

#include "bpvm_internal.h"
#include "bpvm_prims.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;

static void ok(const char* que, int bien) {
    printf("%s %s\n", bien ? "ok   " : "FALLO", que);
    if (!bien) fallos++;
}

/* El diagnóstico, escuchado: cuántos avisos de cadena mal nacida, y el último. */
static int avisos = 0;
static char ultimo[256];

static void sumidero(const char* linea) {
    if (strstr(linea, "MAL NACIDA") == NULL) return;
    avisos++;
    snprintf(ultimo, sizeof ultimo, "%s", linea);
}

/* Una cadena del heap con estos bytes, nacida como nacen todas (bpvm_str_nace: mira si es ASCII). */
static bpref_t nace(bpvm_t* vm, const char* b, uint32_t n) {
    bpref_t h = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, n, n);
    uint32_t s = bpref_deref(vm, h);
    if (n > 0u) memcpy(vm->memory + s + 4u, b, n);
    bpvm_str_nace(vm, s);
    return h;
}

/* Los bytes de una cadena del heap que dio una primitiva. */
static uint32_t bytes_de(bpvm_t* vm, bpref_t h) {
    return bpvm_read_u32_be(vm->memory + bpref_deref(vm, h));
}

int main(void) {
    static uint8_t mem[64 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    if (!vm) { printf("FALLO: bpvm_init\n"); return 1; }
    /* Sin módulo cargado el heap está vacío: una arena mínima, como hace el loader (test_freelist). */
    if (!bpvm_arena_reserve(vm, 256, 4)) { printf("FALLO: arena\n"); return 1; }
    bpvm_set_gc_enabled(vm, 0);   /* las cadenas de la prueba no están en ninguna raíz */
    bpvm_diag_set_sink(sumidero);

    /* ── 1. EL AVISO, al nacer, con sus bytes ─────────────────────────────── */
    (void) nace(vm, "a\xC3\xB1o", 4);   /* «año»: buena */
    (void) nace(vm, "abc", 3);          /* ASCII */
    (void) nace(vm, "", 0);             /* la vacía */
    ok("una buena, una ASCII y la vacía: ningún aviso", avisos == 0);
    bpref_t rota = nace(vm, "\xA9" "ab", 3);   /* la segunda mitad de una «é», y «ab» */
    ok("la que empieza por A9: un aviso", avisos == 1);
    ok("  con sus bytes", strstr(ultimo, "A9 61 62") != NULL);

    /* ── 2. SÓLO LAS TRES PRIMERAS ────────────────────────────────────────── */
    for (int k = 0; k < 4; k++) (void) nace(vm, "\x80", 1);
    ok("cinco rotas: tres avisos", avisos == 3);

    /* ── 3. EL CURSOR NO SE SALE ──────────────────────────────────────────── */
    uint32_t s = bpref_deref(vm, rota), n = 3u;
    bpvm_strcur_t cur;
    char err[96];
    int dentro = 1;
    memset(&cur, 0, sizeof cur);
    for (int vuelta = 0; vuelta < 3; vuelta++) {   /* tres órdenes: el cursor llega a cada llamada de otra forma */
        for (int k = 0; k < 6; k++) {
            int32_t i = (vuelta == 1) ? 4 - k : k - 1;   /* -1..4, o 4..-1 */
            int32_t largo = bpvm_prim_str_len(vm, &cur, rota.v, s);
            if (largo < 0 || (uint32_t) largo > n) dentro = 0;
            int32_t c = bpvm_prim_char_code_at(vm, &cur, rota.v, s, i);
            if (c < -1 || c > 0x10FFFF) dentro = 0;
            bpref_t out;
            if (bpvm_prim_char_at(vm, &cur, rota.v, s, i, &out, err, sizeof err) == 0) {
                uint32_t b = bytes_de(vm, out);
                if (b < 1u || b > 4u) dentro = 0;
            }
            for (int32_t h = -1; h <= 4; h++) {
                int32_t d = (vuelta == 2) ? h : i;
                if (bpvm_prim_substring(vm, &cur, rota, d, (vuelta == 2) ? i : h, &out, err, sizeof err) != 0
                    || bytes_de(vm, out) > n) dentro = 0;
                s = bpref_deref(vm, rota);
            }
            for (uint32_t off = 0u; off <= n; off++) {
                int32_t ci = bpvm_prim_str_cuenta_hasta(vm, &cur, rota.v, s, off);
                if (ci < 0 || (uint32_t) ci > n) dentro = 0;
            }
        }
    }
    ok("longitud, charCodeAt, charAt, substring y el contar de indexOf: dentro de la cadena", dentro);

    printf(fallos ? "\n[status=FALLOS:%d]\n" : "\n[status=OK]\n", fallos);
    return fallos ? 1 : 0;
}
