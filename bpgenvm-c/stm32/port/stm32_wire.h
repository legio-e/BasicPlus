/*
 * stm32_wire.h (STM32) — la API del wire para esta familia.
 *
 * [V6/U2, 25-ago-2026] Casi todo lo que declaraba está ahora en
 * `include/bpvm_wire_v1.h`, el contrato de las dos capas: el CABLE (lo pone
 * cada familia — aquí, el USART del ST-LINK VCP en bare-metal) y el PROTOCOLO
 * (común, `src/wire_v1_proto.c`). Aquí queda sólo lo propio del STM32.
 *
 * Era la CUARTA FORMA: mismos conceptos con otros nombres (`stm32_wire_*`) y
 * `send_error`/`send_fatal` propios que metían `message` SIN ESCAPAR — una
 * comilla en un mensaje de error rompía el framing. Los builders comunes
 * escapan siempre; ese bug se fue con la unificación.
 *
 * V6/#473 — el `-2` de «linea estancada» YA NO es un matiz de esta familia: se
 * subio al contrato (include/bpvm_wire_v1.h, WIRE_V1_ESTANCADA_MS) y lo
 * implementan las cinco. Este port fue el primero en tenerlo, y el motivo que
 * dio entonces resulto ser del protocolo, no del cable. */
#ifndef STM32_WIRE_H
#define STM32_WIRE_H

#include "bpvm_wire_v1.h"   /* el contrato: cable + protocolo */

#ifdef __cplusplus
extern "C" {
#endif

/* Compat: el nombre viejo del límite de línea, sobre el del contrato. */
#define WIRE_LINE_MAX WIRE_V1_LINE_MAX

/* Lee 1 byte del wire (poll, no bloqueante). -1 si no hay. En la DK2 y la Nucleo
 * sale del anillo de 4 KB que llena el DMA (#495); en el resto, lectura directa del
 * registro. */
int  stm32_wire_getchar(void);

/* DK2 y Nucleo (V3/H5.2, H10, #495) — RX a un anillo de 4 KB que llena el DMA (o,
 * si el DMA no arranca, la IRQ de RX): el wire no pierde bytes aunque el lazo no
 * sondee durante ms (el bombeo de LVGL, una escritura de la flash con la ventana). */
void stm32_wire_rx_drain(void);       /* sin DMA, drena la FIFO RX al anillo (lo llama USART1_IRQHandler) */
void stm32_wire_rx_arrancar(void);    /* el DMA circular (o NVIC + RXFNE IE); llamar tras EnableFifoMode */

/* Escribe `len` bytes crudos al USART. */
void stm32_wire_write(const char* buf, size_t len);

/* FATAL con el LED de error de la placa encendido — el matiz de esta familia
 * sobre el `wire_v1_send_fatal` común (que es quien arma y escapa el JSON). */
void stm32_wire_send_fatal(const char* code, const char* message);

/* Escapa el contenido de un string JSON (`src`,`srclen`) en `dst`
 * (null-terminated). Devuelve longitud escrita o -1 si no cabe. Lo usan los
 * replies que el REPL todavía arma a mano (INFO, LIST...) — migran en U3. */
int  stm32_wire_json_escape(const char* src, size_t srclen, char* dst, size_t dstmax);

#ifdef __cplusplus
}
#endif

#endif /* STM32_WIRE_H */
