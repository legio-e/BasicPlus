/*
 * aot_Bench.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando Bench.bp con --aot.
 *
 * Funciones BP marcadas con `function native ...` traducidas a C.
 * El bytecode .mod se sigue generando normalmente; el runtime
 * decide qué versión usar via aot_registry tras link.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"

/* Forward decls de las funciones AOT de este módulo. */
static int32_t aot_Bench_fib(struct bpvm* vm, int32_t arg0);

static int32_t aot_Bench_fib(struct bpvm* vm, int32_t n) {
    (void) vm;   /* no usado en funciones leaf */
    if ((n < 2)) {
        return n;
    }
    return (aot_Bench_fib(vm, (n - 1)) + aot_Bench_fib(vm, (n - 2)));
}

static void thunk_Bench_fib(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = bpvm_read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_Bench_fib(vm, a0);
    bpvm_write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_Bench_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "Bench.fib", thunk_Bench_fib);
}

