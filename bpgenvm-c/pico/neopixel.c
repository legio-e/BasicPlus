/*
 * neopixel.c — H7.4: driver WS2812 (NeoPixel) vía PIO. Ver neopixel.h.
 *
 * Carga el programa `ws2812` (de la pico-sdk, ensamblado por pioasm en
 * ws2812.pio.h) en una state machine de pio0 y empuja palabras de 24 bits.
 * El WS2812 espera GRB MSB-first; la SM saca 24 bits desde el bit 31, así que
 * empujamos (grb << 8).
 */
#include "neopixel.h"

#include "hardware/pio.h"
#include "pico/time.h"
#include "ws2812.pio.h"   /* generado por pico_generate_pio_header */

#define NEOPIXEL_MAX_STRIPS 4
#define NEOPIXEL_HZ         800000.0f

static PIO s_pio = pio0;
static int s_offset = -1;   /* offset del programa ws2812 en s_pio (-1 = no cargado) */

static struct { int pin; int sm; } s_strips[NEOPIXEL_MAX_STRIPS];
static int s_nstrips = 0;

static int find_strip(int pin) {
    for (int i = 0; i < s_nstrips; i++)
        if (s_strips[i].pin == pin) return i;
    return -1;
}

bool neopixel_init(int pin) {
    if (pin < 0) return false;
    if (find_strip(pin) >= 0) return true;            /* ya inicializado */
    if (s_nstrips >= NEOPIXEL_MAX_STRIPS) return false;

    if (s_offset < 0) {
        if (!pio_can_add_program(s_pio, &ws2812_program)) return false;
        s_offset = (int) pio_add_program(s_pio, &ws2812_program);
    }
    int sm = pio_claim_unused_sm(s_pio, false);       /* false: no panic si no hay */
    if (sm < 0) return false;

    /* helper de la SDK (ws2812.pio): pin como side-set, 24-bit, FIFO TX, clkdiv. */
    ws2812_program_init(s_pio, (uint) sm, (uint) s_offset, (uint) pin, NEOPIXEL_HZ, false);

    s_strips[s_nstrips].pin = pin;
    s_strips[s_nstrips].sm  = sm;
    s_nstrips++;
    return true;
}

void neopixel_show(int pin, const uint32_t* grb, int count) {
    int idx = find_strip(pin);
    if (idx < 0 || grb == NULL || count <= 0) return;
    uint sm = (uint) s_strips[idx].sm;
    for (int i = 0; i < count; i++) {
        pio_sm_put_blocking(s_pio, sm, grb[i] << 8u);  /* 24 bits → MSB */
    }
    busy_wait_us(60);   /* latch/reset del WS2812 (>50us a nivel bajo) */
}
