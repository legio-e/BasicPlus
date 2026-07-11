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
#include <time.h>

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
    /* Modo "nolock" (3er arg): NO inicializa vm->smp → bpvm_smp_lock queda no-op →
     * mide el throughput SIN lock (corrompe, pero da el techo = objetivo de per-core). */
    int nolock = (argc > 3 && argv[3][0] == 'n');
    if (!nolock) {
        g_vm->smp = (bpvm_smp_t*) calloc(1, sizeof(bpvm_smp_t));
        if (!g_vm->smp) { fprintf(stderr, "smp calloc fallo\n"); return 2; }
        bpvm_platform_mutex_init(&g_vm->smp->vm_lock);
    }

    /* Paso 7b.1 — FREE CON GENERACIÓN VALIDADA (refuerzo de la maqueta), SECUENCIAL.
     * Un free RANCIO a un slot RECICLADO no debe bumpear la gen del NUEVO ocupante. */
    int genfree_bad = 0;
    {
        bpref_t h1 = bpvm_handle_register(g_vm, 0xAAAAu);   /* idx=K, gen g */
        bpvm_handle_kill(g_vm, h1);                          /* gen g+1, slot K a free-list */
        bpref_t h2 = bpvm_handle_register(g_vm, 0xBBBBu);   /* reusa K, gen g+1 (ocupante vivo) */
        bpvm_handle_kill(g_vm, h1);                          /* RANCIO: sin gen-check mata h2 */
        /* h2 debe SEGUIR vivo: su addr intacto y su gen matchea. */
        if (bpref_deref(g_vm, h2) != 0xBBBBu || bpvm_ref_dead(g_vm, h2)) genfree_bad = 1;
        bpvm_handle_kill(g_vm, h2);                          /* limpieza (kill válido) */
    }
    printf("genfree (free rancio a slot reciclado): %s\n",
           genfree_bad ? "CORROMPE el ocupante (ROJO)" : "no-op seguro (VERDE)");

    /* Pre-calentar: puebla la free-list con 8192 slots reciclables. */
    for (int i = 0; i < 8192; i++) {
        bpref_t h = bpvm_handle_register(g_vm, (uint32_t) i + 1);
        bpvm_handle_kill(g_vm, h);
    }

    pthread_t th[64];
    targ_t    ta[64];
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int t = 0; t < nthreads; t++) {
        ta[t].tid = t; ta[t].corr = 0;
        pthread_create(&th[t], NULL, worker, &ta[t]);
    }
    long total = 0;
    for (int t = 0; t < nthreads; t++) { pthread_join(th[t], NULL); total += ta[t].corr; }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    double ops  = (double) nthreads * (double) g_iters;   /* cada iter = 1 register + 1 kill */
    printf("threads=%d iters=%d corrupciones=%ld  %.3fs  %.2f Mreg-kill/s\n",
           nthreads, g_iters, total, secs, ops / secs / 1e6);
    return (total == 0 && !genfree_bad) ? 0 : 1;
}
