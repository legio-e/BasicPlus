/*
 * test/test_debug_mt.c — H6.b.2.a: pausa/resume del debugger CRUZANDO THREADS.
 *
 * El núcleo (#215) deja el "cómo bloquear" al embedder. Aquí el embedder usa
 * pthreads: el intérprete corre en un thread worker; cuando alcanza una pausa
 * (breakpoint / pausa pedida / step), el pause_cb BLOQUEA en una cond var
 * mientras el thread CONTROLADOR (main) le inyecta la siguiente acción. Esto
 * prueba el patrón sin deadlock que necesita el servidor wire host
 * (H6.b.2.b = sockets + JSON encima) y que mapea al caso Pico (task intérprete
 * bloquea, task comm alimenta comandos).
 *
 * NO usa sockets ni hardware. Uso: build/test_debug_mt <fichero.mod>
 */

#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MEMSZ (512 * 1024)

/* Canal de control entre el thread intérprete y el thread controlador. */
typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  c_paused;   /* el controlador espera aquí una nueva pausa */
    pthread_cond_t  c_cmd;      /* el intérprete espera aquí un comando        */
    long            pause_seq;  /* nº de pausas vistas (monótono, evita races) */
    int             cmd;        /* acción pendiente, o -1 = ninguna            */
    uint32_t        last_pc;    /* pc de la última pausa (reporte crudo)       */
} dbg_chan_t;

/* pause_cb (corre en el thread INTÉRPRETE): reporta la pausa, despierta al
 * controlador, y BLOQUEA hasta que éste inyecta una acción. */
static bpvm_dbg_action_t mt_pause_cb(bpvm_t* vm, bpvm_thread_t* tc,
                                     uint32_t pc, void* user) {
    (void) vm; (void) tc;
    dbg_chan_t* ch = (dbg_chan_t*) user;
    pthread_mutex_lock(&ch->m);
    ch->last_pc = pc;
    ch->cmd     = -1;                 /* limpiar comando previo */
    ch->pause_seq++;
    pthread_cond_signal(&ch->c_paused);
    while (ch->cmd < 0)
        pthread_cond_wait(&ch->c_cmd, &ch->m);
    int act = ch->cmd;
    pthread_mutex_unlock(&ch->m);
    return (bpvm_dbg_action_t) act;
}

typedef struct { bpvm_t* vm; bpvm_status_t status; } worker_ctx_t;

static void* worker_main(void* arg) {
    worker_ctx_t* w = (worker_ctx_t*) arg;
    w->status = bpvm_run(w->vm);     /* corre hasta fin/STOP; bloquea en pausas */
    return NULL;
}

/* Controlador: espera a que ocurra una pausa NUEVA (seq > handled). */
static long wait_new_pause(dbg_chan_t* ch, long handled, uint32_t* out_pc) {
    pthread_mutex_lock(&ch->m);
    while (ch->pause_seq <= handled)
        pthread_cond_wait(&ch->c_paused, &ch->m);
    long seq = ch->pause_seq;
    if (out_pc) *out_pc = ch->last_pc;
    pthread_mutex_unlock(&ch->m);
    return seq;
}

/* Controlador: inyecta una acción y despierta al intérprete. */
static void post_action(dbg_chan_t* ch, bpvm_dbg_action_t act) {
    pthread_mutex_lock(&ch->m);
    ch->cmd = (int) act;
    pthread_cond_signal(&ch->c_cmd);
    pthread_mutex_unlock(&ch->m);
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2) { fprintf(stderr, "uso: test_debug_mt <fichero.mod>\n"); return 2; }

    uint8_t* mem = (uint8_t*) calloc(1, MEMSZ);
    bpvm_t*  vm  = bpvm_init(mem, MEMSZ, 0);
    if (!vm || bpvm_load_mod(vm, argv[1]) != BPVM_OK) {
        fprintf(stderr, "init/load fallo: %s\n", argv[1]); free(mem); return 1;
    }

    dbg_chan_t ch;
    memset(&ch, 0, sizeof ch);
    pthread_mutex_init(&ch.m, NULL);
    pthread_cond_init(&ch.c_paused, NULL);
    pthread_cond_init(&ch.c_cmd, NULL);
    ch.cmd = -1;

    bpvm_set_pause_cb(vm, mt_pause_cb, &ch);
    bpvm_debug_request_pause(vm);     /* 1ª pausa en el 1er opcode */

    /* Lanzar el intérprete en su propio thread. */
    worker_ctx_t w = { vm, BPVM_OK };
    pthread_t th;
    if (pthread_create(&th, NULL, worker_main, &w) != 0) {
        fprintf(stderr, "pthread_create fallo\n"); free(mem); return 1;
    }

    /* Controlador: 4 STEP cruzando threads, luego CONTINUE hasta el final. */
    int fails = 0;
    long handled = 0;
    uint32_t pcs[8]; int npc = 0;
    for (int k = 1; k <= 5; k++) {
        uint32_t pc = 0;
        handled = wait_new_pause(&ch, handled, &pc);
        if (npc < 8) pcs[npc++] = pc;
        post_action(&ch, (k < 5) ? BPVM_DBG_STEP : BPVM_DBG_CONTINUE);
    }
    pthread_join(th, NULL);

    if (ch.pause_seq != 5) { printf("FAIL: esperaba 5 pausas, vi %ld\n", ch.pause_seq); fails++; }
    if (w.status != BPVM_OK) {
        printf("FAIL: tras CONTINUE esperaba OK, vi %s\n", bpvm_status_str(w.status)); fails++;
    }
    printf("pausas cruzando threads (pc):");
    for (int i = 0; i < npc; i++) printf(" %u", pcs[i]);
    printf("  -> status=%s\n", bpvm_status_str(w.status));

    pthread_mutex_destroy(&ch.m);
    pthread_cond_destroy(&ch.c_paused);
    pthread_cond_destroy(&ch.c_cmd);
    bpvm_destroy(vm); free(mem);

    if (fails == 0) printf("OK: H6.b.2.a pausa/resume cruzando threads (worker bloquea, controlador reanuda)\n");
    else            printf("FALLOS: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
