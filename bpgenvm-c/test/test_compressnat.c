/*
 * test_compressnat.c — driver host de #240 (Compress.decompress LZSS, variante native).
 *
 * Carga CompressNat.mod y registra el thunk AOT de decompress (LZSS: byte[] +
 * aritmética entera shl/shr/mod, helpers array_load_u8 / array_store_i8 de #193).
 * Main (interpretado) construye un stream hecho a mano y llama a decompress
 * (native). El run NATIVE debe dar el MISMO resultado que el interpretado
 * (VM-Java) y que la versión plain-BP de bpstdlib/Compress.bp:
 *   n = 6
 *   out = ABABAB
 * status final: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_CompressNat_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "CompressNat.mod";
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

    aot_CompressNat_register(vm);   /* hijack: decompress -> thunk native (LZSS byte[]) */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
