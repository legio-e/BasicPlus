/*
 * gpio_stm32.h — registro de los backends de HW del STM32 (GPIO + Pico).
 */
#ifndef GPIO_STM32_H
#define GPIO_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

/* Registra los backends de hardware (GPIO + info de MCU) en la VM. Llamar
 * una vez al boot, después de inicializar la HAL/BSP. */
void stm32_hw_register(void);

/* H10 — causa del último reset (decodifica RCC->CSR). Se latchea en la 1ª
 * llamada (que limpia los flags), así sirve en el boot y luego desde BP. */
const char* stm32_reset_cause(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_STM32_H */
