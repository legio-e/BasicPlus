/*
 * stm32_wire.h — framing wire v1 sobre el USART del ST-LINK VCP (H9.2),
 * bare-metal. Equivalente STM32 de pico/wire_v1.c: misma semántica de
 * framing (JSON line-delimited + bulk inline) pero la E/S va por el
 * hcom_uart[COM1] del BSP en vez de stdout/USB-CDC.
 *
 * MVP H9.2.a: solo line I/O (recv_line + send). El bulk (recv/send) se
 * añade en H9.2.b para PUT/GET.
 */
#ifndef STM32_WIRE_H
#define STM32_WIRE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIRE_LINE_MAX 2048

/* Lee 1 byte del USART (poll, no bloqueante). -1 si no hay. */
int  stm32_wire_getchar(void);

/* Escribe `len` bytes crudos al USART. */
void stm32_wire_write(const char* buf, size_t len);

/* Lee una línea JSON hasta '\n' (sin eco). `first_char` = char ya
 * consumido por el dispatcher (p.ej. '{'), o -1. Devuelve la longitud
 * (sin '\n', sin null); -1 si excede `max`; -2 si la línea se estanca
 * (timeout ~300ms sin bytes → byte perdido). Tolera '\r' (CRLF). */
int  stm32_wire_recv_line(int first_char, char* buf, size_t max);

/* Escribe `data` + '\n' (un mensaje JSON). */
void stm32_wire_send_line(const char* data, size_t len);

/* Conveniencia: C-string entera + '\n'. */
void stm32_wire_send_cstr(const char* s);

/* {"type":"ERROR","id":<id>,"code":"<code>","message":"<msg>"} */
void stm32_wire_send_error(long id, const char* code, const char* message);

/* {"type":"FATAL","code":"<code>","message":"<msg>"} */
void stm32_wire_send_fatal(const char* code, const char* message);

/* Lee exactamente `n` bytes raw (tras un mensaje JSON con "bulk":n). 0 OK,
 * -1 si se estanca (timeout). Usa el getchar rápido → sigue 115200. */
int  stm32_wire_recv_bulk(uint8_t* buf, size_t n);

/* Escribe `n` bytes raw (sin separador). Para GET_REPLY tras el send_line. */
void stm32_wire_send_bulk(const uint8_t* data, size_t n);

/* Escapa el contenido de un string JSON (`src`,`srclen`) en `dst`
 * (null-terminated). Devuelve longitud escrita o -1 si no cabe. Escapa
 * " \ \n \r \t y los control < 0x20; UTF-8 (>=0x80) pasa tal cual. */
int  stm32_wire_json_escape(const char* src, size_t srclen, char* dst, size_t dstmax);

#ifdef __cplusplus
}
#endif

#endif /* STM32_WIRE_H */
