/*
 * test_kill.c — P-run-stop (#257): KILL cooperativo de la VM-C.
 *
 * Fase 1: KillLoop.mod (bucle infinito) con bpvm_run — el poll_cb
 *         "recibe el KILL" en el poll nº 20 → espera BPVM_KILLED rápido.
 * Fase 2: KillLoop.mod con bpvm_run_smp(1) — mismo poll → BPVM_KILLED.
 * Fase 3: KillSleep.mod (sleepSec(3600), CERO threads runnable) con
 *         bpvm_run — el tope de 50 ms de las esperas debe dar paso al
 *         poll → BPVM_KILLED en << 2 s (sin el tope tardaría 1 hora).
 *
 * Salida esperada:
 *   arrancado
 *   [kill] fase 1: status=terminado por KILL (... ms) PASS
 *   arrancado
 *   [kill] fase 2: status=terminado por KILL (... ms) PASS
 *   durmiendo
 *   [kill] fase 3: status=terminado por KILL (... ms) PASS
 * exit code 0 si las tres fases pasan.
 */
#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int s_polls = 0;

static int kill_after_20(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    s_polls++;
    return s_polls >= 20;
}

static long long now_ms(void) {
    return (long long) clock() * 1000LL / CLOCKS_PER_SEC;
}

/* Corre `mod_path` con el poll asesino; devuelve 0 si PASS. */
static int run_phase(int phase, const char* mod_path, int smp) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return 1;
    }

    s_polls = 0;
    bpvm_set_poll(vm, kill_after_20, NULL);

    long long t0 = now_ms();
    s = smp ? bpvm_run_smp(vm, 1) : bpvm_run(vm);
    long long dt = now_ms() - t0;

    int pass = (s == BPVM_KILLED) && (dt < 5000);
    fprintf(stderr, "[kill] fase %d: status=%s (%lld ms) %s\n",
            phase, bpvm_status_str(s), dt, pass ? "PASS" : "FAIL");

    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* loop_mod  = (argc > 1) ? argv[1] : "KillLoop.mod";
    const char* sleep_mod = (argc > 2) ? argv[2] : "KillSleep.mod";

    int rc = 0;
    rc |= run_phase(1, loop_mod, 0);
    rc |= run_phase(2, loop_mod, 1);
    rc |= run_phase(3, sleep_mod, 0);
    return rc;
}
