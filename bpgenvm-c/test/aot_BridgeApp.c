/*
 * test/aot_BridgeApp.c — thunk AOT del cross-module #169 (FIXTURE del test C).
 *
 * Equivale EXACTAMENTE a lo que AotCEmitter emite al compilar
 * `native function compute` de BridgeApp.bp (verificado vía `--compile`, que
 * emitió la llamada-puente cross-module sin error + el aviso). Se escribe a
 * mano sólo porque la herramienta standalone AotMain aún no resuelve imports
 * para generar el .c (ver tarea de follow-up); el `--compile` del frontend SÍ
 * lo valida con imports resueltos.
 *
 * Lo NUEVO que prueba (runtime): código native llamando a una función BP de
 * OTRO módulo. find_function resuelve "BridgeLib.triple" en vm->symbols (el
 * loader auto-cargó BridgeLib.mod vía discover_deps), y el puente deriva el CS
 * del módulo destino (BridgeLib) a partir de la dirección absoluta.
 */

#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"

static int32_t aot_BridgeApp_compute(struct bpvm* vm, int32_t n) {
    const struct aot_helpers_v1* H = vm->aot_helpers;
    /* compute(n) = BridgeLib.triple(n) + 1  — triple es BP de otro módulo. */
    return (H->call_bp_i32(vm, H->find_function(vm, "BridgeLib.triple"),
                           (int32_t[]){ n }, 1) + 1);
}

static void thunk_BridgeApp_compute(struct bpvm* vm,
                              uint32_t* sp_p,
                              uint32_t* bp_p) {
    (void) bp_p;
    const struct aot_helpers_v1* H = vm->aot_helpers;
    uint8_t* mem = vm->memory;
    uint32_t sp = *sp_p;
    int32_t a0 = H->read_i32_be(mem + sp - 4); sp -= 4;
    int32_t r = aot_BridgeApp_compute(vm, a0);
    H->write_i32_be(mem + sp, r); sp += 4;
    *sp_p = sp;
}

void aot_BridgeApp_register(struct bpvm* vm) {
    bpvm_aot_register_by_name(vm, "BridgeApp.compute", thunk_BridgeApp_compute);
}
