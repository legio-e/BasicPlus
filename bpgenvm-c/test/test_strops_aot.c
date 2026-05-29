/*
 * test_strops_aot.c — driver host de #173 P-aot-strings.
 *
 * Carga StrOps.mod, registra los thunks AOT (aot_StrOps_register) y
 * corre Main. La salida debe ser IDÉNTICA al intérprete:
 *   Hello, BP!
 *   1
 *   0
 *   h
 *   104
 *   abc
 *   n=42
 *
 * Ejercita: literal de string, concat (+), igualdad (==), charAt,
 * charCodeAt, substring, y concat con coerción int→string.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_StrOps_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "StrOps.mod";
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

    aot_StrOps_register(vm);

    printf("=== AOT-active run ===\n");
    s = bpvm_run(vm);
    printf("=== status=%s ===\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
