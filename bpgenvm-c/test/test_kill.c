/*
 * test_kill.c — P-run-stop (#257): KILL cooperativo de la VM-C.
 *
 * Fase 1: KillLoop.mod (bucle infinito) con bpvm_run — el poll_cb
 *         "recibe el KILL" en el poll nº 20 → espera BPVM_KILLED rápido.
 * Fase 2: KillLoop.mod con bpvm_run_smp(1) — mismo poll → BPVM_KILLED.
 * Fase 3: KillSleep.mod (sleepSec(3600), CERO threads runnable) con
 *         bpvm_run — el tope de 50 ms de las esperas debe dar paso al
 *         poll → BPVM_KILLED en << 2 s (sin el tope tardaría 1 hora).
 * Fases 4-6 (V7/H7, 27-sep): con el hilo `io` en marcha el wire es SUYO —el
 *         KILL llega por su `ops.poll`, como en una placa— y el planificador
 *         NO llama al `poll_cb` viejo (lo cuenta y tiene que dar 0). La 4 es el
 *         CONTROL (bpvm_run, que ya lo guardaba); la 5 y la 6, el SMP, con un
 *         programa que calcula y con uno que duerme (sus dos llamadas).
 * Fases 7-8 (V7/H12, etapa 2a.2, 4-oct): EL KILL DESPIERTA. KillSleep con `io` y
 *         SIN ningún poll_cb (el planificador no tiene por qué mirar nada cada
 *         50 ms): con un run (7) y con dos (8), los dos dormidos en la cola con
 *         el sueño de una hora por delante. El KILL avisa a la cola y el programa
 *         para en << 2 s. Con `--sabotaje-despertar` (el aviso apagado), no se
 *         para: la red lo corre con un `timeout` y tiene que dar rojo.
 *
 * Salida esperada:
 *   arrancado
 *   [kill] fase 1: status=terminado por KILL (... ms) PASS
 *   arrancado
 *   [kill] fase 2: status=terminado por KILL (... ms) PASS
 *   durmiendo
 *   [kill] fase 3: status=terminado por KILL (... ms) PASS
 *   (y las fases 4, 5 y 6: «..., poll viejo x0 PASS»)
 *   OK: ...
 * exit code 0 si las seis fases pasan (y entonces, la línea «OK:» de la red).
 */
#include "bpvm.h"
#include "bpvm_io.h"
#include "bpvm_internal.h"   /* V7/H12, 2a.2: el sabotaje del aviso (bpvm_la_cola) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bpvm_fs.h"

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

/* Fases 4-6 — el KILL llega por el `ops.poll` de `io` (a la 20.ª vuelta de su
 * lazo, unos 100 ms); el `poll_cb` viejo sólo cuenta. */
static int s_io_polls = 0;

static int io_kill_after_20(void* user) {
    (void) user;
    s_io_polls++;
    return s_io_polls >= 20;
}

static int poll_viejo(bpvm_t* vm, void* user) {
    (void) vm; (void) user;
    s_polls++;
    return 0;
}

/* `runs`: 0 = bpvm_run; N = bpvm_run_smp(vm, N). `con_poll_viejo`: el poll_cb que sólo cuenta (fases 4-6) o ninguno
 * (7-8: nada obliga al planificador a mirar cada 50 ms; sólo el aviso del KILL lo despierta). */
static int run_phase_io(int phase, const char* mod_path, int runs, int con_poll_viejo, long long tope_ms) {
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
    s_io_polls = 0;
    if (con_poll_viejo) bpvm_set_poll(vm, poll_viejo, NULL);
    bpvm_io_ops_t ops;
    memset(&ops, 0, sizeof ops);
    ops.poll = io_kill_after_20;
    if (bpvm_io_start(vm, &ops, 0) != 0) {
        fprintf(stderr, "[kill] fase %d: no arranca el hilo io FAIL\n", phase);
        bpvm_destroy(vm); free(mem); return 1;
    }

    long long t0 = now_ms();
    s = runs > 0 ? bpvm_run_smp(vm, runs) : bpvm_run(vm);
    long long dt = now_ms() - t0;
    bpvm_io_stop(vm);

    int pass = (s == BPVM_KILLED) && (dt < tope_ms) && (s_polls == 0);
    if (con_poll_viejo)
        fprintf(stderr, "[kill] fase %d: status=%s (%lld ms), poll viejo x%d %s\n",
                phase, bpvm_status_str(s), dt, s_polls, pass ? "PASS" : "FAIL");
    else
        fprintf(stderr, "[kill] fase %d: dormido una hora con %d run%s y sin poll: status=%s (%lld ms) %s\n",
                phase, runs > 1 ? runs : 1, runs > 1 ? "s" : "", bpvm_status_str(s), dt, pass ? "PASS" : "FAIL");

    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    /* #344 — el nucleo resuelve por la FACHADA del FS (antes fopen a
     * pelo), asi que un driver de host tiene que registrar su backend. */
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* loop_mod  = (argc > 1) ? argv[1] : "KillLoop.mod";
    const char* sleep_mod = (argc > 2) ? argv[2] : "KillSleep.mod";
    /* V7/H12, 2a.2 — SÓLO PRUEBAS: el KILL no avisa a la cola. Sólo las fases 7-8 (las demás tienen un poll que las
     * despierta); y no acaban: quien lo corre pone un `timeout` y espera rojo. */
    if (argc > 3 && !strcmp(argv[3], "--sabotaje-despertar")) {
        bpvm_la_cola.sabotaje_despertar = 1;
        int rs = run_phase_io(7, sleep_mod, 0, 0, 2000) | run_phase_io(8, sleep_mod, 2, 0, 2000);
        return rs ? 1 : 0;
    }

    int rc = 0;
    rc |= run_phase(1, loop_mod, 0);
    rc |= run_phase(2, loop_mod, 1);
    rc |= run_phase(3, sleep_mod, 0);
    rc |= run_phase_io(4, loop_mod, 0, 1, 5000);
    rc |= run_phase_io(5, loop_mod, 1, 1, 5000);
    rc |= run_phase_io(6, sleep_mod, 1, 1, 5000);
    rc |= run_phase_io(7, sleep_mod, 0, 0, 2000);
    rc |= run_phase_io(8, sleep_mod, 2, 0, 2000);
    if (rc == 0)   /* la señal buena de check-testc (compat.sh) */
        printf("OK: KILL cooperativo, 8 fases (sin io, con io y con io en SMP: el poll viejo, 0 veces; y el KILL que "
               "despierta a un programa dormido, con uno y dos runs)\n");
    return rc;
}
