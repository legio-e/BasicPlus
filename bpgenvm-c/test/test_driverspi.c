/*
 * test_driverspi.c — V7/H6.c G3: el driver de pantalla GENÉRICO contra el de la C5,
 * en el PC.
 *
 * Los dos drivers (`driver_c5.c`, el que se VIO pintar el 22-sep, y `driver_spi.c`,
 * el genérico que lee la placa del ENV) se compilan para el host y se ejecutan
 * contra UNA BIOS falsa que GRABA los cables: cada byte de SPI con el nivel de DC y
 * de CS, cada escritura y lectura de I2C, cada espera. Con el perfil de la C5 en el
 * ENV, lo que el genérico manda a la pantalla tiene que ser lo de DriverC5:
 *
 *   · el volcado, BYTE A BYTE (ventana, datos y troceado a 64);
 *   · el giro de 0/90/180/270, byte a byte;
 *   · el táctil, con su tráfico I2C y sus coordenadas (y, sólo el genérico, el MODO
 *     NORMAL del fabricante al arrancar y su latido ESD en la traza, 23-sep);
 *   · el arranque, con LOS MISMOS comandos y parámetros (el orden cambia en uno:
 *     RAMCTRL va en el .seq, antes de lo de la placa), y el expansor igual al final.
 *
 * Y lo que DriverC5 no sabe hacer, contra sus oráculos: el desfase del C6 (la tabla
 * de su imagen, confirmada en placa: 0/0, 0/0, 0/80, 80/0) y el de las pantallas de
 * 135×240 (52/40, 40/53, 53/40, 40/52), que salen de `ram` y `off`; el reset por
 * GPIO y el SWRESET; y que cada errata del ENV o del .seq se diga CON NOMBRE en
 * `estado` — y que al arreglarla pase sin recargar nada.
 *
 * El .seq es el de verdad (`resources/st7789.seq`, primer argumento): lo que se
 * prueba es lo que se graba.
 *
 *   make test-driverspi
 */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_pant.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern int32_t c5_pack_init(const bpvm_bios_t* bios);
extern int32_t spi_pack_init(const bpvm_bios_t* bios);

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* ── LA CINTA: todo lo que sale por los cables, una línea por evento ─────────── */
static char   g_cinta[1 << 20];
static size_t g_cn = 0;
static void cinta(const char* fmt, ...) {
    va_list ap;
    if (g_cn >= sizeof g_cinta - 1) return;
    va_start(ap, fmt);
    g_cn += (size_t) vsnprintf(g_cinta + g_cn, sizeof g_cinta - g_cn, fmt, ap);
    va_end(ap);
    if (g_cn > sizeof g_cinta - 1) g_cn = sizeof g_cinta - 1;
}
static void cinta_hex(const uint8_t* d, size_t n) { size_t i; for (i = 0; i < n; i++) cinta(" %02X", d[i]); }

/* Y LOS COMANDOS de la pantalla, reconstruidos: una línea por comando con sus
 * datos detrás ("B0 00 F0"). Lo que va con DC en bajo abre línea. */
static char   g_cmds[1 << 16];
static size_t g_mn = 0;
static void cmds_pon(const char* s) { size_t n = strlen(s); if (g_mn + n < sizeof g_cmds) { memcpy(g_cmds + g_mn, s, n + 1); g_mn += n; } }

static void grabar_desde_cero(void) { g_cn = 0; g_cinta[0] = 0; g_mn = 0; g_cmds[0] = 0; }

/* El estado de los cables que importa para leer el SPI. */
static int g_pin_dc = -1, g_pin_cs = -1;
static int g_dc = 1, g_cs = 1;
/* El expansor CH32 (I2C 0x24): lo último escrito en cada registro. */
static int g_exp_dir = -1, g_exp_out = -1, g_exp_pwm = -1;
/* El táctil CST3530 (I2C 0x58): la trama que devuelve, y su LATIDO ESD (4 bytes, al
 * revés, cuando lo último escrito fue D0 04 04 00): avanza en cada lectura si
 * `g_esd_vivo`, y se queda quieto si no. */
static uint8_t  g_trama[9];
static uint8_t  g_ult58[4];
static uint32_t g_esd = 0x00100000u;
static int      g_esd_vivo = 1;
/* El reloj de la BIOS: lo mueve el test (hasta el 23-sep era siempre 0). */
static uint32_t g_ahora = 0;

/* ── LA BIOS FALSA ─────────────────────────────────────────────────────────── */
static char   g_log[1 << 16];
static size_t g_ln = 0;
static int    g_verbose = 0;
static void f_log(const char* m) {
    size_t n = strlen(m);
    if (g_ln + n + 2 < sizeof g_log) { memcpy(g_log + g_ln, m, n); g_ln += n; g_log[g_ln++] = '\n'; g_log[g_ln] = 0; }
    if (g_verbose) printf("        [log] %s\n", m);
}
static int  f_gpio_init(int pin, int mode) { cinta("gpio_init %d %d\n", pin, mode); return 0; }
static void f_gpio_write(int pin, int v) {
    if (pin == g_pin_dc) g_dc = v;
    if (pin == g_pin_cs) g_cs = v;
    cinta("g %d=%d\n", pin, v);
}
static int  f_gpio_read(int pin) { (void) pin; return 0; }
static int  f_spi_init(int bus, int sck, int mosi, int miso, int hz, int mode) {
    cinta("spi_init %d sck %d mosi %d miso %d %d Hz modo %d\n", bus, sck, mosi, miso, hz, mode); return 0;
}
static int g_turno_cs, g_turno_mal, g_exigir_turno;   /* el turno (G4.a2), más abajo */
static int  f_spi_write(int bus, const uint8_t* d, size_t n) {
    char b[8]; size_t i;
    if (g_exigir_turno && g_turno_cs == -2) g_turno_mal++;   /* un byte FUERA de turno */
    cinta("spi %d %s%s:", bus, g_dc ? "D" : "C", g_cs ? " (¡CS ALTO!)" : ""); cinta_hex(d, n); cinta("\n");
    for (i = 0; i < n; i++) {
        if (i == 0 && !g_dc) snprintf(b, sizeof b, "%s%02X", g_mn ? "\n" : "", d[i]);
        else                 snprintf(b, sizeof b, " %02X", d[i]);
        cmds_pon(b);
    }
    return (int) n;
}
static int  f_i2c_init(int bus, int sda, int scl, int hz) { cinta("i2c_init %d sda %d scl %d %d Hz\n", bus, sda, scl, hz); return 0; }
static int  f_i2c_write(int bus, int addr, const uint8_t* d, size_t n) {
    cinta("i2cw %d %02X:", bus, addr); cinta_hex(d, n); cinta("\n");
    if (addr == 0x58 && n == 4) memcpy(g_ult58, d, 4);
    if (addr == 0x24 && n == 2) {
        if (d[0] == 0x02) g_exp_dir = d[1];
        if (d[0] == 0x03) g_exp_out = d[1];
        if (d[0] == 0x05) g_exp_pwm = d[1];
    }
    return (int) n;
}
static int  g_tactil_mudo = 0;     /* el CST3530 no contesta (NACK) */
static int  f_i2c_read(int bus, int addr, uint8_t* d, size_t n) {
    size_t i;
    cinta("i2cr %d %02X %u\n", bus, addr, (unsigned) n);
    if (addr == 0x58 && g_tactil_mudo) return -1;
    if (addr == 0x58 && n == 4 && g_ult58[0] == 0xD0 && g_ult58[1] == 0x04 && g_ult58[2] == 0x04 && g_ult58[3] == 0x00) {
        d[0] = (uint8_t) g_esd; d[1] = (uint8_t) (g_esd >> 8); d[2] = (uint8_t) (g_esd >> 16); d[3] = (uint8_t) (g_esd >> 24);
        if (g_esd_vivo) g_esd += 0x8080u;
        return 4;
    }
    for (i = 0; i < n; i++) d[i] = (addr == 0x58 && i < sizeof g_trama) ? g_trama[i] : 0;
    return (int) n;
}
static uint32_t f_now_ms(void) { return g_ahora; }

/* V7/H6.c G4.a2 — EL TURNO (BIOS v7), como lo hace la HAL BP: baja el CS del turno
 * por el pin (la misma línea «g 10=0» de la cinta que el CS a mano de DriverC5),
 * transfiere como `spi_write` y sube el CS al soltar. Así la cinta de los dos drivers
 * se sigue comparando byte a byte. Y vigila lo que la HAL BP no perdonaría: un turno
 * dentro de otro, y transferir sin turno. */
static int g_turno_cs = -2;            /* -2 = no hay turno */
static int g_turnos = 0, g_turno_hz_mal = 0, g_turno_mal = 0, g_hz_turno = 0;
static int g_exigir_turno = 0;         /* 1 = el driver en prueba es de la v7: TODO su SPI va en turnos */
static int  f_spi_tomar(int bus, int cs, int hz, int modo) {
    (void) bus; (void) modo;
    if (g_turno_cs != -2) g_turno_mal++;            /* un turno dentro de otro */
    g_turno_cs = cs; g_turnos++;
    if (g_hz_turno && hz != g_hz_turno) g_turno_hz_mal++;
    if (cs >= 0) f_gpio_write(cs, 0);
    return 0;
}
static int  f_spi_cs(int bus, int nivel) { (void) bus; if (g_turno_cs < 0) return -1; f_gpio_write(g_turno_cs, nivel); return 0; }
static int  f_spi_transferir(int bus, const uint8_t* tx, uint8_t* rx, size_t n) {
    size_t i;
    if (g_turno_cs == -2) g_turno_mal++;            /* transferir sin turno */
    if (rx) for (i = 0; i < n; i++) rx[i] = 0xFF;
    return tx ? f_spi_write(bus, tx, n) : (int) n;
}
static void f_spi_soltar(int bus) {
    (void) bus;
    if (g_turno_cs == -2) { g_turno_mal++; return; }
    if (g_turno_cs >= 0) f_gpio_write(g_turno_cs, 1);
    g_turno_cs = -2;
}
static int  f_i2c_escribir_leer(int bus, int addr, const uint8_t* tx, size_t ntx, uint8_t* rx, size_t nrx) {
    if (f_i2c_write(bus, addr, tx, ntx) != (int) ntx) return -1;
    return f_i2c_read(bus, addr, rx, nrx);
}

/* V7/H6.c G4.a3 — LA TRANSACCIÓN (BIOS v8), como la hace la HAL BP: UN turno —el CS
 * abajo—, cada operación, y el CS arriba. La cintura del ESP32 trocea a 64 bytes (su
 * FIFO sin DMA): aquí igual, así que la cinta sigue siendo la de DriverC5 byte a byte
 * —el mismo DC, el mismo CS, los mismos trozos—. Y cuenta como un turno. */
static int  g_transacciones = 0;
static int  f_spi_transaccion(int bus, int cs, int hz, int modo, const bpvm_spi_op_t* ops, int nops) {
    int i, total = 0;
    g_transacciones++;
    if (f_spi_tomar(bus, cs, hz, modo) != 0) return -1;
    for (i = 0; i < nops; i++) {
        uint32_t hecho = 0;
        while (hecho < ops[i].n) {
            size_t t = (ops[i].n - hecho > 64u) ? 64u : (size_t) (ops[i].n - hecho);
            f_spi_transferir(bus, ops[i].tx ? ops[i].tx + hecho : 0, ops[i].rx ? ops[i].rx + hecho : 0, t);
            hecho += (uint32_t) t;
        }
        total += (int) ops[i].n;
    }
    f_spi_soltar(bus);
    return total;
}
static int  f_i2c_transaccion(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    int i, total = 0;
    for (i = 0; i < nm; i++) {
        int r = m[i].lee ? f_i2c_read(bus, addr, m[i].buf, m[i].n) : f_i2c_write(bus, addr, m[i].buf, m[i].n);
        if (r != (int) m[i].n) return -2;
        total += r;
    }
    return total;
}
static void f_sleep_ms(int ms) { cinta("espera %d\n", ms); }

/* El ENV: tres líneas, las que se prueben. */
static const char *g_env_lcd = 0, *g_env_exp = 0, *g_env_touch = 0;
/* V7/H6.c G4 — las líneas de los BUSES: el driver sólo mira que estén (sus pines los
 * pone la HAL BP), así que aquí basta con que existan. */
static const char *g_env_spi2 = "sck:6,mosi:7,miso:8", *g_env_i2c0 = "sda:0,scl:1,khz:400";
static int igual_ci(const char* a, const char* b) {
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = (char) (x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = (char) (y - 'A' + 'a');
        if (x != y) return 0;
    }
    return *a == 0 && *b == 0;
}
static int f_env_get(const char* k, char* buf, size_t cap) {
    const char* v; size_t n;
    if (!k || !*k || !buf || !cap) return BPVM_BIOS_ENV_ARGS;
    v = igual_ci(k, "lcd") ? g_env_lcd : igual_ci(k, "exp") ? g_env_exp : igual_ci(k, "touch") ? g_env_touch
      : igual_ci(k, "spi2") ? g_env_spi2 : igual_ci(k, "i2c0") ? g_env_i2c0 : 0;
    if (!v) return BPVM_BIOS_ENV_NO_ESTA;
    n = strlen(v);
    if (n + 1 > cap) { buf[0] = 0; return BPVM_BIOS_ENV_NO_CABE; }
    memcpy(buf, v, n + 1);
    return (int) n;
}

/* Los ficheros: los .seq, por camino. */
static char g_st7789[8192];
static struct { const char* camino; const char* texto; } g_fs[8];
static int g_nfs = 0, g_abiertos = 0;
static void fs_pon(const char* camino, const char* texto) { g_fs[g_nfs].camino = camino; g_fs[g_nfs].texto = texto; g_nfs++; }
static int f_abrir(const char* c, int w) {
    int i;
    if (w) return -1;
    for (i = 0; i < g_nfs; i++) if (strcmp(g_fs[i].camino, c) == 0) { g_abiertos++; return i + 1; }
    return -1;
}
static int  f_cerrar(int fd) { if (fd < 1 || fd > g_nfs) return -1; g_abiertos--; return 0; }
static long f_leer(int fd, uint32_t desde, void* dst, uint32_t n) {
    const char* t; size_t len;
    if (fd < 1 || fd > g_nfs) return -1;
    t = g_fs[fd - 1].texto; len = strlen(t);
    if (desde >= len) return 0;
    if (n > len - desde) n = (uint32_t) (len - desde);
    memcpy(dst, t + desde, n);
    return (long) n;
}
static long f_tamano(int fd) { if (fd < 1 || fd > g_nfs) return -1; return (long) strlen(g_fs[fd - 1].texto); }

/* El punto de encuentro. */
static uint32_t g_marca[8]; static const void* g_tabla[8]; static int g_npub = 0;
static int f_publica(uint32_t marca, const void* t) {
    int i;
    for (i = 0; i < g_npub; i++) if (g_marca[i] == marca) return -3;
    g_marca[g_npub] = marca; g_tabla[g_npub] = t; g_npub++;
    return 0;
}
static const void* f_busca(uint32_t marca) { int i; for (i = 0; i < g_npub; i++) if (g_marca[i] == marca) return g_tabla[i]; return 0; }

static bpvm_bios_t B;
static void bios_monta(void) {
    memset(&B, 0, sizeof B);
    B.magic = BPVM_BIOS_MAGIC; B.version = BPVM_BIOS_VERSION;
    B.log = f_log;
    B.memcpy = memcpy; B.memmove = memmove; B.memset = memset; B.memcmp = memcmp; B.memchr = memchr;
    B.strlen = strlen; B.strcmp = strcmp; B.strncmp = strncmp; B.strchr = strchr; B.strrchr = strrchr;
    B.strspn = strspn; B.strcspn = strcspn;
    B.abrir = f_abrir; B.cerrar = f_cerrar; B.leer = f_leer; B.tamano = f_tamano;
    B.publica = f_publica; B.busca = f_busca;
    B.gpio_init = f_gpio_init; B.gpio_write = f_gpio_write; B.gpio_read = f_gpio_read;
    B.spi_init = f_spi_init; B.spi_write = f_spi_write;
    B.i2c_init = f_i2c_init; B.i2c_write = f_i2c_write; B.i2c_read = f_i2c_read;
    B.now_ms = f_now_ms; B.sleep_ms = f_sleep_ms;
    B.env_get = f_env_get;
    B.spi_tomar = f_spi_tomar; B.spi_cs = f_spi_cs; B.spi_transferir = f_spi_transferir;
    B.spi_soltar = f_spi_soltar; B.i2c_escribir_leer = f_i2c_escribir_leer;
    B.spi_transaccion = f_spi_transaccion; B.i2c_transaccion = f_i2c_transaccion;
}

/* Carga un driver como lo hace la placa: su entrada, y 'PANT' resuelta. */
static int cargar(int32_t (*entrada)(const bpvm_bios_t*), bpvm_pant_t* p) {
    const bpvm_pack_api_t* api;
    g_npub = 0; g_ln = 0; g_log[0] = 0;
    if (entrada(&B) != 0) return -1;
    api = (const bpvm_pack_api_t*) f_busca(BPVM_PANT_MARCA);
    memset(p, 0, sizeof *p);
    return bpvm_pant_resolver(api, p) == 0 ? 0 : -2;
}

/* ── UNA PASADA COMPLETA por un driver: todo lo que se compara ─────────────── */
typedef struct {
    int  w, h, fmt, init_r;
    char init_cinta[32768], init_cmds[2048];
    char flush[8192];
    char giro[4][8192];
    char toque[2][1024];
    int  toque_r[2], tx, ty;
    int  exp_dir, exp_out, exp_pwm;
} pasada_t;
static pasada_t g_c5, g_spi;
static uint8_t  g_px[10 * 10 * 2];

static void copia(char* dst, size_t cap, const char* src) {
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n); dst[n] = 0;
}

static void pasada(bpvm_pant_t* p, pasada_t* r) {
    static const int GRADOS[4] = { 0, 90, 180, 270 };
    int i;
    r->w = r->h = 0;
    p->size(&r->w, &r->h);
    r->fmt = p->formato ? p->formato() : -1;
    g_exp_dir = g_exp_out = g_exp_pwm = -1;
    grabar_desde_cero();
    r->init_r = p->init();
    copia(r->init_cinta, sizeof r->init_cinta, g_cinta);
    copia(r->init_cmds, sizeof r->init_cmds, g_cmds);
    r->exp_dir = g_exp_dir; r->exp_out = g_exp_out; r->exp_pwm = g_exp_pwm;

    grabar_desde_cero();
    p->flush(10, 20, 19, 29, g_px);
    copia(r->flush, sizeof r->flush, g_cinta);

    for (i = 0; i < 4; i++) {
        grabar_desde_cero();
        if (p->rotate) p->rotate(GRADOS[i]);
        p->flush(0, 0, 9, 9, g_px);
        copia(r->giro[i], sizeof r->giro[i], g_cinta);
    }
    if (p->rotate) p->rotate(0);

    /* el táctil: una trama con un dedo en (120, 160), y otra sin dedo */
    memset(g_trama, 0, sizeof g_trama);
    g_trama[3] = 0x01; g_trama[4] = 120; g_trama[5] = 160; g_trama[7] = 0x00; g_trama[8] = 0x10;
    grabar_desde_cero();
    r->tx = r->ty = -1;
    r->toque_r[0] = p->touch ? p->touch(&r->tx, &r->ty) : -9;
    copia(r->toque[0], sizeof r->toque[0], g_cinta);
    memset(g_trama, 0, sizeof g_trama);
    grabar_desde_cero();
    { int x = -1, y = -1; r->toque_r[1] = p->touch ? p->touch(&x, &y) : -9; }
    copia(r->toque[1], sizeof r->toque[1], g_cinta);
}

/* Las líneas de comandos, ORDENADAS: el mismo conjunto aunque cambie el orden. */
static int cmp_str(const void* a, const void* b) { return strcmp(*(const char* const*) a, *(const char* const*) b); }
static int mismos_comandos(const char* a, const char* b) {
    static char ca[2048], cb[2048];
    const char *la[64], *lb[64];
    int na = 0, nb = 0, i;
    char* s;
    copia(ca, sizeof ca, a); copia(cb, sizeof cb, b);
    for (s = strtok(ca, "\n"); s && na < 64; s = strtok(0, "\n")) la[na++] = s;
    for (s = strtok(cb, "\n"); s && nb < 64; s = strtok(0, "\n")) lb[nb++] = s;
    if (na != nb) return 0;
    qsort(la, (size_t) na, sizeof la[0], cmp_str);
    qsort(lb, (size_t) nb, sizeof lb[0], cmp_str);
    for (i = 0; i < na; i++) if (strcmp(la[i], lb[i]) != 0) return 0;
    return 1;
}
static const char* primera(const char* cmds, char* buf, size_t cap) {
    size_t n = strcspn(cmds, "\n"); if (n >= cap) n = cap - 1; memcpy(buf, cmds, n); buf[n] = 0; return buf;
}
static const char* ultima(const char* cmds) { const char* u = strrchr(cmds, '\n'); return u ? u + 1 : cmds; }
static const char* linea_que_empieza(const char* cinta_, const char* pref, char* buf, size_t cap) {
    const char* s = strstr(cinta_, pref);
    size_t n;
    if (!s) { buf[0] = 0; return buf; }
    n = strcspn(s, "\n"); if (n >= cap) n = cap - 1;
    memcpy(buf, s, n); buf[n] = 0;
    return buf;
}

/* El desfase de un giro, leído de la ventana que el driver manda al volcar (0,0). */
static int desfase(int deg, int* gx, int* gy) {
    const char *c, *r;
    unsigned a, b, cc, d;
    bpvm_pant_t p;
    if (cargar(spi_pack_init, &p) != 0) return -1;
    grabar_desde_cero();
    if (p.init() != 0) return -2;
    grabar_desde_cero();
    if (p.rotate(deg) != 0) return -3;
    p.flush(0, 0, 0, 0, g_px);
    c = strstr(g_cmds, "2A "); r = strstr(g_cmds, "2B ");
    if (!c || !r) return -4;
    if (sscanf(c, "2A %x %x %x %x", &a, &b, &cc, &d) != 4) return -5;
    *gx = (int) (a * 256 + b);
    if (sscanf(r, "2B %x %x %x %x", &a, &b, &cc, &d) != 4) return -6;
    *gy = (int) (a * 256 + b);
    return 0;
}

static const char* LCD_C5   = "st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv";
static const char* EXP_C5   = "ch32,i2c:0,addr:0x24";
static const char* TOUCH_C5 = "cst3530,i2c:0,addr:0x58,rst:x0";

/* Un caso de ERROR: el ENV (o el .seq) mal, y lo que tiene que decir `estado`. */
static void caso_mal(const char* lcd, const char* exp, const char* touch, const char* espera, const char* titulo) {
    bpvm_pant_t p;
    const char* e;
    char msg[256];
    int w = 0, h = 0;
    g_env_lcd = lcd; g_env_exp = exp; g_env_touch = touch;
    if (cargar(spi_pack_init, &p) != 0 || !p.estado) { snprintf(msg, sizeof msg, "%s: el driver no carga", titulo); CHECK(0, msg); return; }
    e = p.estado();
    snprintf(msg, sizeof msg, "%s -> estado = \"%s\"", titulo, e ? e : "(NULL)");
    CHECK(e && strstr(e, espera) != 0, msg);
    CHECK(p.size(&w, &h) == 0 && p.init() != 0, "   y ni size ni init siguen adelante");
    CHECK(g_abiertos == 0, "   y no queda ningun fichero abierto");
}

int main(int argc, char** argv) {
    FILE* f;
    size_t n;
    bpvm_pant_t p5, ps;
    char a[256], b[256];
    int i;
    if (argc < 2) { printf("uso: test_driverspi <st7789.seq>\n"); return 2; }
    g_verbose = (argc > 2);
    f = fopen(argv[1], "rb");
    if (!f) { printf("no puedo abrir %s\n", argv[1]); return 2; }
    n = fread(g_st7789, 1, sizeof g_st7789 - 1, f); g_st7789[n] = 0; fclose(f);
    fs_pon("pack:DriverSpi/st7789.seq", g_st7789);
    fs_pon("/sys/mal.seq", "11\nB0 0G F0\n");
    fs_pon("/sys/lenta.seq", "11\nespera 5000\n");
    fs_pon("/sys/vacia.seq", "# nada\n\n");
    fs_pon("/sys/gc.seq", "0xEF\r\n0xEB, 0x14   # con 0x, comas y CRLF\r\nespera 5\r\n11\r\nespera 120");
    for (i = 0; i < (int) sizeof g_px; i++) g_px[i] = (uint8_t) i;
    bios_monta();

    printf("== V7/H6.c G3: el driver generico (DriverSpi) contra el de la C5 (DriverC5) ==\n");

    /* ── 1. LA C5, CON LOS DOS DRIVERS ── */
    g_env_lcd = LCD_C5; g_env_exp = EXP_C5; g_env_touch = TOUCH_C5;
    g_pin_dc = 9; g_pin_cs = 10;
    CHECK(cargar(c5_pack_init, &p5) == 0, "DriverC5 carga y publica 'PANT'");
    pasada(&p5, &g_c5);
    CHECK(cargar(spi_pack_init, &ps) == 0, "DriverSpi carga y publica 'PANT', con el perfil de la C5 en el ENV");
    CHECK(ps.touch != 0 && ps.estado != 0 && ps.rotate != 0, "  y publica touch (hay linea touch=), estado y rotate");
    CHECK(ps.estado() == 0, "  y su estado es LISTO (NULL)");
    g_exigir_turno = 1; g_hz_turno = 40000000; g_turnos = g_turno_mal = g_turno_hz_mal = 0; g_transacciones = 0;
    pasada(&ps, &g_spi);
    g_exigir_turno = 0; g_hz_turno = 0;
    {   char m[160];
        snprintf(m, sizeof m, "V7/H6.c G4.a2: TODO el SPI de DriverSpi va en TURNOS de la BIOS v7 (%d turnos; %d fuera de turno o anidados)",
                 g_turnos, g_turno_mal);
        CHECK(g_turnos > 0 && g_turno_mal == 0 && g_turno_cs == -2, m);
        snprintf(m, sizeof m, "V7/H6.c G4.a3: y cada turno es una TRANSACCION de la v8 (%d de %d; ninguno a mano)",
                 g_transacciones, g_turnos);
        CHECK(g_transacciones == g_turnos, m);
        snprintf(m, sizeof m, "  cada turno con SU configuracion: 40 MHz (%d a otra velocidad)", g_turno_hz_mal);
        CHECK(g_turno_hz_mal == 0, m);
    }
    if (g_verbose) printf("\n--- arranque DriverC5 ---\n%s\n--- arranque DriverSpi ---\n%s\n", g_c5.init_cinta, g_spi.init_cinta);

    CHECK(g_c5.init_r == 0 && g_spi.init_r == 0, "los dos arrancan (init = 0)");
    CHECK(g_c5.w == 240 && g_c5.h == 320 && g_spi.w == 240 && g_spi.h == 320, "el mismo tamano: 240x320");
    CHECK(g_c5.fmt == BPVM_PANT_RGB565_BE && g_spi.fmt == BPVM_PANT_RGB565_BE, "el mismo formato: RGB565 girado");
    CHECK(mismos_comandos(g_c5.init_cmds, g_spi.init_cmds), "el arranque manda LOS MISMOS comandos con los mismos parametros");
    { char* s; static char t[2048]; copia(t, sizeof t, g_c5.init_cmds); for (s = t; *s; s++) if (*s == '\n') *s = '|'; printf("        DriverC5 : %s\n", t);
      copia(t, sizeof t, g_spi.init_cmds); for (s = t; *s; s++) if (*s == '\n') *s = '|'; printf("        DriverSpi: %s\n", t); }
    CHECK(strcmp(primera(g_spi.init_cmds, a, sizeof a), "11") == 0 && strcmp(primera(g_c5.init_cmds, b, sizeof b), "11") == 0,
          "  el primero, en los dos: SLPOUT (11)");
    CHECK(strcmp(ultima(g_spi.init_cmds), "29") == 0 && strcmp(ultima(g_c5.init_cmds), "29") == 0,
          "  el ultimo, en los dos: DISPON (29)");
    CHECK(strstr(g_spi.init_cinta, "(¡CS ALTO!)") == 0 && strstr(g_c5.init_cinta, "(¡CS ALTO!)") == 0,
          "  ni un byte de SPI con CS alto");
    /* V7/H6.c G4: DriverC5 lleva los pines escritos; DriverSpi pide el bus DE LA
     * PLACA (-1 = los de su línea spi2=/i2c0=, los pone la HAL BP) a la misma
     * velocidad y en el mismo modo. */
    CHECK(strcmp(linea_que_empieza(g_c5.init_cinta, "spi_init", a, sizeof a), "spi_init 2 sck 6 mosi 7 miso -1 40000000 Hz modo 0") == 0 &&
          strcmp(linea_que_empieza(g_spi.init_cinta, "spi_init", b, sizeof b), "spi_init 2 sck -1 mosi -1 miso -1 40000000 Hz modo 0") == 0,
          "  el SPI 2: DriverC5 con sus pines (6, 7), DriverSpi el de la placa (-1), los dos a 40 MHz y modo 0");
    CHECK(strcmp(linea_que_empieza(g_c5.init_cinta, "i2c_init", a, sizeof a), "i2c_init 0 sda 0 scl 1 400000 Hz") == 0 &&
          strcmp(linea_que_empieza(g_spi.init_cinta, "i2c_init", b, sizeof b), "i2c_init 0 sda -1 scl -1 0 Hz") == 0,
          "  el I2C 0: DriverC5 con sus pines y 400 kHz, DriverSpi el de la placa (pines y velocidad de i2c0=)");
    CHECK(g_c5.exp_dir == 0xFF && g_spi.exp_dir == 0xFF, "  el expansor: los ocho EXIO como salida, en los dos");
    CHECK(g_c5.exp_out == 0x03 && g_spi.exp_out == 0x03, "  y al acabar, los dos resets SUELTOS (salidas 0x03), en los dos");
    CHECK(g_c5.exp_pwm == 255 && g_spi.exp_pwm == 255, "  y la luz a tope (PWM 255), en los dos");
    CHECK(strcmp(g_c5.flush, g_spi.flush) == 0 && g_c5.flush[0], "el VOLCADO es byte a byte el mismo (ventana, datos, troceado a 64)");
    for (i = 0; i < 4; i++) {
        char m[96];
        snprintf(m, sizeof m, "el GIRO de %d y su volcado, byte a byte el mismo", i * 90);
        CHECK(strcmp(g_c5.giro[i], g_spi.giro[i]) == 0 && g_c5.giro[i][0], m);
    }
    CHECK(g_c5.toque_r[0] == 1 && g_spi.toque_r[0] == 1 && g_c5.tx == 120 && g_spi.tx == 120 && g_c5.ty == 160 && g_spi.ty == 160,
          "el TACTIL: un dedo en (120, 160), en los dos");
    CHECK(strcmp(g_c5.toque[0], g_spi.toque[0]) == 0, "  con el mismo trafico I2C");
    CHECK(g_c5.toque_r[1] == 0 && g_spi.toque_r[1] == 0 && strcmp(g_c5.toque[1], g_spi.toque[1]) == 0,
          "  y sin dedo: 0, con el mismo trafico");
    CHECK(g_abiertos == 0, "el .seq se leyo y se CERRO (ningun fichero abierto)");
    {   /* el táctil que NO contesta: «sin dedo», pero dicho UNA vez */
        int x = -1, y = -1, r1, r2;
        const char* primero;
        g_tactil_mudo = 1;
        r1 = ps.touch(&x, &y); r2 = ps.touch(&x, &y);
        g_tactil_mudo = 0;
        primero = strstr(g_log, "el tactil no contesta en I2C 0x58");
        CHECK(r1 == 0 && r2 == 0, "un tactil que no contesta: sin dedo, y la GUI sigue");
        CHECK(primero && strstr(primero + 1, "el tactil no contesta") == 0, "  pero se DICE en el log, una sola vez");
    }

    /* ── 2. LA TRAZA: lo que ve quien escribe su .seq ── */
    g_env_lcd = "st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv,traza";
    CHECK(cargar(spi_pack_init, &ps) == 0 && ps.init() == 0, "con 'traza' arranca igual");
    CHECK(strstr(g_log, "seq 11") && strstr(g_log, "seq espera 100") && strstr(g_log, "seq B0 00 F0") && strstr(g_log, "placa 36 00"),
          "  y el log dice cada comando: seq 11, espera 100, B0 00 F0, y lo de la placa (36 00...)");
    {   /* la traza del TÁCTIL (23-sep): cada cambio pulsado/suelto, con la trama */
        int x = -1, y = -1;
        memset(g_trama, 0, sizeof g_trama);
        ps.touch(&x, &y);                                  /* sin dedo: la primera lectura */
        g_trama[3] = 0x01; g_trama[4] = 120; g_trama[5] = 160; g_trama[8] = 0x10;
        ps.touch(&x, &y); ps.touch(&x, &y);                /* pulsado (dos lecturas, UN cambio) */
        g_trama[4] = 250;                                  /* x = 250: fuera de 240 */
        ps.touch(&x, &y);
        memset(g_trama, 0, sizeof g_trama);
        ps.touch(&x, &y);                                  /* suelto */
        CHECK(strstr(g_log, "tactil: primera lectura, suelto |") != 0, "  la traza del tactil: la primera lectura (suelto) con su trama");
        CHECK(strstr(g_log, "tactil: pulsado 120,160 | 00 00 00 01 78 A0 00 00 10") != 0, "  un cambio a pulsado, con el punto y la trama");
        CHECK(strstr(g_log, "tactil: pulsado FUERA 250,160") != 0, "  un punto fuera de la pantalla se dice (antes se callaba)");
        CHECK(strstr(strstr(g_log, "pulsado 120,160") + 1, "pulsado 120,160") == 0, "  y dos lecturas pulsadas seguidas son UN cambio, no dos lineas");
    }

    /* ── 2b. EL CST3530 COMO LO DESPIERTA SU FABRICANTE, Y SU LATIDO EN LA TRAZA (23-sep) ──
     * El driver Hynitron pone el chip en MODO NORMAL al despertarlo, y nosotros no lo
     * hacíamos. Y el chip tiene un LATIDO ESD: la traza lo lee cada 5 min (sólo ella:
     * el vigilante que lo leía cada 2 s para reiniciar el chip se quitó el mismo día). */
    {
        const char *s, *r;
        int x = -1, y = -1, k;
        g_env_lcd = "st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv,traza";
        g_env_exp = EXP_C5; g_env_touch = TOUCH_C5;
        g_ahora = 1000; g_esd = 0x00100000u; g_esd_vivo = 1;
        CHECK(cargar(spi_pack_init, &ps) == 0, "el MODO NORMAL y el LATIDO del CST3530 (DriverSpi, la C5, con traza)");
        grabar_desde_cero();
        CHECK(ps.init() == 0, "  arranca");
        s = strstr(g_cinta, "i2cw 0 58: D0 00 04 00\nespera 10\ni2cw 0 58: D0 00 04 00\n"
                            "i2cw 0 58: D0 00 00 00\ni2cw 0 58: D0 00 0C 00\ni2cw 0 58: D0 00 01 00\n");
        r = strstr(g_cinta, "espera 120");
        CHECK(s != 0, "  al arrancar, el modo normal: 04 00 dos veces con 10 ms (bajo consumo del I2C fuera), 00 00, 0C 00, 01 00");
        CHECK(s && r && r < s, "  despues de soltar los resets (tras sus 120 ms), no antes");
        CHECK(strstr(g_log, "no acepta el modo normal") == 0, "  y aceptado: nada en el log");

        memset(g_trama, 0, sizeof g_trama);
        grabar_desde_cero();
        for (k = 0; k < 10; k++) { ps.touch(&x, &y); g_ahora += 2000; }
        CHECK(strstr(g_cinta, "D0 04 04 00") == 0 && strstr(g_cinta, "i2cr 0 58 9") != 0,
              "  veinte segundos de sondeo: se lee la trama, y el latido NO (nadie lo vigila)");
        g_esd = 0x00123456u; g_ahora += 300000; ps.touch(&x, &y);
        CHECK(strstr(g_log, " fall · esd 00123456 | ") != 0,
              "  la traza de cada 5 min lee el latido ESD del chip (D0 04 04 00) y lo dice");
        g_tactil_mudo = 1; g_ahora += 300000; ps.touch(&x, &y); g_tactil_mudo = 0;
        CHECK(strstr(g_log, " fall · esd -- | ") != 0, "  y si el chip no contesta: esd --");
        CHECK(strstr(g_log, "reinici") == 0, "  y nadie reinicia el chip");
        g_ahora = 0;
    }

    /* ── 3. EL C6: reset y luz por GPIO, sin expansor ni tactil, y su DESFASE ── */
    g_env_lcd = "st7789,spi:2,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv";
    g_env_exp = 0; g_env_touch = 0;
    g_pin_dc = 15; g_pin_cs = 14;
    CHECK(cargar(spi_pack_init, &ps) == 0 && ps.touch == 0, "el C6: publica SIN touch (no hay linea touch=)");
    grabar_desde_cero();
    CHECK(ps.init() == 0, "  y arranca");
    CHECK(strstr(g_cinta, "gpio_init 21 1") && strstr(g_cinta, "g 21=0\nespera 10\ng 21=1\nespera 120\n"),
          "  el reset por GPIO 21: abajo 10 ms, arriba, 120 ms");
    CHECK(strstr(g_cinta, "gpio_init 22 1") && strstr(g_cinta, "g 22=0") && strstr(g_cinta, "g 22=1\n") &&
          strstr(g_cinta, "g 22=1\n") > strstr(g_cinta, "C: 29"), "  la luz por GPIO 22: apagada al tomarla, encendida DESPUES de DISPON");
    CHECK(strstr(g_cmds, "\n01") == 0 && strncmp(g_cmds, "01", 2) != 0, "  y sin SWRESET (tiene pin de reset)");
    CHECK(strstr(g_cinta, "i2c") == 0, "  ni una palabra por I2C");
    {
        static const int G[4][2] = { { 0, 0 }, { 0, 0 }, { 0, 80 }, { 80, 0 } };   /* la tabla de la imagen del C6 */
        for (i = 0; i < 4; i++) {
            int gx = -1, gy = -1; char m[128];
            int r = desfase(i * 90, &gx, &gy);
            snprintf(m, sizeof m, "  el desfase a %3d: (%d, %d) — la tabla del C6 dice (%d, %d)", i * 90, gx, gy, G[i][0], G[i][1]);
            CHECK(r == 0 && gx == G[i][0] && gy == G[i][1], m);
        }
    }

    /* ── 4. LAS 135x240 (TTGO T-Display y hermanas): cristal descentrado, y SIN pin de reset ── */
    g_env_lcd = "st7789,spi:2,dc:16,cs:5,w:135,h:240,ram:240x320,off:52x40";
    g_pin_dc = 16; g_pin_cs = 5;
    CHECK(cargar(spi_pack_init, &ps) == 0, "una 135x240 (off:52x40)");
    grabar_desde_cero();
    CHECK(ps.init() == 0, "  arranca");
    CHECK(strncmp(g_cmds, "01\n11", 5) == 0 && strstr(g_cinta, "C: 01\n") && strstr(strstr(g_cinta, "C: 01\n"), "espera 120"),
          "  sin pin de reset: SWRESET (01) y 120 ms, ANTES del .seq");
    CHECK(strstr(g_cmds, "\n20") != 0 && strstr(g_cmds, "\n21") == 0, "  sin 'inv': INVOFF (20), no INVON");
    CHECK(strstr(g_cinta, "spi_init 2 sck -1 mosi -1 miso -1 20000000 Hz") != 0, "  sin 'mhz': 20 MHz");
    {
        static const int G[4][2] = { { 52, 40 }, { 40, 53 }, { 53, 40 }, { 40, 52 } };
        for (i = 0; i < 4; i++) {
            int gx = -1, gy = -1; char m[128];
            int r = desfase(i * 90, &gx, &gy);
            snprintf(m, sizeof m, "  el desfase a %3d: (%d, %d) — el de las 135x240 es (%d, %d)", i * 90, gx, gy, G[i][0], G[i][1]);
            CHECK(r == 0 && gx == G[i][0] && gy == G[i][1], m);
        }
    }
    g_env_lcd = "st7789,spi:2,dc:16,cs:5,w:135,h:240,ram:240x320,off:52x40,bgr";
    {
        uint8_t m0 = 0;
        CHECK(cargar(spi_pack_init, &ps) == 0 && ps.init() == 0, "con 'bgr'");
        grabar_desde_cero(); ps.rotate(90);
        CHECK(sscanf(g_cmds, "36 %hhx", &m0) == 1 && m0 == (0x60 | 0x08), "  MADCTL a 90 lleva el bit BGR: 68 (MV|MX|BGR)");
    }

    /* ── 5. UN .seq DEL USUARIO, con lo que trae un ejemplo de fabricante (0x, comas, CRLF) ── */
    g_env_lcd = "gc9a01,spi:2,dc:9,cs:10,rst:1,w:240,h:240,seq:/sys/gc.seq";
    g_pin_dc = 9; g_pin_cs = 10;
    CHECK(cargar(spi_pack_init, &ps) == 0 && ps.estado() == 0, "un controlador que no traemos, con seq:/sys/gc.seq -> listo");
    grabar_desde_cero();
    CHECK(ps.init() == 0 && strncmp(g_cmds, "EF\nEB 14\n11\n", 12) == 0 && strstr(g_cinta, "espera 5\n"),
          "  y manda SU secuencia: EF, EB 14, 11 (con sus esperas)");

    /* ── 6. LAS ERRATAS: cada una CON NOMBRE en estado, y nada sigue adelante ── */
    caso_mal(0, 0, 0, "falta la linea 'lcd'", "sin linea lcd=");
    caso_mal("st7789,spi:2,cs:10,w:240,h:320", 0, 0, "falta 'dc'", "sin 'dc'");
    caso_mal("st7789,spi:2,dc:9,cs:10,rts:21,w:240,h:320", 0, 0, "no conozco 'rts'", "una errata en una clave (rts)");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320,invertido", 0, 0, "no conozco 'invertido'", "una bandera que no existe");
    caso_mal("st7789,spi:2,dc:9a,cs:10,w:240,h:320", 0, 0, "'dc' no es un numero", "un numero mal (dc:9a)");
    caso_mal("st7789,spi:2,dc:9,cs:10,rst:x1,w:240,h:320", 0, 0, "pide la linea 'exp'", "un pin x1 sin expansor");
    caso_mal("ili9341,spi:2,dc:9,cs:10,w:240,h:320", 0, 0, "no conozco 'ili9341'", "un controlador sin .seq en el pack");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320,seq:/sys/nada.seq", 0, 0, "no encuentro /sys/nada.seq", "un seq: que no existe");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320,seq:/sys/mal.seq", 0, 0, "mal.seq linea 2: '0G' no es un byte", "un .seq con un byte malo");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320,seq:/sys/lenta.seq", 0, 0, "lenta.seq linea 2: 'espera'", "un .seq con una espera de 5 s");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320,seq:/sys/vacia.seq", 0, 0, "no tiene ningun comando", "un .seq sin comandos");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:240,ram:240x320,off:0x81", 0, 0, "no cabe en 'ram'", "un cristal que no cabe (off:0x81)");
    caso_mal("st7789,spi:2,dc:9,cs:10,w:240,h:320", 0, "cst3530,addr:0x58", "falta 'i2c'", "un tactil sin bus y sin expansor");
    caso_mal(LCD_C5, "pcf8574,i2c:0,addr:0x20", TOUCH_C5, "no conozco el expansor 'pcf8574'", "un expansor que no conocemos");
    caso_mal(LCD_C5, "ch32,i2c:0", TOUCH_C5, "ENV exp: falta 'addr'", "un expansor sin addr");
    caso_mal("spi:2,dc:9,cs:10,w:240,h:320", 0, 0, "el primer campo es el tipo", "sin el controlador delante");
    /* V7/H6.c G4 — los pines del BUS van en SU línea, no en la del dispositivo */
    caso_mal("st7789,spi:2,sck:6,mosi:7,dc:9,cs:10,w:240,h:320", 0, 0, "'sck' es del bus: va en su linea spiN=", "sck en la linea lcd= (es del bus)");
    caso_mal(LCD_C5, "ch32,i2c:0,sda:0,scl:1,addr:0x24", TOUCH_C5, "'sda' es del bus: va en su linea i2cN=", "sda en la linea exp= (es del bus)");
    caso_mal(LCD_C5, EXP_C5, "cst3530,i2c:0,addr:0x58,khz:400", "'khz' es del bus: va en su linea i2cN=", "khz en la linea touch= (es del bus)");
    caso_mal("st7789,spi:3,dc:9,cs:10,w:240,h:320", 0, 0, "falta la linea 'spi3' (el bus de la pantalla)", "la pantalla en un SPI sin su linea spi3=");
    caso_mal(LCD_C5, "ch32,i2c:1,addr:0x24", "cst3530,i2c:0,addr:0x58,rst:x0", "falta la linea 'i2c1' (el bus del expansor)", "el expansor en un I2C sin su linea i2c1=");

    /* ── 7. Y ARREGLADO EL ENV, pasa SIN recargar el driver ── */
    g_env_lcd = "st7789,spi:2,cs:10,w:240,h:320"; g_env_exp = 0; g_env_touch = 0;
    CHECK(cargar(spi_pack_init, &ps) == 0 && ps.estado() != 0, "sin 'dc': no esta listo");
    g_env_lcd = "st7789,spi:2,dc:9,cs:10,w:240,h:320";
    CHECK(ps.estado() == 0, "  y anadido 'dc' al ENV, el MISMO driver cargado ya esta listo (se relee hasta que vale)");

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
