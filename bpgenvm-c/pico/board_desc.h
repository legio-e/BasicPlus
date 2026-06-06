/*
 * board_desc.h — H7.3: descriptor de placa para el firmware RP2350.
 *
 * Separa dos capas (decisión usuario 2026-06-06):
 *   - CHIP (variante A/B): caps que dependen del microcontrolador (gpioCount,
 *     nº de PIO/PWM/ADC). Vienen de una TABLA built-in por variante. El core
 *     del firmware NO conoce "Metro"/"Pico", sólo variantes RP2350A/B.
 *   - PLACA: lo board-specific (pines del LED, NeoPixel, CS de la PSRAM…).
 *     Vive en DATOS: /sys/board.json. Una placa distinta = otro board.json,
 *     sin tocar el firmware.
 *
 * Así el MISMO binario vale para Pico 2 (RP2350A) y Metro (RP2350B): el chip
 * es el mismo die; sólo cambian caps (por variante) y pines (por board.json).
 */
#ifndef BOARD_DESC_H
#define BOARD_DESC_H

typedef struct {
    char name[24];        /* nombre legible de la placa */
    char variant;         /* 'A' (RP2350A, 30 GPIO) | 'B' (RP2350B, 48 GPIO) */

    /* --- Caps del CHIP (de la tabla por variante) --- */
    int  gpio_count;      /* GPIO bondeados: 30 (A) / 48 (B) */
    int  pio_count;       /* instancias PIO */
    int  pwm_slices;      /* slices PWM */
    int  adc_channels;    /* canales ADC accesibles */

    /* --- Pines de la PLACA (de /sys/board.json; -1 si no aplica) --- */
    int  led_pin;         /* LED onboard */
    int  neopixel_pin;    /* NeoPixel WS2812 (peculiar de cada placa) */
    int  psram_cs_pin;    /* Chip Select de la PSRAM en el bus QSPI */

    /* --- Rellenado por H7.2 (sondeo PSRAM); 0 por ahora --- */
    int      psram_present;
    unsigned psram_bytes;

    /* --- Detectado al boot (JEDEC ID de la flash; 0 si falla) --- */
    unsigned flash_bytes;   /* 4 MB Pico 2 / 16 MB Metro, etc. */
} board_desc_t;

/* Inicializa el descriptor: defaults por variante + override de
 * /sys/board.json (si existe). Llamar UNA vez tras fs_init() en boot. */
void board_desc_init(void);

/* El descriptor activo (válido tras board_desc_init). */
const board_desc_t* board_desc(void);

#endif /* BOARD_DESC_H */
