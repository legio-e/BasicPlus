/*
 * embedded_mods.h — declaraciones de los .mod incrustados en flash al
 * compilar. Cada .mod tiene su array generado en su propio .c (vía
 * `xxd -i -n <nombre>`). Aquí solo declaramos los símbolos externos.
 */
#ifndef BPVM_EMBEDDED_MODS_H
#define BPVM_EMBEDDED_MODS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t       hello_mod[];
extern const unsigned int  hello_mod_len;

/* #415 — Math e IO. La Metro se habia quedado sin ellos mientras el ESP32 y el
 * STM32 si los llevaban, asi que el mismo `import Math` funcionaba en una placa
 * y fallaba en otra hasta subir el modulo a mano — con la portabilidad como
 * promesa del lenguaje, eso es un agujero. La stdlib BASE tiene que ser la
 * MISMA en las tres familias (Eduardo, 18-ago). */
extern const uint8_t       math_mod[];
extern const unsigned int  math_mod_len;
extern const uint8_t       io_mod[];
extern const unsigned int  io_mod_len;

/* Stdlib pre-instalada (Gpio, Math, IO en su día) — se carga al FS al
 * boot si no está ya persistida. Permite que `import Gpio` en un .bp
 * compilado e instalado a la Pico resuelva sin uploads manuales. */
/* #248 — Core: Exception/RuntimeError únicas (base de todas las excepciones).
 * Va PRIMERO en el install: todo módulo con try/throw depende de él. */
extern const uint8_t       core_mod[];
extern const unsigned int  core_mod_len;
extern const uint8_t       gpio_mod[];
extern const unsigned int  gpio_mod_len;
extern const uint8_t       i2c_mod[];
extern const unsigned int  i2c_mod_len;
extern const uint8_t       spi_mod[];
extern const unsigned int  spi_mod_len;
extern const uint8_t       uart_mod[];
extern const unsigned int  uart_mod_len;
extern const uint8_t       pulse_mod[];
extern const unsigned int  pulse_mod_len;
extern const uint8_t       pwm_mod[];
extern const unsigned int  pwm_mod_len;
extern const uint8_t       pico_mod[];
extern const unsigned int  pico_mod_len;
extern const uint8_t       rtc_mod[];
extern const unsigned int  rtc_mod_len;
extern const uint8_t       adc_mod[];
extern const unsigned int  adc_mod_len;
extern const uint8_t       wdt_mod[];
extern const unsigned int  wdt_mod_len;
extern const uint8_t       timer_mod[];
extern const unsigned int  timer_mod_len;
extern const uint8_t       neopixel_mod[];
extern const unsigned int  neopixel_mod_len;

#ifdef __cplusplus
}
#endif

#endif /* BPVM_EMBEDDED_MODS_H */
