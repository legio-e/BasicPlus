/*
 * test_aottry.c — [V7/#578] `try`/`catch`/`finally` DENTRO de una native, traducido por el AOT (AotCEmitter).
 *
 * Hermano de test_aottryvm.c (que prueba la VM con natives escritas a mano): aquí el C lo escribe el AOT. La receta
 * (`make test-aottry`) corre samples/NatTry.mod con el AOT y, con `--interp`, sin él, con GC en cada reserva, y exige
 * las dos salidas iguales, completas, sin ningún FALLO, y que el programa acabe en OK.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatTry_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatTry.mod";
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

    if (!interp) aot_NatTry_register(vm);   /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación: la excepción que aterriza tiene que seguir viva */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
