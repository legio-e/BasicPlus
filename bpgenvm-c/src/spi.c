/*
 * spi.c — fachada de hooks SPI para la VM C.
 */

#include "bpvm_out.h"
#include "bpvm_spi.h"
#include "bpvm_bus.h"
#include "bpvm_gpio.h"
#include "bpvm.h"          /* bpvm_diag: un turno que no se da se dice en el log */
#include <stdio.h>

static const bpvm_spi_backend_t* g_backend = NULL;

void bpvm_spi_set_backend(const bpvm_spi_backend_t* backend) {
    g_backend = backend;
}

/* ── V7/H6.c G4.a2 — lo que la fachada recuerda de cada bus ──────────────────
 *
 * La velocidad y el modo ACTUALES del bus (Eduardo, 23-sep: «se guarda la actual;
 * si la nueva es la misma no se cambia, se cambia solamente si es diferente»). Es
 * estado del BUS, no de ningún usuario, y vive aquí —no en cada cintura— para que
 * la regla sea la misma en las cinco familias (la de la Pico no lo comprobaba).
 * -1 = no se sabe (recién abierto o cerrado): el primer turno ajusta. */
static int s_hz[BPVM_BUS_ID_MAX], s_modo[BPVM_BUS_ID_MAX];
static int s_hz_listo = 0;
/* El CS que bajó el turno en curso (-1 = ninguno): lo sube `soltar`. */
static int s_cs[BPVM_BUS_ID_MAX];
/* La velocidad de quien usa las ranuras de SIEMPRE (`Spi.Bus` y las funciones sueltas
 * de BP, la `spi_write` v6 de la BIOS), que no la llevan en cada llamada: la de su
 * `abrir`, por usuario. G4.a3: deja de ser temporal —es el contrato de lo de siempre;
 * Eduardo: «la velocidad inicial se puede utilizar por defecto»—; lo nuevo (el
 * dispositivo, las transacciones) la trae en cada petición. */
static int s_leg_hz[BPVM_BUS_ID_MAX][3], s_leg_modo[BPVM_BUS_ID_MAX][3];

static int bus_ok(int bus) { return bus >= 0 && bus < BPVM_BUS_ID_MAX; }
static int idx_quien(int quien) { return quien == BPVM_BUS_PACK ? 2 : quien == BPVM_BUS_FIRMWARE ? 1 : 0; }
static void hz_listo(void) {
    int i;
    if (s_hz_listo) return;
    for (i = 0; i < BPVM_BUS_ID_MAX; i++) { s_hz[i] = s_modo[i] = -1; s_cs[i] = -1; }
    s_hz_listo = 1;
}
static void hz_olvida(int bus) { hz_listo(); if (bus_ok(bus)) s_hz[bus] = s_modo[bus] = -1; }

/* V7/H6.c G4.a1 — la regla la pone bpvm_bus.c; aquí sólo se lleva a la cintura:
 * abrirla SOLO la primera vez. G4.a2: la velocidad ya NO se ajusta al abrir —se
 * cambiaría en mitad del turno de otro—; se ajusta al TOMAR el bus. */
int bpvm_spi_abrir(int bus, int sck, int mosi, int miso, int baudrate, int mode,
                   int quien, int pins[3], char* err, size_t cap) {
    int p[3], recien = 0;
    if (bpvm_bus_abrir(BPVM_BUS_SPI, bus, sck, mosi, miso, 0, quien, p, NULL, &recien, err, cap) != 0) return -1;
    hz_listo();
    if (g_backend && g_backend->abrir) {
        if (recien && g_backend->abrir(bus, p[0], p[1], p[2]) != 0) {
            bpvm_bus_abrir_fallo(BPVM_BUS_SPI, bus, quien);
            snprintf(err, cap, "el SPI %d no se pudo abrir (sck %d, mosi %d, miso %d)", bus, p[0], p[1], p[2]);
            return -1;
        }
        if (recien) hz_olvida(bus);
    } else {
        bpvm_spi_init(bus, p[0], p[1], p[2], baudrate, mode);   /* cintura sin migrar, o el stub del PC */
        s_hz[bus] = baudrate; s_modo[bus] = mode;
    }
    s_leg_hz[bus][idx_quien(quien)] = baudrate;
    s_leg_modo[bus][idx_quien(quien)] = mode;
    if (pins) { pins[0] = p[0]; pins[1] = p[1]; pins[2] = p[2]; }
    return 0;
}

/* G4.a3 — un `Spi.Device` no trae pines ni velocidad de bus: se engancha al que YA
 * está establecido. Si es el primer usuario de un bus del ENV, lo abre en la cintura
 * (como `abrir`), sin tocar la velocidad de las ranuras de siempre. */
int bpvm_spi_enganchar(int bus, int quien, char* err, size_t cap) {
    int p[3], recien = 0;
    if (!bpvm_bus_establecido(BPVM_BUS_SPI, bus)) {
        snprintf(err, cap, "el SPI %d no está establecido: ponlo en el ENV (spi%d=) o ábrelo antes con Spi.Bus", bus, bus);
        return -1;
    }
    if (bpvm_bus_abrir(BPVM_BUS_SPI, bus, -1, -1, -1, 0, quien, p, NULL, &recien, err, cap) != 0) return -1;
    hz_listo();
    if (recien && g_backend && g_backend->abrir) {
        if (g_backend->abrir(bus, p[0], p[1], p[2]) != 0) {
            bpvm_bus_abrir_fallo(BPVM_BUS_SPI, bus, quien);
            snprintf(err, cap, "el SPI %d no se pudo abrir (sck %d, mosi %d, miso %d)", bus, p[0], p[1], p[2]);
            return -1;
        }
        hz_olvida(bus);
    }
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
    hz_olvida(bus);
}

void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode) {
    if (g_backend && g_backend->init) {
        g_backend->init(bus, sck, mosi, miso, baudrate, mode);
        return;
    }
    bpvm_out("[spi] init bus=%d sck=%d mosi=%d miso=%d baud=%d mode=%d\n",
           bus, sck, mosi, miso, baudrate, mode);
}

/* ── lo que va al cable: la cintura, o el stub del PC (su texto es contrato de
 * paridad con miVM, #478) ── */
static int crudo_write(int bus, const uint8_t* data, size_t n) {
    if (g_backend && g_backend->write) {
        return g_backend->write(bus, data, n);
    }
    bpvm_out("[spi] write bus=%d bytes=[", bus);
    for (size_t i = 0; i < n; i++) bpvm_out("%s%02X", i ? " " : "", data[i]);
    bpvm_out("]\n");
    return (int) n;
}

static int crudo_read(int bus, uint8_t* data, size_t n) {
    if (g_backend && g_backend->read) {
        return g_backend->read(bus, data, n);
    }
    for (size_t i = 0; i < n; i++) data[i] = 0;
    /* #478 — TEXTO = CONTRATO DE PARIDAD: identico al de miVM. */
    bpvm_out("[spi] read bus=%d count=%zu (sim → ceros)\n", bus, n);
    return (int) n;
}

static int crudo_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
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

/* ── V7/H6.c G4.a2 — EL TURNO (V7_IDEAS §H6.11.6b) ─────────────────────────── */

static int tomar_(int bus, int cs, int hz, int modo, int quien, char* err, size_t cap) {
    if (bpvm_bus_tomar(BPVM_BUS_SPI, bus, quien, cs, 0, err, cap) != 0) return -1;
    hz_listo();
    /* La velocidad y el modo, SÓLO si cambian: se guarda la actual del bus. */
    if (hz > 0 && (hz != s_hz[bus] || modo != s_modo[bus])) {
        if (g_backend && g_backend->ajustar) g_backend->ajustar(bus, hz, modo);
        s_hz[bus] = hz; s_modo[bus] = modo;
    }
    s_cs[bus] = cs;
    if (cs >= 0) bpvm_gpio_write(cs, 0);
    return 0;
}

/* G4.a3 — el turno A MANO (la TF, los packs v7) se mide al dejarlo: no se puede
 * calcular antes, y si pasa del tope del bus el log lo dice con nombre. */
int bpvm_spi_tomar(int bus, int cs, int hz, int modo, int quien, char* err, size_t cap) {
    if (tomar_(bus, cs, hz, modo, quien, err, cap) != 0) return -1;
    bpvm_bus_medir(BPVM_BUS_SPI, bus);
    return 0;
}

int bpvm_spi_tomar_transaccion(int bus, int cs, int hz, int modo, int quien, char* err, size_t cap) {
    return tomar_(bus, cs, hz, modo, quien, err, cap);
}

int bpvm_spi_cs(int bus, int nivel) {
    if (!bus_ok(bus) || !bpvm_bus_tomado(BPVM_BUS_SPI, bus) || s_cs[bus] < 0) return -1;
    bpvm_gpio_write(s_cs[bus], nivel ? 1 : 0);
    return 0;
}

int bpvm_spi_xfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    if (!bus_ok(bus) || !bpvm_bus_tomado(BPVM_BUS_SPI, bus)) {
        bpvm_diag("SPI %d: transferir sin turno (falta tomar el bus)", bus);
        return -1;
    }
    if (tx && rx) return crudo_transfer(bus, tx, rx, n);
    if (tx)       return crudo_write(bus, tx, n);
    if (rx)       return crudo_read(bus, rx, n);
    return -1;
}

void bpvm_spi_dejar(int bus) {
    if (!bus_ok(bus)) return;
    hz_listo();
    if (s_cs[bus] >= 0) bpvm_gpio_write(s_cs[bus], 1);
    s_cs[bus] = -1;
    bpvm_bus_dejar(BPVM_BUS_SPI, bus);
}

/* ── las ranuras de SIEMPRE, cada llamada un turno entero ─────────────────────
 * Sin CS: el de `Spi.Bus` lo lleva el programa con `Gpio`, y el de una ranura
 * v6 el pack (V7_IDEAS §H6.11.6b: el límite, a sabiendas). */
enum { OP_WRITE, OP_READ, OP_TRANSFER };

/* El turno de las ranuras de siempre, con la velocidad del `abrir` de `quien`. Suelto
 * para la llamada de BP que la fachada ve en varios TRAMOS (#534: sin tope, en tramos de
 * 256): la llamada ENTERA tiene que ser UN turno —si no, otro dispositivo se colaría entre
 * dos tramos con el CS del programa abajo—. -1 con el porqué en el log. */
int bpvm_spi_tomar_como(int quien, int bus) {
    char err[128];
    int q = idx_quien(quien);
    if (!bus_ok(bus)) return -1;
    if (tomar_(bus, -1, s_leg_hz[bus][q], s_leg_modo[bus][q], quien, err, sizeof err) != 0) {
        bpvm_diag("SPI %d: %s", bus, err);
        return -1;
    }
    return 0;
}

static int legado(int quien, int bus, const uint8_t* tx, uint8_t* rx, size_t n, int op) {
    int r;
    if (bpvm_spi_tomar_como(quien, bus) != 0) return -1;
    r = (op == OP_WRITE) ? crudo_write(bus, tx, n)
      : (op == OP_READ)  ? crudo_read(bus, rx, n)
      :                    crudo_transfer(bus, tx, rx, n);
    bpvm_spi_dejar(bus);
    return r;
}

/* ── V7/H6.c G4.a3 — LA TRANSACCIÓN (ver bpvm_spi.h) ──────────────────────────
 * Calcular, tomar, transferir, soltar: en ese orden, y sin nada más dentro. */
int bpvm_spi_transaccion(int bus, int cs, int hz, int modo, int quien,
                         const bpvm_spi_op_t* ops, int nops, char* err, size_t cap) {
    unsigned long long bytes = 0;
    int i, hecho = 0;
    if (!bus_ok(bus)) { snprintf(err, cap, "SPI %d: no hay ese bus", bus); return -1; }
    if (!bpvm_bus_abierto(BPVM_BUS_SPI, bus)) { snprintf(err, cap, "el SPI %d no está abierto", bus); return -1; }
    if (modo < 0 || modo > 3) { snprintf(err, cap, "SPI %d: modo no válido (%d): 0..3", bus, modo); return -1; }
    if (nops < 0 || (nops > 0 && !ops)) { snprintf(err, cap, "SPI %d: la transacción no tiene operaciones", bus); return -1; }
    for (i = 0; i < nops; i++) {
        if (ops[i].n > 0 && !ops[i].tx && !ops[i].rx) {
            snprintf(err, cap, "SPI %d: la operación %d no tiene ni tx ni rx", bus, i + 1);
            return -1;
        }
        bytes += ops[i].n;
    }
    if (bpvm_bus_cabe(BPVM_BUS_SPI, bus, bytes * 8ULL, hz, err, cap) != 0) return -1;
    if (tomar_(bus, cs, hz, modo, quien, err, cap) != 0) return -1;
    for (i = 0; i < nops; i++) {
        int r;
        if (ops[i].n == 0) continue;
        r = bpvm_spi_xfer(bus, ops[i].tx, ops[i].rx, ops[i].n);
        if (r < 0) {
            snprintf(err, cap, "SPI %d: falló la operación %d de %d", bus, i + 1, nops);
            hecho = -1;
            break;
        }
        hecho += r;
    }
    bpvm_spi_dejar(bus);
    return hecho;
}

int bpvm_spi_write(int bus, const uint8_t* data, size_t n)                   { return legado(BPVM_BUS_PROGRAMA, bus, data, NULL, n, OP_WRITE); }
int bpvm_spi_read(int bus, uint8_t* data, size_t n)                          { return legado(BPVM_BUS_PROGRAMA, bus, NULL, data, n, OP_READ); }
int bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n)     { return legado(BPVM_BUS_PROGRAMA, bus, tx, rx, n, OP_TRANSFER); }
int bpvm_spi_write_como(int quien, int bus, const uint8_t* data, size_t n)   { return legado(quien, bus, data, NULL, n, OP_WRITE); }
