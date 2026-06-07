/*
 * platform_stm32.c — backend de plataforma para STM32 bare-metal single-core
 * (H9.1.2). Sustituye a src/platform_pthread.c (host) y a
 * pico/platform_freertos.c (Pico). Implementa la interfaz de bpvm_platform.h.
 *
 * MVP "diseñar para el piso": el U575 es Cortex-M33 single-core y la VM corre
 * single-thread (bpvm_run, sin SMP). Por eso:
 *   - mutex / cond son no-ops (sin concurrencia no hay contención).
 *   - thread_create devuelve error: sin RTOS todavía no hay threads BP. Un
 *     programa que cree un Thread fallará LIMPIO (la VM comprueba el retorno),
 *     no se cuelga. FreeRTOS llega en H9.2+ y este fichero se reemplaza por
 *     una copia de pico/platform_freertos.c (misma API).
 *   - now_ms = HAL_GetTick(); sleep_ms = HAL_Delay(); busy_wait via DWT.
 *
 * IMPORTANTE: este fichero NO toca la salida (eso es bpvm_app.c, vía output_cb)
 * ni la paridad byte-idéntica — solo provee las primitivas de plataforma para
 * que el core enlace.
 */
#include "bpvm_platform.h"
#include "main.h"   /* HAL_GetTick, HAL_Delay, SystemCoreClock, CoreDebug, DWT */

/* ---- Mutex (no-op single-thread; handle = sentinela no-NULL) ---- */
int  bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m)    { *m = (void*)1; return 0; }
void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m) { if (m) *m = NULL; }
void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m)    { (void)m; }
void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m)  { (void)m; }

/* ---- Condvar (no-op; nunca se usa single-thread) ---- */
int  bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c)      { *c = (void*)1; return 0; }
void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c)   { if (c) *c = NULL; }
void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m) { (void)c; (void)m; }
int  bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms) { (void)c; (void)m; (void)ms; return 1; /* timeout */ }
void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c)    { (void)c; }
void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) { (void)c; }

/* ---- Thread (sin RTOS en el MVP → no soportado) ---- */
int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t, bpvm_thread_entry_t entry, void* arg) {
    (void)t; (void)entry; (void)arg;
    return -1;   /* sin threads bare-metal: la VM lo trata como fallo limpio */
}
int bpvm_platform_thread_create_pinned(bpvm_platform_thread_handle_t* t, bpvm_thread_entry_t entry, void* arg, int core_id) {
    (void)core_id;
    return bpvm_platform_thread_create(t, entry, arg);
}
void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) { (void)t; }
void bpvm_platform_thread_yield(void)                           { }
void bpvm_platform_thread_sleep_ms(int ms)                      { if (ms > 0) HAL_Delay((uint32_t)ms); }

/* ---- Tiempo ---- */
int64_t bpvm_platform_now_ms(void) { return (int64_t) HAL_GetTick(); }

void bpvm_platform_busy_wait_us(int us) {
    if (us <= 0) return;
    /* DWT cycle counter (Cortex-M33). Habilitado perezosamente la 1ª vez. */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    uint32_t cycles = (uint32_t)us * (SystemCoreClock / 1000000U);
    uint32_t start  = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) { /* spin */ }
}
