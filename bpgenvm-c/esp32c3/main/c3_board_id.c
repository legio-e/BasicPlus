/*
 * c3_board_id.c — identidad de placa del ESP32-C3 para INFO/HELLO del wire.
 *
 * ─── Por qué existe (V6/P1.C3.3, 31-ago-2026) ───────────────────────────────
 *
 * Sin esto, el ensayo del C3 usaba el `s_default_board` de `repl_esp32.c`, que es
 * el del S3: el HELLO decía `serverName: "bpvm-esp32"` y el INFO anunciaba los
 * GPIOs, el ADC y la SRAM de OTRO silicio. Se vio en la primera respuesta que dio
 * la placa por el cable nuevo, y es **exactamente el bug que `U3.19` cazó en el
 * P4** — la misma causa: una familia nueva hereda la identidad de la anterior y
 * nadie lo nota, porque el wire funciona igual y lo que miente es lo que dice.
 *
 * Sólo afecta a lo informativo (el diálogo INFO del IDE y el saludo); el resto
 * del dispatcher es idéntico. `main` lo llama una vez antes de arrancar el REPL —
 * y ESA referencia es la que fuerza a enlazar este `.o` (ver la nota de
 * `repl_esp32.c` sobre por qué weak/strong NO bastaba en ESP-IDF).
 *
 * ─── Los datos, del silicio ─────────────────────────────────────────────────
 *
 * ESP32-C3: **un solo núcleo** RISC-V (RV32IMC) a 160 MHz, **22 GPIOs**
 * (GPIO0..21), **sin PIO**, **6 canales LEDC** de PWM, **6 canales de ADC**
 * (5 en ADC1 sobre GPIO0..4 + 1 en ADC2 sobre GPIO5), **400 KB de SRAM** y **sin
 * PSRAM**. La flash es embebida y su tamaño lo mide el INFO en runtime, no se
 * pone aquí.
 *
 * ⚠️ El `sram_bytes` es el del CHIP (400 KB), no el que queda libre: parte se la
 * lleva el ROM/IDF antes de que arranque nada. Lo que de verdad hay para la VM
 * lo dice la línea `vm: heap ... DRAM interna libre` del arranque, y en esta
 * placa el techo real es el **bloque contiguo** (136 KB), no el total.
 */
#include "repl_esp32.h"
#include "c3_board_id.h"

static const repl_board_id_t C3_ID = {
    .board_name   = "ESP32-C3",
    .server_name  = "bpvm-esp32c3",
    .cpu_freq_hz  = 160000000L,
    .gpio_count   = 22,          /* GPIO0..21 */
    .pio_count    = 0,           /* no hay PIO: eso es del RP2350 */
    .pwm_slices   = 6,           /* LEDC: 6 canales */
    .adc_channels = 6,           /* ADC1 x5 (GPIO0..4) + ADC2 x1 (GPIO5) */
    .sram_bytes   = 400 * 1024L,
};

void c3_install_board_id(void) { repl_set_board_id(&C3_ID); }
