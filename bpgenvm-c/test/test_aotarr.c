/*
 * test_aotarr.c — [V7/#573] los errores de los OPERADORES de arrays y cadenas desde una native, AOT contra el
 * camino interpretado.
 *
 * El AOT traduce `a[i]`, `a[i] := v` y el `+` de cadenas a helpers de C (array_load_*, array_store_*,
 * string_concat) que eran COPIAS de sus opcodes, con su propio texto: un índice fuera de rango en una native lanzaba
 * «array_load_i32: index out of bounds» donde la misma función interpretada —y miVM— lanza «ALOAD: índice fuera de
 * rango 7 (length=3)». Ahora el texto sale de UNA función que usan el opcode y el helper (bpvm_prim_texto_indice).
 * Hermano de test_aotstr.c: el mismo `.c` que el AOT genera para el `.mdn`, compilado para el PC y ejecutado con
 * `gc_bump_threshold = 1`. La receta (`make test-aotarr`) corre samples/NatArr.mod con el AOT y, con `--interp`,
 * sin él, y exige las dos salidas iguales y sin ningún FALLO. Que la interpretada sea la de miVM lo mira la paridad
 * (NatArr está en el corpus de compat.sh).
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatArr_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatArr.mod";
    int interp = (argc > 2 && strcmp(argv[2], "--interp") == 0);   /* sin AOT: el bytecode */
    size_t mem_size = 512 * 1024;   /* también el techo del `concat sin memoria` */
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    if (!interp) aot_NatArr_register(vm);    /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;     /* GC en CADA alocación */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
