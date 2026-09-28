/*
 * test_compactsmp.c — V7/H7 #595 (28-sep): Heap.compact() y el scheduler SMP, el caso que encontró la revisión.
 *
 * samples/HeapCompactEv.mod: un hilo cuya última sentencia es un `raise` muere debiendo su evento; el scheduler lo
 * resucita para atenderlo (#342), `join` espera a que lo haga y, después, Heap.compact() no ve ningún hilo vivo.
 * Con el scheduler SMP, el join se despertaba ANTES de la resurrección (do_wakeups_locked miraba los joins primero):
 * «join vuelto» salía antes que el handler y la compactación se rechazaba —4 de cada 6 pasadas— o no, según el
 * momento. Y la comprobación de «otros hilos vivos» se hacía fuera del mundo parado. Arreglado: resucitar antes de
 * los joins, un join no despierta con el hilo en deuda, y la comprobación, con el mundo parado (heap.c).
 *
 * Se corre N veces con dos workers (bpvm_run_smp) y una sin SMP, y cada pasada tiene que decir EXACTAMENTE lo que
 * dice miVM. La salida se recoge con el `line` del hilo io.
 *
 * Salida: una línea por modo y, si todo va, «OK: ...» (la señal de check-testc). exit 0 si pasan.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_io.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESPERADO "handler: atendido\njoin vuelto\ncompactado\n"
#define PASADAS_SMP 30

static char s_salida[1024];
static size_t s_n = 0;

static void recoge(const char* s, size_t len, void* user) {
    (void) user;
    if (s_n + len >= sizeof s_salida) len = sizeof s_salida - 1 - s_n;
    memcpy(s_salida + s_n, s, len);
    s_n += len;
    s_salida[s_n] = 0;
}

/* Una pasada; workers = 0 → bpvm_run (sin SMP). Devuelve 0 si dice lo esperado. */
static int corre(const char* mod, int workers) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init fallo\n"); free(mem); return 1; }
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return 1;
    }
    s_n = 0;
    s_salida[0] = 0;
    bpvm_io_ops_t ops;
    memset(&ops, 0, sizeof ops);
    ops.line = recoge;
    if (bpvm_io_start(vm, &ops, 0) != 0) {
        fprintf(stderr, "no arranca el hilo io\n");
        bpvm_destroy(vm); free(mem); return 1;
    }
    s = (workers > 0) ? bpvm_run_smp(vm, workers) : bpvm_run(vm);
    bpvm_io_stop(vm);
    int pass = (s == BPVM_OK) && strcmp(s_salida, ESPERADO) == 0;
    if (!pass) printf("[compactsmp] workers=%d status=%s, el programa dijo:\n%s", workers, bpvm_status_str(s), s_salida);
    bpvm_destroy(vm); free(mem);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();   /* el núcleo resuelve por la fachada del FS (como test_kill) */
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod = (argc > 1) ? argv[1] : "HeapCompactEv.mod";
    int rc = corre(mod, 0);
    printf("[compactsmp] sin SMP: %s\n", rc ? "FAIL" : "PASS");
    int fallos = 0;
    for (int i = 0; i < PASADAS_SMP; i++) fallos += corre(mod, 2);
    printf("[compactsmp] con 2 workers: %d/%d pasadas como miVM\n", PASADAS_SMP - fallos, PASADAS_SMP);
    if (rc == 0 && fallos == 0)
        printf("OK: con SMP, join espera al handler post-mortem y Heap.compact() compacta, como miVM\n");
    return (rc == 0 && fallos == 0) ? 0 : 1;
}
