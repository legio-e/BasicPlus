/*
 * p4_board_id.c — identidad de placa del ESP32-P4 para INFO/HELLO del wire.
 *
 * Instala (vía repl_set_board_id) los datos del P4, pisando el default S3 que
 * trae repl_esp32.c (reutilizado tal cual). Solo afecta a lo informativo del
 * wire: el diálogo INFO del IDE + el serverName del HELLO; el resto del
 * dispatcher es idéntico. main lo llama una vez antes de arrancar el REPL — y
 * ESA referencia es la que fuerza a enlazar este .o (ver la nota en
 * repl_esp32.c sobre por qué weak/strong NO bastaba en ESP-IDF).
 *
 * Datos del ESP32-P4 (confirmados, datasheet): dual RISC-V HP hasta 400 MHz
 * (360 por defecto en IDF), 55 GPIOs (GPIO0..54), SIN PIO, 14 canales PWM
 * (8 LEDC + 6 MCPWM), 2x SAR-ADC de 12 bits (hasta 14 pines analógicos), 768 KB
 * de HP L2MEM. La EV-board
 * lleva ADEMÁS 32 MB de PSRAM y 16 MB de flash EXTERNAS, NO activadas en este
 * build -> INFO las mide en runtime (PSRAM=0, flash=CONFIG_ESPTOOLPY_FLASHSIZE);
 * se habilitan en sdkconfig en la fase de HW/gráficos (PSRAM = bring-up real,
 * como la RP2350B). Para los futuros backends HW del P4: I2C x3 (2 HP + 1 LP),
 * SPI x4. Como en el S3, los backends BP de Pwm/Adc aún no están cableados.
 */
#include "repl_esp32.h"
#include "p4_board_id.h"

void p4_install_board_id(void) {
    static const repl_board_id_t p4 = {
        "esp32p4",        /* board_name   (INFO.boardName)        */
        "bpvm-esp32p4",   /* server_name  (HELLO.serverName)      */
        360000000L,       /* cpu_freq_hz  (360 MHz por defecto; hasta 400) */
        55,               /* gpio_count   (GPIO0..54)             */
        0,                /* pio_count    (el P4 no tiene PIO)    */
        14,               /* pwm_slices   (8 LEDC + 6 MCPWM)       */
        14,               /* adc_channels (2x SAR 12-bit, hasta 14 pines) */
        768L * 1024L,     /* sram_bytes   (HP L2MEM)              */
    };
    repl_set_board_id(&p4);
}
