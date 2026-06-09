/*
 * aot_ByteNat.c — AUTOGENERADO por AotCEmitter (H3 #157).
 * NO EDITAR A MANO. Regenerar compilando ByteNat.bp con --aot.
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
static int32_t aot_ByteNat_sumBytes(struct bpvm* vm, int32_t data, int32_t n);
static void aot_ByteNat_copyBytes(struct bpvm* vm, int32_t src, int32_t dst, int32_t n);

static int32_t aot_ByteNat_sumBytes(struct bpvm* vm, int32_t data, int32_t n) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) data;
    (void) n;
    int32_t total = 0;
    int32_t i = 0;
    while ((i < n)) {
        total = (total + vm->aot_helpers->array_load_u8(vm, data, i));
        i = (i + 1);
    }
    return total;
}

static void thunk_ByteNat_sumBytes(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a1 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_ByteNat_sumBytes(vm, a0, a1);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

static void aot_ByteNat_copyBytes(struct bpvm* vm, int32_t src, int32_t dst, int32_t n) {
    (void) vm;   /* puede no usarse si la función no toca
                  *  globals/arrays/builtins. */
    (void) src;
    (void) dst;
    (void) n;
    int32_t i = 0;
    while ((i < n)) {
        vm->aot_helpers->array_store_i8(vm, dst, i, vm->aot_helpers->array_load_u8(vm, src, i));
        i = (i + 1);
    }
}

static void thunk_ByteNat_copyBytes(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    /* H3 #158 — helpers accedidos indirect via vm.
     * No referencia símbolos del runtime por nombre → el
     * .o resultante con -fpic es 100% relocatable. */
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a2 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a1 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    aot_ByteNat_copyBytes(vm, a0, a1, a2);
    H->write_i32_be(mem + sp, 0); sp += 4;  /* dummy ret para OP_POP del caller */
    *sp_p = sp;
}

/* Registra todas las funciones AOT de este módulo en el AOT
 * registry. Llamar tras link, antes de bpvm_run. Tolerante a
 * símbolos ausentes (skip silente si el .mod no está cargado). */
void aot_ByteNat_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "ByteNat.sumBytes", thunk_ByteNat_sumBytes);
    bpvm_aot_register_by_name(vm, "ByteNat.copyBytes", thunk_ByteNat_copyBytes);
}

