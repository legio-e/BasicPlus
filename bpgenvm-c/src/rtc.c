/*
 * rtc.c — fachada de hooks Rtc para la VM C.
 *
 * Stub portable: mantiene un offset entre epoch y now monotonic. Si hay backend
 * registrado, delega en él.
 *
 * ⚠️ QUIÉN REGISTRA BACKEND, HOY: SÓLO EL STM32 (gpio_stm32.c:790, :1009), que sí
 * tiene RTC de hardware con dominio de respaldo. La Pico y el ESP32 corren el stub,
 * y eso NO es un olvido:
 *   - el RP2350 NO tiene RTC dedicado (el RP2040 sí lo tenía). Lo que tiene es el
 *     AON Timer, un contador de 64 bits de milisegundos en el dominio siempre
 *     encendido — sirve de despertador, pero vive en POWMAN y hoy no enlazamos
 *     `hardware_powman` (ver la lista de pico/CMakeLists.txt:195-213);
 *   - el ESP32 usa el stub a propósito (gpio_esp32.c:595).
 * Una placa RP2350 que anuncie RTC lo lleva en un chip EXTERNO por I2C (PCF85063 y
 * similares): eso es un driver de placa, no silicio, y sólo despierta al micro si su
 * línea INT está cableada a un pin capaz de despertar.
 *
 * Consecuencia para quien lea la hora: en la Pico y el ESP, `Rtc` es un offset por
 * software sobre el reloj monotónico. Muere en cada reset y el programa no tiene
 * forma de saber si la hora es real o inventada. Detalle en docs/BAJO_CONSUMO_CENSO.md.
 */

#include "bpvm_rtc.h"
#include "bpvm_platform.h"
#include <stddef.h>

static const bpvm_rtc_backend_t* g_backend = NULL;

/* Offset entre epoch deseado y now_ms monotonic. Inicializado a 0,
 * lo que significa "sin calibrar" (nowMs devuelve los ms monotonic
 * crudos). */
static int64_t g_epoch_offset_ms = 0;

void bpvm_rtc_set_backend(const bpvm_rtc_backend_t* backend) {
    g_backend = backend;
}

int64_t bpvm_rtc_now_ms(void) {
    if (g_backend && g_backend->nowMs) {
        return g_backend->nowMs();
    }
    return bpvm_platform_now_ms() + g_epoch_offset_ms;
}

void bpvm_rtc_set_now_ms(int64_t epoch_ms) {
    if (g_backend && g_backend->setNowMs) {
        g_backend->setNowMs(epoch_ms);
        return;
    }
    /* offset := epoch - now_monotonic → al sumar en nowMs(), recuperamos epoch. */
    g_epoch_offset_ms = epoch_ms - bpvm_platform_now_ms();
}
