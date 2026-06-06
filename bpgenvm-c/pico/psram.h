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

/* H7.2.a — DETECTA la PSRAM en `cs_pin`: enruta cs_pin a XIP_CS1, sondea el chip
 * (reset + read-ID) y devuelve el tamaño en bytes (0 si el KGD no es el de
 * APS6404, o cs_pin < 0). SÓLO detección. Seguro: esperas acotadas (nunca
 * cuelga), restaura el XIP siempre y la función del pin si no detecta. */
size_t psram_detect_init(int cs_pin);

/* H7.2.b — HABILITA la PSRAM para usarla como memoria: reset + QPI + configura
 * la ventana M1 (read 0xEB / write 0x38) y la mapea ESCRIBIBLE en PSRAM_XIP_BASE.
 * Llamar tras psram_detect_init() con éxito (el pin ya está enrutado). Devuelve
 * 1 si OK, 0 si algún paso QMI no responde (timeout) — y restaura el XIP. */
int psram_enable_xip(void);

/* H7.2.b — AUTO-TEST: escribe/lee patrones repartidos por la PSRAM (mapeada en
 * PSRAM_XIP_BASE tras psram_enable_xip). Devuelve 1 si el camino de datos es
 * fiable, 0 si no. Gate antes de confiarle el heap de la VM. */
int psram_rw_selftest(size_t size);

#endif /* BP_PSRAM_H */
