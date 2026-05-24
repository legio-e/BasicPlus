/*
 * bpvm_rtc.h — hooks de plataforma para los builtins Rtc.*
 *
 * Modelo simple de "wall clock" suave:
 *   - El módulo mantiene un offset entre "epoch" y "now monotonic".
 *   - setNowMs(epoch) recalibra el offset; subsiguiente nowMs()
 *     devuelve epoch + tiempo_transcurrido.
 *
 * No persiste a través de reboots (en el Pico al menos — al volver
 * a arrancar el offset es cero y nowMs() devuelve tiempo desde boot
 * hasta que alguien llame a setNowMs). El IDE puede sincronizar al
 * conectar enviando un comando TIME con el epoch del PC.
 *
 * En el futuro: usar AON timer del RP2350 con coin cell para
 * persistencia, o reservar un sector de flash para serializar el
 * offset al deep-sleep.
 *
 * Convención de "ms":
 *   - Para host: epoch Unix (ms desde 1970-01-01 UTC).
 *   - Para Pico: igual, pero el offset arranca en 0 hasta que se
 *     calibre — durante ese tiempo nowMs() devuelve los ms desde
 *     boot, NO el epoch real.
 */
#ifndef BPVM_RTC_H
#define BPVM_RTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t (*nowMs)(void);
    void    (*setNowMs)(int64_t epoch_ms);
} bpvm_rtc_backend_t;

void bpvm_rtc_set_backend(const bpvm_rtc_backend_t* backend);

/* Funciones efectivas. Si backend NULL caen al stub portable que
 * mantiene el offset en una variable estática + bpvm_platform_now_ms. */
int64_t bpvm_rtc_now_ms(void);
void    bpvm_rtc_set_now_ms(int64_t epoch_ms);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_RTC_H */
