/*
 * test_bus.c — V7/H6.c G4.a1: los buses SPI e I2C como recurso COMPARTIDO de la
 * HAL BP, en el PC (V7_IDEAS §H6.11).
 *
 * La regla, caso por caso, contra una cintura de mentira que CUENTA cuántas veces
 * se abre, se ajusta y se cierra cada bus —el fallo de hoy era justo ése: cada
 * `init` reabría el bus y se lo quitaba al anterior—:
 *
 *   · el ENV declara el bus (`spi2=`, `i2c0=`) y sus pines quedan reservados;
 *   · un pack, el firmware y un programa se ENGANCHAN (con -1 o con los mismos
 *     pines) y la cintura se abre UNA sola vez;
 *   · otra configuración es un error con nombre, y el bus sigue como estaba;
 *   · un bus que el ENV no declara lo establece el primero; se puede soltar, salvo
 *     el del ENV o si lo usa otro; el fin de RUN suelta lo del programa;
 *   · los pines de un bus no se los lleva un Pin(), y un release() de un programa
 *     no le quita el pin a un pack.
 *
 *   make test-bus
 */
#include "bpvm_bus.h"
#include "bpvm_spi.h"
#include "bpvm_i2c.h"
#include "bpvm_gpio.h"
#include "bpvm_machine.h"
#include "bpvm_platform.h"   /* G4.a3: un turno que dura */
#include "bpvm.h"            /* G4.a3: bpvm_diag_set_sink */

#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* ── un micro de mentira: 40 pines digitales, SPI 2 y 3, I2C 0 y 1 (como un ESP32) ── */
static const uint8_t s_i2c[] = { 0, 1 };
static const uint8_t s_spi[] = { 2, 3 };
static uint16_t s_pin[40];
static bpvm_caps_t s_caps;

/* ── la cintura de mentira: cuenta ── */
static int n_abrir[4], n_ajustar[4], n_cerrar[4], n_init[4], ult_hz[4], ult_modo[4], ult_p[4][3];
static int i2c_abrir[2], i2c_cerrar[2], i2c_hz[2];
static int g_abrir_falla = 0;
static int  f_abrir(int bus, int sck, int mosi, int miso) {
    if (g_abrir_falla) return -1;
    n_abrir[bus]++; ult_p[bus][0] = sck; ult_p[bus][1] = mosi; ult_p[bus][2] = miso; return 0;
}
static void f_ajustar(int bus, int hz, int modo) { n_ajustar[bus]++; ult_hz[bus] = hz; ult_modo[bus] = modo; }
static void f_cerrar(int bus) { n_cerrar[bus]++; }
static void f_init(int bus, int a, int b, int c, int hz, int modo) { (void) a; (void) b; (void) c; (void) hz; (void) modo; n_init[bus]++; }
static int  f_w(int bus, const uint8_t* d, size_t n) { (void) bus; (void) d; return (int) n; }
static int  f_r(int bus, uint8_t* d, size_t n) { (void) bus; (void) d; return (int) n; }
static int  f_t(int bus, const uint8_t* t, uint8_t* r, size_t n) { (void) bus; (void) t; (void) r; return (int) n; }
static const bpvm_spi_backend_t SPI_NUEVA  = { f_init, f_w, f_r, f_t, f_abrir, f_ajustar, f_cerrar };
static const bpvm_spi_backend_t SPI_LEGADO = { f_init, f_w, f_r, f_t, 0, 0, 0 };

static int  fi_abrir(int bus, int sda, int scl, int hz) { (void) sda; (void) scl; i2c_abrir[bus]++; i2c_hz[bus] = hz; return 0; }
static void fi_cerrar(int bus) { i2c_cerrar[bus]++; }
static void fi_init(int bus, int sda, int scl, int hz) { (void) bus; (void) sda; (void) scl; (void) hz; }
static int  fi_w(int bus, int a, const uint8_t* d, size_t n) { (void) bus; (void) a; (void) d; return (int) n; }
static int  fi_r(int bus, int a, uint8_t* d, size_t n) { (void) bus; (void) a; (void) d; return (int) n; }
static const bpvm_i2c_backend_t I2C_NUEVA = { fi_init, fi_w, fi_r, fi_abrir, fi_cerrar };

/* G4.a2: los pines de mentira, para ver el CS que mueve el turno (-1 = nunca escrito). */
static int g_nivel[40];
static void fg_init(int pin, int mode) { (void) pin; (void) mode; }
static void fg_pull(int pin, int m) { (void) pin; (void) m; }
static void fg_write(int pin, int v) { if (pin >= 0 && pin < 40) g_nivel[pin] = v ? 1 : 0; }
static int  fg_read(int pin) { return (pin >= 0 && pin < 40) ? g_nivel[pin] : 0; }
static const bpvm_gpio_backend_t GPIO_FALSO = { fg_init, fg_pull, fg_write, fg_read };

/* ── el ENV de mentira ── */
static const char* g_spi2 = 0; static const char* g_i2c0 = 0; static const char* g_i2c1 = 0;
static int f_env(const char* k, char* buf, size_t cap) {
    const char* v = !strcmp(k, "spi2") ? g_spi2 : !strcmp(k, "i2c0") ? g_i2c0 : !strcmp(k, "i2c1") ? g_i2c1 : 0;
    if (!v) return -1;
    if (strlen(v) + 1 > cap) return -2;
    strcpy(buf, v);
    return (int) strlen(v);
}

static void cerrar_cb(int tipo, int id) { if (tipo == BPVM_BUS_SPI) bpvm_spi_cerrar(id); else bpvm_i2c_cerrar(id); }

static char E[160];
static int  err_es(const char* que) { return strstr(E, que) != 0; }
static void caso(const char* titulo, int ok) { char m[256]; snprintf(m, sizeof m, "%s%s%s", titulo, E[0] ? "  -> " : "", E); CHECK(ok, m); E[0] = 0; }

/* ── G4.a3: una cintura que GRABA cada transferencia —cuántos bytes, el primero, cómo
 * estaba el CS del dispositivo y a qué velocidad iba el bus en ese instante— ── */
typedef struct { int n, primero, cs5, cs10, hz; } trozo_t;
static trozo_t g_cinta[32];
static int g_ntrozos = 0;
static void graba(size_t n, const uint8_t* tx, int bus) {
    if (g_ntrozos < 32) {
        trozo_t* t = &g_cinta[g_ntrozos++];
        t->n = (int) n; t->primero = tx ? tx[0] : -1; t->cs5 = g_nivel[5]; t->cs10 = g_nivel[10]; t->hz = ult_hz[bus];
    }
}
static int g_lento_ms = 0;   /* una cintura LENTA: cada escritura tarda esto */
static int  c_w(int bus, const uint8_t* d, size_t n) { graba(n, d, bus); if (g_lento_ms) bpvm_platform_thread_sleep_ms(g_lento_ms); return (int) n; }
static int  c_r(int bus, uint8_t* d, size_t n) { size_t i; graba(n, 0, bus); for (i = 0; i < n; i++) d[i] = (uint8_t) (0xA0 + i); return (int) n; }
static int  c_t(int bus, const uint8_t* t, uint8_t* r, size_t n) { size_t i; graba(n, t, bus); for (i = 0; i < n; i++) r[i] = (uint8_t) ~t[i]; return (int) n; }
static const bpvm_spi_backend_t SPI_CINTA = { f_init, c_w, c_r, c_t, f_abrir, f_ajustar, f_cerrar };

/* Y una de I2C que SÍ sabe hacer la transacción (repeated start): la llama una vez y
 * guarda sus mensajes. `g_nak` = el dispositivo no contesta. */
static int g_tr_llamadas = 0, g_tr_nm = 0, g_tr_addr = -1, g_tr_lee[8], g_tr_n[8], g_nak = 0;
static int g_iw = 0, g_ir = 0;
static int  ti_w(int bus, int a, const uint8_t* d, size_t n) { (void) bus; (void) a; (void) d; g_iw++; return (int) n; }
static int  ti_r(int bus, int a, uint8_t* d, size_t n) { (void) bus; (void) a; (void) d; g_ir++; return (int) n; }
static int  ti_trans(int bus, int addr, const bpvm_i2c_msg_t* m, int nm) {
    int i;
    (void) bus;
    g_tr_llamadas++; g_tr_nm = nm; g_tr_addr = addr;
    for (i = 0; i < nm && i < 8; i++) {
        g_tr_lee[i] = (int) m[i].lee; g_tr_n[i] = (int) m[i].n;
        if (m[i].lee) { uint32_t j; for (j = 0; j < m[i].n; j++) m[i].buf[j] = (uint8_t) (0x50 + j); }
    }
    return g_nak ? -1 : 0;
}
static const bpvm_i2c_backend_t I2C_TRANS = { fi_init, ti_w, ti_r, fi_abrir, fi_cerrar, ti_trans };
static const bpvm_i2c_backend_t I2C_SIN   = { fi_init, ti_w, ti_r, fi_abrir, fi_cerrar, 0 };

/* El log: la última línea de bpvm_diag. */
static char g_diag[200];
static void diag_a_mano(const char* linea) { snprintf(g_diag, sizeof g_diag, "%s", linea); }

/* ── V7/H6.c G4.a3: la TRANSACCIÓN y el TOPE (V7_IDEAS §H6.11.9) ── */
static void g4a3(void) {
    int r;
    uint8_t cab[4] = { 0x02, 0x00, 0x10, 0x00 }, pag[8] = { 1, 2, 3, 4, 5, 6, 7, 8 }, st[2];
    static uint8_t mucho[4000];
    printf("\n== V7/H6.c G4.a3: la TRANSACCIÓN, corta; el tope del BUS ==\n");
    bpvm_bus_reset();
    bpvm_spi_set_backend(&SPI_CINTA);
    bpvm_i2c_set_backend(&I2C_TRANS);
    bpvm_diag_set_sink(diag_a_mano);
    memset(n_ajustar, 0, sizeof n_ajustar);
    g_nivel[5] = g_nivel[10] = -1;

    /* ── el `ms:` del ENV ── */
    caso("spi2 con ms:5 -> el tope de ESE bus es 5 ms",
         bpvm_bus_declarar(BPVM_BUS_SPI, 2, "sck:6,mosi:7,miso:8,ms:5", E, sizeof E) == 0 && bpvm_bus_tope_ms(BPVM_BUS_SPI, 2) == 5);
    CHECK(bpvm_bus_tope_ms(BPVM_BUS_SPI, 3) == BPVM_BUS_TOPE_MS, "  un bus sin ms: -> 20 ms (BPVM_BUS_TOPE_MS)");
    caso("i2c1 con ms:150 -> «ms:150 no vale: el tope de un turno va de 1 a 100 ms» (configurable, pero limitado)",
         bpvm_bus_declarar(BPVM_BUS_I2C, 1, "sda:20,scl:21,ms:150", E, sizeof E) != 0 && err_es("ENV i2c1: ms:150 no vale: el tope de un turno va de 1 a 100 ms"));
    caso("i2c1 con ms:0 -> tampoco", bpvm_bus_declarar(BPVM_BUS_I2C, 1, "sda:20,scl:21,ms:0", E, sizeof E) != 0 && err_es("ms:0 no vale"));
    caso("i2c1 con ms:x -> «'ms' no es un número»", bpvm_bus_declarar(BPVM_BUS_I2C, 1, "sda:20,scl:21,ms:x", E, sizeof E) != 0 && err_es("ENV i2c1: 'ms' no es un número"));
    caso("i2c1 con ms:100 -> vale (el máximo)", bpvm_bus_declarar(BPVM_BUS_I2C, 1, "sda:20,scl:21,khz:100,ms:100", E, sizeof E) == 0 && bpvm_bus_tope_ms(BPVM_BUS_I2C, 1) == 100);

    /* ── la cuenta: entera, y lo que dice ── */
    caso("SPI 3 (20 ms): 2500 bytes a 1 MHz son 20 ms justos -> caben", bpvm_bus_cabe(BPVM_BUS_SPI, 3, 2500ULL * 8, 1000000, E, sizeof E) == 0);
    caso("  2501 bytes -> «SPI 3: la transacción ocuparía el bus 21 ms a 1 MHz (máximo 20 ms)»",
         bpvm_bus_cabe(BPVM_BUS_SPI, 3, 2501ULL * 8, 1000000, E, sizeof E) != 0 && err_es("SPI 3: la transacción ocuparía el bus 21 ms a 1 MHz (máximo 20 ms)"));
    caso("  la velocidad se dice como es: «a 400 kHz»", bpvm_bus_cabe(BPVM_BUS_SPI, 3, 9000ULL * 8, 400000, E, sizeof E) != 0 && err_es("a 400 kHz"));
    caso("  «a 115200 Hz»", bpvm_bus_cabe(BPVM_BUS_SPI, 3, 9000ULL * 8, 115200, E, sizeof E) != 0 && err_es("a 115200 Hz"));
    caso("  sin velocidad -> «SPI 3: velocidad no válida (0)»", bpvm_bus_cabe(BPVM_BUS_SPI, 3, 8, 0, E, sizeof E) != 0 && err_es("SPI 3: velocidad no válida (0)"));

    /* ── la transacción SPI: el CS abajo de principio a fin, y la velocidad del dispositivo ── */
    caso("el programa se engancha al SPI 2 del ENV", bpvm_spi_enganchar(2, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && n_abrir[2] >= 1);
    {
        bpvm_spi_op_t ops[3];
        ops[0].tx = cab; ops[0].rx = 0;   ops[0].n = 4;
        ops[1].tx = pag; ops[1].rx = 0;   ops[1].n = 8;
        ops[2].tx = 0;   ops[2].rx = st;  ops[2].n = 2;
        g_ntrozos = 0;
        r = bpvm_spi_transaccion(2, 5, 4000000, 0, BPVM_BUS_PROGRAMA, ops, 3, E, sizeof E);
        caso("una transacción de 3 operaciones (CS 5, 4 MHz): 14 bytes", r == 14 && g_ntrozos == 3);
        CHECK(g_cinta[0].cs5 == 0 && g_cinta[1].cs5 == 0 && g_cinta[2].cs5 == 0, "  las tres con el CS 5 ABAJO (un solo CS de principio a fin)");
        CHECK(g_cinta[0].primero == 0x02 && g_cinta[1].primero == 1 && g_cinta[2].primero == -1 && st[0] == 0xA0,
              "  en su orden: la cabecera, la página y la lectura (que llega a su sitio)");
        CHECK(g_cinta[0].hz == 4000000 && g_cinta[2].hz == 4000000, "  a la velocidad del DISPOSITIVO (4 MHz)");
        CHECK(g_nivel[5] == 1 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2), "  y al acabar, el CS arriba y el bus libre");
    }
    {
        bpvm_spi_op_t op[1];
        op[0].tx = mucho; op[0].rx = 0; op[0].n = 4000;
        g_ntrozos = 0; g_nivel[5] = 1;
        caso("4000 bytes a 4 MHz son 8 ms, y el SPI 2 tiene ms:5 -> «ocuparía el bus 8 ms a 4 MHz (máximo 5 ms)»",
             bpvm_spi_transaccion(2, 5, 4000000, 0, BPVM_BUS_PROGRAMA, op, 1, E, sizeof E) == -1 && err_es("SPI 2: la transacción ocuparía el bus 8 ms a 4 MHz (máximo 5 ms)"));
        CHECK(g_ntrozos == 0 && g_nivel[5] == 1 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2), "  y NADA llegó al cable: ni un byte, ni el CS abajo, ni el turno");
        op[0].tx = 0;
        caso("una operación sin tx ni rx -> «la operación 1 no tiene ni tx ni rx»",
             bpvm_spi_transaccion(2, 5, 4000000, 0, BPVM_BUS_PROGRAMA, op, 1, E, sizeof E) == -1 && err_es("la operación 1 no tiene ni tx ni rx"));
        caso("un modo 5 -> «modo no válido (5)»", bpvm_spi_transaccion(2, 5, 4000000, 5, BPVM_BUS_PROGRAMA, op, 0, E, sizeof E) == -1 && err_es("modo no válido (5)"));
        caso("un bus sin abrir -> «el SPI 3 no está abierto»", bpvm_spi_transaccion(3, 5, 4000000, 0, BPVM_BUS_PROGRAMA, op, 0, E, sizeof E) == -1 && err_es("el SPI 3 no está abierto"));
        caso("engancharse a un bus sin establecer -> «ponlo en el ENV (spi3=) o ábrelo antes con Spi.Bus»",
             bpvm_spi_enganchar(3, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("el SPI 3 no está establecido: ponlo en el ENV (spi3=) o ábrelo antes con Spi.Bus"));
    }

    /* ── el turno A MANO se mide: pasado el tope, el log lo dice con nombre ── */
    g_diag[0] = 0;
    caso("un turno a mano de un pack (CS 10) que dura 12 ms en un bus de 5",
         bpvm_spi_tomar(2, 10, 40000000, 0, BPVM_BUS_PACK, E, sizeof E) == 0);
    bpvm_platform_thread_sleep_ms(12);
    bpvm_spi_dejar(2);
    {
        char m[260]; snprintf(m, sizeof m, "  -> el log: «%s»", g_diag);
        CHECK(strstr(g_diag, "SPI 2: CS 10, un pack tuvo el bus") != 0 && strstr(g_diag, "el tope de un turno es 5 ms") != 0, m);
    }
    g_diag[0] = 0;
    caso("uno de 0 ms: nada en el log", bpvm_spi_tomar(2, 10, 40000000, 0, BPVM_BUS_PACK, E, sizeof E) == 0);
    bpvm_spi_dejar(2);
    CHECK(g_diag[0] == 0, "  (el log, vacío)");
    {
        bpvm_spi_op_t op[1];
        op[0].tx = mucho; op[0].rx = 0; op[0].n = 16;
        g_diag[0] = 0; g_lento_ms = 12;
        r = bpvm_spi_transaccion(2, 5, 4000000, 0, BPVM_BUS_PROGRAMA, op, 1, E, sizeof E);
        g_lento_ms = 0;
        CHECK(r == 16 && g_diag[0] == 0, "una TRANSACCIÓN no se mide —ya se calculó antes—: ni con una cintura que tarda 12 ms");
    }

    /* ── I2C: la transacción, con repeated start (UNA llamada a la cintura) ── */
    caso("el programa abre el I2C 1 del ENV (100 kHz, ms:100)", bpvm_i2c_abrir(1, -1, -1, 0, BPVM_BUS_PROGRAMA, E, sizeof E) == 0);
    {
        bpvm_i2c_msg_t m[3];
        uint8_t reg[1] = { 0xF7 }, dat[6];
        m[0].buf = reg; m[0].n = 1; m[0].lee = 0;
        m[1].buf = dat; m[1].n = 6; m[1].lee = 1;
        g_tr_llamadas = 0; g_iw = g_ir = 0;
        r = bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 2, E, sizeof E);
        caso("escribir el registro y leer 6: UNA llamada a la cintura, con sus dos mensajes",
             r == 7 && g_tr_llamadas == 1 && g_tr_nm == 2 && g_tr_addr == 0x76 && g_tr_lee[0] == 0 && g_tr_lee[1] == 1 && g_tr_n[1] == 6);
        CHECK(g_iw == 0 && g_ir == 0 && dat[0] == 0x50, "  sin pasar por write/read sueltos (que llevarían STOP), y lo leído llega");
        CHECK(bpvm_i2c_escribir_leer(BPVM_BUS_PACK, 1, 0x58, reg, 1, dat, 4) == 4 && g_tr_llamadas == 2,
              "la ranura escribir_leer de la BIOS es esa misma transacción (repeated start)");
        g_nak = 1;
        r = bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 2, E, sizeof E);
        caso("un dispositivo que no contesta -> -2, «I2C 1: 0x76 no contestó»", r == -2 && err_es("I2C 1: 0x76 no contestó"));
        g_diag[0] = 0;
        CHECK(bpvm_i2c_escribir_leer(BPVM_BUS_PACK, 1, 0x58, reg, 1, dat, 4) == -1 && g_diag[0] == 0,
              "  y en escribir_leer, -1 SIN log: un táctil que se sondea cada pocos ms lo llenaría");
        g_nak = 0;
        caso("una dirección de 8 bits -> «dirección no válida (0x80)»", bpvm_i2c_transaccion(1, 0x80, BPVM_BUS_PROGRAMA, m, 2, E, sizeof E) == -1 && err_es("I2C 1: dirección no válida (0x80)"));
        caso("4 mensajes -> «de 1 a 3 mensajes (tiene 4)» (lo que cabe en el ESP32, en todas)", bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 4, E, sizeof E) == -1 && err_es("una transacción lleva de 1 a 3 mensajes (tiene 4)"));
        m[2].buf = dat; m[2].n = 6; m[2].lee = 1;
        caso("escribir y DOS lecturas de 6 -> «necesita 9 órdenes del bus y caben 8»",
             bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 3, E, sizeof E) == -1 && err_es("I2C 1: la transacción necesita 9 órdenes del bus y caben 8"));
        m[2].n = 0;
        caso("un mensaje vacío -> «el mensaje 3 está vacío»", bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 3, E, sizeof E) == -1 && err_es("I2C 1: el mensaje 3 está vacío"));
        m[2].buf = mucho; m[2].n = 1200; m[2].lee = 0;
        g_tr_llamadas = 0;
        caso("1200 bytes a 100 kHz son 108 ms y el I2C 1 tiene ms:100 -> «ocuparía el bus 109 ms a 100 kHz (máximo 100 ms)»",
             bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 3, E, sizeof E) == -1 && err_es("I2C 1: la transacción ocuparía el bus 109 ms a 100 kHz (máximo 100 ms)"));
        CHECK(g_tr_llamadas == 0, "  y la cintura ni se enteró");
        caso("el I2C 0, sin abrir -> «el I2C 0 no está abierto»", bpvm_i2c_transaccion(0, 0x76, BPVM_BUS_PROGRAMA, m, 2, E, sizeof E) == -1 && err_es("el I2C 0 no está abierto"));
        /* una cintura SIN transacción: los mensajes uno a uno, en el mismo turno */
        bpvm_i2c_set_backend(&I2C_SIN);
        g_iw = g_ir = 0;
        r = bpvm_i2c_transaccion(1, 0x76, BPVM_BUS_PROGRAMA, m, 2, E, sizeof E);
        caso("una cintura sin repeated start: escribir y leer uno tras otro, en UN turno", r == 7 && g_iw == 1 && g_ir == 1 && !bpvm_bus_tomado(BPVM_BUS_I2C, 1));
        bpvm_i2c_set_backend(&I2C_TRANS);
    }
    bpvm_diag_set_sink(0);
}

int main(void) {
    int i, p[3];
    for (i = 0; i < 40; i++) s_pin[i] = BPVM_PIN_DIGITAL;
    memset(&s_caps, 0, sizeof s_caps);
    s_caps.micro = "micro de prueba";
    s_caps.i2c = s_i2c; s_caps.n_i2c = 2; s_caps.spi = s_spi; s_caps.n_spi = 2;
    s_caps.pin = s_pin; s_caps.n_pin = 40;
    bpvm_caps_set(&s_caps);
    bpvm_spi_set_backend(&SPI_NUEVA);
    bpvm_i2c_set_backend(&I2C_NUEVA);
    bpvm_gpio_set_backend(&GPIO_FALSO);
    for (i = 0; i < 40; i++) g_nivel[i] = -1;

    printf("== V7/H6.c G4.a1: el bus es COMPARTIDO; la configuración, una vez ==\n");

    /* ── 1. EL ENV DECLARA (la C5: spi2 e i2c0) ── */
    g_spi2 = "sck:6,mosi:7,miso:8"; g_i2c0 = "sda:0,scl:1,khz:400"; g_i2c1 = "sda:4,scl:x";
    CHECK(bpvm_bus_desde_env(f_env) == 2, "el ENV declara 2 buses (spi2, i2c0); el i2c1 mal escrito se queda sin declarar");
    caso("  y sus pines quedan RESERVADOS: Pin(6) -> «es del SPI 2»", bpvm_gpio_reclamar(6, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("el pin 6 es del SPI 2"));
    caso("  Pin(1) -> «es del I2C 0»", bpvm_gpio_reclamar(1, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("el pin 1 es del I2C 0"));
    CHECK(n_abrir[2] == 0, "  y la cintura NO se abre hasta que alguien lo usa");

    /* ── 2. LOS TRES USUARIOS SE ENGANCHAN: la cintura se abre UNA vez ── */
    caso("un PACK (la pantalla) abre el SPI 2 con -1: los pines del ENV",
         bpvm_spi_abrir(2, -1, -1, -1, 40000000, 0, BPVM_BUS_PACK, p, E, sizeof E) == 0 && p[0] == 6 && p[1] == 7 && p[2] == 8);
    CHECK(n_abrir[2] == 1 && ult_p[2][0] == 6 && ult_p[2][2] == 8 && n_ajustar[2] == 0,
          "  la cintura se abre (sck 6, miso 8) — y NO se ajusta: la velocidad va con cada turno (G4.a2)");
    caso("el FIRMWARE (la TF) abre el SPI 2 con SUS pines (los mismos)",
         bpvm_spi_abrir(2, 6, 7, 8, 400000, 0, BPVM_BUS_FIRMWARE, p, E, sizeof E) == 0);
    CHECK(n_abrir[2] == 1 && n_ajustar[2] == 0, "  la cintura NO se reabre (1 apertura), y tampoco se ajusta");
    caso("un PROGRAMA con -1: se engancha", bpvm_spi_abrir(2, -1, -1, -1, 1000000, 3, BPVM_BUS_PROGRAMA, p, E, sizeof E) == 0);
    CHECK(n_abrir[2] == 1 && n_cerrar[2] == 0, "  y sigue habiendo UNA apertura y ningún cierre");

    /* ── 3. OTRA CONFIGURACIÓN: error con nombre, y el bus como estaba ── */
    caso("un programa pide sck 12 -> «el SPI 2 es sck 6, mosi 7, miso 8 (ENV spi2): pides sck 12»",
         bpvm_spi_abrir(2, 12, 7, 8, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && err_es("el SPI 2 es sck 6, mosi 7, miso 8 (ENV spi2): pides sck 12"));
    CHECK(n_abrir[2] == 1 && n_cerrar[2] == 0 && n_ajustar[2] == 0, "  y la cintura ni se reabre ni se ajusta");
    caso("un programa pide I2C 0 a 100 kHz -> «pides 100 kHz» (la velocidad es del bus)",
         bpvm_i2c_abrir(0, -1, -1, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("el I2C 0 es sda 0, scl 1 a 400 kHz (ENV i2c0): pides 100 kHz"));
    caso("y con 0 (la que tenga) se engancha", bpvm_i2c_abrir(0, 0, 1, 0, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && i2c_abrir[0] == 1 && i2c_hz[0] == 400000);
    caso("el pack del táctil también (sin pines)", bpvm_i2c_abrir(0, -1, -1, -1, BPVM_BUS_PACK, E, sizeof E) == 0 && i2c_abrir[0] == 1);

    /* ── 3b. G4.a2 — EL TURNO: la velocidad va en CADA petición, y la del bus sólo se
     * cambia si la nueva es otra (Eduardo: «se guarda la actual; si la nueva es la misma
     * no se cambia»). Y el CS del turno lo mueve la HAL BP. ── */
    {
        uint8_t tx[4] = { 0xD0, 0x07, 0x00, 0x00 }, rx[9];
        caso("un turno de la pantalla (CS 10, 40 MHz): la cintura se ajusta a 40 MHz y el CS 10 baja",
             bpvm_spi_tomar(2, 10, 40000000, 0, BPVM_BUS_PACK, E, sizeof E) == 0 && n_ajustar[2] == 1 && ult_hz[2] == 40000000
             && g_nivel[10] == 0 && bpvm_bus_tomado(BPVM_BUS_SPI, 2));
        CHECK(bpvm_spi_xfer(2, tx, NULL, 4) == 4, "  dentro del turno se transfiere");
        CHECK(bpvm_spi_cs(2, 1) == 0 && g_nivel[10] == 1 && bpvm_spi_cs(2, 0) == 0 && g_nivel[10] == 0,
              "  y el CS sube y baja SIN soltar el turno (lo que necesita la TF)");
        bpvm_spi_dejar(2);
        CHECK(g_nivel[10] == 1 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2), "  al soltar, el CS 10 sube y el bus queda libre");
        caso("otro turno a 40 MHz: NO se ajusta (se guarda la actual)",
             bpvm_spi_tomar(2, 10, 40000000, 0, BPVM_BUS_PACK, E, sizeof E) == 0 && n_ajustar[2] == 1);
        bpvm_spi_dejar(2);
        caso("un turno de la TF (CS 23, 400 kHz): SÍ, porque es otra",
             bpvm_spi_tomar(2, 23, 400000, 0, BPVM_BUS_FIRMWARE, E, sizeof E) == 0 && n_ajustar[2] == 2 && ult_hz[2] == 400000);
        bpvm_spi_dejar(2);
        caso("la misma velocidad con otro MODO también es otra",
             bpvm_spi_tomar(2, 23, 400000, 3, BPVM_BUS_FIRMWARE, E, sizeof E) == 0 && n_ajustar[2] == 3 && ult_modo[2] == 3);
        bpvm_spi_dejar(2);
        CHECK(bpvm_spi_write(2, tx, 4) == 4 && n_ajustar[2] == 4 && ult_hz[2] == 1000000 && ult_modo[2] == 3 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2),
              "una llamada de Spi.Bus es un turno entero, con la velocidad del abrir del programa (1 MHz, modo 3)");
        CHECK(bpvm_spi_xfer(2, tx, NULL, 4) == -1, "transferir SIN turno: -1 (y dicho en el log)");
        caso("un turno que nadie suelta: el que llega espera el plazo y recibe «SPI 2 ocupado (CS 10, un pack) desde hace…»",
             bpvm_spi_tomar(2, 10, 40000000, 0, BPVM_BUS_PACK, E, sizeof E) == 0
             && bpvm_spi_tomar(2, 23, 400000, 0, BPVM_BUS_FIRMWARE, E, sizeof E) != 0 && err_es("SPI 2 ocupado (CS 10, un pack) desde hace"));
        CHECK(g_nivel[23] == 1, "  y el CS de la TF ni se tocó: sigue arriba");
        bpvm_spi_dejar(2);
        caso("en I2C, escribir y leer van en UN turno (el registro del táctil y su trama)",
             bpvm_i2c_escribir_leer(BPVM_BUS_PACK, 0, 0x58, tx, 4, rx, 9) == 9 && !bpvm_bus_tomado(BPVM_BUS_I2C, 0));
        caso("y un turno de I2C que nadie suelta se dice con la dirección: «I2C 0 ocupado (0x24, el firmware)…»",
             bpvm_bus_tomar(BPVM_BUS_I2C, 0, BPVM_BUS_FIRMWARE, 0x24, 50, E, sizeof E) == 0
             && bpvm_i2c_write(0, 0x58, tx, 4) == -1);
        bpvm_bus_dejar(BPVM_BUS_I2C, 0);
        caso("  (el porqué, con su plazo corto)", bpvm_bus_tomar(BPVM_BUS_I2C, 0, BPVM_BUS_PACK, 0x58, 50, E, sizeof E) == 0
             && bpvm_bus_tomar(BPVM_BUS_I2C, 0, BPVM_BUS_PROGRAMA, 0x24, 50, E, sizeof E) != 0 && err_es("I2C 0 ocupado (0x58, un pack) desde hace"));
        bpvm_bus_dejar(BPVM_BUS_I2C, 0);
    }

    /* ── 4. SOLTAR: el del ENV no, desde un programa ── */
    caso("un programa suelta el SPI 2 -> «es de la placa (ENV spi2)»",
         bpvm_spi_soltar(2, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("es de la placa (ENV spi2)"));
    bpvm_bus_fin_de_run(cerrar_cb);
    CHECK(n_cerrar[2] == 0 && i2c_cerrar[0] == 0, "fin de RUN: el programa se va, pero la pantalla y la TF siguen: nada se cierra");
    caso("y el SPI 2 sigue abierto con sus pines", bpvm_bus_abierto(BPVM_BUS_SPI, 2) && bpvm_gpio_reclamar(7, BPVM_USO_OUTPUT, E, sizeof E) != 0);

    /* ── 5. UN BUS QUE EL ENV NO DECLARA: lo establece el primero ── */
    caso("el SPI 3 sin declarar: con -1 -> «no está declarado: da sck y mosi, o pon spi3=»",
         bpvm_spi_abrir(3, -1, -1, -1, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && err_es("no está declarado: da sck y mosi, o pon spi3= en el ENV"));
    caso("un programa lo ESTABLECE con sck 10, mosi 11 (sin MISO)",
         bpvm_spi_abrir(3, 10, 11, -1, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) == 0 && n_abrir[3] == 1);
    caso("otro con miso 12 -> «no tiene miso»",
         bpvm_spi_abrir(3, 10, 11, 12, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && err_es("el SPI 3 es sck 10, mosi 11 (lo abrió el programa): no tiene miso"));
    caso("Pin(10) -> «es del SPI 3»", bpvm_gpio_reclamar(10, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("el pin 10 es del SPI 3"));
    caso("el programa lo SUELTA: se cierra", bpvm_spi_soltar(3, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && n_cerrar[3] == 1);
    caso("y los pines vuelven a libres: Pin(10) ya vale", bpvm_gpio_reclamar(10, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    bpvm_gpio_liberar(10);
    caso("y otro puede establecerlo DISTINTO (sck 14, mosi 15)",
         bpvm_spi_abrir(3, 14, 15, -1, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) == 0 && n_abrir[3] == 2);

    /* ── 6. SOLTAR con otro dentro: no ── */
    caso("un pack se engancha al SPI 3 del programa", bpvm_spi_abrir(3, -1, -1, -1, 8000000, 0, BPVM_BUS_PACK, p, E, sizeof E) == 0 && p[0] == 14);
    caso("el programa lo suelta -> «lo usa también un pack»",
         bpvm_spi_soltar(3, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("el SPI 3 lo usa también un pack: no se puede soltar"));
    bpvm_bus_fin_de_run(cerrar_cb);
    CHECK(n_cerrar[3] == 1 && bpvm_bus_abierto(BPVM_BUS_SPI, 3), "fin de RUN: el programa se va y el pack se queda con el SPI 3 (sin cerrar)");

    /* ── 7. EL FIN DE RUN cierra lo que sólo usaba el programa ── */
    caso("un programa establece el I2C 1 (sda 20, scl 21, 100 kHz)", bpvm_i2c_abrir(1, 20, 21, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && i2c_abrir[1] == 1);
    bpvm_bus_fin_de_run(cerrar_cb);
    CHECK(i2c_cerrar[1] == 1 && !bpvm_bus_abierto(BPVM_BUS_I2C, 1), "fin de RUN: el I2C 1 se cierra");
    caso("y sus pines vuelven a libres", bpvm_gpio_reclamar(20, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    bpvm_gpio_liberar(20);

    /* ── 8. LOS PINES: un bus no le quita el pin a nadie, y release() no suelta lo ajeno ── */
    caso("Pin(31) de un programa", bpvm_gpio_reclamar(31, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    caso("un bus libre (el I2C 1) que lo quiere de SCL -> «el pin 31 está en uso digital»",
         bpvm_i2c_abrir(1, 30, 31, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("el pin 31 está en uso digital: primero release()"));
    caso("  y el I2C 1 sigue libre, sin el 30 reservado A MEDIAS (o todos o ninguno): Pin(30) vale",
         !bpvm_bus_abierto(BPVM_BUS_I2C, 1) && bpvm_gpio_reclamar(30, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    bpvm_gpio_liberar(30); bpvm_gpio_liberar(31);
    caso("un pack toma el pin 9 (el DC de la pantalla)", bpvm_gpio_reclamar_pack(9, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    bpvm_gpio_liberar(9);        /* un Gpio.release(9) del programa, o el destructor de un Pin(9) fallido */
    caso("un release(9) del programa NO se lo quita: Pin(9) -> «lo usa un código nativo»",
         bpvm_gpio_reclamar(9, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("lo usa un código nativo"));
    bpvm_gpio_liberar(6);
    caso("ni a un bus: Pin(6) sigue siendo «del SPI 2»", bpvm_gpio_reclamar(6, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("es del SPI 2"));

    /* ── 9. LA CINTURA FALLA al abrir: el bus vuelve a libre, sin pines colgados ── */
    bpvm_bus_reset();
    g_abrir_falla = 1;
    caso("la cintura no puede abrir el SPI 3 -> error con nombre",
         bpvm_spi_abrir(3, 16, 17, -1, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && err_es("el SPI 3 no se pudo abrir"));
    g_abrir_falla = 0;
    caso("y el bus y sus pines quedan LIBRES: Pin(16) vale", !bpvm_bus_abierto(BPVM_BUS_SPI, 3) && bpvm_gpio_reclamar(16, BPVM_USO_OUTPUT, E, sizeof E) == 0);
    bpvm_gpio_liberar(16);

    /* ── 10. LA LÍNEA DEL ENV, estricta ── */
    caso("spi2 con una clave que no existe -> «no conozco 'sclk'»", bpvm_bus_declarar(BPVM_BUS_SPI, 2, "sclk:6,mosi:7", E, sizeof E) != 0 && err_es("ENV spi2: no conozco 'sclk'"));
    caso("spi2 sin mosi -> «falta 'mosi'»", bpvm_bus_declarar(BPVM_BUS_SPI, 2, "sck:6", E, sizeof E) != 0 && err_es("ENV spi2: falta 'mosi'"));
    caso("i2c0 con un número malo -> «'scl' no es un número»", bpvm_bus_declarar(BPVM_BUS_I2C, 0, "sda:0,scl:1x", E, sizeof E) != 0 && err_es("ENV i2c0: 'scl' no es un número"));
    caso("i2c0 sin khz -> 100 kHz", bpvm_bus_declarar(BPVM_BUS_I2C, 0, "sda:0,scl:1", E, sizeof E) == 0);
    {
        char d[64]; bpvm_bus_describe(BPVM_BUS_I2C, 0, d, sizeof d);
        CHECK(strcmp(d, "sda 0, scl 1 a 100 kHz") == 0, "  y se describe «sda 0, scl 1 a 100 kHz»");
    }

    /* ── 11. UNA CINTURA SIN MIGRAR sigue con init (y el PC imprime lo de siempre) ── */
    bpvm_bus_reset();
    bpvm_spi_set_backend(&SPI_LEGADO);
    caso("una cintura sin `abrir`: el programa abre el SPI 2", bpvm_spi_abrir(2, 1, 2, 3, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) == 0 && n_init[2] == 1);
    caso("y la regla vale igual (otro sck es error, y init no se llama)",
         bpvm_spi_abrir(2, 4, 2, 3, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && n_init[2] == 1);

    g4a3();

    /* ── 12. V7 #669 — LA PANTALLA DE LA IMAGEN: sus pines, del sistema desde que se publica su driver ── */
    printf("\n== V7 #669: los pines de la pantalla de la imagen, como los de la flash (#574) ==\n");
    bpvm_bus_reset();
    {
        int antes = bpvm_caps_gpio_count(), dig = bpvm_caps_pins_con(BPVM_PIN_DIGITAL);
        caso("la pantalla reserva el pin 37 (libre)", bpvm_caps_reservar_sistema(37, BPVM_PIN_SIS_PANTALLA, E, sizeof E) == 0);
        caso("y otra vez: nada (el driver publicado dos veces)", bpvm_caps_reservar_sistema(37, BPVM_PIN_SIS_PANTALLA, E, sizeof E) == 0);
        CHECK(bpvm_caps_pin_mask(37) == 0 && bpvm_caps_gpio_count() == antes - 1 && bpvm_caps_pins_con(BPVM_PIN_DIGITAL) == dig - 1,
              "  ya no está para BP: su máscara es 0, y Machine.pins() y las cuentas tienen uno menos");
        caso("Pin(37) de un programa -> «el pin 37 de micro de prueba es de la pantalla: no se puede usar»",
             bpvm_gpio_reclamar(37, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("el pin 37 de micro de prueba es de la pantalla: no se puede usar"));
        caso("ni un pack -> «es de la pantalla»", bpvm_gpio_reclamar_pack(37, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("es de la pantalla"));
        caso("ni un bus: el I2C 1 con SCL en el 37 -> «es de la pantalla», y el bus sigue libre",
             bpvm_i2c_abrir(1, 36, 37, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("es de la pantalla") && !bpvm_bus_abierto(BPVM_BUS_I2C, 1));
        caso("  sin su SDA reservado a medias (o todos o ninguno): Pin(36) vale", bpvm_gpio_reclamar(36, BPVM_USO_OUTPUT, E, sizeof E) == 0);
        caso("un pin que ya tiene el programa NO se reserva -> «el pin 36 ya está en uso»",
             bpvm_caps_reservar_sistema(36, BPVM_PIN_SIS_PANTALLA, E, sizeof E) != 0 && err_es("el pin 36 ya está en uso"));
        bpvm_gpio_liberar(36);
        caso("un pin que no existe NO se reserva -> «el pin 45 no existe en micro de prueba»",
             bpvm_caps_reservar_sistema(45, BPVM_PIN_SIS_PANTALLA, E, sizeof E) != 0 && err_es("el pin 45 no existe en micro de prueba"));
        bpvm_gpio_soltar(37);
        bpvm_gpio_estado_reset();
        CHECK(bpvm_caps_pin_mask(37) == 0, "un release() y el fin de RUN no la tocan: sigue siendo de la pantalla");
        bpvm_caps_set(&s_caps);
        CHECK(bpvm_caps_pin_mask(37) == BPVM_PIN_DIGITAL && bpvm_caps_gpio_count() == antes,
              "una tabla nueva (bpvm_caps_set) empieza sin reservas");
    }

    /* ── 13. V7 #669 — EL BUS DEL TÁCTIL que declara la imagen: de la placa, y COMPARTIDO ── */
    printf("\n== V7 #669: el bus del táctil de la imagen, compartido (Eduardo: «tiene que ser compartido») ==\n");
    bpvm_bus_reset();
    bpvm_i2c_set_backend(&I2C_NUEVA);
    memset(i2c_abrir, 0, sizeof i2c_abrir);
    memset(i2c_cerrar, 0, sizeof i2c_cerrar);
    caso("la imagen declara el I2C 0 de su táctil (sda 32, scl 33, 100 kHz): la cintura NO se abre aún",
         bpvm_bus_declarar_imagen(BPVM_BUS_I2C, 0, 32, 33, -1, 100000, "la imagen: el táctil de la pantalla", E, sizeof E) == 0
         && i2c_abrir[0] == 0 && !bpvm_bus_abierto(BPVM_BUS_I2C, 0));
    caso("sus pines son del bus: Pin(32) -> «el pin 32 es del I2C 0»",
         bpvm_gpio_reclamar(32, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("el pin 32 es del I2C 0"));
    caso("un programa con OTROS pines -> «(la imagen: el táctil de la pantalla): pides sda 30»",
         bpvm_i2c_abrir(0, 30, 31, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("(la imagen: el táctil de la pantalla): pides sda 30"));
    caso("a otra velocidad -> «pides 400 kHz»",
         bpvm_i2c_abrir(0, 32, 33, 400000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("pides 400 kHz"));
    caso("con los mismos se engancha, y la cintura se abre ENTONCES (el primer usuario)",
         bpvm_i2c_abrir(0, 32, 33, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && i2c_abrir[0] == 1);
    caso("el táctil (el firmware) también, sin pines: la cintura no se abre otra vez",
         bpvm_i2c_abrir(0, -1, -1, -1, BPVM_BUS_FIRMWARE, E, sizeof E) == 0 && i2c_abrir[0] == 1);
    caso("el programa no lo suelta -> «es de la placa (la imagen: el táctil de la pantalla): no se suelta desde un programa»",
         bpvm_i2c_soltar(0, BPVM_BUS_PROGRAMA, E, sizeof E) != 0
         && err_es("el I2C 0 es de la placa (la imagen: el táctil de la pantalla): no se suelta desde un programa"));
    bpvm_bus_fin_de_run(cerrar_cb);
    CHECK(i2c_cerrar[0] == 0 && bpvm_bus_abierto(BPVM_BUS_I2C, 0),
          "fin de RUN: el programa se va y el táctil sigue con su bus: no se cierra");
    caso("el táctil lo suelta: se cierra, pero sigue declarado (Pin(32) sigue siendo del I2C 0)",
         bpvm_i2c_soltar(0, BPVM_BUS_FIRMWARE, E, sizeof E) == 0 && i2c_cerrar[0] == 1
         && bpvm_gpio_reclamar(32, BPVM_USO_OUTPUT, E, sizeof E) != 0 && err_es("es del I2C 0"));
    caso("un programa SOLO: lo abre, y el fin de RUN lo cierra (nadie más lo usa)",
         bpvm_i2c_abrir(0, -1, -1, -1, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && i2c_abrir[0] == 2);
    bpvm_bus_fin_de_run(cerrar_cb);
    CHECK(i2c_cerrar[0] == 2 && !bpvm_bus_abierto(BPVM_BUS_I2C, 0), "  cerrado, y sigue declarado para el táctil");
    caso("una línea del ENV para el mismo bus sobra -> «ENV i2c0: es el bus de la imagen»",
         bpvm_bus_declarar(BPVM_BUS_I2C, 0, "sda:32,scl:33", E, sizeof E) != 0 && err_es("ENV i2c0: es el bus de la imagen"));
    bpvm_bus_reset();

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
