/*
 * spi.c — fachada de hooks SPI para la VM C.
 */

#include "bpvm_spi.h"
#include <stdio.h>

static const bpvm_spi_backend_t* g_backend = NULL;

void bpvm_spi_set_backend(const bpvm_spi_backend_t* backend) {
    g_backend = backend;
}

void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, sck, mosi, miso, baudrate, mode);
        return;
    }
    printf("[spi] init bus=%d sck=%d mosi=%d miso=%d baud=%d mode=%d\n",
           bus, sck, mosi, miso, baudrate, mode);
}

int bpvm_spi_write(int bus, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, data, n);
    }
    printf("[spi] write bus=%d bytes=[", bus);
    for (size_t i = 0; i < n; i++) printf("%s%02X", i ? " " : "", data[i]);
    printf("]\n");
    return (int) n;
}

int bpvm_spi_read(int bus, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, data, n);
    }
    for (size_t i = 0; i < n; i++) data[i] = 0;
    printf("[spi] read bus=%d n=%zu (stub → ceros)\n", bus, n);
    return (int) n;
}

int bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    if (g_backend && g_backend->transfer) {
        return g_backend->transfer(bus, tx, rx, n);
    }
    printf("[spi] transfer bus=%d tx=[", bus);
    for (size_t i = 0; i < n; i++) {
        printf("%s%02X", i ? " " : "", tx[i]);
        rx[i] = 0;
    }
    printf("] (rx → ceros)\n");
    return (int) n;
}
