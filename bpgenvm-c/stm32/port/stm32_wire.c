/*
 * stm32_wire.c — framing wire v1 sobre el USART del VCP (bare-metal).
 *
 * E/S por el hcom_uart[COM1] del BSP (ya inicializado por BSP_COM_Init en
 * main.c). RX por polling directo del registro (sin ISR ni NVIC → cero
 * cambios en el .ioc); TX por HAL_UART_Transmit.
 *
 * Nota de robustez: RX por lectura DIRECTA del registro (flag RXNE + RDR),
 * NO con HAL_UART_Receive() — su overhead (lock + máquina de estados + tick
 * por byte) a 4 MHz no seguía 115200 baud → overrun, se perdía el '\n' y el
 * REPL se colgaba en recv_line. Limpiamos ORE por si un burst nos adelanta.
 */
#include "stm32_wire.h"

#include "main.h"
#include "stm32u5xx_nucleo.h"   /* hcom_uart[], COM1 */

#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef* wire_uart(void) { return &hcom_uart[COM1]; }

int stm32_wire_getchar(void) {
    UART_HandleTypeDef* h = wire_uart();
    /* Overrun: límpialo para no bloquear la RX si un burst nos adelantó. */
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(h);
    }
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_RXNE)) {
        return (int) (uint8_t) (h->Instance->RDR & 0xFFU);
    }
    return -1;
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
    uint32_t last = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c < 0) {
            /* Anti-cuelgue: si la línea se estanca (byte perdido), abortar en
             * vez de girar para siempre — el REPL sigue vivo, el IDE reintenta. */
            if (HAL_GetTick() - last >= 300U) return -2;
            continue;
        }
        last = HAL_GetTick();
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
    BSP_LED_On(LED_RED);   /* señal de error: rojo encendido hasta el reset */
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"FATAL\",\"code\":\"%s\",\"message\":\"%s\"}", code, message);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

int stm32_wire_recv_bulk(uint8_t* buf, size_t n) {
    size_t got = 0;
    uint32_t last = HAL_GetTick();
    while (got < n) {
        int c = stm32_wire_getchar();
        if (c < 0) {
            if (HAL_GetTick() - last >= 3000U) return -1;   /* bulk estancado */
            continue;
        }
        last = HAL_GetTick();
        buf[got++] = (uint8_t) c;
    }
    return 0;
}

void stm32_wire_send_bulk(const uint8_t* data, size_t n) {
    size_t off = 0;
    while (off < n) {
        size_t chunk = n - off;
        if (chunk > 0xFFFFU) chunk = 0xFFFFU;   /* HAL usa uint16 len */
        HAL_UART_Transmit(wire_uart(), (uint8_t*) (data + off),
                          (uint16_t) chunk, HAL_MAX_DELAY);
        off += chunk;
    }
}

int stm32_wire_json_escape(const char* src, size_t srclen, char* dst, size_t dstmax) {
    size_t o = 0;
    for (size_t i = 0; i < srclen; i++) {
        unsigned char c = (unsigned char) src[i];
        const char* esc = NULL;
        char tmp[8];
        switch (c) {
            case '"':  esc = "\\\""; break;
            case '\\': esc = "\\\\"; break;
            case '\n': esc = "\\n";  break;
            case '\r': esc = "\\r";  break;
            case '\t': esc = "\\t";  break;
            default:
                if (c < 0x20U) { snprintf(tmp, sizeof(tmp), "\\u%04x", c); esc = tmp; }
                break;
        }
        if (esc) {
            size_t l = strlen(esc);
            if (o + l >= dstmax) return -1;
            memcpy(dst + o, esc, l);
            o += l;
        } else {
            if (o + 1 >= dstmax) return -1;
            dst[o++] = (char) c;   /* ASCII imprimible y bytes UTF-8 tal cual */
        }
    }
    if (o >= dstmax) return -1;
    dst[o] = '\0';
    return (int) o;
}
