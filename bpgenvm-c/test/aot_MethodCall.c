/*
 * test/aot_MethodCall.c — thunk AOT a mano para #174 (mitad-VM). Hace el papel
 * del código native de `useBox`, que invoca el MÉTODO público getDoubled() de
 * un objeto Box vía vm->aot_helpers->call_method_i32 (despacho virtual por
 * vtable). Desde #174b AotCEmitter genera un thunk EQUIVALENTE al compilar
 * `native function useBox` (verás `call_method_i32(vm, b, 2, ...)` con
 * AotMain). Este se mantiene a mano como FIXTURE del test C, con el slot
 * hard-coded (getDoubled = slot 2, verificado con el disassembler), para que
 * `make test-method` no dependa de re-ejecutar el frontend Java.
 *
 * Lo NUEVO que prueba (runtime): código native despachando un método virtual
 * según la clase real del receptor (obj→class→vtable[slot]→cs+offset) + el
 * puente con `this` como arg0.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"

/* Box: toString=0, compareTo=1 (heredados de Object), getDoubled=2. */
#define SLOT_GETDOUBLED 2

static int32_t aot_MethodCall_useBox(struct bpvm* vm, int32_t b) {
    const struct aot_helpers_v1* H = vm->aot_helpers;
    /* useBox(b) = b.getDoubled() + 1. getDoubled no toma args (solo this). */
    return H->call_method_i32(vm, (uint32_t) b, SLOT_GETDOUBLED,
                              (const int32_t*) 0, 0) + 1;
}

static void thunk_MethodCall_useBox(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;   /* el Box */
    int32_t r = aot_MethodCall_useBox(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

void aot_MethodCall_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "MethodCall.useBox", thunk_MethodCall_useBox);
}
