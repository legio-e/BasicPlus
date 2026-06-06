/*
 * neopixel.h — H7.4: driver WS2812 (NeoPixel) vía PIO en RP2350.
 *
 * Usa el programa PIO `ws2812.pio` que YA trae la pico-sdk (BSD-3, en
 * pico_status_led) — el `pioasm` lo ensambla en build (pico_generate_pio_header)
 * y aquí sólo lo cargamos en una state machine y empujamos palabras de color.
 * Es la primera infraestructura PIO del firmware; el patrón (suelta un .pio,
 * genera header, driver C fino) vale para futuros protocolos de timing.
 */
#ifndef BP_NEOPIXEL_H
#define BP_NEOPIXEL_H

#include <stdint.h>
#include <stdbool.h>

/* Reclama PIO + state machine para `pin` y carga el programa WS2812 (800 kHz,
 * 24-bit RGB). Idempotente por pin; soporta hasta 4 tiras (4 SM de pio0).
 * Devuelve true si OK (o ya estaba), false si no quedan SM/programa. */
bool neopixel_init(int pin);

/* Empuja `count` palabras de color a la tira de `pin`. Cada palabra lleva el
 * color en los 24 bits bajos como GRB: (g<<16)|(r<<8)|b. Hace el latch (~60us)
 * al final. No-op si el pin no se inicializó. */
void neopixel_show(int pin, const uint32_t* grb, int count);

#endif /* BP_NEOPIXEL_H */
