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

/* V7 #667 — la tira de la PLACA (la de la Metro, de board.json): como neopixel_init, y queda marcada: un programa la
 * usa pero no la suelta (neopixel_release no la toca) y, si un programa se lleva su pin, neopixel_reconectar se lo
 * devuelve. */
bool neopixel_init_placa(int pin);
bool neopixel_es_de_placa(int pin);
void neopixel_reconectar(int pin);

/* V7 #667 — SUELTA la tira de `pin` (el destructor de Neopixel.Strip, que antes la apaga): espera a que salga lo que
 * quedaba en la FIFO, para la máquina y la devuelve a la PIO. El pin lo deja como esté (lo devuelve al reset quien
 * llama). true si soltó algo; la de la placa, no. Y la red del final del RUN: todas las de los programas. */
bool neopixel_release(int pin);
int  neopixel_fin_de_run(void);

#endif /* BP_NEOPIXEL_H */
