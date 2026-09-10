/*
 * uart.c — fachada de hooks UART para la VM C.
 *
 * Mismo patrón que i2c.c / spi.c: si no hay backend registrado,
 * imprimimos por stdout (útil en host para inspección). Cuando el
 * firmware Pico arranca, main.c llama a bpvm_uart_set_backend() con
 * la tabla que envuelve uart_init / uart_write_blocking /
 * uart_read_timeout_us del Pico SDK.
 */

#include "bpvm_out.h"
#include "bpvm_uart.h"
#include <stdio.h>
#include "bpvm_platform.h"   /* V6/#473: reloj y espera del anillo de RX */
#include "bpvm.h"            /* bpvm_diag: los fallos hacen ruido */
#include <string.h>

static const bpvm_uart_backend_t* g_backend = NULL;

/* ─── V6/#473 — EL BUFFER DE RECEPCION, Y POR QUE VIVE AQUI ──────────────────
 *
 * Decision de Eduardo (10-sep): «las UARTs a veces tienen un pequeno buffer que
 * siempre se queda corto, asi que hay que anadir un buffer externo que lo amplie.
 * Eso es necesario yo diria que SIEMPRE, asi que no se trata de adaptarse al
 * hardware sino que el hardware y el software se adapten a nuestras necesidades.»
 *
 * Medido antes de escribir una linea: el ESP32 ya tenia 512 B por software (su
 * driver los llena desde la ISR); la Pico leia el FIFO de 32 B del PL011 a pelo,
 * sin IRQ ni anillo; y el STM32 leia el registro con HAL_UART_Receive, sin nada.
 * O sea que en DOS de las tres familias un programa BP perdia bytes en cuanto
 * llegaban mas de los que caben en el FIFO antes de que el leyera — y el
 * `available()` que contestaba 1/0 era solo el sintoma visible.
 *
 * El anillo se escribe UNA vez, aqui. La familia solo aporta el goteo: enciende su
 * IRQ de RX y llama a bpvm_uart_rx_push() por cada byte. Quien ya trae buffer
 * propio (el ESP32) NO se apunta y sigue contestando por su backend.
 *
 * Dos ranuras porque dos son los buses que la stdlib declara (Pico.UART_BUSES()).
 * Pedir una tercera no degrada en silencio: lo dice. */
#ifndef BPVM_UART_RX_SLOTS
#define BPVM_UART_RX_SLOTS 2
#endif
#ifndef BPVM_UART_RX_CAP
#define BPVM_UART_RX_CAP 512      /* igual que el ESP32: mismo contrato en las cinco */
#endif

typedef struct {
    int bus;                          /* -1 = ranura libre */
    volatile uint16_t head, tail;
    volatile uint32_t perdidos;       /* los que no cupieron: se cuentan y se dicen */
    volatile uint8_t  buf[BPVM_UART_RX_CAP];
} uart_rx_t;

static uart_rx_t g_rx[BPVM_UART_RX_SLOTS];
static int       g_rx_init = 0;

static uart_rx_t* rx_de(int bus) {
    if (!g_rx_init) return NULL;
    for (int i = 0; i < BPVM_UART_RX_SLOTS; i++)
        if (g_rx[i].bus == bus) return &g_rx[i];
    return NULL;
}

void bpvm_uart_rx_ring_enable(int bus) {
    if (!g_rx_init) {
        for (int i = 0; i < BPVM_UART_RX_SLOTS; i++) g_rx[i].bus = -1;
        g_rx_init = 1;
    }
    if (rx_de(bus)) { rx_de(bus)->head = rx_de(bus)->tail = 0; return; }   /* re-init */
    for (int i = 0; i < BPVM_UART_RX_SLOTS; i++) {
        if (g_rx[i].bus < 0) {
            g_rx[i].bus = bus; g_rx[i].head = g_rx[i].tail = 0; g_rx[i].perdidos = 0;
            return;
        }
    }
    bpvm_diag("[uart] sin ranura de buffer RX para el bus %d (hay %d): ese bus seguira perdiendo bytes", bus, BPVM_UART_RX_SLOTS);
}

/* Desde la ISR de la familia. Sin cerrojos: un productor y un consumidor. */
void bpvm_uart_rx_push(int bus, uint8_t b) {
    uart_rx_t* r = rx_de(bus);
    if (!r) return;
    uint16_t nh = (uint16_t) ((r->head + 1u) % BPVM_UART_RX_CAP);
    if (nh == r->tail) { r->perdidos++; return; }   /* lleno: se cuenta, no se calla */
    r->buf[r->head] = b;
    r->head = nh;
}

static int rx_cuantos(const uart_rx_t* r) {
    int d = (int) r->head - (int) r->tail;
    return d >= 0 ? d : d + BPVM_UART_RX_CAP;
}

/* Saca hasta n bytes, esperando a que lleguen hasta agotar el plazo. */
static int rx_drenar(uart_rx_t* r, uint8_t* data, size_t n, int timeout_ms) {
    size_t got = 0;
    int64_t fin = bpvm_platform_now_ms() + (timeout_ms > 0 ? timeout_ms : 0);
    for (;;) {
        while (got < n && r->tail != r->head) {
            data[got++] = r->buf[r->tail];
            r->tail = (uint16_t) ((r->tail + 1u) % BPVM_UART_RX_CAP);
        }
        if (got >= n) break;
        if (timeout_ms > 0 && bpvm_platform_now_ms() >= fin) break;
        if (timeout_ms == 0) break;                 /* sin espera: lo que hubiera */
        bpvm_platform_thread_sleep_ms(1);           /* cede: no giramos a tope */
    }
    if (r->perdidos) {
        uint32_t p = r->perdidos; r->perdidos = 0;
        bpvm_diag("[uart] bus %d: %lu bytes descartados, el buffer de %d se lleno",
                  r->bus, (unsigned long) p, BPVM_UART_RX_CAP);
    }
    return (int) got;
}

void bpvm_uart_set_backend(const bpvm_uart_backend_t* backend) {
    g_backend = backend;
}

void bpvm_uart_init(int bus, int tx, int rx, int baudrate,
                    int data_bits, int stop_bits, int parity) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, tx, rx, baudrate, data_bits, stop_bits, parity);
        return;
    }
    bpvm_out("[uart] init bus=%d tx=%d rx=%d baud=%d %d%c%d\n",
           bus, tx, rx, baudrate, data_bits,
           parity == 1 ? 'O' : parity == 2 ? 'E' : 'N',
           stop_bits);
}

int bpvm_uart_write(int bus, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, data, n);
    }
    bpvm_out("[uart] write bus=%d bytes=[", bus);
    for (size_t i = 0; i < n; i++) bpvm_out("%s%02X", i ? " " : "", data[i]);
    bpvm_out("] (\"");
    for (size_t i = 0; i < n; i++) {
        char c = (char) data[i];
        bpvm_out("%c", (c >= 32 && c < 127) ? c : '.');   /* #480 - NO putchar: ese escribe
                                                    * al FILE de C y se sale del sumidero de
                                                    * la VM, asi que el payload salia DESORDENADO
                                                    * (aparecia al principio de stdout) y el
                                                    * preview quedaba vacio. */
    }
    bpvm_out("\")\n");
    return (int) n;
}

int bpvm_uart_read(int bus, uint8_t* data, size_t n, int timeout_ms) {
    /* V6/#473 — si la familia se apunto al anillo, la verdad esta ahi: su ISR lo
     * llena aunque nadie lea, que es justo lo que el FIFO del hardware no hace. */
    uart_rx_t* r = rx_de(bus);
    if (r) return rx_drenar(r, data, n, timeout_ms);
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, data, n, timeout_ms);
    }
    for (size_t i = 0; i < n; i++) data[i] = 0;
    /* #478 — TEXTO = CONTRATO DE PARIDAD: identico al de miVM. */
    bpvm_out("[uart] read bus=%d count=%zu timeout=%d (sim → ceros)\n",
           bus, n, timeout_ms);
    return (int) n;
}

int bpvm_uart_available(int bus) {
    uart_rx_t* r = rx_de(bus);
    if (r) return rx_cuantos(r);   /* V6/#473: CUANTOS hay, que es lo que promete el contrato */
    if (g_backend && g_backend->available) {
        return g_backend->available(bus);
    }
    bpvm_out("[uart] available bus=%d (sim → 0)\n", bus);
    return 0;
}
