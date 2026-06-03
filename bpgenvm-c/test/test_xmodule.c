/*
 * test_xmodule.c — driver host del puente native→BP CROSS-MODULE (#169).
 *
 * Carga BridgeApp.mod (el loader auto-carga su dependencia BridgeLib.mod vía
 * discover_deps), registra el thunk AOT de BridgeApp.compute (que delega en
 * BridgeLib.triple vía call_bp_i32 con nombre cualificado cross-module) y corre
 * main.
 *
 * Salida esperada (byte-idéntica a la VM-Java):
 *   [BridgeLib.triple] 5
 *   compute(5) = 16
 *   [BridgeLib.triple] 8
 *   compute(8) = 25
 * status final: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_BridgeApp_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "BridgeApp.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);   /* auto-carga BridgeLib */
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_BridgeApp_register(vm);   /* hijack: compute → thunk → puente → BridgeLib.triple */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
