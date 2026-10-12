/*
 * stm32_repl.h — REPL wire v1 bare-metal para el STM32 (H9.2).
 *
 * Super-bucle: lee el USART; al ver '{' procesa un mensaje wire v1.
 * H9.2.a: HELLO/INFO/TIME/PING/LIST/DF/RESET → el IDE conecta y reconoce
 * `bpvm-stm32`. H9.2.b añadirá PUT/RUN/GET/DEL (subir + ejecutar).
 */
#ifndef STM32_REPL_H
#define STM32_REPL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Bucle infinito del REPL: nunca retorna. Llamar tras BSP_COM_Init(). */
void stm32_repl_run(void);

/* V7/H7 — pinta la pila del MSP que no se está usando, para que la línea `mem:`
 * diga cuánto llega a usar. Llamar en main() justo antes de arrancar FreeRTOS. */
void stm32_msp_pintar(void);

/* #691 (12-oct) — si el arranque dejó a la VM SIN bloque (el plan no se lo dio: `vmheap` por debajo del suelo…), el
 * porqué; si lo tiene, NULL. Lo pregunta la capa de la app del arranque (board_mgr_stm32.c), que hasta aquí la daba
 * por buena siempre —el array de la VM es estático— y la placa se decía sana sin VM. */
const char* stm32_vm_sin_bloque(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_REPL_H */
