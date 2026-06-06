/*
 * bp_rp2350b.h — board header GENÉRICO RP2350B para el firmware BasicPlus (H7.1).
 *
 * Es un clon de la pico2.h de la SDK con UN SOLO cambio funcional:
 *   PICO_RP2350A 1  ->  PICO_RP2350A 0
 * que hace que la SDK exponga el package B: NUM_BANK0_GPIOS = 48 (en vez de 30).
 *
 * Filosofía (decisión usuario 2026-06-06): UNA imagen lo más genérica posible.
 * Este header NO declara NADA board-specific (ni NeoPixel, ni CS de PSRAM, ni
 * nombre de placa concreta). Todo eso vive en DATOS: /sys/board.json, leído en
 * runtime por board_desc.c. Así el MISMO .uf2 vale para:
 *   - Pico 2 (RP2350A, 30 GPIO): los pines 30-47 no están bondeados; escribir en
 *     sus registros es un no-op inocuo (no panic) — el build B es un superset.
 *   - Metro / Pimoroni Plus2 / cualquier RP2350B (48 GPIO): pines altos reales.
 *
 * El resto de la config (flash 4MB + boot2 W25Q080, defaults UART/I2C/SPI, LED)
 * es IDÉNTICA a pico2 — combinación ya verificada arrancando en el Metro. 4MB es
 * el mínimo común seguro; placas con más flash simplemente no usan el resto.
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_BP_RP2350B_H
#define _BOARDS_BP_RP2350B_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

// For board detection
#define BP_RP2350B_GENERIC

// --- RP2350 VARIANT: B (package QFN-80, 48 GPIO) ---
#define PICO_RP2350A 0

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
// Default de la SDK (uso interno). El LED REAL de la placa lo declara
// /sys/board.json (board_desc.led_pin); en una placa cuyo LED es NeoPixel,
// board.json pone ledPin:-1 y se usa neopixelPin.
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif
// no PICO_DEFAULT_WS2812_PIN — board-specific, via board.json

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// --- FLASH (idéntico a pico2: 4MB + boot2 W25Q080, probado en el Metro) ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (4 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

// Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
#define PICO_SMPS_MODE_PIN 23

#ifndef PICO_VBUS_PIN
#define PICO_VBUS_PIN 24
#endif

#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN 29
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
