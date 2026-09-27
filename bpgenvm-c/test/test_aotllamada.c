/*
 * test_aotllamada.c — [V7/H5 P2b.3] lo que cuesta LLAMAR a una native desde código interpretado.
 *
 * Corre samples/NatLlamada.mod (200 000 llamadas a una native diminuta) con el AOT y, con `--interp`, sin él, y
 * dice cuánto tardó `bpvm_run`. No es una prueba de la red: es la medida de antes y después de pasar las natives a
 * un ejecutor (`make medir-llamada`).
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "bpvm_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatLlamada_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatLlamada.mod";
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

    if (!interp) aot_NatLlamada_register(vm);    /* las native → sus thunks de C */

    int64_t t0 = bpvm_platform_now_us();
    s = bpvm_run(vm);
    int64_t t1 = bpvm_platform_now_us();
    printf("[%s] %.1f ms (%.2f us por vuelta del bucle)\n", interp ? "interpretado" : "con el AOT",
           (double) (t1 - t0) / 1000.0, (double) (t1 - t0) / 200000.0);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
