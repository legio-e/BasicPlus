/*
 * test_strpass_aot.c — driver host de #171 P-aot-mixed-types.
 *
 * Verifica que la función `native passThrough(s: string, n: integer): string`
 * funcione con AOT activo: lee dos slots del stack (string handle + int),
 * devuelve el handle tal cual. El BP-Main lo invoca 3 veces, esperando
 * "hello AOT" 3 veces en stdout (mismo output que el intérprete).
 *
 * Compilar:
 *   gcc test/test_strpass_aot.c [build/...o sin test main] aot_StrPass.o
 *       -o test_strpass_aot -lpthread
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_StrPass_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "StrPass.mod";
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

    aot_StrPass_register(vm);

    printf("=== AOT-active run ===\n");
    s = bpvm_run(vm);
    printf("=== status=%s ===\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
