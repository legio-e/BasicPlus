/*
 * spi.c — fachada de hooks SPI para la VM C.
 */

#include "bpvm_out.h"
#include "bpvm_spi.h"
#include "bpvm_bus.h"
#include <stdio.h>

static const bpvm_spi_backend_t* g_backend = NULL;

void bpvm_spi_set_backend(const bpvm_spi_backend_t* backend) {
    g_backend = backend;
}

/* V7/H6.c G4.a1 — la regla la pone bpvm_bus.c; aquí sólo se lleva a la cintura:
 * abrirla SOLO la primera vez, y ajustarla (velocidad y modo) en cada apertura.
 * ⚠️ Hasta G4.a2 hay UN ajuste por bus: si la pantalla (40 MHz) y la TF (20 MHz)
 * comparten uno, manda el último que abrió. Que cada dispositivo lleve el suyo en
 * cada transacción es el árbitro (V7_IDEAS §H6.11.3). */
int bpvm_spi_abrir(int bus, int sck, int mosi, int miso, int baudrate, int mode,
                   int quien, int pins[3], char* err, size_t cap) {
    int p[3], recien = 0;
    if (bpvm_bus_abrir(BPVM_BUS_SPI, bus, sck, mosi, miso, 0, quien, p, NULL, &recien, err, cap) != 0) return -1;
    if (g_backend && g_backend->abrir) {
        if (recien && g_backend->abrir(bus, p[0], p[1], p[2]) != 0) {
            bpvm_bus_abrir_fallo(BPVM_BUS_SPI, bus, quien);
            snprintf(err, cap, "el SPI %d no se pudo abrir (sck %d, mosi %d, miso %d)", bus, p[0], p[1], p[2]);
            return -1;
        }
        if (g_backend->ajustar) g_backend->ajustar(bus, baudrate, mode);
    } else {
        bpvm_spi_init(bus, p[0], p[1], p[2], baudrate, mode);   /* cintura sin migrar, o el stub del PC */
    }
    if (pins) { pins[0] = p[0]; pins[1] = p[1]; pins[2] = p[2]; }
    return 0;
}

int bpvm_spi_soltar(int bus, int quien, char* err, size_t cap) {
    int r = bpvm_bus_soltar(BPVM_BUS_SPI, bus, quien, err, cap);
    if (r < 0) return -1;
    if (r == 1) bpvm_spi_cerrar(bus);
    return 0;
}

void bpvm_spi_cerrar(int bus) {
    if (g_backend && g_backend->cerrar) g_backend->cerrar(bus);
}

void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, sck, mosi, miso, baudrate, mode);
        return;
    }
    bpvm_out("[spi] init bus=%d sck=%d mosi=%d miso=%d baud=%d mode=%d\n",
           bus, sck, mosi, miso, baudrate, mode);
}

int bpvm_spi_write(int bus, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, data, n);
    }
    bpvm_out("[spi] write bus=%d bytes=[", bus);
    for (size_t i = 0; i < n; i++) bpvm_out("%s%02X", i ? " " : "", data[i]);
    bpvm_out("]\n");
    return (int) n;
}

int bpvm_spi_read(int bus, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, data, n);
    }
    for (size_t i = 0; i < n; i++) data[i] = 0;
    /* #478 — TEXTO = CONTRATO DE PARIDAD: identico al de miVM. */
    bpvm_out("[spi] read bus=%d count=%zu (sim → ceros)\n", bus, n);
    return (int) n;
}

int bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    if (g_backend && g_backend->transfer) {
        return g_backend->transfer(bus, tx, rx, n);
    }
    bpvm_out("[spi] transfer bus=%d tx=[", bus);
    for (size_t i = 0; i < n; i++) {
        bpvm_out("%s%02X", i ? " " : "", tx[i]);
        rx[i] = 0;
    }
    bpvm_out("] (rx sim → ceros)\n");
    return (int) n;
}
