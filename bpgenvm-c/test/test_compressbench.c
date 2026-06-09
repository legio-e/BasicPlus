/*
 * test_compressbench.c — driver host de #240 (benchmark LZSS, ruta NATIVE).
 *
 * Carga CompressBench.mod y registra el thunk AOT de decompress, de modo que el
 * bucle cronometrado de Main() ejecute la versión native. Para medir la ruta
 * INTERPRETADA se corre el MISMO .mod con ./build/bpgenvm-c.exe (sin registrar
 * thunk). checksum debe ser idéntico en ambas; elapsed_ms es lo que comparamos.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_CompressBench_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "CompressBench.mod";
    size_t mem_size = 4 * 1024 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_CompressBench_register(vm);   /* hijack: decompress -> thunk native */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
