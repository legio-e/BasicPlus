/*
 * test_freelist.c — #355/#357, y V8 #718: los HUECOS, en listas por tamaño.
 *
 * POR QUÉ EXISTE ESTE FICHERO
 * ---------------------------
 * (#355, 2-ago) La lista de huecos pasó a ir por dirección y a fundir los
 * huecos seguidos al soltar un owner. El sample real no ejercitaba la fusión
 * (0 fusiones en 14 altas): este test forzaba la adyacencia que el sample no
 * producía. Código sin ejercitar dentro del alocador es el peor sitio posible
 * para tenerlo.
 *
 * V8 #718 (30-oct): 48 listas por tamaño en vez de una, cada una por dirección,
 * y la reserva elige el hueco de dirección más baja entre las listas que valen
 * —el mismo que elegía la lista única—. Soltar un owner ya NO funde: lo funde el
 * barrido siguiente. Lo que se fuerza ahora:
 *   1. soltar NO funde: dos owners vecinos quedan como dos huecos;
 *   2. la reserva coge el hueco MÁS BAJO que vale, aunque haya uno de su tamaño
 *      justo más arriba (no es «best fit»: ver heap.c, «LOS HUECOS»);
 *   3. lo que sobra va a la lista de su tamaño;
 *   4. la ASTILLA: un hueco que dejaría 4 u 8 B se salta;
 *   5. si no vale ninguno, al final del heap (el bump);
 *   6. el ORDEN: cada lista de menor a mayor dirección, se suelte como se suelte;
 *   7. el barrido rehace las listas y funde lo seguido.
 * Y siempre, las reglas de las listas: cada una por dirección, cada hueco en la
 * de su tamaño, y el mapa igual a las listas que tienen algo.
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;

static void ok(const char* que, int cond) {
    if (cond) { printf("ok   %s\n", que); }
    else      { printf("FALLO %s\n", que); fallos++; }
}

/* La lista de un hueco de `t` B, como huecos_lista (heap.c): las exactas, (t-12)/4; desde 128 B, su potencia de dos. */
static uint32_t lista_de(uint32_t t) {
    if (t < 128u) return (t - BPVM_MIN_FREE_BLOCK) / 4u;
    uint32_t c = BPVM_HUECOS_EXACTAS + (31u - (uint32_t) __builtin_clz(t)) - 7u;
    return c < BPVM_HUECOS_LISTAS ? c : BPVM_HUECOS_LISTAS - 1u;
}

/* Cuántos huecos hay en todas las listas; y si se cumplen las reglas. */
static uint32_t huecos(const bpvm_t* vm, int* reglas) {
    uint32_t n = 0;
    *reglas = 1;
    for (uint32_t c = 0; c < BPVM_HUECOS_LISTAS; c++) {
        if ((int) ((vm->huecos_mapa[c >> 5] >> (c & 31u)) & 1u) != (vm->huecos_cab[c] != 0u)) *reglas = 0;
        uint32_t prev = 0;
        for (uint32_t a = vm->huecos_cab[c]; a != 0u && n < 10000u; a = bpvm_read_u32_be(vm->memory + a + 8)) {
            uint32_t s = bpvm_read_u32_be(vm->memory + a + 4);
            if (a <= prev || lista_de(s) != c) *reglas = 0;   /* ESTRICTAMENTE creciente, y en la suya */
            prev = a;
            n++;
        }
    }
    return n;
}

/* ¿Está el hueco [a, a+s) en su lista? */
static int es_hueco(const bpvm_t* vm, uint32_t a, uint32_t s) {
    for (uint32_t c = vm->huecos_cab[lista_de(s)]; c != 0u; c = bpvm_read_u32_be(vm->memory + c + 8))
        if (c == a) return bpvm_read_u32_be(vm->memory + c + 4) == s;
    return 0;
}

/* Reserva un bloque de `payload` bytes y devuelve su CABECERA (user_ref - 4).
 * Se pide con el GC apagado para que nadie mueva nada a mitad del experimento:
 * lo que se prueba aquí son las listas, no el recolector. Un array de bytes de
 * `payload` ocupa 8 + payload, redondeado a 4. Su handle se apunta, para
 * soltarlo como un owner (suelta). */
static uint32_t g_cab[512];
static bpref_t  g_handle[512];
static uint32_t g_n = 0;
static uint32_t reserva(bpvm_t* vm, uint32_t payload) {
    bpref_t h = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, payload, payload);   /* V7/H7 #593: el método único */
    if (h.v == 0u) { printf("FALLO: no hay heap para reservar %u B\n", (unsigned) payload); exit(1); }
    if (g_n >= 512u) { printf("FALLO: demasiadas reservas para la prueba\n"); exit(1); }
    g_cab[g_n] = bpref_deref(vm, h) - 4u;
    g_handle[g_n] = h;
    return g_cab[g_n++];
}

/* Suelta el bloque de esa cabecera como un owner: su handle muere y el bloque vuelve a las listas. (Soltarlo con
 * bpvm_heap_free_block a secas dejaría vivo el handle, y el GC lo vería apuntar a un hueco: «HEAP DESCARRILADO»,
 * #593.) */
static void suelta(bpvm_t* vm, uint32_t cab) {
    for (uint32_t i = 0; i < g_n; i++)
        if (g_cab[i] == cab && !bpvm_ref_dead(vm, g_handle[i])) { bpvm_heap_soltar_solo(vm, g_handle[i]); return; }
    printf("FALLO: no encuentro el handle de %u\n", (unsigned) cab);
    exit(1);
}

static int sin_huecos(const bpvm_t* vm) { return (vm->huecos_mapa[0] | vm->huecos_mapa[1]) == 0u; }

/* Gasta todos los huecos (cada uno con una reserva de su tamaño justo), para que lo siguiente salga del bump. */
static void vacia_huecos(bpvm_t* vm) {
    while (!sin_huecos(vm)) {
        uint32_t c = vm->huecos_mapa[0] != 0u ? (uint32_t) __builtin_ctz(vm->huecos_mapa[0])
                                              : 32u + (uint32_t) __builtin_ctz(vm->huecos_mapa[1]);
        (void) reserva(vm, bpvm_read_u32_be(vm->memory + vm->huecos_cab[c] + 4) - 8u);
    }
}

int main(void) {
    static uint8_t mem[64 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    if (!vm) { printf("FALLO: bpvm_init\n"); return 1; }
    /* Recién inicializada la VM el heap está VACÍO (heap_start == stack_base):
     * quien lo establece es la reserva de arena que hace el loader al cargar un
     * módulo. Aquí no cargamos ninguno, así que se pide una arena mínima para
     * que quede heap por detrás — mismo camino que usa el loader de verdad. */
    if (!bpvm_arena_reserve(vm, 256, 4)) { printf("FALLO: arena\n"); return 1; }
    bpvm_set_gc_enabled(vm, 0);   /* el GC no debe entrar a mitad del experimento */
    int r = 0;

    /* ── 1. SOLTAR NO FUNDE ──────────────────────────────────────────────────
     * A B C seguidos, de 48 B. Suelto B y luego A: quedan DOS huecos de 48, en
     * la lista de 48 y en orden. Fundirlos es cosa del barrido siguiente. */
    uint32_t A = reserva(vm, 40), B = reserva(vm, 40), C = reserva(vm, 40);
    ok("A B C salen contiguos del bump", B == A + 48u && C == B + 48u);
    suelta(vm,B);
    suelta(vm,A);
    ok("soltar no funde: A y B, dos huecos", huecos(vm, &r) == 2 && es_hueco(vm, A, 48) && es_hueco(vm, B, 48));
    ok("  las reglas de las listas", r);
    ok("  y por dirección: A es la cabeza de la de 48", vm->huecos_cab[lista_de(48)] == A);

    /* ── 2. EL MÁS BAJO QUE VALE, no el de su tamaño justo ───────────────────
     * X (64 B), vivo, Y (48 B), vivo. Suelto los dos y pido 48: valen X (sobran
     * 16) e Y (justo). La lista única cogía X, el primero por dirección; un «best
     * fit» cogería Y. Tiene que salir X. */
    vacia_huecos(vm);
    ok("huecos gastados", huecos(vm, &r) == 0 && sin_huecos(vm));
    uint32_t X = reserva(vm, 56); (void) reserva(vm, 4);
    uint32_t Y = reserva(vm, 40); (void) reserva(vm, 4);
    suelta(vm,Y);
    suelta(vm,X);
    uint32_t P = reserva(vm, 40);
    ok("la reserva de 48 coge X, el más bajo (no Y, el justo)", P == X);
    /* ── 3. LO QUE SOBRA, a la lista de su tamaño ── */
    ok("lo que sobra de X (16 B) va a la de 16", es_hueco(vm, X + 48u, 16));
    ok("Y sigue en la de 48", es_hueco(vm, Y, 48));
    (void) huecos(vm, &r);
    ok("  las reglas de las listas", r);

    /* ── 4. LA ASTILLA ───────────────────────────────────────────────────────
     * Un hueco de 56 B (Z) y la reserva de 48: sobrarían 8, que no pueden ser un
     * hueco (le faltan el tamaño y el siguiente) ni quedarse en el bloque, que no
     * guarda su tamaño (el bug del 15-jul). Se salta: sale del bump. */
    vacia_huecos(vm);
    uint32_t Z = reserva(vm, 48); (void) reserva(vm, 4);
    suelta(vm,Z);
    uint32_t bump = vm->heap_next;
    uint32_t Q = reserva(vm, 40);
    ok("la astilla: el hueco de 56 no vale para 48 (sale del bump)", Q == bump && es_hueco(vm, Z, 56));

    /* ── 4b. LA ASTILLA EN UNA LISTA POR POTENCIA DE DOS (el camino largo) ──
     * Un hueco de 128 B y, más arriba, uno de 136, los dos en la lista de 128 a 255. La reserva de 124: el de 128
     * dejaría 4 (se salta); el de 136, 12. */
    vacia_huecos(vm);
    uint32_t H1 = reserva(vm, 120); (void) reserva(vm, 4);
    uint32_t H2 = reserva(vm, 128); (void) reserva(vm, 4);
    suelta(vm, H1);
    suelta(vm, H2);
    uint32_t R = reserva(vm, 116);
    ok("124 B en la lista de 128: salta el de 128 (astilla) y coge el de 136", R == H2 && es_hueco(vm, H1, 128));
    ok("  y lo que sobra del de 136 (12 B), a la de 12", es_hueco(vm, H2 + 124u, 12));

    /* ── 5. NINGUNO VALE: al final del heap ── */
    bump = vm->heap_next;
    uint32_t G = reserva(vm, 200);
    ok("más grande que todos los huecos: sale del bump", G == bump);

    /* ── 6. ORDEN ────────────────────────────────────────────────────────────
     * Seis bloques del bump; suelto los pares EN DESORDEN (4, 0, 2). Los impares,
     * vivos, los separan: tres huecos sueltos, y la lista de 48 por dirección
     * aunque las altas llegaran desordenadas (la 2 entra entre la 0 y la 4, desde
     * el dedo, que quedó en la 0). */
    vacia_huecos(vm);
    uint32_t b[6];
    for (int i = 0; i < 6; i++) b[i] = reserva(vm, 40);
    suelta(vm,b[4]);
    suelta(vm,b[0]);
    suelta(vm,b[2]);
    uint32_t c48 = vm->huecos_cab[lista_de(48)];
    uint32_t c48b = c48 ? bpvm_read_u32_be(vm->memory + c48 + 8) : 0u;
    uint32_t c48c = c48b ? bpvm_read_u32_be(vm->memory + c48b + 8) : 0u;
    ok("3 huecos sueltos, ninguno fundido", huecos(vm, &r) == 3);
    ok("  las reglas de las listas", r);
    ok("la de 48, por dirección: 0, 2, 4", c48 == b[0] && c48b == b[2] && c48c == b[4]);

    /* ── 7. EL BARRIDO rehace las listas y funde ─────────────────────────────
     * Sin raíces, todo es basura: el barrido lo junta todo en un tramo que llega
     * al final, y el final vuelve al bump. Quedan las listas vacías y el heap,
     * entero, libre. */
    bpvm_set_gc_enabled(vm, 1);
    bpvm_heap_gc(vm);
    ok("tras el GC, ningún hueco (todo vuelve al bump)", huecos(vm, &r) == 0 && sin_huecos(vm));
    ok("  las reglas de las listas", r);
    ok("  y el bump, al principio del heap", vm->heap_next == vm->heap_start);

    printf(fallos ? "\n[status=FALLOS:%d]\n" : "\n[status=OK]\n", fallos);
    return fallos ? 1 : 0;
}
