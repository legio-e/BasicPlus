/*
 * aot_ThrowUser.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando ThrowUser.bp con --aot.
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
static int32_t aot_ThrowUser_checkRange(struct bpvm* vm, int32_t n);

static int32_t aot_ThrowUser_checkRange(struct bpvm* vm, int32_t n) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) n;
    if ((n < 0)) {
        vm->aot_helpers->throw_ref(vm, (uint32_t) vm->aot_helpers->call_bp_i32(vm, vm->aot_helpers->find_function(vm, "ThrowUser.__cls_new_RangoError"), (int32_t[]){ vm->aot_helpers->string_from_cstr(vm, "valor negativo", 14), n }, 2));
    }
    return (n * 2);
}

static void thunk_ThrowUser_checkRange(struct bpvm* vm,
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
    int32_t r = aot_ThrowUser_checkRange(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_ThrowUser_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "ThrowUser.checkRange", thunk_ThrowUser_checkRange);
}

