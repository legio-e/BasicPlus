/*
 * test_reserva.c — #355, V7/H12 2b (5-oct): LA RESERVA DE EMERGENCIA la gasta SÓLO una reserva para construir un error.
 *
 * POR QUÉ EXISTE ESTE FICHERO
 * ---------------------------
 * La reserva de emergencia (#355, idea de Eduardo) es un trozo del final del heap que nadie usa, para que con el heap
 * lleno todavía se pueda construir la excepción que lo dice. Hasta hoy, quién podía gastarla lo decía una BANDERA DE LA
 * VM (`building_error`), puesta mientras bpvm_throw_runtime_error construía el error. Con dos runs (H12, 2b) esa
 * bandera la veía también el otro run: una reserva cualquiera suya, con el heap lleno, se llevaba la reserva del error
 * —y el error salía con la prefabricada en vez de con su mensaje—, y al acabar el primero la bajaba con el otro aún
 * construyendo el suyo. Ahora lo dice cada reserva (bpvm_heap_nuevo_error, bpvm_heap_alloc_string_error): no hay nada
 * compartido que el otro run pueda ver.
 *
 * Y no había NINGUNA prueba que gastara la reserva: test-oom atrapa el OOM, pero su mensaje cabe sin ella.
 *
 * Cubre, con el heap lleno de verdad (el GC apagado, para que nadie recoja nada a mitad):
 *   1. las reservas normales llenan el heap y NO tocan la reserva de emergencia;
 *   2. una normal más, pequeña: nula, y la reserva sigue entera;
 *   3. la de un ERROR (el objeto): cabe, gastando la reserva;
 *   4. la de su MENSAJE (la cadena): cabe en lo que quedó de la reserva.
 * El sabotaje (`--sabotaje`): el objeto del error por el camino normal. Tiene que dar rojo: si no, la prueba no mira.
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

/* Reservas normales de `tam` B hasta que no quepa ninguna; devuelve cuántas cupieron. */
static uint32_t llena(bpvm_t* vm, uint32_t tam) {
    uint32_t n = 0;
    while (bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, tam, tam).v != 0u) n++;
    return n;
}

int main(int argc, char** argv) {
    int sabotaje = (argc > 1 && strcmp(argv[1], "--sabotaje") == 0);
    static uint8_t mem[64 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    if (!vm) { printf("FALLO: bpvm_init\n"); return 1; }
    /* Sin módulo cargado el heap está vacío: una arena mínima, como en test_freelist (el camino del cargador). */
    if (!bpvm_arena_reserve(vm, 256, 4)) { printf("FALLO: arena\n"); return 1; }
    bpvm_set_gc_enabled(vm, 0);
    vm->heap_reserve = 1024u;   /* armada como la arma el cargador (loader.c) */

    /* 1. Llenar: de grande a pequeño, para no gastar la tabla de handles antes que el heap. */
    uint32_t n = llena(vm, 1024) + llena(vm, 256) + llena(vm, 64) + llena(vm, 16) + llena(vm, 0);
    printf("     (el heap, lleno con %u reservas normales; libres hasta el techo: %ld B)\n", (unsigned) n,
           (long) vm->heap_top - (long) vm->heap_reserve - (long) vm->heap_next);
    ok("1. las reservas normales llenan el heap", n > 10);
    ok("1. y no tocan la reserva de emergencia", vm->heap_reserve == 1024u);

    /* 2. Una normal más: nula, y la reserva sigue entera. */
    ok("2. una reserva normal con el heap lleno: nula", bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, 8, 8).v == 0u);
    ok("2. y la reserva sigue entera", vm->heap_reserve == 1024u);

    /* 3. El objeto del error (el RuntimeError: cuatro campos de 8 B, como lo pide exceptions.c). */
    bpref_t e = sabotaje ? bpvm_heap_nuevo(vm, BPVM_TYPE_OBJECT, 0, 4 * 8)
                         : bpvm_heap_nuevo_error(vm, BPVM_TYPE_OBJECT, 0, 4 * 8);
    ok("3. la reserva del objeto de un error cabe", e.v != 0u);
    ok("3. gastando la reserva de emergencia", vm->heap_reserve == 0u);

    /* 4. Su mensaje: cabe en lo que quedó. */
    const char* msg = "No space in heap";
    uint32_t m = bpvm_heap_alloc_string_error(vm, msg, strlen(msg));
    ok("4. la del mensaje del error cabe", m != 0u);
    if (m != 0u) {
        uint32_t a = bpref_deref(vm, bpref_regen(vm, m));
        ok("4. y dice lo que tiene que decir", a != 0u && bpvm_read_u32_be(vm->memory + a) == strlen(msg)
                                              && memcmp(vm->memory + a + 4, msg, strlen(msg)) == 0);
    }

    printf("%s: la reserva de emergencia, sólo para construir un error (#355; H12 2b)%s\n",
           fallos ? "FAIL" : "OK", sabotaje ? " — SABOTEADO" : "");
    return fallos ? 1 : 0;
}
