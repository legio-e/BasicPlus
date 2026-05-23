/*
 * i2c.c — fachada de hooks I2C para la VM C.
 */

#include "bpvm_i2c.h"
#include <stdio.h>

static const bpvm_i2c_backend_t* g_backend = NULL;

void bpvm_i2c_set_backend(const bpvm_i2c_backend_t* backend) {
    g_backend = backend;
}

void bpvm_i2c_init(int bus, int sda, int scl, int baudrate) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, sda, scl, baudrate);
        return;
    }
    printf("[i2c] init bus=%d sda=%d scl=%d baud=%d\n", bus, sda, scl, baudrate);
}

int bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, addr, data, n);
    }
    printf("[i2c] write bus=%d addr=0x%02X bytes=[", bus, addr);
    for (size_t i = 0; i < n; i++) printf("%s%02X", i ? " " : "", data[i]);
    printf("]\n");
    return (int) n;
}

int bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, addr, data, n);
    }
    /* Stub: rellena con ceros. */
    for (size_t i = 0; i < n; i++) data[i] = 0;
    printf("[i2c] read bus=%d addr=0x%02X n=%zu (stub → ceros)\n", bus, addr, n);
    return (int) n;
}
