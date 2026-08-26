/*
 * bpvm_comm.h — comm task (H2 SMP).
 *
 * Interfaz transport-agnóstica para "el hilo dedicado de I/O externo".
 * Los workers BP nunca tocan la salida directamente — encolan via
 * bpvm_comm_output_enqueue(). El hilo comm la drena y la vuelca al
 * transport real:
 *
 *   - comm_host.c (pthread): stdout. No hay RX.
 *   - comm_pico.c (FreeRTOS task pinned core 0): USB CDC TX/RX +
 *     wire v1 parser + dispatch de comandos del IDE.
 *
 * ─── [V6/U2.4, 26-ago-2026] EL MAPA — quién implementa esto y cuándo corre ──
 *
 * Este contrato es EXCLUSIVO DEL MODO SMP, y el modo SMP es opt-in:
 *
 *   host   --smp=N en la CLI            → comm_host.c   (drena a stdout)
 *   Pico   -DBPVM_PICO_SMP_WORKERS      → comm_pico.c   (el build por defecto
 *                                          NO lo define: va single-worker)
 *   S3/P4/STM32   no ofrecen SMP        → enlazan comm_host.c COMO RELLENO
 *
 * En single-worker (todas las placas hoy) la salida NO pasa por aquí: va por
 * `vm->output_cb` — el v1_output_sink de cada REPL — directa al wire. Sólo con
 * `vm->smp` armado los prints se encolan aquí (interp.c, emit_text).
 *
 * ⚠️ El RELLENO es deliberado y hay que saberlo: en S3/P4/STM32 el backend
 * enlazado es el del host, cuyo lado de salida es fwrite(stdout) — que en esas
 * placas no es el wire. No es un bug HOY porque nadie puede armar SMP ahí (no
 * hay opt-in), pero el día que una familia ofrezca SMP tiene que traer su
 * comm_<familia>.c que hable por SU transporte, no heredar este relleno.
 *
 * 📌 Este mapa existe porque su ausencia costó DOS diagnósticos falsos en el
 * censo de V6: primero «los transportes no incluyen bpvm_comm.h» (irrelevante:
 * no es el contrato de los transportes) y luego «sólo 2 de 5 imágenes lo
 * implementan» (falso: las 5 enlazan un backend; 3 lo hacen de relleno).
 *
 * El hilo comm vive en su propio task/pthread. La VM lo arranca con
 * bpvm_comm_start() y lo para con bpvm_comm_stop() durante shutdown.
 *
 * Output queue: ring buffer de bytes con producer (workers) /
 * consumer (comm task) coordinados con mutex + condvar. Si la queue
 * se llena, los workers bloquean en cond_wait (back-pressure
 * natural — preferimos eso a perder output silenciosamente).
 */
#ifndef BPVM_COMM_H
#define BPVM_COMM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/* Init la sub-estructura de comm dentro de vm. Crea la output queue
 * (ring buffer) y arranca el hilo comm. Llamar UNA VEZ tras bpvm_init,
 * antes del primer worker. Devuelve 0 OK, -1 si malloc/thread create
 * fallan. */
int  bpvm_comm_start(struct bpvm* vm);

/* Para el hilo comm + libera. Idempotente. Llamado por bpvm_destroy. */
void bpvm_comm_stop(struct bpvm* vm);

/* Productor: encolar `len` bytes para que el hilo comm los emita.
 * Llamado por los OPs de print del intérprete (vía output_cb). Bloquea
 * si la queue está llena. Re-entrante (varios workers pueden encolar
 * simultáneamente — la queue lo serializa). */
void bpvm_comm_output_enqueue(struct bpvm* vm, const char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_COMM_H */
