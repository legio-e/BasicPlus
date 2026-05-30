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

#include "driver/gpio.h"

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

void esp32_hw_register(void) {
    bpvm_gpio_set_backend(&s_esp32_gpio_backend);
    /* I2C/SPI/UART/PWM/ADC: añadir sus backends ESP-IDF aquí cuando toque. */
}
