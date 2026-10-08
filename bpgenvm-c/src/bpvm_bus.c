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
#include "bpvm_platform.h" /* G4.a2: el mutex, la condvar y el reloj del árbitro */
#include "bpvm.h"          /* bpvm_diag: lo del ENV va al log de la VM */

#include <stdio.h>
#include <string.h>

enum { ORIGEN_LIBRE = 0, ORIGEN_ENV, ORIGEN_USUARIO, ORIGEN_IMAGEN };   /* #669: el de la imagen, como el del ENV */

typedef struct {
    unsigned char origen;       /* ORIGEN_*                                       */
    unsigned char usuarios;     /* máscara BPVM_BUS_PROGRAMA | FIRMWARE | PACK     */
    unsigned char quien;        /* quién lo estableció (ORIGEN_USUARIO), para decirlo */
    unsigned char tope_ms;      /* G4.a3: el `ms:` de su línea del ENV; 0 = BPVM_BUS_TOPE_MS */
    short         p[3];         /* SPI sck/mosi/miso · I2C sda/scl/-1; -1 = no tiene */
    int           hz;           /* I2C: la del bus · SPI: 0                        */
    const char*   que;          /* #669, ORIGEN_IMAGEN: de quién es («la imagen: el táctil…») */
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

/* «(ENV spi2)» / «(lo abrió el programa)» / «(la imagen: el táctil de la pantalla)». */
static void origen_txt(int tipo, int id, const bus_t* b, char* out, size_t cap) {
    if (b->origen == ORIGEN_ENV)         snprintf(out, cap, "ENV %s%d", tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
    else if (b->origen == ORIGEN_IMAGEN) snprintf(out, cap, "%s", b->que ? b->que : "la imagen");   /* #669 */
    else                                 snprintf(out, cap, "lo abrió %s", de_quien(b->quien));
}

/* #669 — de la PLACA: lo declaró el ENV o la imagen. Se abre con su primer usuario, se cierra sin ninguno y sigue
 * declarado; un programa no lo suelta. */
static int de_la_placa(const bus_t* b) { return b->origen == ORIGEN_ENV || b->origen == ORIGEN_IMAGEN; }

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
    int hz = 0, ms = -1;
    const char* s = valor;
    char clave[8];
    snprintf(clave, sizeof clave, "%s%d", tipo == BPVM_BUS_SPI ? "spi" : "i2c", id);
    if (!b) { snprintf(err, cap, "ENV %s: no hay ese bus", clave); return -1; }
    if (b->origen == ORIGEN_IMAGEN) {   /* #669: lo declaró la imagen antes (el táctil de su pantalla) */
        snprintf(err, cap, "ENV %s: es el bus de la imagen (%s): la línea sobra", clave, b->que ? b->que : "la imagen");
        return -1;
    }
    if (b->origen != ORIGEN_LIBRE) { snprintf(err, cap, "ENV %s: el bus ya estaba establecido", clave); return -1; }
    if (!s || !*s) { snprintf(err, cap, "ENV %s: la línea está vacía", clave); return -1; }
    while (*s) {
        const char* v = 0;
        int campo = -1, i, n = 0, digitos = 0;
        while (*s == ' ' || *s == ',' || *s == '\t') s++;
        if (!*s) break;
        for (i = 0; i < n_pines(tipo) && !v; i++) { v = tras_clave(s, nombre_pin(tipo, i)); if (v) campo = i; }
        if (!v && tipo == BPVM_BUS_I2C) { v = tras_clave(s, "khz"); if (v) campo = 9; }
        if (!v) { v = tras_clave(s, "ms"); if (v) campo = 10; }   /* G4.a3: el tope de un turno */
        if (!v) {
            /* ESTRICTA, como las líneas de los dispositivos: una clave que no
             * conozco es una errata con nombre, no algo que se salta. */
            const char* z = s; while (*z && *z != ',' && *z != ':') z++;
            snprintf(err, cap, "ENV %s: no conozco '%.*s'", clave, (int) (z - s), s);
            return -1;
        }
        while (*v >= '0' && *v <= '9') { n = n * 10 + (*v - '0'); v++; digitos++; if (n > 100000) break; }
        if (!digitos || (*v && *v != ',' && *v != ' ')) {
            snprintf(err, cap, "ENV %s: '%s' no es un número", clave,
                     campo == 9 ? "khz" : campo == 10 ? "ms" : nombre_pin(tipo, campo));
            return -1;
        }
        if (campo == 9) hz = n * 1000; else if (campo == 10) ms = n; else p[campo] = (short) n;
        s = v;
    }
    /* G4.a3 — configurable, pero SIEMPRE limitado (Eduardo): un tope mayor que el
     * máximo es una errata con nombre, no un «sin tope». */
    if (ms == 0 || ms > BPVM_BUS_TOPE_MAX_MS) {
        snprintf(err, cap, "ENV %s: ms:%d no vale: el tope de un turno va de 1 a %d ms", clave, ms, BPVM_BUS_TOPE_MAX_MS);
        return -1;
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
    b->tope_ms = (unsigned char) (ms > 0 ? ms : 0);
    b->origen = ORIGEN_ENV;
    b->usuarios = 0;
    return 0;
}

int bpvm_bus_declarar_imagen(int tipo, int id, int p0, int p1, int p2, int hz, const char* que, char* err, size_t cap) {
    bus_t* b = bus_de(tipo, id);
    short p[3];
    if (!b) { snprintf(err, cap, "%s %d: no hay ese bus", tipo == BPVM_BUS_I2C ? "I2C" : "SPI", id); return -1; }
    if (b->origen != ORIGEN_LIBRE) { snprintf(err, cap, "el %s %d ya estaba establecido", NOMBRE[tipo], id); return -1; }
    if (p0 < 0 || p1 < 0) { snprintf(err, cap, "el %s %d de la imagen: faltan sus pines", NOMBRE[tipo], id); return -1; }
    p[0] = (short) p0;
    p[1] = (short) p1;
    p[2] = (short) ((tipo == BPVM_BUS_SPI && p2 >= 0) ? p2 : -1);
    if (reserva(tipo, id, p, err, cap) != 0) return -1;
    memcpy(b->p, p, sizeof p);
    b->hz = (tipo == BPVM_BUS_I2C) ? (hz > 0 ? hz : 100000) : 0;
    b->tope_ms = 0;
    b->origen = ORIGEN_IMAGEN;
    b->usuarios = 0;
    b->que = que;
    return 0;
}

int bpvm_bus_desde_env(bpvm_bus_env_get_fn env_get) {
    const bpvm_caps_t* c = bpvm_caps();
    int tipo, i, n = 0;
    bpvm_bus_arbitro_init();   /* G4.a2: al arrancar, con un solo hilo vivo */
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
            bpvm_diag("bus: %s %d de la placa (ENV %s): %s; tope de un turno %d ms", NOMBRE[tipo], id, clave, desc,
                      bpvm_bus_tope_ms(tipo, id));
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
                char ya[64], ori[48];
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
            char ya[64], ori[48];
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
    if (de_la_placa(b) && quien == BPVM_BUS_PROGRAMA) {
        /* El de la placa son las pistas del circuito: se «libera» quitando su línea (o, el de la imagen, nunca).
         * Dicho con nombre y no un no-op: el programa cree que ha soltado algo. */
        char ori[48];
        origen_txt(tipo, id, b, ori, sizeof ori);
        snprintf(err, cap, "el %s %d es de la placa (%s): no se suelta desde un programa", NOMBRE[tipo], id, ori);
        return -1;
    }
    if (b->usuarios & ~quien) {
        snprintf(err, cap, "el %s %d lo usa también %s: no se puede soltar", NOMBRE[tipo], id,
                 de_quien((b->usuarios & ~quien & BPVM_BUS_PACK) ? BPVM_BUS_PACK
                        : (b->usuarios & ~quien & BPVM_BUS_FIRMWARE) ? BPVM_BUS_FIRMWARE : BPVM_BUS_PROGRAMA));
        return -1;
    }
    b->usuarios = (unsigned char) (b->usuarios & ~quien);
    if (de_la_placa(b)) return b->usuarios == 0 ? 1 : 0;   /* se cierra, pero sigue declarado */
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

/* ── G4.a3: el TOPE de un turno ────────────────────────────────────────────── */

int bpvm_bus_tope_ms(int tipo, int id) {
    bus_t* b = bus_de(tipo, id);
    return (b && b->tope_ms) ? b->tope_ms : BPVM_BUS_TOPE_MS;
}

int bpvm_bus_establecido(int tipo, int id) {
    bus_t* b = bus_de(tipo, id);
    return (b && b->origen != ORIGEN_LIBRE) ? 1 : 0;
}

int bpvm_bus_hz(int tipo, int id) {
    bus_t* b = bus_de(tipo, id);
    return (b && b->origen != ORIGEN_LIBRE) ? b->hz : 0;
}

/* «40 MHz» / «400 kHz» / «1500 kHz» / «115200 Hz». CONTRATO DE PARIDAD con miVM. */
static void velocidad_txt(int hz, char* out, size_t cap) {
    if (hz % 1000000 == 0)   snprintf(out, cap, "%d MHz", hz / 1000000);
    else if (hz % 1000 == 0) snprintf(out, cap, "%d kHz", hz / 1000);
    else                     snprintf(out, cap, "%d Hz", hz);
}

/* La cuenta, ENTERA a propósito (la misma en miVM): los microsegundos que ocupa el bus,
 * truncados; pasa si no superan el tope, y lo que se dice son los ms redondeados hacia
 * arriba —20,001 ms es «21 ms»—. */
int bpvm_bus_cabe(int tipo, int id, unsigned long long relojes, int hz, char* err, size_t cap) {
    int tope = bpvm_bus_tope_ms(tipo, id);
    unsigned long long us;
    char vel[24];
    if (!bus_de(tipo, id)) { snprintf(err, cap, "%s %d: no hay ese bus", tipo == BPVM_BUS_I2C ? "I2C" : "SPI", id); return -1; }
    if (hz <= 0) { snprintf(err, cap, "%s %d: velocidad no válida (%d)", NOMBRE[tipo], id, hz); return -1; }
    us = relojes * 1000000ULL / (unsigned long long) hz;
    if (us <= (unsigned long long) tope * 1000ULL) return 0;
    velocidad_txt(hz, vel, sizeof vel);
    snprintf(err, cap, "%s %d: la transacción ocuparía el bus %lu ms a %s (máximo %d ms)",
             NOMBRE[tipo], id, (unsigned long) ((us + 999ULL) / 1000ULL), vel, tope);
    return -1;
}

/* ── G4.a2: el ÁRBITRO ─────────────────────────────────────────────────────── */

/* El turno en curso de cada bus: quién (0 = libre), su CS o su dirección, desde
 * cuándo. Aparte de `s_bus` a propósito: cerrar o soltar un bus hace memset de su
 * contabilidad, y eso no puede borrar el turno de quien está transfiriendo. */
typedef struct { unsigned char quien, esperan, medir; short cs; int64_t desde; } turno_t;
static turno_t s_turno[2][BPVM_BUS_ID_MAX];

static bpvm_platform_mutex_handle_t s_arb_m;
static bpvm_platform_cond_handle_t  s_arb_c;
static int s_arb_listo = 0;

void bpvm_bus_arbitro_init(void) {
    if (s_arb_listo) return;
    if (bpvm_platform_mutex_init(&s_arb_m) != 0) return;
    if (bpvm_platform_cond_init(&s_arb_c) != 0) { bpvm_platform_mutex_destroy(&s_arb_m); return; }
    s_arb_listo = 1;
}

/* «CS 10, un pack» / «0x58, el programa» / «el firmware». */
static void pinta_turno(int tipo, const turno_t* t, char* out, size_t cap) {
    if (t->cs < 0)                  snprintf(out, cap, "%s", de_quien(t->quien));
    else if (tipo == BPVM_BUS_I2C)  snprintf(out, cap, "0x%02X, %s", (unsigned) t->cs, de_quien(t->quien));
    else                            snprintf(out, cap, "CS %d, %s", t->cs, de_quien(t->quien));
}

int bpvm_bus_tomar(int tipo, int id, int quien, int cs, int plazo_ms, char* err, size_t cap) {
    turno_t* t;
    int64_t t0, ahora;
    if (!bus_de(tipo, id)) { snprintf(err, cap, "%s %d: no hay ese bus", tipo == BPVM_BUS_I2C ? "I2C" : "SPI", id); return -1; }
    bpvm_bus_arbitro_init();
    if (!s_arb_listo) { snprintf(err, cap, "el árbitro de los buses no pudo arrancar (sin mutex)"); return -1; }
    if (plazo_ms <= 0) plazo_ms = BPVM_BUS_PLAZO_MS;
    t = &s_turno[tipo][id];
    t0 = bpvm_platform_now_ms();
    bpvm_platform_mutex_lock(&s_arb_m);
#ifndef BPVM_SIN_ARBITRO
    {
        /* CORTESÍA: quien llega con el bus libre pero con otros ESPERANDO, no se cuela:
         * se pone a esperar también. Sin esto hay INANICIÓN —medido en test-arbitro:
         * quien suelta y vuelve a pedir enseguida gana siempre (ya está corriendo; el
         * que espera tiene que despertar), y el otro agotaba su plazo con cada turno
         * ajeno de 33 ms—. Es lo que pasaría en la C5 con los muchos turnos cortos de
         * un volcado de la pantalla frente a la TF.
         *
         * El plazo es del RELOJ. La guarda de `quietos` es sólo para un reloj que no
         * se mueve (las pruebas de un hilo lo tapan a cero y sus dobles «vencen» en el
         * acto): tres esperas vencidas seguidas sin que el reloj avance un ms no
         * pueden darse en una placa, y sin la guarda aquello no acabaría nunca. */
        int yo_espero = 0, quietos = 0;
        while (t->quien != 0 || (!yo_espero && t->esperan > 0)) {
            int vencio;
            if (!yo_espero) { yo_espero = 1; t->esperan++; }
            ahora = bpvm_platform_now_ms();
            if (ahora - t0 >= plazo_ms || quietos >= 3) {
                char suyo[40];
                t->esperan--;
                bpvm_platform_cond_broadcast(&s_arb_c);   /* el que venía detrás, que no espere a nadie */
                if (t->quien != 0) {
                    pinta_turno(tipo, t, suyo, sizeof suyo);
                    snprintf(err, cap, "%s %d ocupado (%s) desde hace %ld ms", NOMBRE[tipo], id, suyo, (long) (ahora - t->desde));
                } else {
                    snprintf(err, cap, "%s %d: no llegó el turno en %d ms (otros esperaban)", NOMBRE[tipo], id, plazo_ms);
                }
                bpvm_platform_mutex_unlock(&s_arb_m);
                return -1;
            }
            vencio = bpvm_platform_cond_timed_wait(&s_arb_c, &s_arb_m, (int) (plazo_ms - (ahora - t0))) == 1;
            quietos = (vencio && bpvm_platform_now_ms() == ahora) ? quietos + 1 : 0;
        }
        if (yo_espero) t->esperan--;
    }
#else
    /* SÓLO para el CONTROL de `make test-arbitro-control`: el árbitro no arbitra (da el
     * turno aunque esté ocupado). Con esto la prueba de los dos hilos TIENE que fallar;
     * si pasara, no probaría nada. Ninguna imagen se compila con esto. */
    (void) ahora;
#endif
    t->quien = (unsigned char) (quien ? quien : BPVM_BUS_PROGRAMA);
    t->cs = (short) cs;
    t->medir = 0;
    t->desde = bpvm_platform_now_ms();
    bpvm_platform_mutex_unlock(&s_arb_m);
    return 0;
}

/* G4.a3 — sin cerrojo: sólo lo escribe quien TIENE el turno, justo tras tomarlo, y
 * sólo lo lee `dejar`, del mismo hilo y ya con el cerrojo. */
void bpvm_bus_medir(int tipo, int id) {
    if (bus_de(tipo, id)) s_turno[tipo][id].medir = 1;
}

void bpvm_bus_dejar(int tipo, int id) {
    turno_t* t;
    long largo = -1;
    int tope = 0;
    char suyo[40];
    if (!bus_de(tipo, id) || !s_arb_listo) return;
    bpvm_platform_mutex_lock(&s_arb_m);
    t = &s_turno[tipo][id];
    /* G4.a3 — el turno A MANO se mide: si pasó del tope del bus, se dice (fuera del
     * cerrojo) con el nombre de quién. No se corta: dejaría un dispositivo a medias. */
    if (t->medir && t->quien != 0) {
        int64_t d = bpvm_platform_now_ms() - t->desde;
        tope = bpvm_bus_tope_ms(tipo, id);
        if (d > tope) { largo = (long) d; pinta_turno(tipo, t, suyo, sizeof suyo); }
    }
    t->medir = 0;
    t->quien = 0;
    bpvm_platform_cond_broadcast(&s_arb_c);
    bpvm_platform_mutex_unlock(&s_arb_m);
    if (largo >= 0)
        bpvm_diag("%s %d: %s tuvo el bus %ld ms, y el tope de un turno es %d ms", NOMBRE[tipo], id, suyo, largo, tope);
}

/* SIN cerrojo, a propósito: se pregunta en CADA transferencia (la TF va byte a
 * byte) y sólo vale para comprobar que HAY un turno en curso, no para decidir de
 * quién es ni para tomarlo. */
int bpvm_bus_tomado(int tipo, int id) {
    if (!bus_de(tipo, id)) return 0;
    return ((volatile turno_t*) &s_turno[tipo][id])->quien != 0;
}

void bpvm_bus_reset(void) {
    int tipo, id;
    for (tipo = 0; tipo < 2; tipo++)
        for (id = 0; id < BPVM_BUS_ID_MAX; id++)
            if (s_bus[tipo][id].origen != ORIGEN_LIBRE) suelta_pines(tipo, s_bus[tipo][id].p);
    memset(s_bus, 0, sizeof s_bus);
    memset(s_turno, 0, sizeof s_turno);   /* G4.a2: ningún turno en curso */
}
