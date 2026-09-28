/*
 * test_smpfallo.c — V7/H7 #596 (28-sep): una excepción sin atrapar acaba el RUN en ERROR también con el scheduler
 * SMP. Con --smp, el worker que la veía sólo ponía `shutdown`, bpvm_scheduler_run_smp devolvía 0 siempre y el RUN
 * acababa «status=OK» con salida 0: un programa muerto daba por bueno a quien mirara la salida (el CLI, compat, el
 * wire). Ahora devuelve el estado del primer fallo, como el scheduler simple.
 *
 * samples/SmpFallo.mod con el argumento de ejecución: sin él no lanza nadie (el control, BPVM_OK); "main" lanza en
 * Main y "hilo" en un hilo (BPVM_ERR_RUNTIME los dos). Cada caso sin SMP y, varias veces, con dos workers.
 *
 * Salida: una línea por caso y, si todo va, «OK: ...» (la señal de check-testc). exit 0 si pasan.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASADAS_SMP 5

static void calla(const char* s, size_t len, void* user) { (void) s; (void) len; (void) user; }

/* Una pasada; workers = 0 → bpvm_run. Devuelve el estado del RUN. */
static bpvm_status_t corre(const char* mod, const char* arg, int workers) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return BPVM_ERR_OOM; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { free(mem); return BPVM_ERR_OOM; }
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return s;
    }
    bpvm_set_output(vm, calla, NULL);
    bpvm_set_run_arg(vm, arg);
    s = (workers > 0) ? bpvm_run_smp(vm, workers) : bpvm_run(vm);
    bpvm_destroy(vm); free(mem);
    return s;
}

static int caso(const char* mod, const char* arg, bpvm_status_t esperado) {
    int fallos = 0;
    bpvm_status_t s = corre(mod, arg, 0);
    if (s != esperado) fallos++;
    int malos_smp = 0;
    bpvm_status_t visto = BPVM_OK;
    for (int i = 0; i < PASADAS_SMP; i++) {
        bpvm_status_t t = corre(mod, arg, 2);
        if (t != esperado) { malos_smp++; visto = t; }
    }
    fallos += malos_smp;
    printf("[smpfallo] arg=\"%s\": sin SMP %s; con 2 workers %d/%d %s%s%s%s\n", arg, bpvm_status_str(s),
           PASADAS_SMP - malos_smp, PASADAS_SMP, bpvm_status_str(esperado),
           malos_smp ? " (y alguna dio " : "", malos_smp ? bpvm_status_str(visto) : "", malos_smp ? ")" : "");
    return fallos;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod = (argc > 1) ? argv[1] : "SmpFallo.mod";
    int fallos = 0;
    fallos += caso(mod, "",     BPVM_OK);
    fallos += caso(mod, "main", BPVM_ERR_RUNTIME);
    fallos += caso(mod, "hilo", BPVM_ERR_RUNTIME);
    if (fallos == 0)
        printf("OK: una excepcion sin atrapar acaba el RUN en error, venga de Main o de un hilo, con SMP y sin el\n");
    return fallos ? 1 : 0;
}
