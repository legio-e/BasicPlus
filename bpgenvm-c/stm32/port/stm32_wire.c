/*
 * stm32_wire.c — framing wire v1 sobre el USART del VCP (bare-metal).
 *
 * E/S por el hcom_uart[COM1] del BSP (ya inicializado por BSP_COM_Init en
 * main.c). RX por polling directo del registro (sin ISR ni NVIC → cero
 * cambios en el .ioc); TX por HAL_UART_Transmit.
 *
 * Nota de robustez: usamos HAL_UART_Receive con timeout 0 (lectura NO
 * bloqueante de 1 byte) y limpiamos ORE en caso de overrun, para no atascar
 * la recepción. Portable entre versiones de HAL (no toca flags por nombre).
 */
#include "stm32_wire.h"

#include "main.h"
#include "stm32u5xx_nucleo.h"   /* hcom_uart[], COM1 */

#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef* wire_uart(void) { return &hcom_uart[COM1]; }

int stm32_wire_getchar(void) {
    UART_HandleTypeDef* h = wire_uart();
    uint8_t c;
    HAL_StatusTypeDef st = HAL_UART_Receive(h, &c, 1, 0);   /* timeout 0 = no bloquea */
    if (st == HAL_OK) return (int) c;
    if (st == HAL_ERROR) {
        /* overrun u otro error de RX: limpiar para no atascar el flujo. */
        __HAL_UART_CLEAR_OREFLAG(h);
        h->ErrorCode = HAL_UART_ERROR_NONE;
    }
    return -1;   /* HAL_TIMEOUT (sin byte) o error ya limpiado */
}

void stm32_wire_write(const char* buf, size_t len) {
    if (len == 0) return;
    HAL_UART_Transmit(wire_uart(), (uint8_t*) buf, (uint16_t) len, HAL_MAX_DELAY);
}

int stm32_wire_recv_line(int first_char, char* buf, size_t max) {
    size_t n = 0;
    if (first_char >= 0 && first_char != '\n') {
        if (n < max) buf[n++] = (char) first_char;
        else return -1;
    }
    for (;;) {
        int c = stm32_wire_getchar();
        if (c < 0) continue;            /* poll hasta que llegue */
        if (c == '\n') return (int) n;  /* fin de línea */
        if (c == '\r') continue;        /* tolerar CRLF */
        if (n >= max) return -1;        /* overflow */
        buf[n++] = (char) c;
    }
}

void stm32_wire_send_line(const char* data, size_t len) {
    stm32_wire_write(data, len);
    stm32_wire_write("\n", 1);
}

void stm32_wire_send_cstr(const char* s) {
    stm32_wire_send_line(s, strlen(s));
}

void stm32_wire_send_error(long id, const char* code, const char* message) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"ERROR\",\"id\":%ld,\"code\":\"%s\",\"message\":\"%s\"}",
        id, code, message);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

void stm32_wire_send_fatal(const char* code, const char* message) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"FATAL\",\"code\":\"%s\",\"message\":\"%s\"}", code, message);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}
