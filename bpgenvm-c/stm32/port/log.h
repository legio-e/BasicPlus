/*
 * log.h (STM32) — el firmware usa el NÚCLEO PORTABLE del log (src/bpvm_log.c).
 * Este header solo re-expone la API portable (log_printf/flush/dump/clear/stats
 * vienen de bpvm_log.h) y declara log_init(), que construye la cintura del STM32
 * (HAL_GetTick + stm32_flash + sector BP_LOG) y arranca el núcleo. La lógica del
 * log ya NO vive aquí — es idéntica en las 3 familias.
 */
#ifndef BPVM_STM32_LOG_H
#define BPVM_STM32_LOG_H

#include "bpvm_log.h"   /* log_printf / log_flush / log_dump / log_clear_* / stats */

#ifdef __cplusplus
extern "C" {
#endif

/* Construye la cintura del STM32 y recupera el snapshot anterior (post-mortem).
 * Llamar UNA vez antes del primer log_printf. */
void log_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_STM32_LOG_H */
