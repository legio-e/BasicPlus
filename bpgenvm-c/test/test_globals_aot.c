/*
 * test_globals_aot.c — driver de validación host de #172
 * P-aot-module-globals.
 *
 * Inicializa la VM, carga GlobalsAot.mod, registra el thunk
 * generado por AotCEmitter, y ejecuta. La salida esperada es
 * 1 2 3 4 5 — la global counter persiste entre invocaciones de
 * la función nativa.
 *
 * Compilar con (host):
 *   gcc test/test_globals_aot.c [build/...o sin test main] aot_GlobalsAot.o
 *       -o test_globals_aot -lpthread
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>

/* Forward del registrador emitido por AotCEmitter en aot_GlobalsAot.c. */
extern void aot_GlobalsAot_register(struct bpvm* vm);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* mod_path = (argc > 1) ? argv[1] : "GlobalsAot.mod";
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

    /* Registrar el thunk AOT — esto es lo que activa el hijack en el
     * registry. El intérprete consulta el registry en OP_CALL/CALL_EXT;
     * si encuentra match, invoca el thunk en vez de hacer dispatch BP. */
    aot_GlobalsAot_register(vm);

    printf("=== AOT-active run ===\n");
    s = bpvm_run(vm);
    printf("=== status=%s ===\n", bpvm_status_str(s));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
