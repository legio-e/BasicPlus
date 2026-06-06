/*
 * board_desc.c — H7.3: implementación del descriptor de placa.
 *
 * Defaults por variante (tabla del CHIP) + override desde /sys/board.json
 * (datos de la PLACA). Ver board_desc.h para el porqué de la separación.
 */
#include "board_desc.h"
#include "fs.h"
#include "json_min.h"
#include "log.h"

#include <stdint.h>
#include <string.h>

static board_desc_t s_board;

/*
 * Tabla de caps por variante del CHIP (NO de la placa). Es lo único que
 * el firmware "sabe" sobre el RP2350A/B. Todo lo demás (qué pin es el LED,
 * el NeoPixel, etc.) viene de board.json.
 *
 * psram_cs_pin se inicializa aquí como DEFAULT por variante (Adafruit usa
 * GPIO0 en RP2350A y el último pin —GPIO47— en RP2350B); board.json lo
 * sobreescribe si la placa lo cablea distinto. (Pines a confirmar con el
 * esquemático del Metro.)
 */
static void apply_variant_caps(board_desc_t* d, char variant) {
    if (variant == 'B') {
        d->variant      = 'B';
        d->gpio_count   = 48;   /* RP2350B (QFN-80), Metro */
        d->pio_count    = 3;
        d->pwm_slices   = 12;
        d->adc_channels = 8;
        d->psram_cs_pin = 47;   /* default Adafruit RP2350B (a confirmar) */
    } else {                    /* 'A' por defecto */
        d->variant      = 'A';
        d->gpio_count   = 30;   /* RP2350A (QFN-60), Pico 2 */
        d->pio_count    = 3;
        d->pwm_slices   = 12;
        d->adc_channels = 4;
        d->psram_cs_pin = 0;    /* default Adafruit RP2350A (a confirmar) */
    }
}

void board_desc_init(void) {
    board_desc_t* d = &s_board;
    memset(d, 0, sizeof *d);

    /*
     * Default sin board.json: variante 'B' (superset permisivo). El binario
     * se compila para RP2350B; en una placa A los GPIO 30-47 simplemente no
     * existen (escrituras inocuas). H7.2 afinará este default por sondeo de
     * PSRAM (presente → perfil B, ausente → perfil A).
     */
    strncpy(d->name, "rp2350-generic", sizeof d->name - 1);
    apply_variant_caps(d, 'B');
    d->led_pin       = -1;   /* lo declara la placa */
    d->neopixel_pin  = -1;   /* peculiar de cada placa */
    /* psram_cs_pin lo deja apply_variant_caps (default por variante). */
    d->psram_present = 0;    /* lo rellena H7.2 */
    d->psram_bytes   = 0;

    /* --- Override desde /sys/board.json (datos de la PLACA) --- */
    const uint8_t* data = NULL;
    uint32_t       size = 0;
    if (fs_get("/sys/board.json", &data, &size) == FS_OK && data && size > 0) {
        json_obj_t obj;
        if (json_parse((const char*) data, (size_t) size, &obj) == 0) {
            /* variant primero: re-aplica la tabla de caps de la variante. */
            char vbuf[4] = {0};
            if (json_get_str(&obj, "variant", vbuf, sizeof vbuf) >= 0 && vbuf[0]) {
                apply_variant_caps(d, (vbuf[0] == 'b' || vbuf[0] == 'B') ? 'B' : 'A');
            }
            json_get_str(&obj, "name", d->name, sizeof d->name);
            d->led_pin      = (int) json_get_long(&obj, "ledPin",     d->led_pin);
            d->neopixel_pin = (int) json_get_long(&obj, "neopixelPin", d->neopixel_pin);
            d->psram_cs_pin = (int) json_get_long(&obj, "psramCsPin", d->psram_cs_pin);
            /* gpioCount: override explícito de la tabla (placas atípicas). */
            d->gpio_count   = (int) json_get_long(&obj, "gpioCount",  d->gpio_count);
            log_printf("board: /sys/board.json aplicado");
        } else {
            log_printf("board: /sys/board.json invalido, uso defaults");
        }
    } else {
        log_printf("board: sin /sys/board.json, uso defaults por variante");
    }

    log_printf("board: %s variant=%c gpio=%d pio=%d pwm=%d adc=%d led=%d npx=%d psramCs=%d",
               d->name, d->variant, d->gpio_count, d->pio_count, d->pwm_slices,
               d->adc_channels, d->led_pin, d->neopixel_pin, d->psram_cs_pin);
}

const board_desc_t* board_desc(void) { return &s_board; }
