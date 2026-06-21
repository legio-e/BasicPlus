/*
 * board.h — abstracción de placa del firmware STM32 de BasicPlus (V3 / H5.0).
 *
 * Aísla lo único que cambia entre placas STM32 —el UART del VCP/wire, los LEDs
 * de diagnóstico, la identidad e INFO— para que el resto del port
 * (stm32_wire.c, stm32_repl.c) sea idéntico en todas. Cada proyecto CubeIDE
 * elige su placa con un símbolo de preprocesador:
 *
 *   - STM32U5G9J-DK2   → definir  BPVM_BOARD_DK2
 *   - Nucleo-U575ZI-Q  → (sin símbolo: es el default, intacto respecto a V2)
 *
 * LEDs (semántica portable, NO por color): ERR = error/fatal · RUN = programa
 * BP en ejecución · BEAT = heartbeat del idle.
 */
#ifndef BPVM_BOARD_H
#define BPVM_BOARD_H

#include "main.h"   /* HAL + handles + (DK2) etiquetas GPIO generadas en main.h */

#if defined(BPVM_BOARD_DK2)
/* ===================== STM32U5G9J-DK2 ===================== */

extern UART_HandleTypeDef huart1;          /* VCP del ST-LINK = USART1 (PA9/PA10) */
#define BOARD_WIRE_UART         (&huart1)
#define BOARD_WIRE_IRQn         USART1_IRQn  /* IRQ de RX → ring (V3/H5.2) */
#define BOARD_NAME              "u5g9j-dk2"
#define BOARD_SRAM_BYTES        (3008UL * 1024UL)   /* SRAM interna contigua (linker) */
#define BOARD_FS_FLASH_ADDR     0x083E0000u          /* últimos 128 KB de 4 MB (reservados en el .ld) */

/* LEDs de usuario (etiquetas del .ioc): RED_LED=PD2, GREEN_LED=PD4. Solo dos →
 * verde = vivo/ejecutando + heartbeat, rojo = error. Activo-alto (SET=encendido);
 * si en placa salieran invertidos, basta cambiar SET<->RESET aquí. */
#define BOARD_LED_ERR_ON()      HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   GPIO_PIN_SET)
#define BOARD_LED_RUN_ON()      HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET)
#define BOARD_LED_RUN_OFF()     HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET)
#define BOARD_LED_BEAT_TOGGLE() HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin)

#else
/* ===================== Nucleo-U575ZI-Q (default) ===================== */

#include "stm32u5xx_nucleo.h"              /* hcom_uart[], COM1, BSP_LED_* */
#define BOARD_WIRE_UART         (&hcom_uart[COM1])
/* H10 — RX por IRQ+ring también aquí (antes DK2-only). COM1 = USART1 (ver
 * stm32u5xx_nucleo.h: COM1_UART==USART1). OJO: en la Nucleo USART1 lo gestiona el
 * BSP (BSP_COM_Init), NO está en el .ioc → no hay handler ni prioridad de CubeMX.
 * Por eso definimos BOARD_WIRE_IRQ_PRIO: stm32_wire_rx_irq_enable() fija la
 * prioridad NVIC y habilita la IRQ él mismo. El USART1_IRQHandler (que llama a
 * stm32_wire_rx_drain) se añade a mano en Core/Src/stm32u5xx_it.c. */
#define BOARD_WIRE_IRQn         USART1_IRQn
#define BOARD_WIRE_IRQ_PRIO     5            /* 0=máx … 15=mín; > SysTick, sin FreeRTOS */
#define BOARD_NAME              "nucleo-u575zi"
#define BOARD_SRAM_BYTES        (768UL * 1024UL)
#define BOARD_FS_FLASH_ADDR     0x081E0000u          /* últimos 128 KB de 2 MB (.ld limita código a 1920 KB) */
/* H10 — ADC1 habilitado en CubeMX (hadc1) con el canal interno del sensor de
 * temperatura → stm32_temp_c_impl() lee la temperatura del die por ADC. La DK2
 * aún no lo define (sigue con el stub) hasta que se habilite ADC1 en su .ioc. */
#define BOARD_HAS_ADC_TEMP      1
/* H10 — RTC HW habilitado en CubeMX (hrtc, reloj LSI) → la hora la lleva el
 * periférico RTC (sobrevive al reset; con LSI deriva, sin LSE poblado). La DK2
 * usa el stub software hasta que se habilite RTC en su .ioc. */
#define BOARD_HAS_RTC           1

#define BOARD_LED_ERR_ON()      BSP_LED_On(LED_RED)
#define BOARD_LED_RUN_ON()      BSP_LED_On(LED_BLUE)
#define BOARD_LED_RUN_OFF()     BSP_LED_Off(LED_BLUE)
#define BOARD_LED_BEAT_TOGGLE() BSP_LED_Toggle(LED_GREEN)

#endif

#endif /* BPVM_BOARD_H */
