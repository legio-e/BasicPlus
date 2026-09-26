/* driver_spi.c — el DRIVER DE PANTALLA GENÉRICO (V7/H6.c G3): cualquier controlador
 * MIPI-DCS por SPI de 4 hilos (ST7789, ILI9341, GC9A01, ST7796…), descrito por
 * DATOS y no por código. Una placa nueva no necesita que nadie escriba C:
 *
 *   · la PLACA va en el ENV —una línea por BUS y una por dispositivo, con el formato de
 *     `sd=`—. Los pines del bus son del BUS (V7/H6.c G4, V7_IDEAS §H6.11: la TF y la
 *     pantalla de la C5 COMPARTEN el SPI2), y cada dispositivo sólo dice cuál y lo suyo:
 *
 *       spi2=sck:6,mosi:7,miso:8
 *       i2c0=sda:0,scl:1,khz:400
 *       lcd=st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv
 *       exp=ch32,i2c:0,addr:0x24
 *       touch=cst3530,i2c:0,addr:0x58,rst:x0
 *
 *     (ésa es la Waveshare ESP32-C5-Touch-LCD-2.8; el C6 sería `spi2=sck:7,mosi:6` y
 *      `lcd=st7789,spi:2,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv`)
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
 *   spi:N          el bus, en la numeración de Spi.Bus (2 = GPSPI2 en las ESP32): sus
 *                  pines están en SU línea (`spi2=`), no aquí
 *   dc:N cs:N      datos/comando y selección: lo exclusivo de este dispositivo
 *   w:N h:N        el tamaño del cristal, a 0°
 *   opcionales:
 *   rst:P          el reset (sin él: SWRESET por el bus)
 *   bl:P           la luz (sin él: la enciende el hardware)
 *   mhz:N          el reloj del SPI (20 si no se dice); como mucho, el `mhz_max` del .seq del
 *                  controlador —la hoja de datos: 62 en el ST7789— (#567)
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
 * firmware de fábrica), `i2c` (el bus: sus pines y su velocidad, en `i2cN=`) y `addr`.
 * `touch=` (sin ella, la pantalla no tiene táctil): el tipo (`cst3530`), `addr`, `rst`
 * e `i2c` (si no lo dice, el del expansor).
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
 * El volcado es el de DriverC5 (el que se vio pintar el 22-sep): ventana y los datos,
 * una transacción por bloque (G4.a3). `make test-driverspi` compara los dos drivers byte a byte
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
    int  spi, dc, cs, mhz;
    pin_t rst, bl;
    int  w, h, ram_w, ram_h, off_x, off_y;
    int  inv, bgr, traza;
    char seq[64];                    /* "" = el de fábrica, dentro del pack */
} L;
static struct { int hay; int i2c, addr; } X;             /* exp=   */
static struct { int hay; int i2c, addr; pin_t rst; } T;    /* touch= */

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
    L.spi = L.dc = L.cs = -1;
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
        else if (es(a, k, "sck") || es(a, k, "mosi") || es(a, k, "miso"))
            return falla("ENV lcd: '", a, k, "' es del bus: va en su linea spiN=");
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
    if (L.dc   < 0) return falla("ENV lcd: falta 'dc'", 0, 0, "");
    if (L.cs   < 0) return falla("ENV lcd: falta 'cs'", 0, 0, "");
    if (L.w <= 0 || L.h <= 0) return falla("ENV lcd: falta el tamano ('w' y 'h')", 0, 0, "");
    if (L.mhz <= 0) return falla("ENV lcd: 'mhz' no puede ser 0", 0, 0, "");
    if (L.ram_w == 0 && L.ram_h == 0) { L.ram_w = L.w; L.ram_h = L.h; }
    if (L.off_x + L.w > L.ram_w || L.off_y + L.h > L.ram_h)
        return falla("ENV lcd: el cristal (w, h, off) no cabe en 'ram'", 0, 0, "");
    return 0;
}

/* `exp=` y `touch=` dicen su bus y su dirección; los pines y la velocidad son del
 * BUS y van en su línea `i2cN=` (V7/H6.c G4). */
static int parse_i2c(const char* clave, const char* a, const char* k, const char* v, const char* z,
                     int* i2c, int* addr) {
    long n; int* dst; long max;
    if      (es(a, k, "i2c"))  { dst = i2c;  max = 9;    }
    else if (es(a, k, "addr")) { dst = addr; max = 127;  }
    else if (es(a, k, "sda") || es(a, k, "scl") || es(a, k, "khz")) {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV "); t_s(&t, clave); t_s(&t, ": '"); t_r(&t, a, k); t_s(&t, "' es del bus: va en su linea i2cN=");
        return -1;
    }
    else {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV "); t_s(&t, clave); t_s(&t, ": no conozco '"); t_r(&t, a, k); t_s(&t, "'");
        return -1;
    }
    if (numero(v, z, &n) != 0 || n > max || (n == 0 && dst == addr)) {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV "); t_s(&t, clave); t_s(&t, ": '"); t_r(&t, a, k); t_s(&t, "' no es un numero valido");
        return -1;
    }
    *dst = (int) n;
    return 0;
}

static int parse_exp(void) {
    const char* p = s_env; const char *a, *z; char tipo[12];
    X.i2c = X.addr = -1;
    if (tipo_de(&p, "exp", tipo, sizeof tipo) != 0) return -1;
    if (!es(tipo, tipo + g_bios->strlen(tipo), "ch32"))
        return falla("ENV exp: no conozco el expansor '", tipo, tipo + g_bios->strlen(tipo), "' (hay: ch32)");
    while (campo(&p, &a, &z)) {
        const char* v = a;
        while (v < z && *v != ':') v++;
        if (v == z) return falla("ENV exp: no conozco '", a, z, "'");
        if (parse_i2c("exp", a, v, v + 1, z, &X.i2c, &X.addr) != 0) return -1;
    }
    if (X.i2c < 0)  return falla("ENV exp: falta 'i2c'", 0, 0, "");
    if (X.addr < 0) return falla("ENV exp: falta 'addr'", 0, 0, "");
    return 0;
}

static int parse_touch(void) {
    const char* p = s_env; const char *a, *z; char tipo[12];
    T.i2c = T.addr = -1;
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
        if (parse_i2c("touch", a, v, v + 1, z, &T.i2c, &T.addr) != 0) return -1;
    }
    if (T.addr < 0) return falla("ENV touch: falta 'addr'", 0, 0, "");
    if (T.i2c < 0) {                                  /* el bus del expansor */
        if (!X.hay) return falla("ENV touch: falta 'i2c' (o una linea 'exp' con su bus)", 0, 0, "");
        T.i2c = X.i2c;
    }
    return 0;
}

/* ── el bus: comandos con DC en bajo, datos con DC en alto (lo de DriverC5) ──
 * V7/H6.c G4.a3: el SPI se COMPARTE (en la C5, con la TF), así que cada comando y
 * cada bloque de datos es una TRANSACCIÓN de la BIOS v8: la pantalla la pide con SU
 * configuración —su CS, sus MHz, modo 0—, la HAL BP la calcula antes contra el tope
 * del bus (un bloque de LVGL son 11 520 bytes: 2,3 ms a 40 MHz) y baja y sube el CS.
 * El DC se pone ANTES: es sólo de la pantalla, que lo ignora con su CS arriba, y así
 * la cinta queda como la de DriverC5 (DC, CS abajo, bytes, CS arriba). Si no se puede
 * —el tope, el bus ocupado más de 1 s—, el porqué ya está en el log (lo dice la BIOS)
 * y esto no se manda. Ya no se trocea aquí: lo hace la cintura del ESP32 (G4.a3). */
static void lcd_tx(const uint8_t* d, size_t n) {
    bpvm_spi_op_t op;
    op.tx = d; op.rx = 0; op.n = (uint32_t) n;
    (void) g_bios->spi_transaccion(L.spi, L.cs, L.mhz * 1000000, 0, &op, 1);
}
static void lcd_cmd(uint8_t c) {
    g_bios->gpio_write(L.dc, 0);
    lcd_tx(&c, 1);
}
static void lcd_dat(const uint8_t* d, size_t n) {
    g_bios->gpio_write(L.dc, 1);
    lcd_tx(d, n);
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
/* #567 (V7/H6, 25-sep) — el tope del RELOJ que da la hoja del CONTROLADOR (`mhz_max N` en su .seq);
 * 0 = el .seq no lo dice, y vale el tope general de la línea lcd=. */
static long    s_seq_mhz_max = 0;

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
        if (es(s, e, "mhz_max")) {                  /* #567: no es un comando, es un DATO del chip */
            const char* a = e; const char* z; long mhz;
            while (*a == ' ' || *a == '\t') a++;
            z = a; while (*z && *z != ' ' && *z != '\t') z++;
            if (numero(a, z, &mhz) != 0 || mhz <= 0 || mhz > 80)
                return falla_seq(linea, "'mhz_max' quiere los MHz de la hoja (1..80)", 0, 0, "");
            s_seq_mhz_max = mhz;
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
    s_seq_mhz_max = 0;
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
/* ¿Tiene el ENV la línea del bus `<tipo><id>=`? (Sólo que esté: la lee la HAL BP.) */
static int hay_bus(const char* tipo, int id) {
    char clave[8], v[4];
    txt_t t; t_ini(&t, clave, sizeof clave);
    t_s(&t, tipo); t_d(&t, id);
    return g_bios->env_get(clave, v, sizeof v) != BPVM_BIOS_ENV_NO_ESTA;   /* NO_CABE también es «está» */
}
static int falla_bus(const char* tipo, int id, const char* de) {
    txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
    t_s(&t, "ENV: falta la linea '"); t_s(&t, tipo); t_d(&t, id); t_s(&t, "' (el bus "); t_s(&t, de); t_c(&t, ')');
    return -1;
}

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
    /* V7/H6.c G4 — los pines de cada bus están en SU línea del ENV (`spi2=`,
     * `i2c0=`): sin ella la HAL BP no tiene con qué abrirlo. Se dice aquí, con
     * nombre, y no como «la placa no da el SPI 2» al encender. */
    if (!hay_bus("spi", L.spi)) return falla_bus("spi", L.spi, "de la pantalla");
    if (X.hay && !hay_bus("i2c", X.i2c)) return falla_bus("i2c", X.i2c, "del expansor");
    if (T.hay && !hay_bus("i2c", T.i2c)) return falla_bus("i2c", T.i2c, "del tactil");
    if (seq(0) != 0) return -1;
    /* #567 (V7/H6, 25-sep) — el reloj no pasa de la HOJA del controlador. Eduardo, al cancelar los 80 MHz:
     * «nos ceñimos a ese límite… podría ser que a nosotros nos funcionase y a otro usuario no». Un panel
     * que no aguanta el reloj da píxeles corruptos, no un error: por eso se para aquí, con nombre. El tope
     * es del CHIP y va en su .seq (`mhz_max N`); un .seq que no lo dice deja el tope general (80).
     * El mensaje dice de dónde sale el tope (la directiva y el fichero) y cabe en s_motivo: 76 de 79. */
    if (s_seq_mhz_max > 0 && L.mhz > s_seq_mhz_max) {
        txt_t t; t_ini(&t, s_motivo, sizeof s_motivo);
        t_s(&t, "ENV lcd: 'mhz' "); t_d(&t, L.mhz); t_s(&t, " supera los "); t_d(&t, s_seq_mhz_max);
        t_s(&t, " que admite el "); t_s(&t, L.ctrl); t_s(&t, " (mhz_max en "); t_s(&t, base_de(s_camino));
        t_c(&t, ')');
        return -1;
    }
    s_ok = 1;
    {
        txt_t t; t_ini(&t, s_log, sizeof s_log);
        t_s(&t, "driver SPI: "); t_s(&t, L.ctrl); t_c(&t, ' '); t_d(&t, L.w); t_c(&t, 'x'); t_d(&t, L.h);
        t_s(&t, " por SPI"); t_d(&t, L.spi); t_s(&t, " a "); t_d(&t, L.mhz); t_s(&t, " MHz (dc "); t_d(&t, L.dc);
        t_s(&t, ", cs "); t_d(&t, L.cs);
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
static int cst_modo_normal(void);

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
    /* V7/H6.c G4 — el bus es COMPARTIDO: los pines a -1 son «los de su línea del ENV»
     * y la HAL BP no lo reabre si ya lo usa otro (la TF). El porqué de un «no», con
     * nombre, va al log de la VM justo antes de esta línea. */
    if (g_bios->spi_init(L.spi, -1, -1, -1, L.mhz * 1000000, 0) != 0) {
        t_ini(&t, s_log, sizeof s_log); t_s(&t, "driver SPI: la placa no da el SPI "); t_d(&t, L.spi);
        di(s_log); return -2;
    }
    if ((X.hay && g_bios->i2c_init(X.i2c, -1, -1, 0) != 0) ||
        (T.hay && !(X.hay && T.i2c == X.i2c) && g_bios->i2c_init(T.i2c, -1, -1, 0) != 0)) {
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
    /* 3b. El táctil en MODO NORMAL (23-sep): lo que hace al despertarlo el driver Hynitron
     *     original (cst66xx_set_workmode) y que no hacíamos —ni el ejemplo de Waveshare—:
     *     fuera el bajo consumo del I2C, modo e informe normales. No arregla ningún fallo
     *     DEMOSTRADO: el «táctil que se cuelga tras horas» era la herramienta, que reiniciaba
     *     la C5 al cerrar el puerto (V7_IDEAS §H6.12). Es el arranque que pide el fabricante. */
    if (T.hay && cst_modo_normal() != 0) di("driver SPI: el tactil no acepta el modo normal (se sigue)");
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
/* El registro y su lectura en UN turno (BIOS v7): en el I2C 0 de la C5 también
 * habla el expansor, y nadie se mete entre las dos. */
static int cst_lee(uint32_t reg, uint8_t* d, size_t n) {
    uint8_t a[4];
    a[0] = (uint8_t) (reg >> 24); a[1] = (uint8_t) (reg >> 16); a[2] = (uint8_t) (reg >> 8); a[3] = (uint8_t) reg;
    return g_bios->i2c_escribir_leer(T.i2c, T.addr, a, 4, d, n) == (int) n ? 0 : -1;
}
static void cst_fin(void) {
    uint8_t a[4];
    a[0] = 0xD0; a[1] = 0x00; a[2] = 0x02; a[3] = 0xAB;   /* END READ */
    g_bios->i2c_write(T.i2c, T.addr, a, 4);
}
static int cst_orden(uint32_t cmd) {
    uint8_t a[4];
    a[0] = (uint8_t) (cmd >> 24); a[1] = (uint8_t) (cmd >> 16); a[2] = (uint8_t) (cmd >> 8); a[3] = (uint8_t) cmd;
    return g_bios->i2c_write(T.i2c, T.addr, a, 4) == 4 ? 0 : -1;
}
/* El MODO NORMAL, como el driver Hynitron (cst66xx_set_workmode, NOMAL_MODE): dos veces
 * D0 00 04 00 —«desactivar el bajo consumo del I2C»: la primera puede irse en despertarlo—
 * y luego D0 00 00 00 · D0 00 0C 00 · D0 00 01 00. Entre las dos primeras, 10 ms y no el
 * 1 del original: una espera menor que un tick NO espera. Medido en la C5 (C5TouchCst):
 * tras esto la trama vieja se borra y el chip responde a todos los toques. */
static int cst_modo_normal(void) {
    int r;
    (void) cst_orden(0xD0000400u);
    g_bios->sleep_ms(10);
    r  = cst_orden(0xD0000400u);
    r |= cst_orden(0xD0000000u);
    r |= cst_orden(0xD0000C00u);
    r |= cst_orden(0xD0000100u);
    return r;
}
/* El LATIDO ESD (cst66xx_check_esd): 4 bytes de D0 04 04 00, un contador que avanza sin
 * parar mientras el chip escanea —unos 33 000 por segundo, medido en la C5—. Sólo lo lee
 * la TRAZA, cada 5 min: si no avanza entre dos latidos, el chip no escanea. (Un vigilante
 * que lo usaba para reiniciar el chip vivió unas horas del 23-sep: se quitó al saberse que
 * el cuelgue no existía — §H6.12.) */
static int cst_latido(uint32_t* v) {
    uint8_t a[4], b[4];
    a[0] = 0xD0; a[1] = 0x04; a[2] = 0x04; a[3] = 0x00;
    if (g_bios->i2c_escribir_leer(T.i2c, T.addr, a, 4, b, 4) != 4) return -1;
    *v = ((uint32_t) b[3] << 24) | ((uint32_t) b[2] << 16) | ((uint32_t) b[1] << 8) | b[0];
    return 0;
}

/* Una lectura que falla es «sin dedo» —el táctil se sondea cada 30 ms y un NACK
 * suelto no es para parar la GUI—, pero NO en silencio: la primera se dice. Sin
 * esto, un táctil mal cableado o en otra dirección es una pantalla que «no
 * responde» sin una palabra (DriverC5 era así). */
static int s_touch_fallos = 0;

/* ── la TRAZA del táctil (con `traza` en lcd=), 23-sep ────────────────────────
 * Nació para cazar un «táctil que deja de responder tras horas» que resultó ser la
 * herramienta (reiniciaba la placa al cerrar el puerto, V7_IDEAS §H6.12); se queda
 * porque es lo que hay que mirar la próxima vez que un táctil no responda. Dos cosas,
 * pensadas para el log que sobrevive al reinicio (4 KB en la C5: unas tres horas):
 *   · cada CAMBIO pulsado/suelto, con la trama del chip (y «fuera» si el punto cae
 *     fuera de la pantalla);
 *   · un LATIDO cada 5 minutos: lecturas, pulsadas y fallidas desde el anterior, el
 *     latido ESD del chip (`--` si no contesta) y la última trama.
 * Si el latido sigue y la trama no cambia al pulsar, es el chip —y su ESD dice si
 * escanea—; si el latido se para, nadie lo está leyendo (LVGL no bombea). */
#define LATIDO_MS  (5u * 60u * 1000u)
static uint32_t s_t_lect = 0, s_t_puls = 0, s_t_fall = 0, s_t_desde = 0;
static int      s_t_estado = -1;               /* -1 = aún no se ha leído */
static uint8_t  s_t_trama[9];
static void t_trama(txt_t* t, const uint8_t* b) {
    int i;
    t_s(t, " |");
    for (i = 0; i < 9; i++) { t_c(t, ' '); t_x2(t, b[i]); }
}
static void traza_latido(void) {
    uint32_t ahora = g_bios->now_ms();
    txt_t t;
    if (s_t_desde == 0) { s_t_desde = ahora; return; }
    if (ahora - s_t_desde < LATIDO_MS) return;
    t_ini(&t, s_log, sizeof s_log);
    t_s(&t, "tactil: latido "); t_d(&t, (long) s_t_lect); t_s(&t, " lect "); t_d(&t, (long) s_t_puls);
    t_s(&t, " puls "); t_d(&t, (long) s_t_fall); t_s(&t, " fall · esd ");
    {   uint32_t e;
        int k;
        if (cst_latido(&e) == 0) for (k = 3; k >= 0; k--) t_x2(&t, (unsigned) ((e >> (8 * k)) & 0xFFu));
        else                     t_s(&t, "--");
    }
    t_trama(&t, s_t_trama);
    di(s_log);
    s_t_lect = s_t_puls = s_t_fall = 0;
    s_t_desde = ahora;
}
/* estado: 0 suelto · 1 pulsado · 2 pulsado FUERA de la pantalla */
static void traza_cambio(int estado, int tx, int ty) {
    txt_t t;
    if (estado == s_t_estado) return;
    t_ini(&t, s_log, sizeof s_log);
    t_s(&t, s_t_estado < 0 ? "tactil: primera lectura, " : "tactil: ");
    if (estado == 0) t_s(&t, "suelto");
    else { t_s(&t, estado == 2 ? "pulsado FUERA " : "pulsado "); t_d(&t, tx); t_c(&t, ','); t_d(&t, ty); }
    t_trama(&t, s_t_trama);
    di(s_log);
    s_t_estado = estado;
}

static int sp_touch(int* x, int* y) {
    uint8_t b[9];
    int pulsado, tx, ty, i, fuera;
    if (!s_touch_ok) return 0;
    s_t_lect++;
    if (cst_lee(0xD0070000u, b, 9) != 0) {
        s_t_fall++;
        if (s_touch_fallos++ == 0) {
            txt_t t; t_ini(&t, s_log, sizeof s_log);
            t_s(&t, "driver SPI: el tactil no contesta en I2C 0x"); t_x2(&t, (unsigned) T.addr);
            t_s(&t, " (primer fallo; se sigue como sin dedo)");
            di(s_log);
        }
        if (L.traza) traza_latido();
        return 0;
    }
    for (i = 0; i < 9; i++) s_t_trama[i] = b[i];
    pulsado = ((b[3] & 0x0F) != 0) && ((b[8] & 0xF0) != 0);
    cst_fin();
    tx = ((b[7] & 0x0F) << 8) | b[4];
    ty = ((b[7] & 0xF0) << 4) | b[5];
    fuera = pulsado && (tx < 0 || tx >= L.w || ty < 0 || ty >= L.h);
    if (pulsado) s_t_puls++;
    if (L.traza) { traza_cambio(pulsado ? (fuera ? 2 : 1) : 0, tx, ty); traza_latido(); }
    if (!pulsado || fuera) return 0;
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
    if (bios->version <  8u) {              /* la transacción es de la v8 (el turno, de la v7); la tabla crece por el final */
        bios->log("driver SPI: esta BIOS no presta la transaccion de los buses (hace falta la v8): graba una imagen al dia");
        return 3;
    }
    g_bios = bios;
    s_ok = 0; s_touch_ok = 0; s_touch_fallos = 0; s_gap_x = s_gap_y = 0;
    s_t_lect = s_t_puls = s_t_fall = s_t_desde = 0; s_t_estado = -1;
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
