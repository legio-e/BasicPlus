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
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */

int bpvm_oq_sabotaje_lineas = 0;   /* SÓLO PRUEBAS (bpvm_smp.h) */
int bpvm_oq_sabotaje_aviso  = 0;   /* SÓLO PRUEBAS (bpvm_smp.h): #812, el aviso sin marca */

int bpvm_oq_init(bpvm_output_queue_t* q, size_t cap) {
    q->buf = (char*) bpvm_malloc(cap);
    if (!q->buf) return -1;
    q->cap  = cap;
    q->head = q->tail = q->used = 0;
    q->closed = false;
    q->escribiendo = false;
    q->despierta = false;   /* #812 */
    if (bpvm_platform_mutex_init(&q->mtx) != 0) {
        bpvm_free(q->buf); q->buf = NULL; return -1;
    }
    if (bpvm_platform_cond_init(&q->not_empty) != 0) {
        bpvm_platform_mutex_destroy(&q->mtx);
        bpvm_free(q->buf); q->buf = NULL; return -1;
    }
    if (bpvm_platform_cond_init(&q->not_full) != 0) {
        bpvm_platform_cond_destroy(&q->not_empty);
        bpvm_platform_mutex_destroy(&q->mtx);
        bpvm_free(q->buf); q->buf = NULL; return -1;
    }
    return 0;
}

void bpvm_oq_destroy(bpvm_output_queue_t* q) {
    if (!q || !q->buf) return;
    bpvm_platform_cond_destroy(&q->not_full);
    bpvm_platform_cond_destroy(&q->not_empty);
    bpvm_platform_mutex_destroy(&q->mtx);
    bpvm_free(q->buf);
    q->buf = NULL;
}

/* Copia `len` bytes al ring (con la vuelta), sabiendo que caben. Con el cerrojo tomado. */
static void oq_copiar(bpvm_output_queue_t* q, const char* src, size_t len) {
    size_t first = q->cap - q->head;
    if (first > len) first = len;
    memcpy(q->buf + q->head, src, first);
    size_t rest = len - first;
    if (rest) memcpy(q->buf, src + first, rest);
    q->head = (q->head + len) % q->cap;
    q->used += len;
}

/* Productor — push síncrono (bloquea si lleno). Soporta wrap-around. */
void bpvm_oq_push(bpvm_output_queue_t* q, const char* src, size_t len) {
    bpvm_trozo_t t;
    t.p = src; t.n = len;
    bpvm_oq_pushv(q, &t, 1);
}

/* V7/H7 #600 — los trozos, SEGUIDOS. Antes el push copiaba lo que cupiera, esperaba (soltando el cerrojo) y
 * seguía: con la cola medio llena, otro productor entraba en medio. Ahora, si el total cabe en la cola, se espera
 * a que haya sitio para todo y se copia de una vez; sólo lo que no cabe ni con la cola vacía va por partes. */
void bpvm_oq_pushv(bpvm_output_queue_t* q, const bpvm_trozo_t* t, int n) {
    size_t total = 0;
    for (int i = 0; i < n; i++) total += t[i].n;
    if (total == 0) return;
    bpvm_platform_mutex_lock(&q->mtx);
    /* V7/H12, 2b (la revisión) — si otro productor va a mitad de una línea que no cabe (abajo, a trozos), se espera a
     * que acabe: con dos runs, dos `print` a la vez, y uno no cabe en la cola (1 KB en la Pico). Con un run nunca pasa:
     * el que escribe a trozos es el único productor. (El que acaba avisa con not_full.) */
    while (q->escribiendo && !q->closed && !bpvm_oq_sabotaje_lineas) {
        bpvm_platform_cond_wait(&q->not_full, &q->mtx);
    }
    if (total <= q->cap) {
        while (q->cap - q->used < total && !q->closed) {
            bpvm_platform_cond_wait(&q->not_full, &q->mtx);
        }
        if (!q->closed) {                      /* cerrada: se descarta, como siempre */
            for (int i = 0; i < n; i++) if (t[i].n) oq_copiar(q, t[i].p, t[i].n);
            bpvm_platform_cond_signal(&q->not_empty);
        }
    } else {
        q->escribiendo = true;                 /* V7/H12, 2b: los demás, a esperar a que acabe la línea */
        for (int i = 0; i < n && !q->closed; i++) {
            size_t pushed = 0;
            while (pushed < t[i].n) {
                while (q->used == q->cap && !q->closed) {
                    bpvm_platform_cond_wait(&q->not_full, &q->mtx);
                }
                if (q->closed) break;          /* consumer cerrado — discard */
                size_t free_space = q->cap - q->used;
                size_t to_write = t[i].n - pushed;
                if (to_write > free_space) to_write = free_space;
                oq_copiar(q, t[i].p + pushed, to_write);
                pushed += to_write;
                bpvm_platform_cond_signal(&q->not_empty);
            }
        }
        q->escribiendo = false;
        bpvm_platform_cond_broadcast(&q->not_full);   /* los que esperaban a que acabase */
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

/* V6/A1 — la variante con tope de espera que usa el hilo `io` (ver comm_queue.h).
 * Misma mecanica que el pop de arriba; lo unico distinto es que la espera es
 * acotada y que decimos si la cola quedo cerrada y seca. */
size_t bpvm_oq_pop_timed(bpvm_output_queue_t* q, char* dst, size_t max,
                         int ms, int* eof) {
    if (eof) *eof = 0;
    bpvm_platform_mutex_lock(&q->mtx);
    /* V8 #812 — un aviso que llegó sin nadie esperando (la marca) no se pierde: no se duerme, y quien llama mira por
     * qué le despertaban (en `io`, el `fin` del programa). */
    if (q->used == 0 && !q->closed && !q->despierta) {
        (void) bpvm_platform_cond_timed_wait(&q->not_empty, &q->mtx, ms);
    }
    q->despierta = false;
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
    if (eof && q->closed && q->used == 0) *eof = 1;
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

void bpvm_oq_despertar(bpvm_output_queue_t* q) {
    bpvm_platform_mutex_lock(&q->mtx);
    if (!bpvm_oq_sabotaje_aviso) q->despierta = true;   /* #812: si nadie espera todavía, la espera siguiente lo ve */
    bpvm_platform_cond_broadcast(&q->not_empty);
    bpvm_platform_mutex_unlock(&q->mtx);
}
