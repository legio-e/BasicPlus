/*
 * test_aotcede.c — [V7/H5 P2b.1.c] una native que llama a una función BP que se BLOQUEA, mientras otro hilo
 * avanza y reserva memoria (GC en cada reserva).
 *
 * Hermano de test_aotswitch.c: la receta (`make test-aotcede`) corre samples/NatCede.mod con el AOT y, con
 * `--interp`, sin él, y exige las dos salidas iguales, completas y sin ningún FALLO. Con el AOT, la native
 * se suspende en su pila prestada; sin suspensión, el puente daba «cedió/falló (no soportado)».
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatCede_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatCede.mod";
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

    if (!interp) aot_NatCede_register(vm);    /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
