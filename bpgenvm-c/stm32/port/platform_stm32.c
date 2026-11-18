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
/*
 * platform_stm32.c - LO QUE DE LA PLATAFORMA ES DE ESTE SILICIO, y nada mas.
 *
 * V6/A1.5 (5-sep-2026): el STM32 deja de ser bare-metal. Los mutex, las
 * condvar y los hilos -que eran no-ops porque no habia con quien competir- se
 * han ido a `src/platform_freertos.c`, que es LA implementacion del contrato
 * sobre FreeRTOS y la comparten las familias que lo usan. Aqui queda lo que de
 * verdad depende de esta placa: el reloj, la espera activa y el azar.
 *
 * Por que asi y no copiando el fichero de la Pico: de sus 363 lineas solo
 * cuatro eran suyas. Copiarlo habria repetido el fallo que este proyecto ya
 * tiene documentado tres veces -el comun crece y la copia privada no-.
 *
 * Lo que este fichero NO toca: la salida (eso es el sink de stm32_repl.c) ni la
 * paridad byte-identica. Solo provee primitivas.
 */
#include "bpvm_platform.h"
#include "main.h"   /* HAL_GetTick, HAL_Delay, SystemCoreClock, CoreDebug, DWT */
#include "gpio_stm32.h"   /* V8 (la tabla de la robustez): stm32_fatal y la marca del arranque */

int64_t bpvm_platform_now_ms(void) { return (int64_t) HAL_GetTick(); }

/* V7/H2 (#523, V0) — us por el DWT (ciclos / MHz): HAL_GetTick es de 1 ms y el
 * enlace de un módulo dura menos. Sólo vale para DIFERENCIAS cortas (< 2^32
 * ciclos ≈ 26 s a 160 MHz), que es lo que se mide con él. */
int64_t bpvm_platform_now_us(void) {
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    return (int64_t) (DWT->CYCCNT / (SystemCoreClock / 1000000U));
}

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

/* ===================== Los ganchos que el kernel exige =====================
 *
 * Los tres estan encendidos en stm32/port/FreeRTOSConfig.h a proposito: en un
 * micro, un desbordamiento de pila o un malloc fallido se manifiestan -si nadie
 * mira- como un cuelgue mudo tres pasos mas alla. Aqui se convierten en una
 * linea en el log persistente, que sobrevive al reset y se lee con LOG_DUMP.
 * Es la norma del proyecto: errores si, silenciosos no. */
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"

/* V8 (18-nov, la tabla de la robustez, V8_IDEAS §E15) — UN FALLO FATAL: EL INFORME Y REINICIAR, como la Pico y las
 * ESP32 (la regla de Eduardo del 5-oct: «fatal error, el log y reset»). Hasta hoy la STM32 daba su informe y se quedaba
 * parada: en el campo, hasta que alguien la desenchufara. El perro (el IWDG), LO PRIMERO, como `pico_fatal`: una vez
 * armado sólo lo para un reinicio —por eso esta familia no tiene `Wdt` (#480)—, que es justo lo que se quiere aquí: pase
 * lo que pase después (el informe, el volcado del log a la flash, que tarda unas decenas de ms), a los ~2 s la placa se
 * reinicia. Y una marca en la RAM que sobrevive al reinicio (`.noinit`, como el log): el arranque siguiente la ve, lo
 * dice en el log, el INFO dice «fallo fatal» y no hay autorun (un programa que falla al arrancar, en bucle, no). */
#define STM32_FATAL_MAGIA 0xFA7A1B0Du
static volatile uint32_t s_fatal_marca __attribute__((section(".noinit")));
static int s_tras_fatal = 0;

void stm32_fatal(void) {
    /* KR=0xCCCC lo arranca (y enciende el LSI, ~32 kHz); 0x5555 deja escribir el divisor y la recarga: /64, 500 Hz, y
     * 1000 cuentas, ~2 s. Si el cambio no se asienta (el bucle, con tope), queda el de serie, ~0,5 s: también reinicia. */
    IWDG->KR = 0xCCCCu;
    IWDG->KR = 0x5555u;
    IWDG->PR = 4u;
    IWDG->RLR = 999u;
    for (uint32_t n = 0; n < 200000u && (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0u; n++) { }
    IWDG->KR = 0xAAAAu;
    s_fatal_marca = STM32_FATAL_MAGIA;
    bpvm_log_set_enabled(1);   /* un fatal se registra aunque el log esté apagado (#423) */
}

void stm32_fatal_mirar_arranque(void) {
    if (s_fatal_marca == STM32_FATAL_MAGIA) {
        s_tras_fatal = 1;
        log_printf("boot: el arranque anterior acabo en un FALLO FATAL (su informe, arriba): sin autoarranque");
    }
    s_fatal_marca = 0u;
}

int stm32_arranque_tras_fatal(void) { return s_tras_fatal; }

static void stm32_rtos_parar(const char* que, const char* det, unsigned num) {
    stm32_fatal();             /* V8: el perro, lo primero; después, el informe */
    log_printf("RTOS: %s (%s:%u) - parado; la placa se reinicia en ~2 s", que, det ? det : "?", num);
    log_flush();
    __disable_irq();
    for (;;) { }
}

void bpvm_stm32_assert_rtos(const char* fichero, unsigned linea) {
    stm32_rtos_parar("configASSERT", fichero, linea);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char* nombre);
void vApplicationStackOverflowHook(TaskHandle_t task, char* nombre) {
    (void) task;
    stm32_rtos_parar("pila desbordada", nombre, 0);
}

void vApplicationMallocFailedHook(void);
void vApplicationMallocFailedHook(void) {
    stm32_rtos_parar("sin heap de FreeRTOS", "configTOTAL_HEAP_SIZE", 0);
}
