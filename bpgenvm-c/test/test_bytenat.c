/*
 * test_bytenat.c — driver host de #193 (helpers AOT byte[]).
 *
 * Carga ByteNat.mod y registra el thunk AOT de sumBytes/fillRamp (que usan
 * array_load_u8 / array_store_i8). El run NATIVE debe dar el mismo resultado
 * que el interpretado (VM-Java): "sum = 10" (0+1+2+3+4).
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_ByteNat_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "ByteNat.mod";
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

    aot_ByteNat_register(vm);   /* hijack: sumBytes/fillRamp → thunk native (byte[]) */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
