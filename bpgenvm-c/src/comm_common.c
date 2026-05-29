/*
 * comm_common.c — ring buffer compartido por comm_host.c y comm_pico.c.
 *
 * 100% transport-agnóstico: sólo usa bpvm_platform_* (mutex + condvar)
 * y libc (malloc, memcpy). El layout vive en bpvm_smp.h (struct
 * bpvm_output_queue_t) — aquí están las ops.
 *
 * Modelo: ring buffer de bytes con UN productor coordinado (varios
 * workers, serializados por el mutex) y UN consumer (el comm task).
 * Si el ring se llena, los productores bloquean en cond_wait
 * (not_full); cuando el consumer libera espacio hace cond_broadcast.
 * Si el ring se vacía, el consumer bloquea en not_empty hasta que un
 * productor encole — o hasta que closed=true (drena + sale).
 */

#include "comm_queue.h"
#include "bpvm_platform.h"

#include <stdlib.h>
#include <string.h>

int bpvm_oq_init(bpvm_output_queue_t* q, size_t cap) {
    q->buf = (char*) malloc(cap);
    if (!q->buf) return -1;
    q->cap  = cap;
    q->head = q->tail = q->used = 0;
    q->closed = false;
    if (bpvm_platform_mutex_init(&q->mtx) != 0) {
        free(q->buf); q->buf = NULL; return -1;
    }
    if (bpvm_platform_cond_init(&q->not_empty) != 0) {
        bpvm_platform_mutex_destroy(&q->mtx);
        free(q->buf); q->buf = NULL; return -1;
    }
    if (bpvm_platform_cond_init(&q->not_full) != 0) {
        bpvm_platform_cond_destroy(&q->not_empty);
        bpvm_platform_mutex_destroy(&q->mtx);
        free(q->buf); q->buf = NULL; return -1;
    }
    return 0;
}

void bpvm_oq_destroy(bpvm_output_queue_t* q) {
    if (!q || !q->buf) return;
    bpvm_platform_cond_destroy(&q->not_full);
    bpvm_platform_cond_destroy(&q->not_empty);
    bpvm_platform_mutex_destroy(&q->mtx);
    free(q->buf);
    q->buf = NULL;
}

/* Productor — push síncrono (bloquea si lleno). Soporta wrap-around. */
void bpvm_oq_push(bpvm_output_queue_t* q, const char* src, size_t len) {
    bpvm_platform_mutex_lock(&q->mtx);
    size_t pushed = 0;
    while (pushed < len) {
        while (q->used == q->cap && !q->closed) {
            bpvm_platform_cond_wait(&q->not_full, &q->mtx);
        }
        if (q->closed) break;                  /* consumer cerrado — discard */
        size_t free_space = q->cap - q->used;
        size_t to_write = len - pushed;
        if (to_write > free_space) to_write = free_space;
        size_t first = q->cap - q->head;
        if (first > to_write) first = to_write;
        memcpy(q->buf + q->head, src + pushed, first);
        size_t rest = to_write - first;
        if (rest) memcpy(q->buf, src + pushed + first, rest);
        q->head = (q->head + to_write) % q->cap;
        q->used += to_write;
        pushed += to_write;
        bpvm_platform_cond_signal(&q->not_empty);
    }
    bpvm_platform_mutex_unlock(&q->mtx);
}

/* Consumer — pop bloqueante. Devuelve nº bytes leídos a dst. 0 ⇒
 * closed + drenado (señal de salida para el comm task). */
size_t bpvm_oq_pop(bpvm_output_queue_t* q, char* dst, size_t max) {
    bpvm_platform_mutex_lock(&q->mtx);
    while (q->used == 0 && !q->closed) {
        bpvm_platform_cond_wait(&q->not_empty, &q->mtx);
    }
    size_t to_read = q->used < max ? q->used : max;
    if (to_read > 0) {
        size_t first = q->cap - q->tail;
        if (first > to_read) first = to_read;
        memcpy(dst, q->buf + q->tail, first);
        size_t rest = to_read - first;
        if (rest) memcpy(dst + first, q->buf, rest);
        q->tail = (q->tail + to_read) % q->cap;
        q->used -= to_read;
        bpvm_platform_cond_broadcast(&q->not_full);
    }
    bpvm_platform_mutex_unlock(&q->mtx);
    return to_read;
}

void bpvm_oq_close(bpvm_output_queue_t* q) {
    bpvm_platform_mutex_lock(&q->mtx);
    q->closed = true;
    bpvm_platform_cond_broadcast(&q->not_empty);
    bpvm_platform_cond_broadcast(&q->not_full);
    bpvm_platform_mutex_unlock(&q->mtx);
}
