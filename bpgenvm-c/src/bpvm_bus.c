/* bpvm_bus.c — los buses SPI e I2C como recurso compartido de la HAL BP (V7/H6.c
 * G4.a1). El porqué, en bpvm_bus.h y en V7_IDEAS §H6.11.
 *
 * Sólo la CONTABILIDAD: qué configuración tiene cada bus, de dónde salió y quién
 * lo usa. Ni una llamada al hardware —eso es de las fachadas `spi.c`/`i2c.c`, que
 * preguntan aquí antes de tocar la cintura—, así que se prueba entera en el PC
 * (`make test-bus`). Los pines del bus se reservan en la tabla de H4 (`gpio.c`):
 * un `Pin(6, OUTPUT)` sobre el reloj de la pantalla recibe «el pin 6 es del SPI 2».
 */
#include "bpvm_bus.h"
#include "bpvm_gpio.h"
#include "bpvm_machine.h"
#include "bpvm.h"          /* bpvm_diag: lo del ENV va al log de la VM */

#include <stdio.h>
#include <string.h>

enum { ORIGEN_LIBRE = 0, ORIGEN_ENV, ORIGEN_USUARIO };

typedef struct {
    unsigned char origen;       /* ORIGEN_*                                       */
    unsigned char usuarios;     /* máscara BPVM_BUS_PROGRAMA | FIRMWARE | PACK     */
    unsigned char quien;        /* quién lo estableció (ORIGEN_USUARIO), para decirlo */
    short         p[3];         /* SPI sck/mosi/miso · I2C sda/scl/-1; -1 = no tiene */
    int           hz;           /* I2C: la del bus · SPI: 0                        */
} bus_t;

static bus_t s_bus[2][BPVM_BUS_ID_MAX];

static const char* NOMBRE[2] = { "SPI", "I2C" };
static const char* PIN_SPI[3] = { "sck", "mosi", "miso" };
static const char* PIN_I2C[3] = { "sda", "scl", "-" };
static const char* nombre_pin(int tipo, int i) { return tipo == BPVM_BUS_SPI ? PIN_SPI[i] : PIN_I2C[i]; }
static int         n_pines(int tipo)          { return tipo == BPVM_BUS_SPI ? 3 : 2; }

static bus_t* bus_de(int tipo, int id) {
    if ((tipo != BPVM_BUS_SPI && tipo != BPVM_BUS_I2C) || id < 0 || id >= BPVM_BUS_ID_MAX) return NULL;
    return &s_bus[tipo][id];
}

static const char* de_quien(int quien) {
    return quien == BPVM_BUS_PACK ? "un pack" : quien == BPVM_BUS_FIRMWARE ? "el firmware" : "el programa";
}

/* «sck 6, mosi 7, miso 8» / «sda 0, scl 1 a 400 kHz». */
static void pinta(int tipo, const bus_t* b, char* out, size_t cap) {
    size_t o = 0;
    int i;
    out[0] = 0;
    for (i = 0; i < n_pines(tipo); i++) {
        if (b->p[i] < 0) continue;
        o += (size_t) snprintf(out + o, cap > o ? cap - o : 0, "%s%s %d", o ? ", " : "", nombre_pin(tipo, i), b->p[i]);
    }
    if (tipo == BPVM_BUS_I2C && b->hz > 0 && o < cap)
        o += (size_t) snprintf(out + o, cap - o, " a %d kHz", b->hz / 1000);
}

/* «(ENV spi2)» / «(lo abrió el programa)». */
static void origen_txt(int tipo, int id, const bus_t* b, char* out, size_t cap) {
    if (b->origen == ORIGEN_ENV) snprintf(out, cap, "ENV %s%d", tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
    else                         snprintf(out, cap, "lo abrió %s", de_quien(b->quien));
}

/* Reserva en la tabla de H4 los pines de un bus que se ESTABLECE. Si uno no se
 * puede, suelta los que ya reservó: o todos o ninguno. */
static int reserva(int tipo, int id, const short p[3], char* err, size_t cap) {
    int i, j;
    for (i = 0; i < n_pines(tipo); i++) {
        if (p[i] < 0) continue;
        if (bpvm_gpio_reservar_bus(p[i], tipo, id, err, cap) != 0) {
            for (j = 0; j < i; j++) if (p[j] >= 0) bpvm_gpio_soltar_bus(p[j]);
            return -1;
        }
    }
    return 0;
}
static void suelta_pines(int tipo, const short p[3]) {
    int i;
    for (i = 0; i < n_pines(tipo); i++) if (p[i] >= 0) bpvm_gpio_soltar_bus(p[i]);
}

/* ── declarar (el ENV) ─────────────────────────────────────────────────────── */

static const char* tras_clave(const char* s, const char* k) {
    while (*k) { if (*s != *k) return 0; s++; k++; }
    return (*s == ':') ? s + 1 : 0;
}

int bpvm_bus_declarar(int tipo, int id, const char* valor, char* err, size_t cap) {
    bus_t* b = bus_de(tipo, id);
    short p[3] = { -1, -1, -1 };
    int hz = 0;
    const char* s = valor;
    char clave[8];
    snprintf(clave, sizeof clave, "%s%d", tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
    if (!b) { snprintf(err, cap, "ENV %s: no hay ese bus", clave); return -1; }
    if (b->origen != ORIGEN_LIBRE) { snprintf(err, cap, "ENV %s: el bus ya estaba establecido", clave); return -1; }
    if (!s || !*s) { snprintf(err, cap, "ENV %s: la línea está vacía", clave); return -1; }
    while (*s) {
        const char* v = 0;
        int campo = -1, i, n = 0, digitos = 0;
        while (*s == ' ' || *s == ',' || *s == '\t') s++;
        if (!*s) break;
        for (i = 0; i < n_pines(tipo) && !v; i++) { v = tras_clave(s, nombre_pin(tipo, i)); if (v) campo = i; }
        if (!v && tipo == BPVM_BUS_I2C) { v = tras_clave(s, "khz"); if (v) campo = 9; }
        if (!v) {
            /* ESTRICTA, como las líneas de los dispositivos: una clave que no
             * conozco es una errata con nombre, no algo que se salta. */
            const char* z = s; while (*z && *z != ',' && *z != ':') z++;
            snprintf(err, cap, "ENV %s: no conozco '%.*s'", clave, (int) (z - s), s);
            return -1;
        }
        while (*v >= '0' && *v <= '9') { n = n * 10 + (*v - '0'); v++; digitos++; if (n > 100000) break; }
        if (!digitos || (*v && *v != ',' && *v != ' ')) {
            snprintf(err, cap, "ENV %s: '%s' no es un número", clave, campo == 9 ? "khz" : nombre_pin(tipo, campo));
            return -1;
        }
        if (campo == 9) hz = n * 1000; else p[campo] = (short) n;
        s = v;
    }
    if (p[0] < 0 || p[1] < 0) {
        snprintf(err, cap, "ENV %s: falta '%s'", clave, p[0] < 0 ? nombre_pin(tipo, 0) : nombre_pin(tipo, 1));
        return -1;
    }
    if (tipo == BPVM_BUS_I2C && hz <= 0) hz = 100000;     /* el I2C estándar, si la línea no lo dice */
    if (reserva(tipo, id, p, err, cap) != 0) {
        char tmp[96];
        snprintf(tmp, sizeof tmp, "ENV %s: %s", clave, err);
        snprintf(err, cap, "%s", tmp);
        return -1;
    }
    memcpy(b->p, p, sizeof p);
    b->hz = hz;
    b->origen = ORIGEN_ENV;
    b->usuarios = 0;
    return 0;
}

int bpvm_bus_desde_env(bpvm_bus_env_get_fn env_get) {
    const bpvm_caps_t* c = bpvm_caps();
    int tipo, i, n = 0;
    if (!env_get) return 0;
    for (tipo = 0; tipo < 2; tipo++) {
        const unsigned char* l = tipo == BPVM_BUS_SPI ? c->spi : c->i2c;
        int nl = tipo == BPVM_BUS_SPI ? c->n_spi : c->n_i2c;
        for (i = 0; i < nl; i++) {
            char clave[8], valor[96], err[128], desc[64];
            int id = l[i];
            snprintf(clave, sizeof clave, "%s%d", tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
            if (env_get(clave, valor, sizeof valor) < 0) continue;       /* no lo declara: libre */
            if (bpvm_bus_declarar(tipo, id, valor, err, sizeof err) != 0) {
                bpvm_diag("bus: %s — el bus queda sin declarar", err);
                continue;
            }
            bpvm_bus_describe(tipo, id, desc, sizeof desc);
            bpvm_diag("bus: %s %d de la placa (ENV %s): %s", NOMBRE[tipo], id, clave, desc);
            n++;
        }
    }
    return n;
}

/* ── abrir / soltar ────────────────────────────────────────────────────────── */

int bpvm_bus_abrir(int tipo, int id, int p0, int p1, int p2, int hz, int quien,
                   int pins[3], int* hz_ef, int* recien, char* err, size_t cap) {
    bus_t* b = bus_de(tipo, id);
    int pide[3], i;
    pide[0] = p0; pide[1] = p1; pide[2] = (tipo == BPVM_BUS_SPI) ? p2 : -1;
    if (recien) *recien = 0;
    if (!b) { snprintf(err, cap, "%s %d: no hay ese bus", tipo == BPVM_BUS_I2C ? "I2C" : "SPI", id); return -1; }

    if (b->origen == ORIGEN_LIBRE) {
        /* Nadie lo ha establecido: lo establece éste, y tiene que decir sus pines. */
        short p[3];
        if (pide[0] < 0 || pide[1] < 0) {
            snprintf(err, cap, "el %s %d no está declarado: da %s y %s, o pon %s%d= en el ENV",
                     NOMBRE[tipo], id, nombre_pin(tipo, 0), nombre_pin(tipo, 1),
                     tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
            return -1;
        }
        for (i = 0; i < 3; i++) p[i] = (short) (pide[i] < 0 ? -1 : pide[i]);
        if (reserva(tipo, id, p, err, cap) != 0) return -1;
        memcpy(b->p, p, sizeof p);
        b->hz = (tipo == BPVM_BUS_I2C) ? (hz > 0 ? hz : 100000) : 0;
        b->origen = ORIGEN_USUARIO;
        b->quien = (unsigned char) quien;
        b->usuarios = (unsigned char) quien;
        if (recien) *recien = 1;
    } else {
        /* Ya tiene configuración: se engancha si pide LA MISMA (o -1). */
        for (i = 0; i < n_pines(tipo); i++) {
            if (pide[i] < 0 || pide[i] == b->p[i]) continue;
            {
                char ya[64], ori[24];
                pinta(tipo, b, ya, sizeof ya);
                origen_txt(tipo, id, b, ori, sizeof ori);
                if (b->p[i] < 0)
                    snprintf(err, cap, "el %s %d es %s (%s): no tiene %s", NOMBRE[tipo], id, ya, ori, nombre_pin(tipo, i));
                else
                    snprintf(err, cap, "el %s %d es %s (%s): pides %s %d", NOMBRE[tipo], id, ya, ori, nombre_pin(tipo, i), pide[i]);
            }
            return -1;
        }
        if (tipo == BPVM_BUS_I2C && hz > 0 && hz != b->hz) {
            char ya[64], ori[24];
            pinta(tipo, b, ya, sizeof ya);
            origen_txt(tipo, id, b, ori, sizeof ori);
            snprintf(err, cap, "el %s %d es %s (%s): pides %d kHz", NOMBRE[tipo], id, ya, ori, hz / 1000);
            return -1;
        }
        /* Un bus del ENV se abre en la cintura con su primer usuario. */
        if (b->usuarios == 0 && recien) *recien = 1;
        b->usuarios = (unsigned char) (b->usuarios | quien);
    }
    if (pins) for (i = 0; i < 3; i++) pins[i] = b->p[i];
    if (hz_ef) *hz_ef = b->hz;
    return 0;
}

void bpvm_bus_abrir_fallo(int tipo, int id, int quien) {
    bus_t* b = bus_de(tipo, id);
    if (!b) return;
    b->usuarios = (unsigned char) (b->usuarios & ~quien);
    if (b->origen == ORIGEN_USUARIO && b->usuarios == 0) {
        suelta_pines(tipo, b->p);
        memset(b, 0, sizeof *b);
    }
}

int bpvm_bus_soltar(int tipo, int id, int quien, char* err, size_t cap) {
    bus_t* b = bus_de(tipo, id);
    if (!b || b->origen == ORIGEN_LIBRE || !(b->usuarios & quien)) {
        snprintf(err, cap, "el %s %d no lo tienes abierto", (tipo == BPVM_BUS_I2C) ? "I2C" : "SPI", id);
        return -1;
    }
    if (b->origen == ORIGEN_ENV && quien == BPVM_BUS_PROGRAMA) {
        /* El de la placa son las pistas del circuito: se «libera» quitando su línea.
         * Dicho con nombre y no un no-op: el programa cree que ha soltado algo. */
        snprintf(err, cap, "el %s %d es de la placa (ENV %s%d): no se suelta desde un programa",
                 NOMBRE[tipo], id, tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
        return -1;
    }
    if (b->usuarios & ~quien) {
        snprintf(err, cap, "el %s %d lo usa también %s: no se puede soltar", NOMBRE[tipo], id,
                 de_quien((b->usuarios & ~quien & BPVM_BUS_PACK) ? BPVM_BUS_PACK
                        : (b->usuarios & ~quien & BPVM_BUS_FIRMWARE) ? BPVM_BUS_FIRMWARE : BPVM_BUS_PROGRAMA));
        return -1;
    }
    b->usuarios = (unsigned char) (b->usuarios & ~quien);
    if (b->origen == ORIGEN_ENV) return b->usuarios == 0 ? 1 : 0;   /* se cierra, pero sigue declarado */
    suelta_pines(tipo, b->p);
    memset(b, 0, sizeof *b);
    return 1;
}

void bpvm_bus_fin_de_run(void (*cerrar)(int tipo, int id)) {
    int tipo, id;
    for (tipo = 0; tipo < 2; tipo++)
        for (id = 0; id < BPVM_BUS_ID_MAX; id++) {
            bus_t* b = &s_bus[tipo][id];
            if (b->origen == ORIGEN_LIBRE || !(b->usuarios & BPVM_BUS_PROGRAMA)) continue;
            b->usuarios = (unsigned char) (b->usuarios & ~BPVM_BUS_PROGRAMA);
            if (b->usuarios != 0) continue;                  /* lo sigue usando un pack o el firmware */
            if (cerrar) cerrar(tipo, id);
            if (b->origen == ORIGEN_USUARIO) { suelta_pines(tipo, b->p); memset(b, 0, sizeof *b); }
        }
}

int bpvm_bus_abierto(int tipo, int id) {
    bus_t* b = bus_de(tipo, id);
    return (b && b->origen != ORIGEN_LIBRE && b->usuarios != 0) ? 1 : 0;
}

int bpvm_bus_describe(int tipo, int id, char* out, size_t cap) {
    bus_t* b = bus_de(tipo, id);
    if (!out || !cap) return -1;
    if (!b || b->origen == ORIGEN_LIBRE) { snprintf(out, cap, "libre"); return 0; }
    pinta(tipo, b, out, cap);
    return 0;
}

void bpvm_bus_reset(void) {
    int tipo, id;
    for (tipo = 0; tipo < 2; tipo++)
        for (id = 0; id < BPVM_BUS_ID_MAX; id++)
            if (s_bus[tipo][id].origen != ORIGEN_LIBRE) suelta_pines(tipo, s_bus[tipo][id].p);
    memset(s_bus, 0, sizeof s_bus);
}
