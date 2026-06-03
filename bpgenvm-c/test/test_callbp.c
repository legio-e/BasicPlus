/*
 * test_callbp.c — driver host de P-aot-call-bp (puente native→BP, §8).
 *
 * Carga NativeBridge.mod, registra el thunk AOT a mano de `compute` (que
 * delega en la BP `helper` vía vm->aot_helpers->call_bp_i32) y corre main.
 *
 * Lo CRÍTICO: `compute` queda HIJACKEADO por el thunk native; su única forma
 * de obtener el resultado correcto es ejecutar el cuerpo BP de `helper` a
 * través del puente (intérprete anidado). Si el puente funciona, la salida es
 * byte-idéntica a la de la VM-Java (que interpreta compute como BP normal):
 *
 *   [helper BP] recibe 5
 *   compute(5) = 54
 *   [helper BP] recibe 8
 *   compute(8) = 84
 *
 * status final esperado: BPVM_OK.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_NativeBridge_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "NativeBridge.mod";
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

    aot_NativeBridge_register(vm);   /* hijack: compute → thunk → puente → helper */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
