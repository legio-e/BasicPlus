/*
 * test_method.c — driver host de #174 (mitad-VM): native invoca un método
 * virtual de un objeto vía call_method_i32.
 *
 * Carga MethodCall.mod, registra el thunk de useBox (que llama a Box.getDoubled
 * por la vtable, slot 2) y corre Main.
 *
 * Salida esperada (byte-idéntica a la VM-Java):
 *   [Box.getDoubled] v=5
 *   11
 * status final: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_MethodCall_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "MethodCall.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_MethodCall_register(vm);   /* hijack: useBox → thunk → call_method → getDoubled */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
