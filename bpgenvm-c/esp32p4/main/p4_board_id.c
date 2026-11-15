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
#include "bpvm_machine.h"    /* H14: backend Pico.* (info real del P4 para los builtins BP) */
#include "hw_esp32.h"     /* paso 4 cierre: esp32_reset_cause (compartido con el S3) */
#include "esp_mac.h"      /* uniqueId desde efuse */
#include "esp_timer.h"    /* uptime */
#include <stdio.h>
#include <string.h>

/* H14 — backend Pico.* (info de MCU para los builtins Pico.* desde BP). El
 * gpio_esp32.c registra el del S3 (esp32s3/240 MHz/45 GPIO) en
 * esp32_hw_register(); aquí lo PISAMOS con los datos reales del P4, igual que
 * hacemos con el board_id del wire. */
static void p4_pico_unique_id(char* buf, size_t len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    if (buf && len > 0)
        snprintf(buf, len, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
/* Los DOS nombres (V6, 9-sep): el micro es real, la placa dice `generic` porque
 * esta imagen sirve a las DOS P4 —el kit y la Waveshare— y no sabe en cual esta
 * (el panel si, y eso sale del ENV; el modelo de placa no). */
static void p4_pico_micro_name(char* buf, size_t len) {
    if (buf && len > 0) { strncpy(buf, "esp32p4", len - 1); buf[len - 1] = '\0'; }
}
static void p4_pico_board_name(char* buf, size_t len) {
    if (buf && len > 0) { strncpy(buf, "generic", len - 1); buf[len - 1] = '\0'; }
}
/* Sensor de temperatura interno del P4 (periférico PROPIO, no ADC): V8 #826 (15-nov), el de la familia
 * (gpio_esp32.c, esp32_temp_c), que se enciende una sola vez aunque lo pidan dos hilos a la vez; antes, uno suyo aquí,
 * con el rango de -10 a 80 °C. */
static int   p4_pico_uptime_ms(void)       { return (int) (esp_timer_get_time() / 1000); }

static const bpvm_machine_backend_t s_p4_pico_backend = {
    .uniqueId      = p4_pico_unique_id,
    .microName     = p4_pico_micro_name,
    .boardName     = p4_pico_board_name,
    .tempC         = esp32_temp_c,            /* V8 #826: el de la familia (hw_esp32.h) */
    .cpuFreqHz     = esp32_cpu_freq_hz,       /* V8 #784: la de verdad (hw_esp32.h) */
    .uptimeMs      = p4_pico_uptime_ms,
    .setCpuFreqMHz = esp32_set_cpu_freq_mhz,   /* V8 #784: el silicio de la P4 sólo da 360 */
    .resetCause    = esp32_reset_cause,       /* paso 4 cierre — compartido con el S3 */
};

void p4_install_board_id(void) {
    static const repl_board_id_t p4 = {
        "esp32p4",        /* board_name   (INFO.boardName)        */
        "bpvm-esp32p4",   /* server_name  (HELLO.serverName)      */
        55,               /* gpio_count   (GPIO0..54)             */
        0,                /* pio_count    (el P4 no tiene PIO)    */
        14,               /* pwm_slices   (8 LEDC + 6 MCPWM)       */
        14,               /* adc_channels (2x SAR 12-bit, hasta 14 pines) */
        768L * 1024L,     /* sram_bytes   (HP L2MEM)              */
    };
    repl_set_board_id(&p4);
    bpvm_machine_set_backend(&s_p4_pico_backend);   /* H14: Pico.* con info real del P4 */
}
