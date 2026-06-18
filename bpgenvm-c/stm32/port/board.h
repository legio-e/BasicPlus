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
#define BOARD_NAME              "nucleo-u575zi"
#define BOARD_SRAM_BYTES        (768UL * 1024UL)
#define BOARD_FS_FLASH_ADDR     0x081E0000u          /* últimos 128 KB de 2 MB (.ld limita código a 1920 KB) */

#define BOARD_LED_ERR_ON()      BSP_LED_On(LED_RED)
#define BOARD_LED_RUN_ON()      BSP_LED_On(LED_BLUE)
#define BOARD_LED_RUN_OFF()     BSP_LED_Off(LED_BLUE)
#define BOARD_LED_BEAT_TOGGLE() BSP_LED_Toggle(LED_GREEN)

#endif

#endif /* BPVM_BOARD_H */
