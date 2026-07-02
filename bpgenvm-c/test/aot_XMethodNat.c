/*
 * aot_XMethodNat.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando XMethodNat.bp con --aot.
 *
 * Funciones BP marcadas con `function native ...` traducidas a C.
 * El bytecode .mod se sigue generando normalmente; el runtime
 * decide qué versión usar via aot_registry tras link.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158 — helpers indirect */

/* Forward decls de las funciones AOT de este módulo. */
static int32_t aot_XMethodNat_useBox(struct bpvm* vm, int32_t b);

static int32_t aot_XMethodNat_useBox(struct bpvm* vm, int32_t b) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) b;
    return (vm->aot_helpers->call_method_i32(vm, b, 2, (const int32_t*) 0, 0) + 1);
}

static void thunk_XMethodNat_useBox(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_XMethodNat_useBox(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_XMethodNat_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "XMethodNat.useBox", thunk_XMethodNat_useBox);
}

