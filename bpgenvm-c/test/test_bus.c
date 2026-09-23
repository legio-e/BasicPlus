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
    CHECK(n_abrir[2] == 1 && ult_p[2][0] == 6 && ult_p[2][2] == 8 && ult_hz[2] == 40000000, "  la cintura se abre (sck 6, miso 8) y se ajusta a 40 MHz");
    caso("el FIRMWARE (la TF) abre el SPI 2 con SUS pines (los mismos)",
         bpvm_spi_abrir(2, 6, 7, 8, 400000, 0, BPVM_BUS_FIRMWARE, p, E, sizeof E) == 0);
    CHECK(n_abrir[2] == 1 && n_ajustar[2] == 2 && ult_hz[2] == 400000, "  la cintura NO se reabre (1 apertura) y sólo se ajusta (400 kHz)");
    caso("un PROGRAMA con -1: se engancha", bpvm_spi_abrir(2, -1, -1, -1, 1000000, 3, BPVM_BUS_PROGRAMA, p, E, sizeof E) == 0);
    CHECK(n_abrir[2] == 1 && n_cerrar[2] == 0, "  y sigue habiendo UNA apertura y ningún cierre");

    /* ── 3. OTRA CONFIGURACIÓN: error con nombre, y el bus como estaba ── */
    caso("un programa pide sck 12 -> «el SPI 2 es sck 6, mosi 7, miso 8 (ENV spi2): pides sck 12»",
         bpvm_spi_abrir(2, 12, 7, 8, 1000000, 0, BPVM_BUS_PROGRAMA, p, E, sizeof E) != 0 && err_es("el SPI 2 es sck 6, mosi 7, miso 8 (ENV spi2): pides sck 12"));
    CHECK(n_abrir[2] == 1 && n_cerrar[2] == 0 && ult_hz[2] == 1000000, "  y la cintura ni se reabre ni se ajusta");
    caso("un programa pide I2C 0 a 100 kHz -> «pides 100 kHz» (la velocidad es del bus)",
         bpvm_i2c_abrir(0, -1, -1, 100000, BPVM_BUS_PROGRAMA, E, sizeof E) != 0 && err_es("el I2C 0 es sda 0, scl 1 a 400 kHz (ENV i2c0): pides 100 kHz"));
    caso("y con 0 (la que tenga) se engancha", bpvm_i2c_abrir(0, 0, 1, 0, BPVM_BUS_PROGRAMA, E, sizeof E) == 0 && i2c_abrir[0] == 1 && i2c_hz[0] == 400000);
    caso("el pack del táctil también (sin pines)", bpvm_i2c_abrir(0, -1, -1, -1, BPVM_BUS_PACK, E, sizeof E) == 0 && i2c_abrir[0] == 1);

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

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
