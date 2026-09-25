/*
 * test_aotfloat.c — [V7/H5 #559] la coma flotante DENTRO de una native, traducida a C por el
 * AOT y EJECUTADA en el PC.
 *
 * Hermano de test_aotswitch.c. El mismo `.c` que el AOT genera para el `.mdn` se compila con
 * el gcc del host DOS veces: tal cual —la FPU del PC hace los `float`, como el P4 y los
 * Cortex-M33— y con `-DBPVM_AOT_FLOAT_SOFT=1`, que lleva cada operación y cada conversión de
 * `float` a su helper de la tabla, como en las placas SIN FPU (la C5, la C3, la C6). Ese
 * segundo camino no se podía ejecutar en ningún sitio más que en esas placas; aquí sí.
 *
 * Lo que se mira (samples/FloatNat.bp): cada caso es una native y su gemela interpretada, y
 * el programa imprime `ok`/`FALLO`. La receta (`make test-aotfloat`) corre el MISMO .mod con
 * los dos binarios y, con `--interp`, sin el AOT; exige las tres salidas ENTERAS iguales —así
 * también cuenta el `print` de un float hecho desde la native— y ningún FALLO.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_FloatNat_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "FloatNat.mod";
    int interp = (argc > 2 && strcmp(argv[2], "--interp") == 0);   /* sin AOT: el bytecode */
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

    if (!interp) aot_FloatNat_register(vm);    /* las native → sus thunks de C */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
