/*
 * log.h (ESP32, S3 + P4) — el firmware usa el NÚCLEO PORTABLE del log
 * (src/bpvm_log.c). Este header solo re-expone la API portable
 * (log_printf/flush/dump/clear/stats vienen de bpvm_log.h) y declara log_init(),
 * que construye la cintura del ESP32 (esp_timer + esp_partition) y arranca el
 * núcleo. Espejo exacto de stm32/port/log.h: la lógica del log NO vive aquí, es
 * idéntica en las 3 familias.
 */
#ifndef BPVM_ESP32_LOG_H
#define BPVM_ESP32_LOG_H

#include "bpvm_log.h"   /* log_printf / log_flush / log_dump / log_clear_* / stats */

#ifdef __cplusplus
extern "C" {
#endif

/* Construye la cintura del ESP32 y recupera el snapshot anterior (post-mortem).
 * Llamar UNA vez, lo ANTES posible en app_main — antes del climb del boot, para
 * que el propio arranque quede registrado (que es justo cuando más falta hace).
 * Si no hay partición donde persistir, el log sigue funcionando en RAM. */
void log_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_LOG_H */
