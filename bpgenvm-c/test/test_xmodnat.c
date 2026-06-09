/*
 * test_xmodnat.c — driver host de #212 (AotMain resuelve imports → AOT cross-module).
 *
 * Carga XmodNat.mod (el loader auto-carga su dep Helper.mod vía discover_deps),
 * registra el thunk AOT de XmodNat.compute (que llama a Helper.dbl por el puente
 * call_bp_i32 con nombre cualificado — emitido AUTOMÁTICAMENTE por AotMain gracias
 * a #212) y corre main. Salida esperada (byte-idéntica a la VM-Java):
 *   compute(5) = 11
 * status final: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_XmodNat_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "XmodNat.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);   /* auto-carga Helper.mod */
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_XmodNat_register(vm);   /* hijack: compute → thunk → puente → Helper.dbl */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
