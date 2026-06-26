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
#include "bpvm_pico.h"    /* H14: backend Pico.* (info real del P4 para los builtins BP) */
#include "esp_mac.h"      /* uniqueId desde efuse */
#include "esp_timer.h"    /* uptime */
#include "driver/temperature_sensor.h"  /* H14: sensor de temperatura interno */
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
static void p4_pico_board_name(char* buf, size_t len) {
    if (buf && len > 0) { strncpy(buf, "esp32p4", len - 1); buf[len - 1] = '\0'; }
}
/* Sensor de temperatura interno del P4 (periférico PROPIO, no ADC). Install +
 * enable PEREZOSO en la 1ª lectura; rango -10..80 °C. */
static temperature_sensor_handle_t s_p4_tsens = NULL;
static float p4_pico_temp_c(void) {
    if (s_p4_tsens == NULL) {
        temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
        if (temperature_sensor_install(&cfg, &s_p4_tsens) != ESP_OK) return 0.0f;
        temperature_sensor_enable(s_p4_tsens);
    }
    float t = 0.0f;
    if (temperature_sensor_get_celsius(s_p4_tsens, &t) != ESP_OK) return 0.0f;
    return t;
}
static int   p4_pico_cpu_freq_hz(void)     { return 360000000; }   /* 360 MHz por defecto */
static int   p4_pico_uptime_ms(void)       { return (int) (esp_timer_get_time() / 1000); }
static int   p4_pico_set_cpu_freq(int mhz) { (void) mhz; return 0; } /* runtime no soportado */
static int   p4_pico_gpio_count(void)      { return 55; }          /* GPIO0..54 */
static int   p4_pico_adc_channels(void)    { return 14; }          /* H14: 2x SAR 12-bit, hasta 14 pines */
static int   p4_pico_pwm_slices(void)      { return 14; }          /* H14: 8 LEDC + 6 MCPWM */

static const bpvm_pico_backend_t s_p4_pico_backend = {
    .uniqueId      = p4_pico_unique_id,
    .boardName     = p4_pico_board_name,
    .tempC         = p4_pico_temp_c,
    .cpuFreqHz     = p4_pico_cpu_freq_hz,
    .uptimeMs      = p4_pico_uptime_ms,
    .setCpuFreqMHz = p4_pico_set_cpu_freq,
    .gpioCount     = p4_pico_gpio_count,
    .adcChannels   = p4_pico_adc_channels,   /* H14 */
    .pwmSlices     = p4_pico_pwm_slices,      /* H14 */
};

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
    bpvm_pico_set_backend(&s_p4_pico_backend);   /* H14: Pico.* con info real del P4 */
}
