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
#include "board.h"   /* placa: BOARD_WIRE_UART, BOARD_LED_ERR_ON */

#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef* wire_uart(void) { return BOARD_WIRE_UART; }

#if defined(BPVM_BOARD_DK2)
/* ===== RX por IRQ + ring (DK2, V3/H5.2) =====
 * La lectura directa del registro (#else) no tiene buffer software: si el lazo
 * no sondea durante >~700µs (la FIFO HW son 8 bytes) se pierden bytes por
 * overrun. Eso mataba el KILL durante Gui.run() — el bombeo de LVGL deja el UART
 * sin atender varios ms. Aquí la IRQ de RX del USART1 drena la FIFO a un ring de
 * 256B en cuanto llega cada byte, independiente de lo que haga el lazo principal:
 * getchar() saca del ring y no se pierde nada (mientras el ring no desborde;
 * 256B ≈ 5 líneas de comando, y el consumidor de bulk drena en continuo).
 * SPSC monocore: la ISR es el único productor (head), getchar el único consumidor
 * (tail); índices uint16 alineados → acceso atómico en M33, volatile basta. */
#define RX_RING_SZ 256u
static volatile uint8_t  s_rx_ring[RX_RING_SZ];
static volatile uint16_t s_rx_head = 0;   /* productor: la ISR  */
static volatile uint16_t s_rx_tail = 0;   /* consumidor: getchar */

void stm32_wire_rx_drain(void) {
    UART_HandleTypeDef* h = wire_uart();
    if (__HAL_UART_GET_FLAG(h, UART_FLAG_ORE)) __HAL_UART_CLEAR_OREFLAG(h);
    while (__HAL_UART_GET_FLAG(h, UART_FLAG_RXNE)) {        /* RXFNE con FIFO on */
        uint8_t b = (uint8_t) (h->Instance->RDR & 0xFFU);
        uint16_t nh = (uint16_t) ((s_rx_head + 1u) % RX_RING_SZ);
        if (nh != s_rx_tail) { s_rx_ring[s_rx_head] = b; s_rx_head = nh; }
        /* ring lleno → descartar (no debería pasar con tráfico de comandos) */
    }
}

void stm32_wire_rx_irq_enable(void) {
    UART_HandleTypeDef* h = wire_uart();
    HAL_NVIC_EnableIRQ(BOARD_WIRE_IRQn);        /* NVIC (CubeMX ya fijó prioridad) */
    __HAL_UART_ENABLE_IT(h, UART_IT_RXNE);      /* = RXFNE con FIFO → IRQ al llegar byte */
}

int stm32_wire_getchar(void) {
    if (s_rx_tail != s_rx_head) {
        uint8_t b = s_rx_ring[s_rx_tail];
        s_rx_tail = (uint16_t) ((s_rx_tail + 1u) % RX_RING_SZ);
        return (int) b;
    }
    return -1;
}

#else  /* ---- resto de placas STM32: RX directo del registro (sin cambios) ---- */

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

#endif

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
    BOARD_LED_ERR_ON();    /* señal de error: LED de error encendido hasta el reset */
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
