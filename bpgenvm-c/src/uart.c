/*
 * uart.c — fachada de hooks UART para la VM C.
 *
 * Mismo patrón que i2c.c / spi.c: si no hay backend registrado,
 * imprimimos por stdout (útil en host para inspección). Cuando el
 * firmware Pico arranca, main.c llama a bpvm_uart_set_backend() con
 * la tabla que envuelve uart_init / uart_write_blocking /
 * uart_read_timeout_us del Pico SDK.
 */

#include "bpvm_uart.h"
#include <stdio.h>

static const bpvm_uart_backend_t* g_backend = NULL;

void bpvm_uart_set_backend(const bpvm_uart_backend_t* backend) {
    g_backend = backend;
}

void bpvm_uart_init(int bus, int tx, int rx, int baudrate,
                    int data_bits, int stop_bits, int parity) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, tx, rx, baudrate, data_bits, stop_bits, parity);
        return;
    }
    printf("[uart] init bus=%d tx=%d rx=%d baud=%d %d%c%d\n",
           bus, tx, rx, baudrate, data_bits,
           parity == 1 ? 'O' : parity == 2 ? 'E' : 'N',
           stop_bits);
}

int bpvm_uart_write(int bus, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, data, n);
    }
    printf("[uart] write bus=%d bytes=[", bus);
    for (size_t i = 0; i < n; i++) printf("%s%02X", i ? " " : "", data[i]);
    printf("] (\"");
    for (size_t i = 0; i < n; i++) {
        char c = (char) data[i];
        putchar((c >= 32 && c < 127) ? c : '.');
    }
    printf("\")\n");
    return (int) n;
}

int bpvm_uart_read(int bus, uint8_t* data, size_t n, int timeout_ms) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, data, n, timeout_ms);
    }
    for (size_t i = 0; i < n; i++) data[i] = 0;
    printf("[uart] read bus=%d n=%zu timeout=%d (stub → ceros)\n",
           bus, n, timeout_ms);
    return (int) n;
}

int bpvm_uart_available(int bus) {
    if (g_backend && g_backend->available) {
        return g_backend->available(bus);
    }
    printf("[uart] available bus=%d (stub → 0)\n", bus);
    return 0;
}
