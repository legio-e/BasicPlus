/*
 * hw_esp32.h — registro de los backends de HW del ESP32-S3 (H4.5+).
 *
 * La VM core llama a las fachadas bpvm_gpio_ / bpvm_i2c_ / ... que
 * delegan en un backend de punteros a función registrable. Aquí
 * registramos las implementaciones ESP-IDF al boot. Por ahora: GPIO.
 * (I2C, SPI, UART... se irán añadiendo igual.)
 */
#ifndef BPVM_ESP32_HW_H
#define BPVM_ESP32_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Registra todos los backends de HW disponibles para ESP32-S3.
 * Llamar una vez al arranque (tras bpvm/fs init, antes del REPL). */
void esp32_hw_register(void);

/* paso 4 cierre — causa del último reset (esp_reset_reason → string). La
 * comparten el backend del S3 (gpio_esp32.c) y el del P4 (p4_board_id.c). */
const char* esp32_reset_cause(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ESP32_HW_H */
