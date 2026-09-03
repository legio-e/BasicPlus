/*
 * c6_board_id.c — identidad de placa del ESP32-C6 para INFO/HELLO del wire.
 *
 * El mismo porqué que `c3_board_id.c`: sin esto el ensayo hereda el
 * `s_default_board` del S3 y el HELLO dice `bpvm-esp32` con los GPIOs, el ADC y
 * la SRAM de OTRO silicio (el bug que `U3.19` cazó en el P4 y `P1.C3.3` en el C3).
 * `main` lo llama una vez antes del REPL, y esa referencia es la que enlaza este
 * `.o` (weak/strong no bastaba en ESP-IDF).
 *
 * ─── Los datos, del silicio (datasheet ESP32-C6; por confirmar en placa) ─────
 *
 * ESP32-C6: un núcleo RISC-V (RV32IMAC) a 160 MHz + un núcleo LP a 20 MHz,
 * **31 GPIOs** (GPIO0..30; el QFN40 expone 30), **sin PIO**, **6 canales LEDC**
 * de PWM, **7 canales de ADC** (ADC1 sobre GPIO0..6), **512 KB de SRAM HP**
 * (+16 KB LP) y **sin PSRAM**. La flash es embebida (4 MB en este devkit, lo dijo
 * esptool) y su tamaño lo mide el INFO en runtime, no se pone aquí.
 *
 * ⚠️ El `sram_bytes` es el del CHIP, no el que queda libre: lo que hay para la VM
 * lo dice la línea `vm: … DRAM interna libre` del arranque (P1.C6.3 lo mide).
 */
#include "repl_esp32.h"
#include "c6_board_id.h"

static const repl_board_id_t C6_ID = {
    .board_name   = "ESP32-C6",
    .server_name  = "bpvm-esp32c6",
    .cpu_freq_hz  = 160000000L,
    .gpio_count   = 31,          /* GPIO0..30 */
    .pio_count    = 0,           /* no hay PIO: eso es del RP2350 */
    .pwm_slices   = 6,           /* LEDC: 6 canales */
    .adc_channels = 7,           /* ADC1 x7 (GPIO0..6) */
    .sram_bytes   = 512 * 1024L,
};

void c6_install_board_id(void) { repl_set_board_id(&C6_ID); }
