/*
 * test_xmethodnat.c — driver host de #169 (método de clase EXTERNA desde native).
 *
 * Carga XMethodNat.mod (el loader auto-carga su dep XClassLib.mod vía
 * discover_deps), registra el thunk AOT de XMethodNat.useBox — que invoca
 * XClassLib.Box.getDoubled() por call_method_i32 con el SLOT resuelto del .bpi
 * (ClassSig/slotOf, la base de H13.1) — y corre main. Salida esperada
 * (byte-idéntica a la VM-Java):
 *   useBox = 11
 * status final: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_XMethodNat_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "XMethodNat.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);   /* auto-carga XClassLib.mod */
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_XMethodNat_register(vm);   /* hijack: useBox → thunk → call_method_i32 (slot externo) */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
