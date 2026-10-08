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

static struct { int pin; int sm; int placa; } s_strips[NEOPIXEL_MAX_STRIPS];   /* V7 #667: placa = de board.json */
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
    s_strips[s_nstrips].placa = 0;
    s_nstrips++;
    return true;
}

bool neopixel_init_placa(int pin) {
    if (!neopixel_init(pin)) return false;
    s_strips[find_strip(pin)].placa = 1;
    return true;
}

bool neopixel_es_de_placa(int pin) {
    int idx = find_strip(pin);
    return idx >= 0 && s_strips[idx].placa;
}

/* V7 #667 — el pin de la tira de la placa, de vuelta a la PIO (un programa lo tomó como Gpio.Pin, PWM…). */
void neopixel_reconectar(int pin) {
    if (neopixel_es_de_placa(pin)) pio_gpio_init(s_pio, (uint) pin);
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

/* V7 #667 — SOLTARLA. show() vuelve con hasta 8 palabras aún en la FIFO (unida: TX de 8), ~30 us cada una, y la
 * Strip acaba de mandar su trama de apagado: se espera a que salga entera —la FIFO vacía, la última palabra y el
 * latch— antes de parar la máquina; si no, la trama se corta y algún LED se queda encendido. Después la máquina
 * vuelve a la PIO; el programa ws2812 se queda cargado (no lo usa nadie más en esta imagen). */
bool neopixel_release(int pin) {
    int idx = find_strip(pin);
    if (idx < 0 || s_strips[idx].placa) return false;
    uint sm = (uint) s_strips[idx].sm;
    absolute_time_t tope = make_timeout_time_ms(20);   /* 8 palabras son ~240 us: 20 ms es de sobra */
    while (!pio_sm_is_tx_fifo_empty(s_pio, sm) && !time_reached(tope)) tight_loop_contents();
    busy_wait_us(100);   /* la última palabra (~30 us) y el latch (>50 us) */
    pio_sm_set_enabled(s_pio, sm, false);
    pio_sm_unclaim(s_pio, sm);
    s_strips[idx] = s_strips[--s_nstrips];   /* se buscan por pin: el orden no importa */
    return true;
}

int neopixel_fin_de_run(void) {
    int n = 0;
    for (int i = s_nstrips - 1; i >= 0; i--) {
        int pin = s_strips[i].pin;
        if (!s_strips[i].placa && neopixel_release(pin)) n++;
    }
    return n;
}
