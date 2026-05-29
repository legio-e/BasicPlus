/*
 * bpvm_smp.h — estado interno multi-worker (H2 SMP).
 *
 * Solo lo ve la VM. Los callers usan la API pública via bpvm.h /
 * bpvm_comm.h. Esta struct se aloca on-demand cuando se llama a
 * bpvm_smp_init(vm, n_workers); si vale NULL en vm->smp, la VM está
 * en modo single-worker (F4 v1 legacy) — el scheduler clásico de
 * scheduler.c sigue funcionando.
 *
 * El layout sigue el patrón de la VM-Java (VirtualMachine.java líneas
 * 194-260): UN solo vm_lock global. Los workers lo agarran SÓLO para
 * tocar estado compartido — el interp loop corre lock-free salvo el
 * check de stop_the_world entre opcodes.
 */
#ifndef BPVM_SMP_H
#define BPVM_SMP_H

#include "bpvm_platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/* ---------- Output queue (workers → comm task) -----------
 * Ring buffer de bytes. Si se llena, productor bloquea (back-pressure).
 * Tamaño fijo, configurable en init. */
typedef struct bpvm_output_queue {
    char*    buf;            /* anillo de bytes */
    size_t   cap;            /* capacidad total */
    size_t   head;           /* siguiente posición de escritura */
    size_t   tail;           /* siguiente posición de lectura */
    size_t   used;           /* bytes ocupados; cap - used = libre */
    bpvm_platform_mutex_handle_t mtx;
    bpvm_platform_cond_handle_t  not_empty;   /* consumer waits when used=0 */
    bpvm_platform_cond_handle_t  not_full;    /* producer waits when used=cap */
    volatile bool closed;    /* true tras stop → consumer drena y sale */
} bpvm_output_queue_t;

/* ---------- Estado multi-worker --------------------------- */
typedef struct bpvm_smp {
    /* Configuración */
    int n_workers;

    /* vm_lock global. Cubre: threads[].status, runQueue (implícita en
     * threads[]), thread_count, heap_next/free_list, mutexes[]/mutex_count,
     * symbols[]/symbol_count, gc_in_progress. */
    bpvm_platform_mutex_handle_t vm_lock;

    /* sched_cond: signal cuando un tc pasa a RUNNABLE (sleep wake,
     * mutex unlock, join completado, GC unblock). Workers cond_wait
     * cuando no hay tc para correr. */
    bpvm_platform_cond_handle_t  sched_cond;

    /* Safepoint para STW GC. Volatile para que el interp loop lo lea
     * sin tomar lock (hot path). Cuando un worker la pone true, todos
     * los demás workers ceden en el siguiente safepoint. */
    volatile bool stop_the_world;

    /* True mientras un worker orquesta GC. Otros workers que entren a
     * heap_alloc esperan a que pase a false en vez de iniciar SU
     * propio dance (deadlock). Guardada bajo vm_lock. */
    bool gc_in_progress;

    /* Shutdown: tras todos los tc TERMINATED, scheduler hace
     * notifyAll + return. Workers salen del loop. Comm task también. */
    volatile bool shutdown;

    /* Threads de workers + comm. Allocated in init. */
    bpvm_platform_thread_handle_t* worker_threads;     /* [n_workers] */
    bpvm_platform_thread_handle_t  comm_thread;

    /* Contador de workers actualmente DENTRO del interp loop. Lo
     * incrementa el worker tras pickear tc, lo decrementa al volver.
     * GC orquestador espera a que llegue a 0 (mundo parado). */
    int running_workers;

    /* Output queue (workers print → comm drena). */
    bpvm_output_queue_t output_queue;
} bpvm_smp_t;

/* Inicializa el sub-estado SMP en vm. Aloca y arranca workers + comm.
 * Devuelve 0 OK, -1 si falla algo. Caller marca vm->smp = NULL en
 * caso de modo single-worker legacy. */
int  bpvm_smp_init(struct bpvm* vm, int n_workers);
void bpvm_smp_destroy(struct bpvm* vm);

/* Entry point del scheduler SMP. Spawnea workers, hace join del main
 * tc (tid=0), espera a shutdown, hace join de los workers y devuelve.
 * Análogo a bpvm_scheduler_run() pero multi-worker. */
int  bpvm_scheduler_run_smp(struct bpvm* vm);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SMP_H */
