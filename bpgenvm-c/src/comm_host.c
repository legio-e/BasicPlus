/*
 * comm_host.c — backend del comm task para HOST (Linux/macOS/MinGW).
 *
 * Responsabilidad: drenar la output queue (workers BP producen via
 * print() → bpvm_comm_output_enqueue) y volcarla a stdout. SIN RX:
 * en host la VM corre como CLI single-run, no hay REPL bidireccional
 * — eso es comm_pico.c.
 *
 * El ring buffer + sus ops viven en comm_common.c. Aquí sólo el comm
 * task entry específico de host + el cableado al API público.
 */

#include "bpvm_comm.h"
#include "bpvm_smp.h"
#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include "comm_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT_QUEUE_CAP_HOST 4096

/* ---------- Comm task body (host: drena → output_cb o stdout) ---------- */

static void comm_task_entry(void* arg) {
    bpvm_t* vm = (bpvm_t*) arg;
    if (!vm || !vm->smp) return;
    bpvm_output_queue_t* q = &vm->smp->output_queue;
    char tmp[256];
    for (;;) {
        size_t n = bpvm_oq_pop(q, tmp, sizeof(tmp));
        if (n == 0) {
            /* closed + drained */
            return;
        }
        /* Si la VM tiene callback instalado (test harness, IDE), la
         * llamamos AQUÍ — single thread, sin race. Si no, fwrite directo
         * a stdout (host CLI). */
        if (vm->output_cb) {
            vm->output_cb(tmp, n, vm->output_user);
        } else {
            fwrite(tmp, 1, n, stdout);
            fflush(stdout);
        }
    }
}

/* ---------- API pública (bpvm_comm.h) ---------- */

int bpvm_comm_start(bpvm_t* vm) {
    if (!vm || !vm->smp) return -1;
    bpvm_output_queue_t* q = &vm->smp->output_queue;
    if (bpvm_oq_init(q, OUTPUT_QUEUE_CAP_HOST) != 0) return -1;
    /* Pinning expresado por consistencia con comm_pico.c (intent: comm
     * en core 0). En pthread es no-op — el OS decide. */
    if (bpvm_platform_thread_create_pinned(&vm->smp->comm_thread,
                                            comm_task_entry, vm, 0) != 0) {
        bpvm_oq_destroy(q);
        return -1;
    }
    return 0;
}

void bpvm_comm_stop(bpvm_t* vm) {
    if (!vm || !vm->smp) return;
    bpvm_output_queue_t* q = &vm->smp->output_queue;
    if (!q->buf) return;       /* nunca arrancó */
    bpvm_oq_close(q);
    bpvm_platform_thread_join(&vm->smp->comm_thread);
    bpvm_oq_destroy(q);
}

void bpvm_comm_output_enqueue(bpvm_t* vm, const char* buf, size_t len) {
    if (!vm || !vm->smp || !buf || len == 0) return;
    bpvm_oq_push(&vm->smp->output_queue, buf, len);
}
