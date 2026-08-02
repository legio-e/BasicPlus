/*
 * test_freelist.c — #355/#357: la LISTA DE LIBRES, ordenada y con fusión.
 *
 * POR QUÉ EXISTE ESTE FICHERO
 * ---------------------------
 * La lista pasó a estar ordenada por dirección y a fusionar huecos contiguos
 * (propuesta de Eduardo, 2-ago). Al instrumentarlo salió esto ejecutando el
 * sample real:
 *
 *     [gc] libres: 3 huecos, 7628 B | altas=14 fusion_izq=0 fusion_der=0
 *
 * CERO fusiones en 14 altas. Y no es un fallo: ese programa apenas fragmenta, y
 * los huecos que da de alta el barrido están separados por bloques VIVOS por
 * construcción (el barrido ya junta lo que encuentra seguido en su recorrido).
 * O sea que las dos ramas de fusión estaban escritas, compiladas... y sin
 * ejecutar ni una vez. Código sin ejercitar dentro del alocador es el peor sitio
 * posible para tenerlo.
 *
 * Este test FUERZA la adyacencia que el sample no produce: reserva bloques
 * seguidos y los libera en el orden que obliga a entrar por cada rama.
 *
 * Cubre:
 *   1. fusión por la DERECHA  (libero un hueco pegado por delante de otro)
 *   2. fusión por la IZQUIERDA (libero un hueco pegado por detrás de otro)
 *   3. las DOS a la vez       (un hueco que tapa el agujero entre otros dos)
 *   4. NO fusionar cuando hay un bloque vivo en medio
 *   5. el ORDEN de la lista: siempre de menor a mayor dirección
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

/* Recorre la lista y devuelve nº de huecos; rellena addrs/sizes (hasta cap). */
static uint32_t lista(const bpvm_t* vm, uint32_t* addrs, uint32_t* sizes, uint32_t cap) {
    uint32_t n = 0, c = vm->free_list_head;
    while (c != 0u && n < cap) {
        if (addrs) addrs[n] = c;
        if (sizes) sizes[n] = bpvm_read_u32_be(vm->memory + c + 4);
        n++;
        c = bpvm_read_u32_be(vm->memory + c + 8);
    }
    return n;
}

static int ordenada(const bpvm_t* vm) {
    uint32_t prev = 0, c = vm->free_list_head;
    while (c != 0u) {
        if (c <= prev) return 0;          /* debe ser ESTRICTAMENTE creciente */
        prev = c;
        c = bpvm_read_u32_be(vm->memory + c + 8);
    }
    return 1;
}

/* Reserva un bloque de `payload` bytes y devuelve su CABECERA (user_ref - 4).
 * Se pide con el GC apagado para que nadie mueva nada a mitad del experimento:
 * lo que se prueba aquí es la lista, no el recolector. */
static uint32_t reserva(bpvm_t* vm, uint32_t payload) {
    uint32_t user = bpvm_heap_alloc(vm, payload, BPVM_TYPE_ARRAY_I8);
    if (user == 0) { printf("FALLO: no hay heap para reservar %u B\n", (unsigned) payload); exit(1); }
    bpvm_write_u32_be(vm->memory + user, payload);   /* length, como hace el caller real */
    return user - 4u;
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

    uint32_t a[8], s[8];

    /* ── 1. FUSIÓN POR LA DERECHA ────────────────────────────────────────────
     * Reservo A B C seguidos. Libero B, luego A. Al liberar A, su vecino de la
     * DERECHA (B) ya está libre y es contiguo → deben quedar en UN solo hueco. */
    uint32_t A = reserva(vm, 40), B = reserva(vm, 40), C = reserva(vm, 40);
    ok("A B C salen contiguos del bump", B > A && C > B);

    bpvm_heap_free_block(vm, B);
    ok("tras liberar B: 1 hueco", lista(vm, a, s, 8) == 1);

    bpvm_heap_free_block(vm, A);
    uint32_t n = lista(vm, a, s, 8);
    ok("fusion DERECHA: A+B quedan en 1 solo hueco", n == 1);
    ok("fusion DERECHA: el hueco empieza en A",      n == 1 && a[0] == A);
    ok("fusion DERECHA: mide A+B enteros",           n == 1 && a[0] + s[0] == C);

    /* ── 2. FUSIÓN POR LA IZQUIERDA ──────────────────────────────────────────
     * Ahora libero C, que está pegado por detrás del hueco [A..C). Su vecino de
     * la IZQUIERDA es contiguo → debe crecer ese, sin nodo nuevo. */
    uint32_t antes = s[0];
    bpvm_heap_free_block(vm, C);
    n = lista(vm, a, s, 8);
    ok("fusion IZQUIERDA: sigue habiendo 1 solo hueco", n == 1);
    ok("fusion IZQUIERDA: el hueco creció",             n == 1 && s[0] > antes);
    ok("fusion IZQUIERDA: sigue empezando en A",        n == 1 && a[0] == A);

    /* ── 3. LAS DOS A LA VEZ ─────────────────────────────────────────────────
     * OJO: hay que VACIAR la lista antes. Ahora que se sirve desde abajo, las
     * reservas siguientes REUTILIZAN el hueco de arriba en vez de ir al bump —
     * que es justo lo que buscábamos, pero rompe cualquier test que dé por hecho
     * que lo nuevo sale detrás de lo viejo. (Esta suposición mía la cazó el
     * propio test, que para eso está.)
     *
     * Con la lista vacía, D E F sí salen del bump y sí son contiguos. Libero D y
     * F —que NO son contiguos entre sí, E los separa— y luego E, que tapa el
     * agujero: debe fusionar por AMBOS lados y dejarlo todo en uno. */
    while (vm->free_list_head != 0u) (void) reserva(vm, 8);
    ok("lista vaciada antes de la fase 3", lista(vm, a, s, 8) == 0);

    uint32_t D = reserva(vm, 40), E = reserva(vm, 40), F = reserva(vm, 40);
    ok("D E F contiguos", E > D && F > E);
    bpvm_heap_free_block(vm, D);
    bpvm_heap_free_block(vm, F);
    ok("con E VIVO en medio NO se fusiona: 2 huecos", lista(vm, a, s, 8) == 2);

    bpvm_heap_free_block(vm, E);
    n = lista(vm, a, s, 8);
    ok("fusion POR LOS DOS LADOS: D+E+F en 1 solo hueco", n == 1);
    ok("fusion POR LOS DOS LADOS: empieza en D",          n == 1 && a[0] == D);

    /* ── 4. ORDEN ────────────────────────────────────────────────────────────
     * Patrón alterno: 6 bloques del bump (lista vacía primero), libero los pares
     * EN DESORDEN. Los impares vivos los separan, así que no se fusiona ninguno
     * y quedan 3 huecos sueltos. Si la lista se ordena de verdad, el orden en
     * que llegaron las altas da igual. */
    while (vm->free_list_head != 0u) (void) reserva(vm, 8);
    uint32_t b[6];
    for (int i = 0; i < 6; i++) b[i] = reserva(vm, 40);
    bpvm_heap_free_block(vm, b[4]);
    bpvm_heap_free_block(vm, b[0]);
    bpvm_heap_free_block(vm, b[2]);
    n = lista(vm, a, s, 8);
    ok("3 huecos sueltos, ninguno fusionado", n == 3);
    ok("la lista queda ORDENADA por direccion", ordenada(vm));
    ok("y el primero es el de menor direccion", n == 3 && a[0] == b[0]);

    printf(fallos ? "\n[status=FALLOS:%d]\n" : "\n[status=OK]\n", fallos);
    return fallos ? 1 : 0;
}
