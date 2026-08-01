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

/* #347 — 32 bits al azar. EL U575 SÍ TRAE UN RNG DE VERDAD, pero CubeMX no lo
 * inicializa en este proyecto, así que aquí va PRNG (xorshift32) sembrado del
 * RTC — que sí está configurado (LSI ~32 kHz, ver gpio_stm32.c).
 *
 * La consecuencia hay que tenerla escrita, porque es la trampa de esta familia:
 * un PRNG en un micro que arranca DETERMINISTA repite la MISMA secuencia en
 * cada reset si la semilla no varía. En el host eso lo resuelve el reloj del
 * sistema; aquí lo resuelve el RTC, y sólo mientras el RTC conserve la hora.
 * Si el RTC está a cero (sin VBAT y recién alimentado), la secuencia se repite.
 * Es un límite ANUNCIADO, no un fallo mudo.
 *
 * Cuando haga falta azar de verdad: encender el RNG desde ESTE fichero, que es
 * código nuestro. Nunca tocando los generados de CubeMX — un generado parcheado
 * a mano muere en la siguiente regeneración.
 *
 * Las subsegundas del RTC entran en la mezcla a propósito: son el único dígito
 * que cambia rápido, y son las que hacen que dos arranques seguidos difieran. */
extern RTC_HandleTypeDef hrtc;

uint32_t bpvm_platform_random_u32(void) {
    static uint32_t s = 0;
    if (s == 0) {
        RTC_TimeTypeDef t; RTC_DateTypeDef d;
        HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);  /* GetTime ANTES de GetDate */
        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);  /* (desbloquea el shadow)   */
        s = ((uint32_t) t.Hours   << 24)
          ^ ((uint32_t) t.Minutes << 16)
          ^ ((uint32_t) t.Seconds <<  8)
          ^  (uint32_t) d.Date
          ^  (uint32_t) t.SubSeconds
          ^  HAL_GetTick();
        if (s == 0) s = 0x9E3779B9u;   /* xorshift se queda clavado en 0 */
    }
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}
