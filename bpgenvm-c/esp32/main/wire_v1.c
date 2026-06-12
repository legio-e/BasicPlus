/*
 * wire_v1.c (ESP32-S3) — framing wire BPVM v1 sobre UART0.
 *
 * Misma API que pico/wire_v1.h, pero el I/O va por el driver UART de
 * ESP-IDF en UART0 (el puerto del bridge USB-UART, donde conecta el
 * IDE) en vez de stdio/USB-CDC. Los logs de ESP-IDF van por la consola
 * (USB-Serial-JTAG), así que el binario del wire en UART0 queda limpio.
 *
 * Los builders JSON (msg_begin, field_*, etc.) son idénticos al de Pico
 * (C portable) — copiados aquí para no acoplar los dos firmwares.
 */
#include "wire_v1.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include <stdio.h>
#include <string.h>

#define WIRE_UART        UART_NUM_0
#define WIRE_UART_BAUD   115200
#define WIRE_RX_BUF      4096
/* Pines por defecto de UART0 en el ESP32-S3 (U0TXD/U0RXD), cableados al
 * bridge USB-UART de la DevKitC. Hay que enrutarlos EXPLÍCITAMENTE: con
 * la consola en USB-Serial-JTAG, ESP-IDF no configura UART0 en la app,
 * así que sin uart_set_pin el periférico no queda conectado a los pines
 * → RX/TX muertos (era el motivo de que no llegara el HELLO). */
#define WIRE_UART_TX_PIN 43
#define WIRE_UART_RX_PIN 44

void wire_v1_uart_init(void) {
    const uart_config_t cfg = {
        .baud_rate = WIRE_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /* Orden canónico ESP-IDF: param_config → set_pin → driver_install.
     * tx_buffer=0 ⇒ uart_write_bytes bloquea hasta vaciar (envío sínc.). */
    uart_param_config(WIRE_UART, &cfg);
    uart_set_pin(WIRE_UART, WIRE_UART_TX_PIN, WIRE_UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(WIRE_UART, WIRE_RX_BUF, 0, 0, NULL, 0);
}

/* ===================== Lectura ===================== */

/* Lee 1 byte BLOQUEANDO hasta `ms`. CLAVE: deja que el driver UART ceda
 * CPU mientras espera, en vez de pollear con timeout 0 + vTaskDelay. El
 * bug original: pdMS_TO_TICKS(5) con el tick de ESP-IDF (100 Hz = 10 ms/
 * tick) = 0 ticks → vTaskDelay(0) NO cede → busy-spin → el task watchdog
 * mata IDLE0. Bloquear en uart_read_bytes lo evita. Devuelve el byte o
 * -1 si venció el timeout sin datos. uart_read_bytes retorna en cuanto
 * llega un byte, así que la latencia de respuesta es mínima. */
static int wire_read_byte(int ms) {
    uint8_t b;
    int n = uart_read_bytes(WIRE_UART, &b, 1, pdMS_TO_TICKS(ms));
    return (n == 1) ? (int) b : -1;
}

/* P-run-stop (#257) — variante NO bloqueante (0 ticks) para el poll de
 * KILL que la VM invoca entre quanta durante un RUN. */
int wire_v1_try_getchar(void) {
    uint8_t b;
    int n = uart_read_bytes(WIRE_UART, &b, 1, 0);
    return (n == 1) ? (int) b : -1;
}

int wire_v1_recv_line(int first_char_already_read, char* buf, size_t buf_max) {
    size_t n = 0;
    if (first_char_already_read >= 0) {
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) first_char_already_read;
    }
    for (;;) {
        int c = wire_read_byte(100);    /* bloquea ≤100 ms cediendo CPU */
        if (c < 0) continue;             /* timeout sin datos: reintenta */
        if (c == '\n') return (int) n;
        if (c == '\r') continue;
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) c;
    }
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    if (n > buf_max) return -1;
    size_t got = 0;
    while (got < n) {
        /* uart_read_bytes bloquea (cede CPU) hasta tener datos o timeout. */
        int r = uart_read_bytes(WIRE_UART, buf + got, n - got, pdMS_TO_TICKS(200));
        if (r > 0) got += (size_t) r;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

void wire_v1_send_line(const char* data, size_t len) {
    if (len) uart_write_bytes(WIRE_UART, data, len);
    uart_write_bytes(WIRE_UART, "\n", 1);
}

void wire_v1_send_cstr(const char* cstr) {
    wire_v1_send_line(cstr, strlen(cstr));
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    if (n) uart_write_bytes(WIRE_UART, (const char*) data, n);
}

/* ===================== Builders JSON (idénticos a Pico) ============ */

static int put_raw(char* buf, size_t buf_max, size_t off, const char* s, size_t n) {
    if (off + n > buf_max) return -1;
    memcpy(buf + off, s, n);
    return (int)(off + n);
}
static int put_cstr(char* buf, size_t buf_max, size_t off, const char* s) {
    return put_raw(buf, buf_max, off, s, strlen(s));
}
static int put_escaped(char* buf, size_t buf_max, size_t off, const char* s) {
    while (*s) {
        char c = *s++;
        const char* esc = NULL;
        char one[3];
        size_t esc_n = 0;
        switch (c) {
            case '"':  esc = "\\\""; esc_n = 2; break;
            case '\\': esc = "\\\\"; esc_n = 2; break;
            case '\n': esc = "\\n";  esc_n = 2; break;
            case '\r': esc = "\\r";  esc_n = 2; break;
            case '\t': esc = "\\t";  esc_n = 2; break;
            default:
                if ((unsigned char) c < 0x20) {
                    if (off + 6 > buf_max) return -1;
                    static const char* HEX = "0123456789abcdef";
                    buf[off++] = '\\'; buf[off++] = 'u'; buf[off++] = '0'; buf[off++] = '0';
                    buf[off++] = HEX[(c >> 4) & 0xF];
                    buf[off++] = HEX[c & 0xF];
                    continue;
                }
                one[0] = c; esc = one; esc_n = 1; break;
        }
        if (off + esc_n > buf_max) return -1;
        memcpy(buf + off, esc, esc_n);
        off += esc_n;
    }
    return (int) off;
}
static int put_long(char* buf, size_t buf_max, size_t off, long v) {
    char tmp[24];
    int n = snprintf(tmp, sizeof(tmp), "%ld", v);
    if (n < 0) return -1;
    return put_raw(buf, buf_max, off, tmp, (size_t) n);
}

int wire_v1_msg_begin(char* buf, size_t buf_max, size_t off, const char* type, long id) {
    int r = put_cstr(buf, buf_max, off, "{\"type\":\""); if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, type);            if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\",\"id\":");       if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, id);                  if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_msg_begin_event(char* buf, size_t buf_max, size_t off, const char* type) {
    int r = put_cstr(buf, buf_max, off, "{\"type\":\""); if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, type);            if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\"");                if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_field_long(char* buf, size_t buf_max, size_t off, const char* key, long value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":");      if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, value);      if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_field_bool(char* buf, size_t buf_max, size_t off, const char* key, int value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":");      if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, value ? "true" : "false"); if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_field_string(char* buf, size_t buf_max, size_t off, const char* key, const char* value) {
    int r = put_cstr(buf, buf_max, off, ",\""); if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, key);        if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\":\"");    if (r < 0) return -1; off = (size_t) r;
    r = put_escaped(buf, buf_max, off, value);   if (r < 0) return -1; off = (size_t) r;
    r = put_cstr(buf, buf_max, off, "\"");        if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_field_bulk(char* buf, size_t buf_max, size_t off, size_t n) {
    int r = put_cstr(buf, buf_max, off, ",\"bulk\":"); if (r < 0) return -1; off = (size_t) r;
    r = put_long(buf, buf_max, off, (long) n);          if (r < 0) return -1; off = (size_t) r;
    return (int) off;
}
int wire_v1_msg_end(char* buf, size_t buf_max, size_t off) {
    if (off + 1 > buf_max) return -1;
    buf[off++] = '}';
    return (int) off;
}

/* ===================== Senders pre-empaquetados ===================== */

void wire_v1_send_reply_empty(const char* type, long id) {
    char buf[64];
    int off = wire_v1_msg_begin(buf, sizeof(buf), 0, type, id);
    if (off < 0) return;
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) return;
    wire_v1_send_line(buf, (size_t) off);
}
void wire_v1_send_error(long id, const char* code, const char* message) {
    char buf[512];
    int off = wire_v1_msg_begin(buf, sizeof(buf), 0, "ERROR", id);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "code", code);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "message", message ? message : "");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"ERROR\",\"id\":0}"); return; }
    wire_v1_send_line(buf, (size_t) off);
}
void wire_v1_send_fatal(const char* code, const char* message) {
    char buf[512];
    int off = wire_v1_msg_begin_event(buf, sizeof(buf), 0, "FATAL");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "code", code);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_field_string(buf, sizeof(buf), (size_t) off, "message", message ? message : "");
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    off = wire_v1_msg_end(buf, sizeof(buf), (size_t) off);
    if (off < 0) { wire_v1_send_cstr("{\"type\":\"FATAL\"}"); return; }
    wire_v1_send_line(buf, (size_t) off);
}
