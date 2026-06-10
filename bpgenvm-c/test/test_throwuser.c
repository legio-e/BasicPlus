/*
 * test_throwuser.c — driver host de #213: `throw <ClaseDeUsuario>(args)`
 * desde native, cazado por try/catch BP con la clase del usuario.
 *
 * Carga ThrowUser.mod (+ Core.mod como dependencia implícita: RangoError
 * extiende Core.Exception), registra el thunk AOT de checkRange y corre Main.
 * El thunk construye la instancia EN EL INTÉRPRETE via la factory
 * ThrowUser.__cls_new_RangoError (puente call_bp_i32) y la lanza con
 * throw_ref → pending_ref del boundary → eh_unwind.
 *
 * Salida esperada (byte-idéntica a la VM-Java y a la VM-C interpretada):
 *   10
 *   caught: valor negativo (valor=-7)
 *   fin
 * status final: BPVM_OK (el throw fue cazado por el try/catch BP).
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

extern void aot_ThrowUser_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "ThrowUser.mod";
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

    aot_ThrowUser_register(vm);

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
