/*
 * test_raices.c — V8 #833 (17-nov): EN LA MEMORIA DE BP, SÓLO RETIENE UNA REFERENCIA VIVA.
 *
 * El GC recorre palabra a palabra las pilas de los hilos BP y los bloques de datos de los módulos (las globales y las
 * constantes). Una palabra cuenta como referencia si lleva la marca de handle, su número está en la tabla y los 4 bytes
 * de delante son el contador del slot (heap.c, gc_es_referencia; la regla de Eduardo: «antes de dar por bueno que es
 * una referencia ha de verificar que el contador coincide»). Antes, en los bloques de datos contaba cualquier palabra:
 * un número que valía justo la dirección de un objeto lo retenía —en la prueba de resistencia, `minLibre` en la C6 y la
 * constante TOPE_LOG en la Pico 2—, y una referencia caducada retenía al nuevo ocupante de su slot. Aquí, con un módulo
 * de mentira cuyo bloque de datos son unos bytes de la arena (por debajo del heap, como el de verdad):
 *   1. un número con la dirección de un objeto, en una global, no lo retiene;
 *   2. la referencia entera, con su contador, sí (el control);
 *   3. una referencia caducada no retiene al nuevo ocupante de su slot.
 * En 1 y 3, el sabotaje (--sabotaje-raices: el recorrido de antes) tiene que retener: si no, la prueba no vale.
 * Lo mismo en la pila de un hilo BP, de punta a punta, lo prueba raices_smoke (en el micro simulado); el espejo en la
 * VM-Java, RaicesTest.
 *
 *   make test-raices
 */

#include "bpvm_internal.h"
#include <stdio.h>
#include <string.h>

static int fallos = 0;

static void ok(const char* que, int bien) {
    printf("%s %s\n", bien ? "ok   " : "FALLO", que);
    if (!bien) fallos++;
}

static uint32_t datos;   /* el bloque de datos de mentira: su offset en vm->memory */

/* ¿Sigue vivo el objeto de este handle? Su slot, con su contador y con una dirección. (No con bpvm_ref_dead, que
 * avisaría de un use-after-free por el diagnóstico: aquí, que esté muerto es lo que se busca.) */
static int vivo(const bpvm_t* vm, bpref_t h) {
    uint32_t idx = (uint32_t) h.v & ~BPVM_HANDLE_TAG;
    return idx != 0u && idx < vm->handles.next && vm->handles.gen[idx] == (uint32_t) (h.v >> 32)
        && vm->handles.addr[idx] != 0u;
}

static uint32_t slot(bpref_t h) { return (uint32_t) h.v & ~BPVM_HANDLE_TAG; }

/* Un objeto nuevo que nada referencia: un array de 64 enteros. */
static bpref_t nuevo(bpvm_t* vm) {
    return bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I32, 64u, 64u * 4u);
}

static void limpia_datos(bpvm_t* vm) { memset(vm->memory + datos, 0, 64); }

int main(void) {
    static uint8_t mem[64 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    if (!vm) { printf("FALLO: bpvm_init\n"); return 1; }
    /* Sin módulo cargado el heap está vacío: una arena, como hace el loader (test_freelist); sus 64 primeros bytes, el
     * bloque de datos de un módulo de mentira. */
    uint8_t* arena = bpvm_arena_reserve(vm, 256, 4);
    if (!arena) { printf("FALLO: arena\n"); return 1; }
    datos = (uint32_t) (arena - vm->memory);
    memset(&vm->modules[0], 0, sizeof vm->modules[0]);
    vm->modules[0].data_start = datos;
    vm->modules[0].data_size  = 64u;
    vm->module_count = 1;
    limpia_datos(vm);
    bpvm_set_gc_enabled(vm, 1);

    /* ── 1. UN NÚMERO NO RETIENE ─────────────────────────────────────────── */
    bpref_t a = nuevo(vm);
    ok("el objeto nace", vivo(vm, a));
    bpvm_write_u32_be(vm->memory + datos + 8u, bpref_deref(vm, a));   /* una global entera que vale su dirección */
    bpvm_heap_gc(vm);
    ok("un número con la dirección de un objeto, en una global: no lo retiene", !vivo(vm, a));
    bpvm_sabotaje_raices = 1;
    bpref_t a2 = nuevo(vm);
    limpia_datos(vm);
    bpvm_write_u32_be(vm->memory + datos + 8u, bpref_deref(vm, a2));
    bpvm_heap_gc(vm);
    ok("  con el sabotaje (el recorrido de antes), sí lo retiene: la prueba vale", vivo(vm, a2));
    bpvm_sabotaje_raices = 0;
    limpia_datos(vm);
    bpvm_heap_gc(vm);
    ok("  y sin el número, se suelta", !vivo(vm, a2));

    /* ── 2. LA REFERENCIA ENTERA, SÍ ─────────────────────────────────────── */
    bpref_t b = nuevo(vm);
    bpref_store(vm, datos + 16u, b);   /* una global de referencia: el handle de 8 bytes, su contador delante */
    bpvm_heap_gc(vm);
    ok("la referencia entera, con su contador, en una global: lo retiene", vivo(vm, b));

    /* ── 3. UNA REFERENCIA CADUCADA NO RETIENE AL NUEVO OCUPANTE ─────────── */
    limpia_datos(vm);
    bpvm_heap_gc(vm);                   /* sin la global, b se suelta: su slot vuelve a la lista con el contador +1 */
    ok("sin la global, b se suelta", !vivo(vm, b));
    bpref_t c = nuevo(vm);              /* el siguiente objeto reutiliza el slot de b (la lista de slots es LIFO) */
    ok("  c ocupa el slot de b (si no, la prueba no vale)", slot(c) == slot(b));
    bpref_store(vm, datos + 16u, b);    /* la global se quedó con la referencia de antes: caducada */
    bpvm_heap_gc(vm);
    ok("una referencia caducada en una global no retiene al nuevo ocupante de su slot", !vivo(vm, c));
    bpvm_sabotaje_raices = 1;
    bpref_t c2 = nuevo(vm);
    ok("  c2 ocupa el mismo slot", slot(c2) == slot(b));
    bpvm_heap_gc(vm);                   /* la global sigue con la caducada */
    ok("  con el sabotaje (el recorrido de antes), sí lo retiene: la prueba vale", vivo(vm, c2));
    bpvm_sabotaje_raices = 0;

    printf(fallos ? "\n[status=FALLOS:%d]\n" : "\n[status=OK]\n", fallos);
    return fallos ? 1 : 0;
}
