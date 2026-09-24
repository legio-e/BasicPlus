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

/* ── V7/H6.c G4.a3 — LA TRANSACCIÓN (ver bpvm_i2c.h) ──────────────────────────── */

/* Una cintura sin repeated start: un mensaje tras otro, cada uno con su STOP —dentro
 * del mismo turno: nadie más habla en medio—. 0, o -1 si uno no llega entero. */
static int por_mensajes(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    int i;
    for (i = 0; i < nm; i++) {
        int r = m[i].lee ? crudo_read(bus, addr, m[i].buf, m[i].n)
                         : crudo_write(bus, addr, m[i].buf, m[i].n);
        if (r != (int) m[i].n) return -1;
    }
    return 0;
}

/* El PC: la transacción en UNA línea —es una sola, START…STOP—, y lo leído a cero.
 * TEXTO = CONTRATO DE PARIDAD con miVM. */
static int stub_transaccion(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    int i, lee = 0;
    uint32_t j;
    bpvm_out("[i2c] transaction bus=%d addr=0x%02X", bus, addr);
    for (i = 0; i < nm; i++) {
        if (m[i].lee) {
            for (j = 0; j < m[i].n; j++) m[i].buf[j] = 0;
            bpvm_out(" read=%u", (unsigned) m[i].n);
            lee = 1;
        } else {
            bpvm_out(" write=[");
            for (j = 0; j < m[i].n; j++) bpvm_out("%s%02X", j ? " " : "", m[i].buf[j]);
            bpvm_out("]");
        }
    }
    bpvm_out(lee ? " (sim → ceros)\n" : "\n");
    return 0;
}

int bpvm_i2c_transaccion(int bus, int addr, int quien, const bpvm_i2c_msg_t* m, int nm,
                         char* err, size_t cap) {
    unsigned long long relojes = 0;
    int i, r, total = 0;
    if (bus < 0 || bus >= BPVM_BUS_ID_MAX) { snprintf(err, cap, "I2C %d: no hay ese bus", bus); return -1; }
    if (!bpvm_bus_abierto(BPVM_BUS_I2C, bus)) { snprintf(err, cap, "el I2C %d no está abierto", bus); return -1; }
    if (addr < 0 || addr > 0x7F) { snprintf(err, cap, "I2C %d: dirección no válida (0x%X)", bus, (unsigned) addr); return -1; }
    if (nm < 1 || nm > BPVM_I2C_MENSAJES_MAX || !m) {
        snprintf(err, cap, "I2C %d: una transacción lleva de 1 a %d mensajes (tiene %d)", bus, BPVM_I2C_MENSAJES_MAX, nm);
        return -1;
    }
    {
        int ordenes = 1;                             /* el STOP */
        for (i = 0; i < nm; i++) {
            if (m[i].n == 0) { snprintf(err, cap, "I2C %d: el mensaje %d está vacío", bus, i + 1); return -1; }
            if (!m[i].buf) { snprintf(err, cap, "I2C %d: el mensaje %d no tiene dónde", bus, i + 1); return -1; }
            ordenes += (m[i].lee && m[i].n > 1) ? 3 : 2;
            relojes += 9ULL * (1ULL + m[i].n);        /* la dirección y cada byte, con su ACK */
            total += (int) m[i].n;
        }
        if (ordenes > BPVM_I2C_ORDENES_MAX) {
            snprintf(err, cap, "I2C %d: la transacción necesita %d órdenes del bus y caben %d (cada mensaje 2, "
                     "una lectura de más de un byte 3, y el STOP)", bus, ordenes, BPVM_I2C_ORDENES_MAX);
            return -1;
        }
    }
    if (bpvm_bus_cabe(BPVM_BUS_I2C, bus, relojes, bpvm_bus_hz(BPVM_BUS_I2C, bus), err, cap) != 0) return -1;
    if (turno(quien, bus, addr, err, cap) != 0) return -1;
    if (!g_backend)                 r = stub_transaccion(bus, addr, m, nm);
    else if (g_backend->transaccion) r = g_backend->transaccion(bus, addr, m, nm);
    else                            r = por_mensajes(bus, addr, m, nm);
    bpvm_bus_dejar(BPVM_BUS_I2C, bus);
    if (r != 0) { snprintf(err, cap, "I2C %d: 0x%02X no contestó", bus, (unsigned) addr); return -2; }
    return total;
}

/* Escribir y leer en UNA transacción: nadie habla en medio (el registro de un táctil y
 * su trama), y desde G4.a3 con repeated start entre las dos. Bytes leídos, o -1. Lo
 * que no la deja empezar va al log; un dispositivo que no contesta, no —es la
 * respuesta, y un táctil que se sondea cada pocos ms llenaría el log—, como siempre. */
int bpvm_i2c_escribir_leer(int quien, int bus, int addr, const uint8_t* tx, size_t ntx,
                           uint8_t* rx, size_t nrx) {
    char err[128];
    bpvm_i2c_msg_t m[2];
    int r;
    m[0].buf = (uint8_t*) tx; m[0].n = (uint32_t) ntx; m[0].lee = 0;
    m[1].buf = rx;            m[1].n = (uint32_t) nrx; m[1].lee = 1;
    r = bpvm_i2c_transaccion(bus, addr, quien, m, 2, err, sizeof err);
    if (r == -1) bpvm_diag("%s", err);
    return r < 0 ? -1 : (int) nrx;
}

int bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n) { return bpvm_i2c_write_como(BPVM_BUS_PROGRAMA, bus, addr, data, n); }
int bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n)        { return bpvm_i2c_read_como(BPVM_BUS_PROGRAMA, bus, addr, data, n); }
