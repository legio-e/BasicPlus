/*
 * gpio_esp32.c — backend GPIO del HAL para ESP32-S3 (H4.5).
 *
 * Implementa bpvm_gpio_backend_t (la VM core llama bpvm_gpio_init/write/
 * read/pull → estas funciones tocan el HW vía driver/gpio.h de ESP-IDF).
 * Hermano de la impl Pico (pico/main.c s_pico_gpio_backend).
 *
 * Convención BP (igual que en Pico):
 *   - mode: 0 = INPUT, 1 = OUTPUT
 *   - value: 0 = LOW, !=0 = HIGH
 *   - pull: 0 = none, 1 = up, 2 = down
 */
#include "hw_esp32.h"
#include "bpvm_gpio.h"
#include "bpvm_pico.h"

#include "driver/gpio.h"
#include "esp_mac.h"      /* uniqueId desde la MAC de efuse */
#include "esp_timer.h"    /* uptime */

#include <stdio.h>
#include <string.h>

static void esp32_gpio_init_impl(int pin, int mode) {
    gpio_num_t g = (gpio_num_t) pin;
    gpio_reset_pin(g);
    gpio_set_direction(g, (mode == 1) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
}

static void esp32_gpio_pull_impl(int pin, int pull_mode) {
    gpio_num_t g = (gpio_num_t) pin;
    switch (pull_mode) {
        case 1:  gpio_set_pull_mode(g, GPIO_PULLUP_ONLY);   break;
        case 2:  gpio_set_pull_mode(g, GPIO_PULLDOWN_ONLY); break;
        default: gpio_set_pull_mode(g, GPIO_FLOATING);      break;
    }
}

static void esp32_gpio_write_impl(int pin, int value) {
    gpio_set_level((gpio_num_t) pin, value != 0);
}

static int esp32_gpio_read_impl(int pin) {
    return gpio_get_level((gpio_num_t) pin) ? 1 : 0;
}

static const bpvm_gpio_backend_t s_esp32_gpio_backend = {
    .init  = esp32_gpio_init_impl,
    .pull  = esp32_gpio_pull_impl,
    .write = esp32_gpio_write_impl,
    .read  = esp32_gpio_read_impl,
};

/* ---- Pico (info de MCU) backend ----
 * Igual que el STM32 (s_pico_backend en gpio_stm32.c): da datos reales del
 * ESP32-S3 a los builtins Pico.* desde BP (boardName/uniqueId/cpuFreqHz/
 * gpioCount...). Sin esto, Pico.gpioCount() caia al stub host (30 GPIO,
 * board "host") aunque el INFO del wire ya diera los valores reales.
 * ESP32-S3 (DevKitC, WROOM-1): 45 GPIO utiles (0-21, 26-48), 240 MHz. */
static void esp32_unique_id_impl(char* buf, size_t len) {
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);   /* MAC base de fabrica (efuse) */
    if (buf && len > 0) {
        snprintf(buf, len, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

static void esp32_board_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        strncpy(buf, "esp32s3-devkitc", len - 1);
        buf[len - 1] = '\0';
    }
}

static float esp32_temp_c_impl(void) {
    return 0.0f;   /* sensor interno no cableado en el MVP (igual que STM32) */
}

static int esp32_cpu_freq_hz_impl(void) {
    return 240000000;   /* 240 MHz nominal del ESP32-S3 (sdkconfig.defaults) */
}

static int esp32_uptime_ms_impl(void) {
    return (int) (esp_timer_get_time() / 1000);   /* us -> ms */
}

static int esp32_set_cpu_freq_mhz_impl(int mhz) {
    (void) mhz;
    return 0;   /* cambiar el reloj en runtime no soportado */
}

static int esp32_gpio_count_impl(void) {
    return 45;   /* GPIO utiles del ESP32-S3 (0-21, 26-48) */
}

static const bpvm_pico_backend_t s_esp32_pico_backend = {
    .uniqueId      = esp32_unique_id_impl,
    .boardName     = esp32_board_name_impl,
    .tempC         = esp32_temp_c_impl,
    .cpuFreqHz     = esp32_cpu_freq_hz_impl,
    .uptimeMs      = esp32_uptime_ms_impl,
    .setCpuFreqMHz = esp32_set_cpu_freq_mhz_impl,
    .gpioCount     = esp32_gpio_count_impl,
};

void esp32_hw_register(void) {
    bpvm_gpio_set_backend(&s_esp32_gpio_backend);
    bpvm_pico_set_backend(&s_esp32_pico_backend);   /* info real (no stub host) */
    /* I2C/SPI/UART/PWM/ADC: añadir sus backends ESP-IDF aquí cuando toque. */
}
