/*
 * test_faultprop_aot.c — driver host de #186 (P-aot-native-robust).
 *
 * Carga ThrowProp.mod, registra el thunk AOT de readAt (que hace
 * array_load_i32 → throw_runtime en out-of-bounds) y corre Main.
 *
 * Comportamiento esperado:
 *   stdout:
 *     20                                       (acceso valido)
 *     caught: array_load_i32: index out of bounds   (fault cazado por try/catch BP)
 *     sigue vivo                               (la VM sobrevivio al catch)
 *   stderr:
 *     [bpvm-c] excepción no atrapada ...       (4º acceso sin try → thread muere limpio)
 *   status final: != BPVM_OK (RuntimeError no atrapado).
 *
 * Lo CRÍTICO: antes de #186, el fault en native imprimia a stderr y
 * SEGUIA con basura (return 0), nunca cazado. Ahora hace longjmp al
 * boundary del intérprete y propaga por el eh_stack.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_ThrowProp_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "ThrowProp.mod";
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

    aot_ThrowProp_register(vm);

    printf("=== AOT-active run ===\n");
    s = bpvm_run(vm);
    printf("=== status=%s ===\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return 0;   /* el status != OK es esperado (fault no atrapado al final) */
}
