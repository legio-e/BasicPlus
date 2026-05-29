/*
 * comm_queue.h — API INTERNA del ring buffer compartido por comm_host.c
 * y comm_pico.c. NO se expone fuera del módulo comm (no va en include/).
 *
 * Los símbolos viven en comm_common.c (transport-agnóstico, sólo usa
 * bpvm_platform_*). Cada backend de comm task (comm_host.c, comm_pico.c)
 * implementa SU comm_task_entry + bpvm_comm_{start,stop}, pero ambos
 * delegan en estas ops para el ring.
 *
 * Decisión de capacidad: el host usa 4 KiB porque tiene RAM de sobra;
 * la Pico la dimensiona más conservador (1 KiB) para no robar memoria
 * al heap BP. Se pasa como argumento a oq_init().
 */
#ifndef BPVM_COMM_QUEUE_INTERNAL_H
#define BPVM_COMM_QUEUE_INTERNAL_H

#include "bpvm_smp.h"

#include <stddef.h>

/* Inicializa el ring buffer. Aloca cap bytes. Mutex + 2 condvars.
 * Devuelve 0 OK, -1 si malloc / platform init fallan. */
int    bpvm_oq_init(bpvm_output_queue_t* q, size_t cap);

/* Libera el ring buffer. Idempotente. */
void   bpvm_oq_destroy(bpvm_output_queue_t* q);

/* Productor — push len bytes. Bloquea en cond_wait si lleno
 * (back-pressure). Si closed=true en mitad de un push largo, descarta
 * lo que queda silenciosamente (la sesión está terminando). */
void   bpvm_oq_push(bpvm_output_queue_t* q, const char* src, size_t len);

/* Consumer — pop hasta max bytes a dst. Bloquea en cond_wait si vacío.
 * Devuelve nº bytes leídos. 0 ⇒ closed + drenado (señal de salida). */
size_t bpvm_oq_pop(bpvm_output_queue_t* q, char* dst, size_t max);

/* Marca cerrado + despierta productores y consumer para que terminen.
 * Tras esto, push descarta y pop drena lo que queda y devuelve 0. */
void   bpvm_oq_close(bpvm_output_queue_t* q);

#endif /* BPVM_COMM_QUEUE_INTERNAL_H */
