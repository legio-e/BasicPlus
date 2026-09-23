/*
 * i2c.c — fachada de hooks I2C para la VM C.
 */

#include "bpvm_out.h"
#include "bpvm_i2c.h"
#include "bpvm_bus.h"
#include "bpvm.h"          /* bpvm_diag: un turno que no se da se dice en el log */
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

/* ── lo que va al cable: la cintura, o el stub del PC (su texto es contrato de
 * paridad con miVM, #478) ── */
static int crudo_write(int bus, int addr, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, addr, data, n);
    }
    bpvm_out("[i2c] write bus=%d addr=0x%02X bytes=[", bus, addr);
    for (size_t i = 0; i < n; i++) bpvm_out("%s%02X", i ? " " : "", data[i]);
    bpvm_out("]\n");
    return (int) n;
}

static int crudo_read(int bus, int addr, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, addr, data, n);
    }
    /* Stub: rellena con ceros. */
    for (size_t i = 0; i < n; i++) data[i] = 0;
    /* #478 — TEXTO = CONTRATO DE PARIDAD: identico al de miVM. */
    bpvm_out("[i2c] read bus=%d addr=0x%02X count=%zu (sim → ceros)\n", bus, addr, n);
    return (int) n;
}

/* ── V7/H6.c G4.a2 — cada llamada, un TURNO (V7_IDEAS §H6.11.6b) ──────────────
 * La dirección hace de CS: sólo para decir de quién es el turno si alguien espera
 * demasiado («I2C 0 ocupado (0x58, un pack) desde hace 1000 ms»). */
static int turno(int quien, int bus, int addr, char* err, size_t cap) {
    return bpvm_bus_tomar(BPVM_BUS_I2C, bus, quien, addr, 0, err, cap);
}

int bpvm_i2c_write_como(int quien, int bus, int addr, const uint8_t* data, size_t n) {
    char err[128];
    int r;
    if (turno(quien, bus, addr, err, sizeof err) != 0) { bpvm_diag("I2C %d: %s", bus, err); return -1; }
    r = crudo_write(bus, addr, data, n);
    bpvm_bus_dejar(BPVM_BUS_I2C, bus);
    return r;
}

int bpvm_i2c_read_como(int quien, int bus, int addr, uint8_t* data, size_t n) {
    char err[128];
    int r;
    if (turno(quien, bus, addr, err, sizeof err) != 0) { bpvm_diag("I2C %d: %s", bus, err); return -1; }
    r = crudo_read(bus, addr, data, n);
    bpvm_bus_dejar(BPVM_BUS_I2C, bus);
    return r;
}

/* Escribir y leer en UN turno: nadie habla en medio (el registro de un táctil y su
 * trama). Con STOP entre las dos por ahora; el repeated start de verdad, G4.a3.
 * Devuelve los bytes leídos, o -1 si falla cualquiera de las dos. */
int bpvm_i2c_escribir_leer(int quien, int bus, int addr, const uint8_t* tx, size_t ntx,
                           uint8_t* rx, size_t nrx) {
    char err[128];
    int r;
    if (turno(quien, bus, addr, err, sizeof err) != 0) { bpvm_diag("I2C %d: %s", bus, err); return -1; }
    r = crudo_write(bus, addr, tx, ntx);
    if (r == (int) ntx) r = crudo_read(bus, addr, rx, nrx);
    else                r = -1;
    bpvm_bus_dejar(BPVM_BUS_I2C, bus);
    return r;
}

int bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n) { return bpvm_i2c_write_como(BPVM_BUS_PROGRAMA, bus, addr, data, n); }
int bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n)        { return bpvm_i2c_read_como(BPVM_BUS_PROGRAMA, bus, addr, data, n); }
