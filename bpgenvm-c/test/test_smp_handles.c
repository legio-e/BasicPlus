/*
 * test_smp_handles.c — Paso 7 (V4): estrés de la TABLA DE HANDLES bajo
 * concurrencia real. N pthreads martillean bpvm_handle_register +
 * bpvm_handle_kill sobre un vm COMPARTIDO — igual que en la VM real, donde
 * handle_register corre FUERA del vm_lock (heap_alloc suelta el lock antes).
 *
 * Detección de corrupción: cada thread registra un `addr` único (!=0), lo
 * derefea y comprueba que sale lo mismo. Si la free-list / free_top /
 * handle_next se corrompen por la carrera, un register roba el idx de otro →
 * handle_addr[idx] queda pisado → el deref devuelve el addr EQUIVOCADO (cuenta
 * corrupción), o el estado revienta (free_top underflow → OOB → SIGSEGV).
 *
 * CONTROL (idea de Eduardo): con 1 thread NO hay carrera → 0 corrupciones y
 * sin crash. Prueba que el test es sano y aísla la CONCURRENCIA como causa.
 *
 * Uso: test_smp_handles [nthreads] [iters]    (default 4, 200000)
 * Salida: "threads=N iters=M corrupciones=K"  · exit 0 si K==0, 1 si K>0.
 *
 * Pre-calienta la tabla single-threaded (registra+mata 8192) para poblar la
 * free-list → la fase concurrente REUSA slots (aísla la carrera de la
 * free-list de la de crecimiento/realloc).
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static bpvm_t* g_vm;
static int     g_iters;

typedef struct { int tid; long corr; } targ_t;

static void* worker(void* p) {
    targ_t* a = (targ_t*) p;
    uint32_t base = (uint32_t)(a->tid + 1) * 1000000u;   /* !=0, único por thread */
    for (int i = 0; i < g_iters; i++) {
        uint32_t addr = base + (uint32_t) i;             /* !=0 */
        bpref_t  h    = bpvm_handle_register(g_vm, addr);
        uint32_t got  = bpref_deref(g_vm, h);
        if (got != addr) a->corr++;                      /* otro thread robó el slot */
        bpvm_handle_kill(g_vm, h);
    }
    return NULL;
}

int main(int argc, char** argv) {
    int nthreads = argc > 1 ? atoi(argv[1]) : 4;
    g_iters      = argc > 2 ? atoi(argv[2]) : 200000;
    if (nthreads < 1)  nthreads = 1;
    if (nthreads > 64) nthreads = 64;

    static uint8_t mem[262144];
    g_vm = bpvm_init(mem, sizeof(mem), sizeof(mem) / 2);
    if (!g_vm) { fprintf(stderr, "bpvm_init fallo\n"); return 2; }

    /* Activar el vm_lock (modo SMP): bpvm_smp_lock es no-op si vm->smp==NULL, y este
     * test simula varios workers — el único escenario donde varios threads tocan la
     * tabla. Inicializamos un bpvm_smp_t mínimo (solo su vm_lock; los demás campos
     * quedan a 0, que es lo que el lock necesita). Así el fix (paso 7) queda ejercitado. */
    g_vm->smp = (bpvm_smp_t*) calloc(1, sizeof(bpvm_smp_t));
    if (!g_vm->smp) { fprintf(stderr, "smp calloc fallo\n"); return 2; }
    bpvm_platform_mutex_init(&g_vm->smp->vm_lock);

    /* Pre-calentar: puebla la free-list con 8192 slots reciclables. */
    for (int i = 0; i < 8192; i++) {
        bpref_t h = bpvm_handle_register(g_vm, (uint32_t) i + 1);
        bpvm_handle_kill(g_vm, h);
    }

    pthread_t th[64];
    targ_t    ta[64];
    for (int t = 0; t < nthreads; t++) {
        ta[t].tid = t; ta[t].corr = 0;
        pthread_create(&th[t], NULL, worker, &ta[t]);
    }
    long total = 0;
    for (int t = 0; t < nthreads; t++) { pthread_join(th[t], NULL); total += ta[t].corr; }

    printf("threads=%d iters=%d corrupciones=%ld\n", nthreads, g_iters, total);
    return total == 0 ? 0 : 1;
}
