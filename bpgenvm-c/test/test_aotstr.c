/*
 * test_aotstr.c — [V7/#569, #570] la lista A MANO del AOT contra el camino interpretado.
 *
 * El AOT traduce charAt, charCodeAt, substring, intToString y los newXArray a helpers de C propios
 * (bpvm_aot_helpers.c), COPIAS de los builtins que corre el intérprete. Una copia se queda atrás sin
 * que nada lo diga: `charAt("abc", 7)` daba "" con el AOT y un RuntimeError interpretado (#569). Hermano
 * de test_aotswitch.c: el mismo `.c` que el AOT genera para el `.mdn`, compilado para el PC y ejecutado
 * con `gc_bump_threshold = 1`. La receta (`make test-aotstr`) corre samples/NatStr.mod con el AOT y, con
 * `--interp`, sin él, y exige las dos salidas iguales y sin ningún FALLO.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatStr_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatStr.mod";
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

    if (!interp) aot_NatStr_register(vm);    /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
