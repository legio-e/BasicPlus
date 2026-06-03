/*
 * test/aot_NativeBridge.c — thunk AOT ESCRITO A MANO (prueba de P-aot-call-bp).
 *
 * Vive en test/ (no en samples/out/, que está gitignored) porque NO es
 * autogenerado: hace el papel del código que AotCEmitter emitirá para
 * `native function compute` una vez cableado el follow-up (#211). Lo
 * interesante es `compute`: en vez de replicar la lógica, DELEGA en la función
 * BP interpretada `helper` a través del puente native→BP
 * (vm->aot_helpers->call_bp_i32). Es decir, código native (este .o compilado)
 * llamando a una función BP que corre en el intérprete.
 *
 * Patrón idéntico a samples/out/aot_FaultProp.c (#186): un thunk por función
 * que convierte la convención de stack BP en C-ABI; helpers accedidos siempre
 * vía vm->aot_helpers (indirect) para que el .o con -fpic sea relocatable.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"

/* compute(n) = helper(n) + 1, donde helper es BP interpretada. */
static int32_t aot_NativeBridge_compute(struct bpvm* vm, int32_t n) {
    const struct aot_helpers_v1* H = vm->aot_helpers;
    /* Resolver helper UNA vez y cachear (patrón §4 Opción A). */
    static uint32_t s_helper = 0;
    if (s_helper == 0) {
        s_helper = H->find_function(vm, "NativeBridge.helper");
    }
    int32_t args[1];
    args[0] = n;
    /* >>> PUENTE native→BP: ejecuta el cuerpo BP de helper en el intérprete
     *     anidado y nos devuelve su retorno. <<< */
    int32_t r = H->call_bp_i32(vm, s_helper, args, 1);
    return r + 1;
}

static void thunk_NativeBridge_compute(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_NativeBridge_compute(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

/* Registra el thunk en el AOT registry. Llamar tras load_mod, antes de run. */
void aot_NativeBridge_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "NativeBridge.compute", thunk_NativeBridge_compute);
}
