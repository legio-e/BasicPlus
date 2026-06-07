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

#ifdef __cplusplus
}
#endif

#endif /* STM32_REPL_H */
