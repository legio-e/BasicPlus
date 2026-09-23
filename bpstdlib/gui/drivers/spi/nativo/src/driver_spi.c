/* driver_spi.c — el DRIVER DE PANTALLA GENÉRICO (V7/H6.c G3): cualquier controlador
 * MIPI-DCS por SPI de 4 hilos (ST7789, ILI9341, GC9A01, ST7796…), descrito por
 * DATOS y no por código. Una placa nueva no necesita que nadie escriba C:
 *
 *   · la PLACA va en el ENV —una línea por dispositivo, con el formato de `sd=`—:
 *
 *       lcd=st7789,spi:2,sck:6,mosi:7,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv
 *       exp=ch32,i2c:0,sda:0,scl:1,addr:0x24
 *       touch=cst3530,addr:0x58,rst:x0
 *
 *     (ésa es la Waveshare ESP32-C5-Touch-LCD-2.8; el C6 sería
 *      `lcd=st7789,spi:2,sck:7,mosi:6,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv`)
 *
 *   · el CONTROLADOR va en un fichero de texto, `<controlador>.seq`: su secuencia de
 *     arranque, un comando por línea. Los que damos probados viajan DENTRO de este
 *     pack (`resources/`, se leen de `pack:DriverSpi/st7789.seq`); el de uno que no
 *     traemos lo escribe el usuario —copiando la secuencia del ejemplo del
 *     fabricante— y lo nombra con `seq:/sys/mio.seq`.
 *
 * Es la corrección de V7_IDEAS §H6.7.9: no son N drivers, es UN driver y N perfiles.
 * Lo que separa a un ST7789 de un ILI9341 es la secuencia de arranque y unos números;
 * ventana (CASET/RASET/RAMWR), giro (MADCTL) y bits (COLMOD) son el estándar, y los
 * pone este fichero.
 *
 * ─── LA LÍNEA `lcd=` ────────────────────────────────────────────────────────
 *
 *   <controlador>  el primer campo: el nombre del .seq (st7789 → st7789.seq)
 *   spi:N          el bus, en la numeración de Spi.Bus (2 = GPSPI2 en las ESP32)
 *   sck:N mosi:N   los pines del bus          dc:N cs:N   datos/comando y selección
 *   w:N h:N        el tamaño del cristal, a 0°
 *   opcionales:
 *   rst:P          el reset (sin él: SWRESET por el bus)
 *   bl:P           la luz (sin él: la enciende el hardware)
 *   mhz:N          el reloj del SPI (20 si no se dice)
 *   ram:WxH        la memoria del controlador, si es mayor que el cristal (el ST7789: 240x320)
 *   off:XxY        dónde cae el cristal dentro de esa memoria, a 0° (0x0 si no se dice)
 *   inv            colores invertidos (INVON): los IPS
 *   bgr            orden BGR: el bit 3 de MADCTL, lo hace el chip
 *   seq:CAMINO     la secuencia del controlador, de un fichero del usuario
 *   traza          escribe en el log cada comando del arranque
 *
 *   Un pin P es un GPIO (`21`), una salida del expansor (`x1`) o su PWM (`xpwm`,
 *   sólo para la luz).
 *
 * `exp=` (si algún pin es `xN`): el tipo (`ch32`, el CH32V003 de Waveshare con su
 * firmware de fábrica), `i2c`, `sda`, `scl`, `addr` y `khz` (400 si no se dice).
 * `touch=` (sin ella, la pantalla no tiene táctil): el tipo (`cst3530`), `addr`,
 * `rst` y, si no comparte el bus del expansor, `i2c`/`sda`/`scl`/`khz`.
 *
 * ⚠️ ESTRICTO A PROPÓSITO: una clave que no conozco es un ERROR con su nombre, no se
 * salta. La línea `sd=` las ignora porque una misma línea sirve a dos placas; ésta
 * no tiene esa excusa, y una errata en `rst` (o una clave de un driver más nuevo que
 * éste no sabe hacer) daría una pantalla negra sin una palabra. Por lo mismo, el
 * driver publica `estado`: la VM lo pregunta en el primer `Gui.Screen()` y el
 * programa recibe «GUI: pantalla: ENV lcd: falta 'dc'» en vez de nada.
 *
 * ─── EL DESFASE POR ORIENTACIÓN SALE DE DOS DATOS FÍSICOS ────────────────────
 *
 * El C6 tiene 80 filas de desfase en dos de sus cuatro giros y la C5 ninguno, con el
 * mismo chip: el cristal de 240×240 del C6 ocupa las filas 0..239 de una memoria de
 * 240×320, y al espejar las filas (MY) lo que se ve empieza en la 80. No hace falta
 * una tabla por giro: MX espeja las COLUMNAS de la memoria y MY sus FILAS (con MV o
 * sin él), así que con `ram` y `off` sale solo. Comprobado contra la tabla del C6
 * confirmada en placa (0/0, 0/0, 0/80, 80/0) y contra la de las pantallas de 135×240
 * (52/40, 40/53, 53/40, 40/52): lo verifica `make test-driverspi`.
 *
 * ─── EL ORDEN DEL ARRANQUE ──────────────────────────────────────────────────
 *
 *   1. los pines y los buses, reclamados como de pack (H4: con nombre si no se puede)
 *   2. el expansor, si hay
 *   3. los resets del panel y del táctil (o SWRESET, si el panel no tiene pin)
 *   4. el .seq del controlador: sólo lo PROPIO del chip
 *   5. lo de la PLACA, después: MADCTL (giro y RGB/BGR), COLMOD (16 bits), la
 *      inversión y DISPON. Si el .seq también lo manda, gana esto, que va detrás.
 *   6. la luz
 *
 * El volcado es el de DriverC5 (el que se vio pintar el 22-sep): ventana y los datos
 * troceados a `SPI_MAX`. `make test-driverspi` compara los dos drivers byte a byte
 * contra una BIOS que graba los cables. */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_pant.h"
#include <stdint.h>
#include <stddef.h>

const bpvm_bios_t* g_bios = 0;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

/* El .seq de fábrica se lee de `pack:<este pack>/<controlador>.seq`. */
#define PACK_NOMBRE  "DriverSpi"

/* ── texto sin libc (un pack no tiene snprintf) ───────────────────────────── */
typedef struct { char* b; unsigned cap, n; } txt_t;
static void t_ini(txt_t* t, char* b, unsigned cap) { t->b = b; t->cap = cap; t->n = 0; if (cap) b[0] = 0; }
static void t_c(txt_t* t, char c) { if (t->n + 1 < t->cap) { t->b[t->n++] = c; t->b[t->n] = 0; } }
static void t_s(txt_t* t, const char* s) { while (s && *s) t_c(t, *s++); }
static void t_r(txt_t* t, const char* a, const char* z) { while (a < z) t_c(t, *a++); }
static void t_d(txt_t* t, long v) {
    char d[12]; int i = 0; unsigned long u;
    if (v < 0) { t_c(t, '-'); u = (unsigned long) (-v); } else u = (unsigned long) v;
    do { d[i++] = (char) ('0' + (int) (u % 10u)); u /= 10u; } while (u && i < 12);
    while (i) t_c(t, d[--i]);
}
static void t_x2(txt_t* t, unsigned v) {
    static const char H[] = "0123456789ABCDEF";
    t_c(t, H[(v >> 4) & 15u]); t_c(t, H[v & 15u]);
}

static char s_log[128];
static char s_motivo[80];            /* por qué la config no vale ("" = vale) */

/* Deja el motivo: a + [k0,k1) + c. Devuelve -1, para `return falla(…)`. */
static int falla(const char* a, const char* k0, const char* k1, const char* c) {
    txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
    t_s(&t, a); if (k0) t_r(&t, k0, k1); t_s(&t, c);
    return -1;
}
static void di_motivo(void) {
    txt_t t; t_ini(&t, s_log, sizeof s_log);
    t_s(&t, "driver SPI: "); t_s(&t, s_motivo);
    di(s_log);
}

/* ── la línea del ENV, campo a campo ──────────────────────────────────────── */

/* El siguiente campo de `*p` (separados por comas, sin blancos alrededor) en
 * [*a,*z). 0 = no quedan. */
static int campo(const char** p, const char** a, const char** z) {
    const char* s = *p;
    while (*s == ' ' || *s == '\t' || *s == ',') s++;
    if (!*s) { *p = s; return 0; }
    *a = s;
    while (*s && *s != ',') s++;
    *z = s;
    while (*z > *a && ((*z)[-1] == ' ' || (*z)[-1] == '\t')) (*z)--;
    *p = s;
    return 1;
}
static char minus(char c) { return (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c; }
/* ¿[a,z) es `k`? Sin distinguir mayúsculas, como las claves del ENV. */
static int es(const char* a, const char* z, const char* k) {
    while (a < z && *k) { if (minus(*a) != *k) return 0; a++; k++; }
    return a == z && *k == 0;
}
/* Un número decimal, o hexadecimal con 0x. -1 = no lo es. */
static int numero(const char* a, const char* z, long* out) {
    long v = 0; int base = 10;
    if (a >= z) return -1;
    if (z - a > 2 && a[0] == '0' && (a[1] == 'x' || a[1] == 'X')) { base = 16; a += 2; }
    for (; a < z; a++) {
        char c = minus(*a); int d;
        if (c >= '0' && c <= '9')                   d = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else return -1;
        v = v * base + d;
        if (v > 100000000L) return -1;
    }
    *out = v;
    return 0;
}
/* `ANCHOxALTO` (también vale para `off:XxY`). */
static int medida(const char* a, const char* z, int* w, int* h) {
    const char* x = a; long v1, v2;
    while (x < z && *x != 'x' && *x != 'X') x++;
    if (x >= z || numero(a, x, &v1) != 0 || numero(x + 1, z, &v2) != 0) return -1;
    if (v1 > 4096 || v2 > 4096) return -1;
    *w = (int) v1; *h = (int) v2;
    return 0;
}

/* Un pin de la placa: un GPIO, una salida del expansor o su PWM. */
enum { PIN_NO = 0, PIN_GPIO, PIN_EXIO, PIN_EXPWM };
typedef struct { int tipo; int n; } pin_t;
static int pin_de(const char* a, const char* z, pin_t* p) {
    long v;
    if (a < z && minus(*a) == 'x') {
        if (es(a, z, "xpwm")) { p->tipo = PIN_EXPWM; p->n = 0; return 0; }
        if (numero(a + 1, z, &v) != 0 || v > 7) return -1;
        p->tipo = PIN_EXIO; p->n = (int) v;
        return 0;
    }
    if (numero(a, z, &v) != 0 || v > 127) return -1;
    p->tipo = PIN_GPIO; p->n = (int) v;
    return 0;
}
static void t_pin(txt_t* t, pin_t p) {
    if (p.tipo == PIN_GPIO)       { t_s(t, "GPIO "); t_d(t, p.n); }
    else if (p.tipo == PIN_EXIO)  { t_s(t, "x"); t_d(t, p.n); }
    else if (p.tipo == PIN_EXPWM) t_s(t, "xpwm");
    else                          t_s(t, "-");
}

/* ── LA CONFIGURACIÓN ─────────────────────────────────────────────────────── */

static struct {
    char ctrl[16];                   /* el controlador: da el nombre del .seq */
    int  spi, sck, mosi, dc, cs, mhz;
    pin_t rst, bl;
    int  w, h, ram_w, ram_h, off_x, off_y;
    int  inv, bgr, traza;
    char seq[64];                    /* "" = el de fábrica, dentro del pack */
} L;
static struct { int hay; int i2c, sda, scl, khz, addr; } X;          /* exp=   */
static struct { int hay; int i2c, sda, scl, khz, addr; pin_t rst; } T; /* touch= */

static char s_env[160];              /* el valor de una línea del ENV */

/* 1 = la clave está (en s_env), 0 = no está, -1 = no se puede leer (motivo). */
static int lee_env(const char* clave) {
    int n = g_bios->env_get(clave, s_env, sizeof s_env);
    if (n >= 0) return 1;
    if (n == BPVM_BIOS_ENV_NO_ESTA) return 0;
    if (n == BPVM_BIOS_ENV_NO_CABE) return falla("ENV ", clave, clave + g_bios->strlen(clave), ": la linea es demasiado larga");
    return falla("ENV ", clave, clave + g_bios->strlen(clave), ": no se puede leer");
}

/* El primer campo de una línea: el TIPO (st7789, ch32, cst3530), sin ':'. */
static int tipo_de(const char** p, const char* clave, char* dst, unsigned cap) {
    const char *a, *z, *q;
    unsigned i = 0;
    if (!campo(p, &a, &z)) return falla("ENV ", clave, clave + g_bios->strlen(clave), ": esta vacia");
    for (q = a; q < z; q++) if (*q == ':') break;
    if (q < z) return falla("ENV ", clave, clave + g_bios->strlen(clave), ": el primer campo es el tipo, sin ':'");
    for (q = a; q < z; q++) {
        char c = minus(*q);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-') || i + 1 >= cap)
            return falla("ENV ", clave, clave + g_bios->strlen(clave), ": tipo no valido");
        dst[i++] = c;
    }
    dst[i] = 0;
    return 0;
}

static int parse_lcd(void) {
    const char* p = s_env; const char *a, *z;
    L.spi = L.sck = L.mosi = L.dc = L.cs = -1;
    L.mhz = 20;
    L.rst.tipo = L.bl.tipo = PIN_NO; L.rst.n = L.bl.n = 0;
    L.w = L.h = 0; L.ram_w = L.ram_h = 0; L.off_x = L.off_y = 0;
    L.inv = L.bgr = L.traza = 0;
    L.seq[0] = 0;
    if (tipo_de(&p, "lcd", L.ctrl, sizeof L.ctrl) != 0) return -1;
    while (campo(&p, &a, &z)) {
        const char* k = a; const char* v = a; long n; int* dst = 0; long max = 0;
        while (v < z && *v != ':') v++;
        if (v == z) {                                   /* una bandera */
            if      (es(a, z, "inv"))   L.inv = 1;
            else if (es(a, z, "bgr"))   L.bgr = 1;
            else if (es(a, z, "traza")) L.traza = 1;
            else return falla("ENV lcd: no conozco '", a, z, "'");
            continue;
        }
        k = v++;                                        /* la clave es [a,k), el valor [v,z) */
        if (es(a, k, "rst")) {
            if (pin_de(v, z, &L.rst) != 0 || L.rst.tipo == PIN_EXPWM) return falla("ENV lcd: 'rst' no es un pin: '", v, z, "'");
            continue;
        }
        if (es(a, k, "bl")) {
            if (pin_de(v, z, &L.bl) != 0) return falla("ENV lcd: 'bl' no es un pin: '", v, z, "'");
            continue;
        }
        if (es(a, k, "ram")) {
            if (medida(v, z, &L.ram_w, &L.ram_h) != 0) return falla("ENV lcd: 'ram' es ANCHOxALTO (240x320)", 0, 0, "");
            continue;
        }
        if (es(a, k, "off")) {
            if (medida(v, z, &L.off_x, &L.off_y) != 0) return falla("ENV lcd: 'off' es XxY (0x80)", 0, 0, "");
            continue;
        }
        if (es(a, k, "seq")) {
            unsigned i = 0;
            if ((unsigned) (z - v) + 1 > sizeof L.seq || v == z) return falla("ENV lcd: 'seq' vacio o demasiado largo", 0, 0, "");
            while (v < z) L.seq[i++] = *v++;
            L.seq[i] = 0;
            continue;
        }
        if      (es(a, k, "spi"))  { dst = &L.spi;  max = 9;    }
        else if (es(a, k, "sck"))  { dst = &L.sck;  max = 127;  }
        else if (es(a, k, "mosi")) { dst = &L.mosi; max = 127;  }
        else if (es(a, k, "dc"))   { dst = &L.dc;   max = 127;  }
        else if (es(a, k, "cs"))   { dst = &L.cs;   max = 127;  }
        else if (es(a, k, "mhz"))  { dst = &L.mhz;  max = 80;   }
        else if (es(a, k, "w"))    { dst = &L.w;    max = 4096; }
        else if (es(a, k, "h"))    { dst = &L.h;    max = 4096; }
        else return falla("ENV lcd: no conozco '", a, k, "'");
        if (numero(v, z, &n) != 0 || n > max) return falla("ENV lcd: '", a, k, "' no es un numero valido");
        *dst = (int) n;
    }
    /* El chivato que justifica las etiquetas: decir CUÁL falta. */
    if (L.spi  < 0) return falla("ENV lcd: falta 'spi'", 0, 0, "");
    if (L.sck  < 0) return falla("ENV lcd: falta 'sck'", 0, 0, "");
    if (L.mosi < 0) return falla("ENV lcd: falta 'mosi'", 0, 0, "");
    if (L.dc   < 0) return falla("ENV lcd: falta 'dc'", 0, 0, "");
    if (L.cs   < 0) return falla("ENV lcd: falta 'cs'", 0, 0, "");
    if (L.w <= 0 || L.h <= 0) return falla("ENV lcd: falta el tamano ('w' y 'h')", 0, 0, "");
    if (L.mhz <= 0) return falla("ENV lcd: 'mhz' no puede ser 0", 0, 0, "");
    if (L.ram_w == 0 && L.ram_h == 0) { L.ram_w = L.w; L.ram_h = L.h; }
    if (L.off_x + L.w > L.ram_w || L.off_y + L.h > L.ram_h)
        return falla("ENV lcd: el cristal (w, h, off) no cabe en 'ram'", 0, 0, "");
    return 0;
}

/* `exp=` y `touch=` comparten las claves del bus: cada una pasa sus campos. */
static int parse_i2c(const char* clave, const char* a, const char* k, const char* v, const char* z,
                     int* i2c, int* sda, int* scl, int* khz, int* addr) {
    long n; int* dst; long max;
    if      (es(a, k, "i2c"))  { dst = i2c;  max = 9;    }
    else if (es(a, k, "sda"))  { dst = sda;  max = 127;  }
    else if (es(a, k, "scl"))  { dst = scl;  max = 127;  }
    else if (es(a, k, "khz"))  { dst = khz;  max = 1000; }
    else if (es(a, k, "addr")) { dst = addr; max = 127;  }
    else {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV "); t_s(&t, clave); t_s(&t, ": no conozco '"); t_r(&t, a, k); t_s(&t, "'");
        return -1;
    }
    if (numero(v, z, &n) != 0 || n > max || (n == 0 && (dst == khz || dst == addr))) {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV "); t_s(&t, clave); t_s(&t, ": '"); t_r(&t, a, k); t_s(&t, "' no es un numero valido");
        return -1;
    }
    *dst = (int) n;
    return 0;
}

static int parse_exp(void) {
    const char* p = s_env; const char *a, *z; char tipo[12];
    X.i2c = X.sda = X.scl = X.addr = -1; X.khz = 400;
    if (tipo_de(&p, "exp", tipo, sizeof tipo) != 0) return -1;
    if (!es(tipo, tipo + g_bios->strlen(tipo), "ch32"))
        return falla("ENV exp: no conozco el expansor '", tipo, tipo + g_bios->strlen(tipo), "' (hay: ch32)");
    while (campo(&p, &a, &z)) {
        const char* v = a;
        while (v < z && *v != ':') v++;
        if (v == z) return falla("ENV exp: no conozco '", a, z, "'");
        if (parse_i2c("exp", a, v, v + 1, z, &X.i2c, &X.sda, &X.scl, &X.khz, &X.addr) != 0) return -1;
    }
    if (X.i2c < 0)  return falla("ENV exp: falta 'i2c'", 0, 0, "");
    if (X.sda < 0)  return falla("ENV exp: falta 'sda'", 0, 0, "");
    if (X.scl < 0)  return falla("ENV exp: falta 'scl'", 0, 0, "");
    if (X.addr < 0) return falla("ENV exp: falta 'addr'", 0, 0, "");
    return 0;
}

static int parse_touch(void) {
    const char* p = s_env; const char *a, *z; char tipo[12];
    T.i2c = T.sda = T.scl = T.addr = -1; T.khz = -1;
    T.rst.tipo = PIN_NO; T.rst.n = 0;
    if (tipo_de(&p, "touch", tipo, sizeof tipo) != 0) return -1;
    if (!es(tipo, tipo + g_bios->strlen(tipo), "cst3530"))
        return falla("ENV touch: no conozco el tactil '", tipo, tipo + g_bios->strlen(tipo), "' (hay: cst3530)");
    while (campo(&p, &a, &z)) {
        const char* v = a;
        while (v < z && *v != ':') v++;
        if (v == z) return falla("ENV touch: no conozco '", a, z, "'");
        if (es(a, v, "rst")) {
            if (pin_de(v + 1, z, &T.rst) != 0 || T.rst.tipo == PIN_EXPWM) return falla("ENV touch: 'rst' no es un pin: '", v + 1, z, "'");
            continue;
        }
        if (parse_i2c("touch", a, v, v + 1, z, &T.i2c, &T.sda, &T.scl, &T.khz, &T.addr) != 0) return -1;
    }
    if (T.addr < 0) return falla("ENV touch: falta 'addr'", 0, 0, "");
    if (T.i2c < 0 && T.sda < 0 && T.scl < 0) {         /* el bus del expansor */
        if (!X.hay) return falla("ENV touch: falta 'i2c' (o una linea 'exp' con su bus)", 0, 0, "");
        T.i2c = X.i2c; T.sda = X.sda; T.scl = X.scl;
        if (T.khz < 0) T.khz = X.khz;
    }
    if (T.i2c < 0 || T.sda < 0 || T.scl < 0) return falla("ENV touch: el bus es 'i2c', 'sda' y 'scl', los tres", 0, 0, "");
    if (T.khz < 0) T.khz = 400;
    if (X.hay && T.i2c == X.i2c && (T.sda != X.sda || T.scl != X.scl || T.khz != X.khz))
        return falla("ENV touch y exp: el mismo i2c con otros pines", 0, 0, "");
    return 0;
}

/* ── el bus: comandos con DC en bajo, datos con DC en alto (lo de DriverC5) ── */
static void lcd_cmd(uint8_t c) {
    g_bios->gpio_write(L.dc, 0); g_bios->gpio_write(L.cs, 0);
    g_bios->spi_write(L.spi, &c, 1);
    g_bios->gpio_write(L.cs, 1);
}
/* ⚠️ TROCEADO A `SPI_MAX`, y no es una precaución: la fachada SPI del ESP32 abre
 * el bus SIN DMA, así que una transacción no pasa del búfer del periférico —64
 * bytes—. La palanca, si molesta, es la FACHADA (G4), no este fichero. */
#define SPI_MAX 64
static void lcd_dat(const uint8_t* d, size_t n) {
    g_bios->gpio_write(L.dc, 1); g_bios->gpio_write(L.cs, 0);
    while (n > 0) {
        size_t t = (n > SPI_MAX) ? SPI_MAX : n;
        g_bios->spi_write(L.spi, d, t);
        d += t; n -= t;
    }
    g_bios->gpio_write(L.cs, 1);
}
static void lcd_cmd1(uint8_t c, uint8_t p) { lcd_cmd(c); lcd_dat(&p, 1); }

/* El desfase de la orientación ACTUAL (ver la cabecera). */
static int s_gap_x = 0, s_gap_y = 0;
static void lcd_ventana(int x1, int y1, int x2, int y2) {
    uint8_t c[4], r[4];
    x1 += s_gap_x; x2 += s_gap_x; y1 += s_gap_y; y2 += s_gap_y;
    c[0] = (uint8_t) (x1 >> 8); c[1] = (uint8_t) x1; c[2] = (uint8_t) (x2 >> 8); c[3] = (uint8_t) x2;
    r[0] = (uint8_t) (y1 >> 8); r[1] = (uint8_t) y1; r[2] = (uint8_t) (y2 >> 8); r[3] = (uint8_t) y2;
    lcd_cmd(0x2A); lcd_dat(c, 4);          /* CASET */
    lcd_cmd(0x2B); lcd_dat(r, 4);          /* RASET */
    lcd_cmd(0x2C);                         /* RAMWR */
}

/* ── el expansor CH32V003 de Waveshare (su firmware de fábrica) ───────────────
 * Registros: 0x02 dirección (1 = salida), 0x03 salidas, 0x05 PWM de la luz.
 * Pone los ocho EXIO como salida —lo que hace el BSP de Waveshare, y lo que hacía
 * DriverC5—, y lleva una copia de las salidas para tocar un bit sin leer. */
#define CH32_REG_DIR  0x02
#define CH32_REG_OUT  0x03
#define CH32_REG_PWM  0x05
static uint8_t s_exp_out = 0;
static int exp_escribe(uint8_t reg, uint8_t v) {
    uint8_t d[2];
    d[0] = reg; d[1] = v;
    return g_bios->i2c_write(X.i2c, X.addr, d, 2) == 2 ? 0 : -1;
}
static int exp_init(void) {
    if (exp_escribe(CH32_REG_DIR, 0xFF) != 0) return -1;
    s_exp_out = 0;
    return exp_escribe(CH32_REG_OUT, s_exp_out);
}
static void exp_pin(int n, int v) {
    if (v) s_exp_out = (uint8_t) (s_exp_out | (1u << n));
    else   s_exp_out = (uint8_t) (s_exp_out & ~(1u << n));
    exp_escribe(CH32_REG_OUT, s_exp_out);
}

static void pin_pon(pin_t p, int v) {
    if (p.tipo == PIN_GPIO)       g_bios->gpio_write(p.n, v);
    else if (p.tipo == PIN_EXIO)  exp_pin(p.n, v);
    else if (p.tipo == PIN_EXPWM) exp_escribe(CH32_REG_PWM, (uint8_t) (v ? 255 : 0));
}

/* ── el .seq: la secuencia del controlador, como TEXTO ────────────────────────
 *
 *     # comentario
 *     11              un comando sin parámetros (SLPOUT)
 *     espera 100      milisegundos
 *     B0 00 F0        el comando y sus parámetros, en hexadecimal (0x y comas valen)
 *
 * Se lee en DOS pasadas con la misma rutina: `enviar = 0` sólo comprueba (lo hace
 * `estado`, antes de que el programa cree su screen) y `enviar = 1` lo manda. */
#define SEQ_PARAMS 48
static char    s_linea[160];
static uint8_t s_trozo[64];
static uint8_t s_par[SEQ_PARAMS];
static char    s_camino[96];
static int     s_seq_cmds = 0;

static const char* base_de(const char* c) {
    const char* b = c;
    for (; *c; c++) if (*c == '/' || *c == '\\') b = c + 1;
    return b;
}
static int falla_seq(int linea, const char* que, const char* k0, const char* k1, const char* fin) {
    txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
    t_s(&t, base_de(s_camino)); t_s(&t, " linea "); t_d(&t, linea); t_s(&t, ": ");
    t_s(&t, que); if (k0) t_r(&t, k0, k1); t_s(&t, fin);
    return -1;
}
static int hexdig(char c) {
    c = minus(c);
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int seq_linea(int enviar, int linea) {
    char* s = s_linea; char* q; int np = 0, cmd = -1;
    for (q = s; *q; q++) if (*q == '#') { *q = 0; break; }
    while (*s == ' ' || *s == '\t') s++;
    if (!*s) return 0;
    {   /* «espera N» */
        const char* e = s;
        while (*e && *e != ' ' && *e != '\t') e++;
        if (es(s, e, "espera")) {
            const char* a = e; const char* z; long ms;
            while (*a == ' ' || *a == '\t') a++;
            z = a; while (*z && *z != ' ' && *z != '\t') z++;
            if (numero(a, z, &ms) != 0 || ms > 2000) return falla_seq(linea, "'espera' quiere milisegundos (0..2000)", 0, 0, "");
            if (enviar) {
                if (L.traza) { txt_t t; t_ini(&t, s_log, sizeof s_log); t_s(&t, "driver SPI: seq espera "); t_d(&t, ms); di(s_log); }
                g_bios->sleep_ms((int) ms);
            }
            return 0;
        }
    }
    while (*s) {                                        /* los bytes */
        const char *a, *z; int h, l, v;
        while (*s == ' ' || *s == '\t' || *s == ',') s++;
        if (!*s) break;
        a = s; while (*s && *s != ' ' && *s != '\t' && *s != ',') s++;
        z = s;
        if (z - a > 2 && a[0] == '0' && minus(a[1]) == 'x') a += 2;
        if (z - a == 1)      { h = 0; l = hexdig(a[0]); }
        else if (z - a == 2) { h = hexdig(a[0]); l = hexdig(a[1]); }
        else                 { h = l = -1; }
        if (h < 0 || l < 0) return falla_seq(linea, "'", a, z, "' no es un byte (hex)");
        v = h * 16 + l;
        if (cmd < 0) cmd = v;
        else if (np >= SEQ_PARAMS) return falla_seq(linea, "mas de 48 parametros", 0, 0, "");
        else s_par[np++] = (uint8_t) v;
    }
    if (cmd < 0) return 0;                              /* sólo separadores */
    if (enviar) {
        if (L.traza) {
            txt_t t; int i;
            t_ini(&t, s_log, sizeof s_log);
            t_s(&t, "driver SPI: seq "); t_x2(&t, (unsigned) cmd);
            for (i = 0; i < np && i < 8; i++) { t_c(&t, ' '); t_x2(&t, s_par[i]); }
            if (np > 8) { t_s(&t, " ... ("); t_d(&t, np); t_s(&t, " B)"); }
            di(s_log);
        }
        lcd_cmd((uint8_t) cmd);
        if (np) lcd_dat(s_par, (size_t) np);
    }
    s_seq_cmds++;
    return 0;
}

static int seq(int enviar) {
    txt_t t; int fd; long tam; uint32_t pos = 0; unsigned n = 0; int linea = 1;
    t_ini(&t, s_camino, sizeof s_camino);
    if (L.seq[0]) t_s(&t, L.seq);
    else { t_s(&t, "pack:" PACK_NOMBRE "/"); t_s(&t, L.ctrl); t_s(&t, ".seq"); }
    s_seq_cmds = 0;
    fd = g_bios->abrir(s_camino, 0);
    if (fd < 0) {
        if (L.seq[0]) return falla("no encuentro ", s_camino, s_camino + t.n, "");
        return falla("no conozco '", L.ctrl, L.ctrl + g_bios->strlen(L.ctrl), "': no hay su .seq en el pack");
    }
    tam = g_bios->tamano(fd);
    if (tam < 0) { g_bios->cerrar(fd); return falla("no puedo leer ", base_de(s_camino), base_de(s_camino) + g_bios->strlen(base_de(s_camino)), ""); }
    while ((long) pos < tam) {
        long k = g_bios->leer(fd, pos, s_trozo, sizeof s_trozo), i;
        if (k <= 0) { g_bios->cerrar(fd); return falla("no puedo leer ", base_de(s_camino), base_de(s_camino) + g_bios->strlen(base_de(s_camino)), ""); }
        for (i = 0; i < k; i++) {
            char c = (char) s_trozo[i];
            if (c == '\n') {
                s_linea[n] = 0;
                if (seq_linea(enviar, linea) != 0) { g_bios->cerrar(fd); return -1; }
                n = 0; linea++;
            } else if (c != '\r') {
                if (n + 1 >= sizeof s_linea) { g_bios->cerrar(fd); return falla_seq(linea, "demasiado larga", 0, 0, ""); }
                s_linea[n++] = c;
            }
        }
        pos += (uint32_t) k;
    }
    g_bios->cerrar(fd);
    if (n) { s_linea[n] = 0; if (seq_linea(enviar, linea) != 0) return -1; }
    if (s_seq_cmds == 0) return falla(base_de(s_camino), 0, 0, ": no tiene ningun comando");
    return 0;
}

/* ── leer y comprobar TODO, una vez ───────────────────────────────────────────
 * Hasta que vale se vuelve a leer en cada llamada: si el programa falla con
 * «falta 'dc'» y el usuario arregla el ENV, el siguiente RUN ya lo ve (los
 * pines no se han tomado: eso es de `init`). Una vez vale, se queda. */
static int s_ok = 0;
static int config(void) {
    int r;
    if (s_ok) return 0;
    if (!g_bios) return -1;
    s_motivo[0] = 0;
    X.hay = T.hay = 0;
    r = lee_env("lcd");
    if (r < 0) return -1;
    if (r == 0) return falla("ENV: falta la linea 'lcd' (la pantalla)", 0, 0, "");
    if (parse_lcd() != 0) return -1;
    r = lee_env("exp");
    if (r < 0) return -1;
    if (r == 1) { if (parse_exp() != 0) return -1; X.hay = 1; }
    r = lee_env("touch");
    if (r < 0) return -1;
    if (r == 1) { if (parse_touch() != 0) return -1; T.hay = 1; }
    if (!X.hay && (L.rst.tipo >= PIN_EXIO || L.bl.tipo >= PIN_EXIO || (T.hay && T.rst.tipo >= PIN_EXIO)))
        return falla("ENV lcd: un pin 'x' pide la linea 'exp' (el expansor)", 0, 0, "");
    if (seq(0) != 0) return -1;
    s_ok = 1;
    {
        txt_t t; t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: "); t_s(&t, L.ctrl); t_c(&t, ' '); t_d(&t, L.w); t_c(&t, 'x'); t_d(&t, L.h);
        t_s(&t, " por SPI"); t_d(&t, L.spi); t_s(&t, " a "); t_d(&t, L.mhz); t_s(&t, " MHz (sck "); t_d(&t, L.sck);
        t_s(&t, ", mosi "); t_d(&t, L.mosi); t_s(&t, ", dc "); t_d(&t, L.dc); t_s(&t, ", cs "); t_d(&t, L.cs);
        t_s(&t, "), reset "); t_pin(&t, L.rst); t_s(&t, ", luz "); t_pin(&t, L.bl);
        di(s_log);
        t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: "); t_d(&t, s_seq_cmds); t_s(&t, " comandos en "); t_s(&t, base_de(s_camino));
        if (X.hay) { t_s(&t, "; expansor ch32 en I2C "); t_d(&t, X.i2c); }
        if (T.hay) { t_s(&t, "; tactil cst3530, reset "); t_pin(&t, T.rst); }
        else       t_s(&t, "; sin tactil");
        di(s_log);
    }
    return 0;
}

/* ── LO QUE SE PUBLICA BAJO 'PANT' ─────────────────────────────────────────── */

static const char* sp_estado(void) { return config() == 0 ? 0 : s_motivo; }

static int sp_size(int* w, int* h) {
    if (config() != 0) { di_motivo(); return 0; }
    *w = L.w; *h = L.h;
    return 1;
}

/* Un controlador por SPI quiere el byte alto primero: se lo pide a LVGL, que gira
 * los bytes con su rutina antes del flush. Aquí sólo se mandan. */
static int sp_formato(void) { return BPVM_PANT_RGB565_BE; }

/* MADCTL de un giro, y su desfase (ver la cabecera: MX espeja las columnas de la
 * memoria y MY sus filas; MV cambia qué eje de la pantalla recorre cada una). */
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_BGR 0x08
static int orienta(int deg, uint8_t* madctl) {
    int mv, mx, my, gc, gr;
    uint8_t m;
    switch (deg) {
        case 0:   m = 0;                     break;
        case 90:  m = MADCTL_MV | MADCTL_MX; break;
        case 180: m = MADCTL_MX | MADCTL_MY; break;
        case 270: m = MADCTL_MV | MADCTL_MY; break;
        default:  return -1;
    }
    mv = (m & MADCTL_MV) != 0; mx = (m & MADCTL_MX) != 0; my = (m & MADCTL_MY) != 0;
    gc = mx ? L.ram_w - L.off_x - L.w : L.off_x;      /* en las columnas de la memoria */
    gr = my ? L.ram_h - L.off_y - L.h : L.off_y;      /* en sus filas */
    s_gap_x = mv ? gr : gc;
    s_gap_y = mv ? gc : gr;
    *madctl = (uint8_t) (m | (L.bgr ? MADCTL_BGR : 0));
    return 0;
}

static int s_touch_ok = 0;

static int sp_init(void) {
    uint8_t m = 0;
    txt_t t;
    if (config() != 0) { di_motivo(); return -1; }
    /* ⚠️ MIGAS DE PAN, como en DriverC5: aquí dentro no hay depurador, y un
     * arranque de pantalla es donde una placa se reinicia sin decir nada. */
    di("driver SPI — entrando");

    /* 1. Los pines, reclamados como DE PACK (H4: con nombre si no se puede). */
    if (g_bios->gpio_init(L.dc, 1) != 0 || g_bios->gpio_init(L.cs, 1) != 0) {
        t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: la placa no da dc/cs (GPIO "); t_d(&t, L.dc); t_c(&t, '/'); t_d(&t, L.cs); t_c(&t, ')');
        di(s_log); return -1;
    }
    g_bios->gpio_write(L.cs, 1);
    if ((L.rst.tipo == PIN_GPIO && g_bios->gpio_init(L.rst.n, 1) != 0) ||
        (L.bl.tipo  == PIN_GPIO && g_bios->gpio_init(L.bl.n, 1) != 0)  ||
        (T.hay && T.rst.tipo == PIN_GPIO && g_bios->gpio_init(T.rst.n, 1) != 0)) {
        di("driver SPI: la placa no da un pin de reset o de la luz"); return -1;
    }
    if (L.rst.tipo == PIN_GPIO) g_bios->gpio_write(L.rst.n, 1);
    if (L.bl.tipo  == PIN_GPIO) g_bios->gpio_write(L.bl.n, 0);      /* apagada hasta que haya imagen */
    if (g_bios->spi_init(L.spi, L.sck, L.mosi, -1, L.mhz * 1000000, 0) != 0) {
        t_ini(&t, s_log, sizeof s_log); t_s(&t, "driver SPI: la placa no da el SPI "); t_d(&t, L.spi);
        di(s_log); return -2;
    }
    if ((X.hay && g_bios->i2c_init(X.i2c, X.sda, X.scl, X.khz * 1000) != 0) ||
        (T.hay && !(X.hay && T.i2c == X.i2c) && g_bios->i2c_init(T.i2c, T.sda, T.scl, T.khz * 1000) != 0)) {
        di("driver SPI: la placa no da el I2C del expansor o del tactil"); return -3;
    }
    di("driver SPI — pines y buses tomados");

    /* 2. El expansor. */
    if (X.hay && exp_init() != 0) {
        t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: el expansor ch32 no contesta en I2C 0x"); t_x2(&t, (unsigned) X.addr);
        t_s(&t, " (sin el, ni reset ni luz)");
        di(s_log); return -4;
    }

    /* 3. Los resets, el del panel y el del táctil a la vez: 10 ms abajo, y 120 ms
     *    hasta el primer comando (lo que pide el ST7789 antes de SLPOUT; y le da
     *    tiempo al táctil a arrancar). Sin pin de reset, por el bus. */
    if (L.rst.tipo != PIN_NO || (T.hay && T.rst.tipo != PIN_NO)) {
        pin_pon(L.rst, 0); if (T.hay) pin_pon(T.rst, 0);
        g_bios->sleep_ms(10);
        if (T.hay) pin_pon(T.rst, 1);
        pin_pon(L.rst, 1);
        g_bios->sleep_ms(120);
    }
    if (L.rst.tipo == PIN_NO) { lcd_cmd(0x01); g_bios->sleep_ms(120); }   /* SWRESET */
    di("driver SPI — resets dados");

    /* 4. El controlador: su .seq. */
    if (seq(1) != 0) { di_motivo(); return -5; }

    /* 5. Lo de la placa, DESPUÉS del .seq: si él también lo manda, gana esto. */
    orienta(0, &m);
    lcd_cmd1(0x36, m);                        /* MADCTL: giro 0 y RGB/BGR */
    lcd_cmd1(0x3A, 0x55);                     /* COLMOD: RGB565 */
    lcd_cmd(L.inv ? 0x21 : 0x20);             /* INVON / INVOFF */
    lcd_cmd(0x29);                            /* DISPON */
    if (L.traza) {
        t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: placa 36 "); t_x2(&t, m); t_s(&t, " · 3A 55 · "); t_s(&t, L.inv ? "21" : "20"); t_s(&t, " · 29");
        di(s_log);
    }

    /* 6. La luz. */
    pin_pon(L.bl, 1);
    s_touch_ok = T.hay;
    t_ini(&t, s_log, sizeof s_log);
    t_s(&t, "driver SPI: "); t_s(&t, L.ctrl); t_c(&t, ' '); t_d(&t, L.w); t_c(&t, 'x'); t_d(&t, L.h);
    t_s(&t, T.hay ? " con tactil" : " sin tactil"); t_s(&t, " — listo");
    di(s_log);
    return 0;
}

/* La zona ya viene en el orden de bytes que pedimos (`formato`). */
static void sp_flush(int x1, int y1, int x2, int y2, const uint8_t* px) {
    lcd_ventana(x1, y1, x2, y2);
    lcd_dat(px, (size_t) (x2 - x1 + 1) * (size_t) (y2 - y1 + 1) * 2u);
}

static int sp_rotate(int deg) {
    uint8_t m;
    if (orienta(deg, &m) != 0) return -1;
    lcd_cmd1(0x36, m);
    return 0;
}

/* ── el táctil CST3530 ─────────────────────────────────────────────────────────
 * Registro de 32 bits big-endian: DATA 0xD0070000 (9 bytes), END 0xD00002AB. Las
 * coordenadas son las de 0°: girarlas con la pantalla sigue pendiente (H6.a). */
static int cst_lee(uint32_t reg, uint8_t* d, size_t n) {
    uint8_t a[4];
    a[0] = (uint8_t) (reg >> 24); a[1] = (uint8_t) (reg >> 16); a[2] = (uint8_t) (reg >> 8); a[3] = (uint8_t) reg;
    if (g_bios->i2c_write(T.i2c, T.addr, a, 4) != 4) return -1;
    return g_bios->i2c_read(T.i2c, T.addr, d, n) == (int) n ? 0 : -1;
}
static void cst_fin(void) {
    uint8_t a[4];
    a[0] = 0xD0; a[1] = 0x00; a[2] = 0x02; a[3] = 0xAB;   /* END READ */
    g_bios->i2c_write(T.i2c, T.addr, a, 4);
}
/* Una lectura que falla es «sin dedo» —el táctil se sondea cada 30 ms y un NACK
 * suelto no es para parar la GUI—, pero NO en silencio: la primera se dice. Sin
 * esto, un táctil mal cableado o en otra dirección es una pantalla que «no
 * responde» sin una palabra (DriverC5 era así). */
static int s_touch_fallos = 0;
static int sp_touch(int* x, int* y) {
    uint8_t b[9];
    int pulsado, tx, ty;
    if (!s_touch_ok) return 0;
    if (cst_lee(0xD0070000u, b, 9) != 0) {
        if (s_touch_fallos++ == 0) {
            txt_t t; t_ini(&t, s_log, sizeof s_log);
            t_s(&t, "driver SPI: el tactil no contesta en I2C 0x"); t_x2(&t, (unsigned) T.addr);
            t_s(&t, " (primer fallo; se sigue como sin dedo)");
            di(s_log);
        }
        return 0;
    }
    pulsado = ((b[3] & 0x0F) != 0) && ((b[8] & 0xF0) != 0);
    cst_fin();
    if (!pulsado) return 0;
    tx = ((b[7] & 0x0F) << 8) | b[4];
    ty = ((b[7] & 0xF0) << 4) | b[5];
    if (tx < 0 || tx >= L.w || ty < 0 || ty >= L.h) return 0;
    *x = tx; *y = ty;
    return 1;
}

/* DOS tablas, la misma lista con y sin la última: un driver sin táctil NO publica
 * `touch` —así lo dice el contrato— y el pack de LVGL no crea un puntero que nunca
 * se pulsa. Se elige en la entrada, por si el ENV tiene la línea `touch`. */
static const bpvm_pack_sym_t s_syms[] = {
    { "init",    (bpvm_pack_fn_t) sp_init    },
    { "size",    (bpvm_pack_fn_t) sp_size    },
    { "flush",   (bpvm_pack_fn_t) sp_flush   },
    { "rotate",  (bpvm_pack_fn_t) sp_rotate  },
    { "formato", (bpvm_pack_fn_t) sp_formato },
    { "estado",  (bpvm_pack_fn_t) sp_estado  },
    { "touch",   (bpvm_pack_fn_t) sp_touch   },   /* la ÚLTIMA, a propósito */
};
#define N_SYMS ((uint32_t) (sizeof s_syms / sizeof s_syms[0]))
static const bpvm_pack_api_t s_api_tactil = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, N_SYMS,      s_syms };
static const bpvm_pack_api_t s_api_sin    = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, N_SYMS - 1u, s_syms };

/* La ENTRADA. Sólo publica: la configuración se lee en el primer uso (y hasta que
 * vale) y la pantalla se enciende cuando el pack de LVGL la pide (`init`) — un
 * programa sin GUI no se queda sin los pines. */
int32_t bp_pack_init(const bpvm_bios_t* bios)
{
    char t[8];
    int r, tactil;
    if (bios == 0)                          return 1;
    if (bios->magic   != BPVM_BIOS_MAGIC)   return 2;
    if (bios->log     == 0)                 return 4;
    if (bios->version <  6u) {              /* env_get es de la v6; la tabla crece por el final */
        bios->log("driver SPI: esta BIOS no presta env_get (hace falta la v6): no tengo de donde leer la placa");
        return 3;
    }
    g_bios = bios;
    s_ok = 0; s_touch_ok = 0; s_touch_fallos = 0; s_gap_x = s_gap_y = 0;
    tactil = bios->env_get("touch", t, sizeof t) != BPVM_BIOS_ENV_NO_ESTA;
    r = bios->publica(BPVM_PANT_MARCA, tactil ? &s_api_tactil : &s_api_sin);
    if (r != 0) {
        di(r == -3 ? "driver SPI: 'PANT' ya estaba publicada (¿dos drivers de pantalla grabados?)"
         : r == -4 ? "driver SPI: la BIOS RECHAZO la tabla (la reja)"
                   : "driver SPI: publica('PANT') fallo");
        return 5;
    }
    di(tactil ? "driver SPI: publicado como 'PANT', con tactil (la placa, del ENV: lcd=, exp=, touch=)"
              : "driver SPI: publicado como 'PANT', sin tactil (la placa, del ENV: lcd=, exp=)");
    return 0;
}
