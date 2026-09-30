/*
 * platform_pico_hw.c — lo de la Pico que el contrato de plataforma sobre FreeRTOS
 * (src/platform_freertos.c, común) deja a cada familia: el reloj, la espera activa,
 * el azar y el core víctima del bloqueo de flash.
 *
 * V7/H7 (30-sep): el paso 0 de fundir los backends de FreeRTOS
 * (docs/FUSION_BACKENDS_FREERTOS.md) — medir la Pico con el común contra su
 * fichero de siempre (pico/platform_freertos.c), del mismo commit. Se compila con
 * -DBPVM_PICO_PLATFORM_COMUN=ON (pico/CMakeLists.txt); sin la opción, el de siempre.
 * Las cuatro funciones son las de pico/platform_freertos.c, tal cual.
 */
#include "bpvm_platform.h"

#include "pico/time.h"   /* time_us_64, busy_wait_us */
#include "pico/rand.h"   /* get_rand_32 (#347) */

#include "flash_lock.h"  /* #153 — registro de core víctima de lockout */

int64_t bpvm_platform_now_ms(void) {
    /* NO xTaskGetTickCount(): el SysTick de FreeRTOS deriva de clk_sys, y un
     * Pico.setCpuFreqMHz() lo desajustaría en proporción. time_us_64() lee el
     * timer hardware del RP2350, a 1 MHz fijos desde clk_ref/XOSC. */
    return (int64_t) (time_us_64() / 1000ULL);
}

int64_t bpvm_platform_now_us(void) { return (int64_t) time_us_64(); }

void bpvm_platform_busy_wait_us(int us) {
    if (us <= 0) return;
    busy_wait_us((uint64_t) us);   /* no cede el procesador: timings críticos */
}

/* El RP2350 trae TRNG: get_rand_32() se alimenta de él. */
uint32_t bpvm_platform_random_u32(void) {
    return get_rand_32();
}

/* El gancho débil del común (src/platform_freertos.c): corre en la tarea nueva
 * antes de que empiece. Un hilo fijado a un core que no es el 0 se registra como
 * víctima del bloqueo de flash, como hacía pico/platform_freertos.c
 * (`lockout_victim = (core_id != 0)`); sin afinidad (-1), no. */
void bpvm_platform_thread_arranca(int core_id) {
    if (core_id > 0) bpvm_flash_lock_init_victim();
}
