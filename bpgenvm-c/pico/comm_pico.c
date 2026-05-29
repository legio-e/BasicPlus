/*
 * comm_pico.c — backend del comm task para PICO / FreeRTOS.
 *
 * Espejo de comm_host.c, con dos diferencias:
 *
 *   - Capacidad del ring buffer: 1 KiB (vs 4 KiB en host). La Pico tiene
 *     264 KiB SRAM total, gran parte del cual es el heap BP (mem[]).
 *     1 KiB es suficiente para soportar back-pressure típica de prints,
 *     y NO es la última línea de defensa — si se llena, el productor
 *     bloquea (correcto), no se pierde nada.
 *
 *   - Pendiente (#153 P-smp-tx-exclusive): pin del task FreeRTOS a un
 *     core concreto del RP2350. Por ahora usamos el mismo
 *     bpvm_platform_thread_create() del host — FreeRTOS SMP del Pico SDK
 *     distribuye automáticamente entre ambos cores. Eso es seguro para
 *     v1 con 1 worker (el comm task corre donde haya hueco). Cuando
 *     bumpemos a 2 workers (#153), fijaremos: comm en core 0, worker
 *     extra en core 1 con afinidad exclusiva, vía vTaskCoreAffinitySet.
 *
 * El cuerpo del comm task delega en output_cb (v1_output_sink — wire v1
 * JSON wrapping) si está instalado. Esa serialización es la razón de
 * que SMP funcione con el wire: dos workers no pueden entrelazar los
 * fputs del wrapper porque ESTA es la única task que lo invoca.
 *
 * Sin output_cb (caso degenerado: no se montó el REPL antes de arrancar
 * SMP), fallback a fputs(stdout) crudo — útil para diagnóstico en
 * boot, no para producción.
 */

#include "bpvm_comm.h"
#include "bpvm_smp.h"
#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include "comm_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BPVM_PICO_SMP_TRACE
#include "log.h"   /* migas a flash para depurar el cuelgue SMP (#153) */
#endif

#define OUTPUT_QUEUE_CAP_PICO 1024

/* ---------- Comm task body ---------- */

static void comm_task_entry(void* arg) {
    bpvm_t* vm = (bpvm_t*) arg;
    if (!vm || !vm->smp) return;
    bpvm_output_queue_t* q = &vm->smp->output_queue;
    /* Chunk pequeño en stack: la task tiene 4 KB total (configurado en
     * platform_freertos.c::xTaskCreate), gastar 128 bytes en stack
     * temporal es asumible. */
    char tmp[128];
#ifdef BPVM_PICO_SMP_TRACE
    int dbg = 0;
    log_printf("comm: task entry, cb=%p", (void*) vm->output_cb);
    log_flush();
#endif
    for (;;) {
#ifdef BPVM_PICO_SMP_TRACE
        /* Flush SÓLO en los dos puntos "a punto de bloquear" (pop y cb),
         * primeras 4 vueltas. El flush hace erase de flash (IRQs off
         * ~unos ms) → no abusar para no perturbar el propio USB que
         * medimos. Interpretación tras un cuelgue (leer con LOG):
         *   último = "pop #k"    → bloqueado esperando al worker (no
         *                          produce): deadlock scheduler/cola.
         *   último = "pre-cb #k" → bloqueado ESCRIBIENDO a USB. */
        if (dbg < 4) { log_printf("comm: pop #%d", dbg); log_flush(); }
#endif
        size_t n = bpvm_oq_pop(q, tmp, sizeof(tmp));
        if (n == 0) {
            /* closed + drained — workers han terminado, salimos. */
#ifdef BPVM_PICO_SMP_TRACE
            log_printf("comm: closed, exit (popped #%d)", dbg); log_flush();
#endif
            return;
        }
        if (vm->output_cb) {
            /* Path normal: v1_output_sink envuelve en wire v1 JSON
             * OUTPUT y hace fputs(stdout). Single thread → JSON entero. */
#ifdef BPVM_PICO_SMP_TRACE
            if (dbg < 4) { log_printf("comm: pre-cb #%d n=%u", dbg, (unsigned) n); log_flush(); }
#endif
            vm->output_cb(tmp, n, vm->output_user);
        } else {
            fwrite(tmp, 1, n, stdout);
            fflush(stdout);
        }
#ifdef BPVM_PICO_SMP_TRACE
        dbg++;
#endif
    }
}

/* ---------- API pública (bpvm_comm.h) ---------- */

int bpvm_comm_start(bpvm_t* vm) {
    if (!vm || !vm->smp) return -1;
    bpvm_output_queue_t* q = &vm->smp->output_queue;
    if (bpvm_oq_init(q, OUTPUT_QUEUE_CAP_PICO) != 0) return -1;
    /* Pinning intent: comm task en core 0 (donde vive USB CDC del SDK).
     * Hoy (configNUMBER_OF_CORES=1) la afinidad se ignora — el thread
     * va al único core. Al bumpear a 2 cores (#153), el comm queda
     * "anclado" lado USB y los workers de cómputo no roban CPU al
     * draining del output. */
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
