/*
 * log.h (Pico/RP2350) — el firmware usa el NÚCLEO PORTABLE del log
 * (`src/bpvm_log.c`). Este header sólo re-expone la API portable —`log_printf`,
 * `log_flush`, `log_dump`, `log_clear_*`, el interruptor y las estadísticas
 * vienen de `bpvm_log.h`— y declara `log_init()`, que construye la cintura del
 * RP2350 (FreeRTOS + XIP + `flash_lock` sobre el sector BP_LOG) y arranca el
 * núcleo.
 *
 * La lógica del log ya NO vive aquí: es idéntica en las cuatro imágenes. Gemelo
 * de `stm32/port/log.h` y del `log.h` del ESP32. Ver `pico/log.c` para qué se
 * conserva de la versión anterior y qué cambió a propósito.
 */
#ifndef BPVM_PICO_LOG_H
#define BPVM_PICO_LOG_H

#include "bpvm_log.h"   /* log_printf / log_flush / log_dump / log_clear_* / stats */

#ifdef __cplusplus
extern "C" {
#endif

/* Construye la cintura del RP2350 y recupera el snapshot anterior (post-mortem
 * #439: si la RAM sobrevivió al reset, manda sobre el flash). Llamar UNA vez
 * antes del primer `log_printf`. */
void log_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_LOG_H */
