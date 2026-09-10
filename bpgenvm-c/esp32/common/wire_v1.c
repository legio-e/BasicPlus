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
#include "bpvm_platform.h"   /* V6/#473: bpvm_platform_now_ms */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include <stdio.h>
#include <string.h>

#define WIRE_UART        UART_NUM_0
#define WIRE_UART_BAUD   115200
#define WIRE_RX_BUF      8192   /* holgado para tramas largas (bulk PUT de .mod/recursos) */
/* Pines de UART0 cableados al bridge USB-UART de la placa. Hay que enrutarlos
 * EXPLÍCITAMENTE: con la consola en USB-Serial-JTAG, ESP-IDF no configura UART0
 * en la app, así que sin uart_set_pin el periférico no queda conectado a los
 * pines → RX/TX muertos (era el motivo de que no llegara el HELLO en el S3).
 *  - S3 DevKitC: U0TXD/U0RXD del bridge = GPIO43/44 (NO son los IO_MUX default).
 *  - P4: el bridge está en los pines IO_MUX por defecto de UART0 → NO_CHANGE (no
 *    reenrutar; con la consola movida a USB-Serial-JTAG, UART0 queda libre).
 *    A VERIFICAR en placa: si no llega el HELLO, fijar aquí los pines reales. */
/* ── V6/P1.C3.3 — EL DEFECTO ES «LO QUE DIGA EL SILICIO», NO «LO QUE DICE EL S3»
 *
 * Esto era un `#if P4 / #else` cuyo `#else` fijaba GPIO43/44 — **los pines del
 * S3 DevKitC**. En el ESP32-C3 esos pines NO EXISTEN (tiene GPIO0..21), así que
 * el ensayo del C3 enrutaba el wire a la nada: `uart_set_pin` devuelve error y
 * no aborta, o sea un RX/TX muerto EN SILENCIO — el peor modo de fallo.
 *
 * Es el mismo error que `#465`, con otra cara: **dar por hecha la familia en vez
 * de preguntar al silicio**. La forma correcta es al revés de como estaba: por
 * defecto se dejan los pines IO_MUX que el SoC asigna a UART0 (que es lo que ya
 * usan la ROM y el bootloader, así que son los de verdad), y sólo se REENRUTA
 * donde la PLACA cablea el bridge a otro sitio — que es información de placa, no
 * de familia, y por eso va nombrada una a una. */
#if defined(CONFIG_IDF_TARGET_ESP32P4)
/* P4 EV board: el bridge USB-UART está en GPIO37/38. Hay que fijarlos
 * EXPLÍCITAMENTE (con la consola en USB-Serial-JTAG, ESP-IDF no enruta UART0
 * solo → NO_CHANGE deja RX/TX muertos, que fue el fallo del handshake). */
#define WIRE_UART_TX_PIN 37
#define WIRE_UART_RX_PIN 38
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
/* S3 DevKitC: U0TXD/U0RXD del bridge = GPIO43/44, que NO son los IO_MUX
 * por defecto — de ahí el reenrutado. */
#define WIRE_UART_TX_PIN 43
#define WIRE_UART_RX_PIN 44
#else
/* Cualquier otro silicio (C3 hoy; C6 y S31 mañana): los de UART0 del SoC. En el
 * C3 son GPIO21/GPIO20. Si una placa concreta lleva el bridge a otros pines, se
 * añade su rama arriba CON EL NOMBRE DE LA PLACA. */
#define WIRE_UART_TX_PIN UART_PIN_NO_CHANGE
#define WIRE_UART_RX_PIN UART_PIN_NO_CHANGE
#endif

void wire_v1_transport_init(void) {
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

const char* wire_v1_transport_name(void) { return "UART0 @115200"; }

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
    int64_t ultimo = bpvm_platform_now_ms();   /* V6/#473: plazo de linea estancada */
    for (;;) {
        int c = wire_read_byte(100);    /* bloquea <=100 ms cediendo CPU */
        if (c < 0) {                     /* timeout sin datos */
            /* Sin plazo, un truncado se come el mensaje siguiente. El porque
             * entero, en include/bpvm_wire_v1.h (WIRE_V1_ESTANCADA_MS). */
            if (n > 0 && bpvm_platform_now_ms() - ultimo >= (int64_t) WIRE_V1_ESTANCADA_MS)
                return -2;
            continue;
        }
        ultimo = bpvm_platform_now_ms();
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


void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    if (n) uart_write_bytes(WIRE_UART, (const char*) data, n);
}


