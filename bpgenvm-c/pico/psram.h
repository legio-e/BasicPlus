/*
 * psram.h — H7.2.a: detección + init de PSRAM externa (APS6404 sobre QMI CS1)
 * en RP2350. Portado de la referencia BSD-3 de SparkFun (sparkfun-pico,
 * sfe_psram.c) — secuencia QMI/APS6404 verificada por ellos en placas RP2350B.
 *
 * La PSRAM cuelga del bus QSPI con un Chip Select aparte (board-specific; GP47
 * en Metro/Pimoroni). La pico-sdk NO trae driver, así que el init QMI se hace a
 * mano y DEBE correr desde RAM con IRQs off (el direct-mode suspende el XIP).
 */
#ifndef BP_PSRAM_H
#define BP_PSRAM_H

#include <stddef.h>

/* Base de la ventana XIP del CS1 (PSRAM) en RP2350: 0x10000000 (CS0/flash) +
 * 0x01000000. Tras psram_detect_init() OK, la PSRAM es accesible y ESCRIBIBLE
 * aquí (mapeada en QPI). La usará H7.2.b para los buffers grandes. */
#define PSRAM_XIP_BASE 0x11000000u

/* Detecta e inicializa la PSRAM en `cs_pin`:
 *   - Enruta cs_pin a la función XIP_CS1.
 *   - Sondea el chip (reset + read-ID); si el KGD no es el de APS6404 → no hay.
 *   - Si hay, lo pasa a QPI y configura la ventana M1 (read 0xEB / write 0x38)
 *     + XIP escribible.
 * Devuelve el tamaño en bytes (0 si no hay PSRAM o cs_pin < 0). Idempotente y
 * seguro: si algo falla, restaura el XIP y devuelve 0 (el device arranca igual). */
size_t psram_detect_init(int cs_pin);

#endif /* BP_PSRAM_H */
