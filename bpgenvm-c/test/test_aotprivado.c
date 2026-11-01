/*
 * test_aotprivado.c — [V8/#752] un método PRIVADO native sobre null.
 *
 * Un método privado va fuera de la vtable y se llama con un CALL directo; con el AOT, la VM lo secuestra hacia la
 * native y el prólogo del interpretado (el que mira el receptor) no corre. La native lo mira al entrar. Hermano de
 * test_aotnombres.c: la receta (`make test-aotprivado`) corre samples/NatPrivado.mod con el AOT y, con `--interp`, sin
 * él, y exige las dos salidas iguales, completas y sin ningún FALLO.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatPrivado_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatPrivado.mod";
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

    if (!interp) aot_NatPrivado_register(vm);    /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
