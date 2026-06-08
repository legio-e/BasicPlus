/*
 * gpio_stm32.c — backends de HW del STM32U575 (H9.4).
 *
 * Implementa los hooks portables de la VM sobre la HAL de ST:
 *   - GPIO  (bpvm_gpio_backend_t):  init/pull/write/read sobre HAL_GPIO_*.
 *   - Pico  (bpvm_pico_backend_t):  info de placa (gpioCount, freq, uptime,
 *                                   uniqueId, boardName) para que el módulo
 *                                   stdlib `Pico` y la clase `Gpio.Pin`
 *                                   (que valida contra Pico.gpioCount())
 *                                   funcionen en esta familia.
 *
 * Modelo de pin: la fachada portable usa un `int pin` plano; aquí lo
 * decodificamos a (puerto, bit) de STM32 con el convenio:
 *
 *     pin = (puerto << 4) | bit       puerto: 0=A,1=B,2=C,...,7=H ; bit: 0..15
 *
 * Ejemplos en la Nucleo-U575ZI-Q:  PA5 = 5,  PB7 = 23,  PC7 = 39 (LED verde),
 * PG2 = 98 (LED rojo).  gpioCount() reporta 128 (8 puertos × 16) → la clase
 * Pin acepta cualquiera de ellos.
 *
 * Registro: stm32_hw_register() se llama una vez al boot (desde el REPL),
 * después de la init de la HAL/BSP.
 */
#include "gpio_stm32.h"

#include "bpvm_gpio.h"
#include "bpvm_pico.h"

#include "main.h"   /* HAL + CMSIS (GPIOA.., UID_BASE, SystemCoreClock) */

#include <stdio.h>
#include <string.h>

#define STM32_GPIO_PORTS  8           /* A..H */
#define STM32_PIN_COUNT   (STM32_GPIO_PORTS * 16)

/* Estado por pin para reconfigurar sin perder el modo cuando llega un pull()
 * tras el init() (la HAL fija modo+pull juntos en HAL_GPIO_Init). */
static uint8_t s_mode[STM32_PIN_COUNT];   /* 0=INPUT, 1=OUTPUT */
static uint8_t s_pull[STM32_PIN_COUNT];   /* 0=none, 1=up, 2=down */

static GPIO_TypeDef* port_of(int idx) {
    switch (idx) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        case 5: return GPIOF;
        case 6: return GPIOG;
        case 7: return GPIOH;
        default: return NULL;
    }
}

static void enable_port_clk(int idx) {
    switch (idx) {
        case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case 4: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case 5: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        case 6: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
        case 7: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
        default: break;
    }
}

/* Aplica modo+pull cacheados a la HAL. */
static void reconfigure(int pin) {
    int idx = pin >> 4;
    int bit = pin & 0x0F;
    GPIO_TypeDef* port = port_of(idx);
    if (!port) return;

    enable_port_clk(idx);

    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = (uint32_t) (1u << bit);
    cfg.Mode  = (s_mode[pin] == 1) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    cfg.Pull  = (s_pull[pin] == 1) ? GPIO_PULLUP
              : (s_pull[pin] == 2) ? GPIO_PULLDOWN
              : GPIO_NOPULL;
    HAL_GPIO_Init(port, &cfg);
}

/* ---- GPIO backend ---- */

static void stm32_gpio_init_impl(int pin, int mode) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_mode[pin] = (mode == 1) ? 1 : 0;
    s_pull[pin] = 0;   /* init resetea el pull */
    reconfigure(pin);
}

static void stm32_gpio_pull_impl(int pin, int pull_mode) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    s_pull[pin] = (pull_mode == 1) ? 1 : (pull_mode == 2) ? 2 : 0;
    reconfigure(pin);   /* preserva el modo cacheado */
}

static void stm32_gpio_write_impl(int pin, int value) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return;
    GPIO_TypeDef* port = port_of(pin >> 4);
    if (!port) return;
    HAL_GPIO_WritePin(port, (uint16_t) (1u << (pin & 0x0F)),
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static int stm32_gpio_read_impl(int pin) {
    if (pin < 0 || pin >= STM32_PIN_COUNT) return 0;
    GPIO_TypeDef* port = port_of(pin >> 4);
    if (!port) return 0;
    return (HAL_GPIO_ReadPin(port, (uint16_t) (1u << (pin & 0x0F))) == GPIO_PIN_SET) ? 1 : 0;
}

static const bpvm_gpio_backend_t s_gpio_backend = {
    .init  = stm32_gpio_init_impl,
    .pull  = stm32_gpio_pull_impl,
    .write = stm32_gpio_write_impl,
    .read  = stm32_gpio_read_impl,
};

/* ---- Pico (info de MCU) backend ---- */

static int stm32_gpio_count_impl(void) {
    return STM32_PIN_COUNT;   /* 128: puertos A..H del U575 */
}

static int stm32_cpu_freq_hz_impl(void) {
    return (int) SystemCoreClock;
}

static int stm32_uptime_ms_impl(void) {
    return (int) HAL_GetTick();
}

static void stm32_unique_id_impl(char* buf, size_t len) {
    /* UID de 96 bits del U575; reportamos los 64 bits bajos como 16 hex
     * (convenio del backend: 16 chars + null → len >= 17). */
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    if (buf && len > 0) {
        snprintf(buf, len, "%08lX%08lX", (unsigned long) u1, (unsigned long) u0);
    }
}

static void stm32_board_name_impl(char* buf, size_t len) {
    if (buf && len > 0) {
        strncpy(buf, "nucleo-u575zi", len - 1);
        buf[len - 1] = '\0';
    }
}

static float stm32_temp_c_impl(void) {
    return 0.0f;   /* sensor interno no cableado en el MVP */
}

static int stm32_set_cpu_freq_mhz_impl(int mhz) {
    (void) mhz;
    return 0;   /* reloj fijo (160 MHz); cambiar en runtime no soportado */
}

static const bpvm_pico_backend_t s_pico_backend = {
    .uniqueId      = stm32_unique_id_impl,
    .boardName     = stm32_board_name_impl,
    .tempC         = stm32_temp_c_impl,
    .cpuFreqHz     = stm32_cpu_freq_hz_impl,
    .uptimeMs      = stm32_uptime_ms_impl,
    .setCpuFreqMHz = stm32_set_cpu_freq_mhz_impl,
    .gpioCount     = stm32_gpio_count_impl,
};

void stm32_hw_register(void) {
    bpvm_gpio_set_backend(&s_gpio_backend);
    bpvm_pico_set_backend(&s_pico_backend);
    /* I2C/SPI/UART/PWM/ADC: añadir sus backends HAL aquí cuando toque. */
}
