/*
 * log.h — log persistente de diagnóstico (STM32/U5). ESPEJO del log del Pico
 * (pico/log.h): MISMO API (log_init/printf/flush/dump/clear/stats) para que el
 * firmware se comporte IGUAL en las 3 familias. Lo único distinto vive en log.c
 * (cintura de plataforma: HAL_GetTick + stm32_flash + FLASH_BASE).
 *
 * Motivación: en placa no hay consola. Si el arranque escalonado (H9) se queda
 * degradado, o el firmware peta, sin esto se pierde el "qué pasó". Con esta capa:
 *   - cada mensaje importante va a un buffer en RAM (~8 KB);
 *   - en momentos críticos (boot, antes de RESET) se persiste a un sector de
 *     flash dedicado (BP_LOG, fuera de la región del firmware → sobrevive al
 *     reflasheo);
 *   - al arrancar, log_init() recupera el snapshot anterior → LOG muestra qué
 *     pasó en la sesión previa (post-mortem).
 *
 * No es un logger completo (sin niveles, sin rotación): solo "qué ha hecho el
 * firmware hasta ahora, en orden". Suficiente para bring-up.
 *
 * TODO homogeneización: unificar los 3 logs (Pico/STM32/ESP32) en un núcleo
 * portable + cintura por micro, como se hizo con el FS (bpvm_fs).
 */
#ifndef BPVM_STM32_LOG_H
#define BPVM_STM32_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el log. Recupera el snapshot de flash si hay magic válido. Llamar
 * UNA vez antes del primer log_printf. */
void log_init(void);

/* Añade una línea al log. Auto-prefija timestamp (ms desde boot, HAL_GetTick) +
 * newline. No bloquea, no toca flash. */
void log_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/* Persiste el buffer RAM al sector de flash (erase + program). Llamar en
 * momentos críticos (boot completado, antes de RESET). */
void log_flush(void);

/* Vacía el buffer RAM (no toca flash). log_clear_flash() borra además el sector. */
void log_clear_ram(void);
void log_clear_flash(void);

/* Vuelca el log actual (RAM) a un sink, en chunks. */
typedef void (*log_sink_t)(const char* data, size_t len, void* user);
void log_dump(log_sink_t cb, void* user);

/* Stats. */
uint32_t log_used_bytes(void);
uint32_t log_total_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_STM32_LOG_H */
