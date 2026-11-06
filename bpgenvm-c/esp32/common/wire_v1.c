/*
 * wire_v1.c (ESP32-S3) — framing wire BPVM v1 sobre UART0.
 *
 * Misma API que pico/wire_v1.h, pero el I/O va por el driver UART de
 * ESP-IDF en UART0 (el puerto del bridge USB-UART, donde conecta el
 * IDE) en vez de stdio/USB-CDC. Los logs de ESP-IDF van por la consola
 * (USB-Serial-JTAG), así que el binario del wire en UART0 queda limpio.
 *
 * V8/H3 (6-nov): lo que queda aquí es lo de la UART; lo que no es del cable está en src/wire_cables.c.
 */
#include "wire_v1.h"
#include "bpvm_wire_cables.h"   /* V8/H3: este cable, y encima los dos */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "soc/uart_pins.h"   /* #574: U0TXD_GPIO_NUM / U0RXD_GPIO_NUM, los de UART0 del SoC */

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

const char* wire_v1_transport_name(void) { return "UART0 @115200"; }

/* #574 — los pines que este cable le quita al programa: los que se le dan a uart_set_pin arriba o, si no se tocan
 * (UART_PIN_NO_CHANGE), los de UART0 del SoC. */
uint64_t wire_v1_transport_pines(void) {
    int tx = (WIRE_UART_TX_PIN == UART_PIN_NO_CHANGE) ? U0TXD_GPIO_NUM : WIRE_UART_TX_PIN;
    int rx = (WIRE_UART_RX_PIN == UART_PIN_NO_CHANGE) ? U0RXD_GPIO_NUM : WIRE_UART_RX_PIN;
    return (1ULL << tx) | (1ULL << rx);
}

/* ===================== El cable ===================== */

/* V8/H3 (6-nov) — ESTE CABLE SÓLO MUEVE BYTES: la línea, el bulk, sus plazos y el cerrojo son de src/wire_cables.c, que pone
 * las funciones `wire_v1_*` encima de éste y del de la red.
 *
 * Bloquea hasta `ms`, cediendo la CPU dentro del driver. CLAVE: pollear con timeout 0 + vTaskDelay no vale: el bug original
 * era pdMS_TO_TICKS(5) con el tick de ESP-IDF (100 Hz = 10 ms/tick) = 0 ticks → vTaskDelay(0) NO cede → busy-spin → el task
 * watchdog mata IDLE0. Por lo mismo, una espera de menos de un tick es un tick. uart_read_bytes vuelve con los `n` bytes o
 * al acabar el tiempo: para uno, en cuanto llega. */
static int uart_leer(uint8_t* buf, size_t n, int ms) {
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ms > 0 && ticks == 0) ticks = 1;
    int r = uart_read_bytes(WIRE_UART, buf, n, ticks);
    return (r > 0) ? r : 0;
}

/* tx_buffer=0: bloquea hasta vaciar. */
static void uart_escribir(const uint8_t* datos, size_t n) {
    uart_write_bytes(WIRE_UART, (const char*) datos, n);
}

static const bpvm_wire_cable_t CABLE_UART = { "UART0", uart_leer, uart_escribir, NULL, NULL, NULL };

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
    bpvm_wire_cables_local(&CABLE_UART);
}
