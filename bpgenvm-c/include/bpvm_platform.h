/*
 * bpvm_platform.h — abstracción thin de threading + sincronización.
 *
 * La VM C habla SÓLO con esta interfaz. El backend concreto se
 * selecciona al enlazar:
 *   - `platform_pthread.c` (Linux/macOS/MinGW): backend de host para
 *     desarrollo. Usa pthread_mutex_t, pthread_cond_t, pthread_t.
 *   - `platform_freertos.c` (MCU): backend de target. Usa
 *     SemaphoreHandle_t, xTaskCreate, etc. (futuro, F5+).
 *
 * La interfaz es pequeña a propósito: mutex + condvar + thread +
 * sleep. Suficiente para cubrir todas las primitivas BP de
 * concurrencia (Thread, Mutex, sleep, yield, join).
 *
 * Los tipos son **opaque pointers** (`void*`): cada backend define
 * internamente su layout. La VM nunca des-referencia un
 * `bpvm_platform_mutex_handle_t` directamente.
 */
#ifndef BPVM_PLATFORM_H
#define BPVM_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Handles opacos. El backend decide el layout interno. */
typedef void* bpvm_platform_mutex_handle_t;
typedef void* bpvm_platform_cond_handle_t;
typedef void* bpvm_platform_thread_handle_t;

/* Mutex no-recursivo. Cero retorno OK; valor negativo = error. */
int  bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m);
void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m);
void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m);
void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m);

/* Condvar asociada a un mutex (el caller mantiene el mutex tomado
 * durante wait/signal). */
int  bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c);
void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c);
void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m);
/* timed_wait con timeout en ms. Devuelve 0 si fue notificado,
 * 1 si timeout. */
int  bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms);
void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c);
void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c);

/* Thread. La función entry recibe el argumento `arg` y no devuelve
 * nada útil. El thread se considera "running" hasta que entry retorne. */
typedef void (*bpvm_thread_entry_t)(void* arg);

int  bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                  bpvm_thread_entry_t entry, void* arg);
void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t);
void bpvm_platform_thread_yield(void);
void bpvm_platform_thread_sleep_ms(int ms);

/* Timestamp en ms (monotonic). Para Java equiv de System.currentTimeMillis().
 * El offset 0 es arbitrario; sólo importan diferencias. */
int64_t bpvm_platform_now_ms(void);

/* Busy-wait sub-milisegundo. NO cede CPU al scheduler — gira en un
 * loop comprobando el reloj hasta que pasan `us` microsegundos. Útil
 * para timing crítico (setup/hold de chips, bit-bang fino) donde un
 * context switch costaría más que la pausa misma. Implementación:
 *  - Pico SDK: busy_wait_us() del SDK (lectura del timer hardware).
 *  - pthread: clock_gettime(CLOCK_MONOTONIC) + spin. */
void bpvm_platform_busy_wait_us(int us);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PLATFORM_H */
