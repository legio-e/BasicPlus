/*
 * i2c.c — fachada de hooks I2C para la VM C.
 */

#include "bpvm_out.h"
#include "bpvm_i2c.h"
#include "bpvm_bus.h"
#include <stdio.h>

static const bpvm_i2c_backend_t* g_backend = NULL;

void bpvm_i2c_set_backend(const bpvm_i2c_backend_t* backend) {
    g_backend = backend;
}

/* V7/H6.c G4.a1 — como bpvm_spi_abrir: la regla en bpvm_bus.c, la cintura se abre
 * SOLO la primera vez. En I2C la velocidad es parte de la configuración del bus. */
int bpvm_i2c_abrir(int bus, int sda, int scl, int baudrate, int quien, char* err, size_t cap) {
    int p[3], hz = 0, recien = 0;
    if (bpvm_bus_abrir(BPVM_BUS_I2C, bus, sda, scl, -1, baudrate, quien, p, &hz, &recien, err, cap) != 0) return -1;
    if (g_backend && g_backend->abrir) {
        if (recien && g_backend->abrir(bus, p[0], p[1], hz) != 0) {
            bpvm_bus_abrir_fallo(BPVM_BUS_I2C, bus, quien);
            snprintf(err, cap, "el I2C %d no se pudo abrir (sda %d, scl %d)", bus, p[0], p[1]);
            return -1;
        }
    } else {
        bpvm_i2c_init(bus, p[0], p[1], hz);   /* cintura sin migrar, o el stub del PC */
    }
    return 0;
}

int bpvm_i2c_soltar(int bus, int quien, char* err, size_t cap) {
    int r = bpvm_bus_soltar(BPVM_BUS_I2C, bus, quien, err, cap);
    if (r < 0) return -1;
    if (r == 1) bpvm_i2c_cerrar(bus);
    return 0;
}

void bpvm_i2c_cerrar(int bus) {
    if (g_backend && g_backend->cerrar) g_backend->cerrar(bus);
}

void bpvm_i2c_init(int bus, int sda, int scl, int baudrate) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, sda, scl, baudrate);
        return;
    }
    bpvm_out("[i2c] init bus=%d sda=%d scl=%d baud=%d\n", bus, sda, scl, baudrate);
}

int bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, addr, data, n);
    }
    bpvm_out("[i2c] write bus=%d addr=0x%02X bytes=[", bus, addr);
    for (size_t i = 0; i < n; i++) bpvm_out("%s%02X", i ? " " : "", data[i]);
    bpvm_out("]\n");
    return (int) n;
}

int bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, addr, data, n);
    }
    /* Stub: rellena con ceros. */
    for (size_t i = 0; i < n; i++) data[i] = 0;
    /* #478 — TEXTO = CONTRATO DE PARIDAD: identico al de miVM. */
    bpvm_out("[i2c] read bus=%d addr=0x%02X count=%zu (sim → ceros)\n", bus, addr, n);
    return (int) n;
}
