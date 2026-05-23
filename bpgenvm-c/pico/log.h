/*
 * log.h — log persistente para diagnóstico post-mortem.
 *
 * Motivación: si el firmware se cuelga o el USB CDC deja de responder,
 * perdíamos toda la info de qué estaba pasando. Con esta capa:
 *
 *   - Cada mensaje importante va a un buffer en RAM (4 KB).
 *   - En momentos críticos (boot, assert, antes de RESET) se persiste a
 *     un sector de flash dedicado.
 *   - Al arrancar, log_init() carga el último snapshot — así podemos
 *     hacer LOG y ver qué pasó en la sesión anterior antes de morir.
 *
 * Layout flash:
 *    0x3FC000  ┌─ LOG sector (4 KB)
 *              │   header: magic + version + size
 *              │   data:   bytes UTF-8, line-based
 *    0x3FD000  ├─ FS region (12 KB)
 *
 * No es un sistema de logging completo (no levels, no timestamps a
 * subsegundo, no rotación). Solo "qué ha hecho el firmware hasta ahora,
 * en orden". Suficiente para post-mortem en bring-up.
 */
#ifndef BPVM_PICO_LOG_H
#define BPVM_PICO_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el log. Intenta cargar el snapshot de flash si hay magic
 * válido. Llamar UNA vez antes del primer log_printf. */
void log_init(void);

/* Añade una línea al log. Auto-añade timestamp (ms desde boot actual)
 * + newline. No bloquea, no toca flash. */
void log_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/* Persiste el buffer RAM a flash (erase + program del sector). Bloquea
 * ~50 ms con IRQs OFF. Llamar en momentos críticos. */
void log_flush(void);

/* Vacía el buffer RAM (no toca flash). Útil al iniciar una nueva sesión
 * de pruebas. log_clear_flash() también borra el sector. */
void log_clear_ram(void);
void log_clear_flash(void);

/* Vuelca el log actual (RAM) a un sink. Cada chunk se entrega a `cb`. */
typedef void (*log_sink_t)(const char* data, size_t len, void* user);
void log_dump(log_sink_t cb, void* user);

/* Stats. */
uint32_t log_used_bytes(void);
uint32_t log_total_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_LOG_H */
