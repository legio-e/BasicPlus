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
 * llamada (que limpia los flags), así sirve en el boot y luego desde BP.
 * V8 (la tabla de la robustez): tras un fallo fatal, «fallo fatal», como la Pico. */
const char* stm32_reset_cause(void);

/* V8 (18-nov, la tabla de la robustez, V8_IDEAS §E15) — UN FALLO FATAL: EL INFORME Y REINICIAR (platform_stm32.c).
 *   stm32_fatal: lo primero de cada camino fatal (el HardFault, el assert, la pila, el heap del RTOS): arma el IWDG
 *     (~2 s) y deja la marca en la RAM que sobrevive; después, su informe, y el perro reinicia la placa.
 *   stm32_fatal_mirar_arranque: UNA vez al arrancar, tras log_init: si la marca está, lo dice en el log.
 *   stm32_arranque_tras_fatal: 1 si el arranque anterior acabó en un fallo fatal (sin autorun). */
void stm32_fatal(void);
void stm32_fatal_mirar_arranque(void);
int  stm32_arranque_tras_fatal(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_STM32_H */
