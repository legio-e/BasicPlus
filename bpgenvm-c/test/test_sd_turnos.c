/*
 * test_sd_turnos.c — V7/H3 SD.2 (24-sep): LA TF SUELTA EL BUS EN SUS ESPERAS.
 *
 * La TF de la C5 comparte el SPI 2 con la pantalla (G4). Hasta SD.2 retenía el bus en sus dos esperas
 * largas: mientras ACMD41 sacaba a la tarjeta de IDLE (hasta 1 s) y mientras la tarjeta grababa (hasta
 * 2 s); la pantalla se quedaba ese rato sin bus. La especificación de la SD deja subir el CS en las dos,
 * y ahora cada intento es un turno corto.
 *
 * Aquí, sin placa: una TARJETA DE MENTIRA detrás de la cintura SPI —entiende CMD0, CMD8, CMD55/ACMD41,
 * CMD58, CMD10, CMD9, CMD17 y CMD24; se queda en IDLE 250 ms tras CMD0 y ocupada 200 ms tras recibir
 * un bloque— y una PANTALLA en otro hilo del SO pidiendo turnos sin parar. Lo que se mide es lo que se ve
 * en la placa: cuántos turnos consigue la pantalla MIENTRAS la TF arranca y MIENTRAS la tarjeta graba, y
 * lo más que espera por uno. Y que la TF siga funcionando: arranca, se identifica, escribe y relee.
 *
 * EL CONTROL (`make test-sd-turnos-control`): el mismo test con la TF de antes (-DBPVM_SD_TURNO_UNICO)
 * TIENE que fallar. Si pasara, esta prueba no probaría nada.
 *
 *   make test-sd-turnos
 */
#include "bpvm_sd.h"
#include "bpvm_bus.h"
#include "bpvm_spi.h"
#include "bpvm_gpio.h"
#include "bpvm_machine.h"
#include "bpvm_platform.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

#define CS_PANT   10
#define CS_TF     23
#define HZ_PANT   40000000
#define IDLE_MS   250          /* la tarjeta tarda esto en salir de IDLE */
#define GRABA_MS  200          /* y esto en grabar un bloque */
/* Las fases, largas a propósito: con la máquina cargada (la red compila en paralelo) un hilo puede tardar
 * en despertar. El tope de la espera es UN TERCIO de la fase: la TF de antes deja a la pantalla fuera la
 * fase ENTERA, y la de ahora, microsegundos. */

/* ── los pines ── */
static volatile int g_nivel[40];
static void fg_init(int pin, int mode) { (void) pin; (void) mode; }
static void fg_pull(int pin, int m) { (void) pin; (void) m; }
/* SD.4 (24-sep) — la tarjeta REAL, al volver a elegirla mientras graba, TARDA en tirar otra vez de la línea:
 * el primer byte sale del pull-up (FF) aunque siga ocupada. La de mentira contestaba 00 desde el primer
 * byte —más amable que la de verdad—, y así la TF de SD.2 pasaba aquí y en la Metro volvía antes de tiempo
 * (el comando siguiente chocaba con la tarjeta grabando: CMD17 -> 00 00…). Ahora se porta como la real. */
static volatile int g_recien = 0;            /* 1 = el CS de la TF acaba de bajar */
static void fg_write(int pin, int v) {
    if (pin >= 0 && pin < 40) {
        if (pin == CS_TF && g_nivel[pin] && !v) g_recien = 1;
        g_nivel[pin] = v ? 1 : 0;
    }
}
static int  fg_read(int pin) { return (pin >= 0 && pin < 40) ? g_nivel[pin] : 0; }
static const bpvm_gpio_backend_t GPIO_FALSO = { fg_init, fg_pull, fg_write, fg_read };

/* ── LA TARJETA DE MENTIRA ── */
static const uint8_t CSD[16] = { 0x40, 0x0E, 0x00, 0x32, 0x5B, 0x59, 0x00, 0x00,   /* la de test_sd.c: */
                                 0xF3, 0xFF, 0x7F, 0x80, 0x0A, 0x40, 0x00, 0x01 };  /* 63.963.136 bloques */
static const uint8_t CID[16] = { 0x03, 'S', 'D', 'S', 'U', '3', '2', 'G',
                                 0x80, 0x12, 0x34, 0x56, 0x78, 0x01, 0x59, 0x01 };
enum { T_CMD, T_TOKEN, T_DATOS, T_OCUPADA };
static int      g_estado = T_CMD;
static uint8_t  g_cmd[6];
static int      g_ncmd = 0;
static uint8_t  g_sal[600];                  /* lo que la tarjeta tiene que decir */
static int      g_nsal = 0, g_psal = 0;
static int      g_lista = 0, g_app = 0, g_ndatos = 0;
static int64_t  g_idle_hasta = 0, g_ocupada_hasta = 0;
static uint8_t  g_disco[512];                /* un bloque: el que se escribe y se relee */
static uint8_t  g_recibido[512];

static void di(const uint8_t* b, int n) { memcpy(g_sal + g_nsal, b, (size_t) n); g_nsal += n; }
static void di1(uint8_t b) { g_sal[g_nsal++] = b; }
static void bloque(const uint8_t* b, int n) { di1(0xFF); di1(0xFE); di(b, n); di1(0xFF); di1(0xFF); }

static void orden(void) {
    uint8_t c = g_cmd[0] & 0x3F;
    int64_t ahora = bpvm_platform_now_ms();
    g_nsal = g_psal = 0;
    di1(0xFF);                                            /* Ncr: un byte antes del R1 */
    if (g_app && c == 41) {                               /* ACMD41 */
        g_app = 0;
        if (ahora >= g_idle_hasta) g_lista = 1;
        di1(g_lista ? 0x00 : 0x01);
        return;
    }
    g_app = 0;
    switch (c) {
    case 0:  g_lista = 0; g_idle_hasta = ahora + IDLE_MS; di1(0x01); break;
    case 8:  { static const uint8_t e[5] = { 0x01, 0x00, 0x00, 0x01, 0xAA }; di(e, 5); } break;
    case 55: g_app = 1; di1(g_lista ? 0x00 : 0x01); break;
    case 58: { static const uint8_t o[5] = { 0x00, 0xC0, 0xFF, 0x80, 0x00 }; di(o, 5); } break;   /* CCS */
    case 10: di1(0x00); bloque(CID, 16); break;
    case 9:  di1(0x00); bloque(CSD, 16); break;
    case 17: di1(0x00); bloque(g_disco, 512); break;
    case 24: di1(0x00); g_estado = T_TOKEN; break;
    default: di1(0x04); break;                            /* comando ilegal */
    }
}

static uint8_t tarjeta(uint8_t tx) {
    if (g_nivel[CS_TF]) return 0xFF;                      /* CS arriba: suelta la línea */
    if (g_recien) {                                       /* recién elegida: */
        g_recien = 0;
        if (g_estado == T_OCUPADA) return 0xFF;           /* aún no tira de la línea (el pull-up) */
    }
    switch (g_estado) {
    case T_TOKEN:
        if (g_psal < g_nsal) return g_sal[g_psal++];      /* el R1 del CMD24 */
        if (tx == 0xFE) { g_estado = T_DATOS; g_ndatos = 0; }
        return 0xFF;
    case T_DATOS:
        if (g_ndatos < 512) g_recibido[g_ndatos] = tx;
        if (++g_ndatos == 514) {                          /* 512 + el CRC */
            memcpy(g_disco, g_recibido, 512);
            g_estado = T_OCUPADA;
            g_ocupada_hasta = bpvm_platform_now_ms() + GRABA_MS;
            g_nsal = g_psal = 0;
            di1(0x05);                                    /* datos aceptados */
        }
        return 0xFF;
    case T_OCUPADA:
        if (g_psal < g_nsal) return g_sal[g_psal++];
        if (bpvm_platform_now_ms() < g_ocupada_hasta) return 0x00;   /* ocupada: MISO abajo */
        g_estado = T_CMD;
        return 0xFF;
    default:
        break;
    }
    if (g_ncmd > 0 || (tx & 0xC0) == 0x40) {             /* un comando entrando */
        g_cmd[g_ncmd++] = tx;
        if (g_ncmd == 6) { g_ncmd = 0; orden(); }
        return 0xFF;
    }
    return (g_psal < g_nsal) ? g_sal[g_psal++] : 0xFF;
}

/* ── la cintura SPI: la TF habla con la tarjeta; la pantalla, con nadie (sólo cuenta) ── */
static int  f_abrir(int bus, int sck, int mosi, int miso) { (void) bus; (void) sck; (void) mosi; (void) miso; return 0; }
static void f_ajustar(int bus, int hz, int modo) { (void) bus; (void) hz; (void) modo; }
static void f_cerrar(int bus) { (void) bus; }
static void f_init(int bus, int a, int b, int c, int hz, int m) { (void) bus; (void) a; (void) b; (void) c; (void) hz; (void) m; }
static int  f_t(int bus, const uint8_t* t, uint8_t* r, size_t n) {
    (void) bus;
    for (size_t i = 0; i < n; i++) {
        uint8_t rx = tarjeta(t ? t[i] : 0xFF);
        if (r) r[i] = rx;
    }
    return (int) n;
}
static int  f_w(int bus, const uint8_t* d, size_t n) { return f_t(bus, d, NULL, n); }
static int  f_r(int bus, uint8_t* d, size_t n) { return f_t(bus, NULL, d, n); }
static const bpvm_spi_backend_t SPI_TARJETA = { f_init, f_w, f_r, f_t, f_abrir, f_ajustar, f_cerrar };

/* ── LA PANTALLA: pide turnos sin parar y apunta cuántos consigue en cada fase y lo más que espera ── */
static volatile int g_fase = 0;               /* 0 fuera, 1 la TF arranca, 2 la tarjeta graba */
static volatile int g_sigue = 1;
static int     g_turnos[3];
static int64_t g_peor[3];
static int     g_fallos_pant = 0;
static char    g_err[160];

static void* pantalla(void* arg) {
    (void) arg;
    while (g_sigue) {
        int fase = g_fase;
        int64_t t0 = bpvm_platform_now_ms();
        int r = bpvm_spi_tomar(2, CS_PANT, HZ_PANT, 0, BPVM_BUS_PACK, g_err, sizeof g_err);
        int64_t espera = bpvm_platform_now_ms() - t0;
        if (r != 0) { g_fallos_pant++; continue; }
        uint8_t b[3] = { 'P', 'P', 'P' };
        bpvm_spi_xfer(2, b, NULL, 3);
        bpvm_spi_dejar(2);
        if (fase == g_fase) {                  /* la fase no cambió mientras esperaba: cuenta en ella */
            g_turnos[fase]++;
            if (espera > g_peor[fase]) g_peor[fase] = espera;
        }
        /* Un fotograma cada poco, no un glotón. En Windows un sleep de 2 ms dura un tic del temporizador
         * (~15,6 ms): en los 150 ms del arranque caben unos 9 turnos, no 50. Por eso lo que decide es LA
         * ESPERA —con la TF de antes, la fase entera— y de turnos basta con que haya unos cuantos. */
        bpvm_platform_thread_sleep_ms(2);
    }
    return NULL;
}

int main(void) {
    static uint16_t pines[40];
    static const uint8_t spis[1] = { 2 };
    static bpvm_caps_t caps;
    char e[160], m[256];
    int i;

    for (i = 0; i < 40; i++) { pines[i] = BPVM_PIN_DIGITAL; g_nivel[i] = 1; }
    memset(&caps, 0, sizeof caps);
    caps.micro = "micro de prueba"; caps.spi = spis; caps.n_spi = 1; caps.pin = pines; caps.n_pin = 40;
    bpvm_caps_set(&caps);
    bpvm_spi_set_backend(&SPI_TARJETA);
    bpvm_gpio_set_backend(&GPIO_FALSO);
    bpvm_bus_arbitro_init();

    printf("== V7/H3 SD.2: la TF suelta el bus en sus esperas (la pantalla, en otro hilo) ==\n");
    CHECK(bpvm_spi_abrir(2, 6, 7, 8, HZ_PANT, 0, BPVM_BUS_PACK, NULL, e, sizeof e) == 0,
          "la pantalla (un pack) establece el SPI 2, con MISO");

    pthread_t hp;
    pthread_create(&hp, NULL, pantalla, NULL);
    bpvm_platform_thread_sleep_ms(20);

    /* ── 1. EL ARRANQUE ── */
    bpvm_sd_pines_t p;
    memset(&p, 0, sizeof p);
    p.bus = 2; p.sck = -1; p.mosi = -1; p.miso = -1; p.cs = CS_TF; p.cd = -1;
    bpvm_sd_info_t info;
    g_fase = 1;
    int64_t t0 = bpvm_platform_now_ms();
    bpvm_sd_res_t r = bpvm_sd_init(&p, 20000000, &info);
    int64_t dura = bpvm_platform_now_ms() - t0;
    g_fase = 0;
    snprintf(m, sizeof m, "la TF arranca (%s, %ld ms: la tarjeta tarda %d en salir de IDLE)",
             bpvm_sd_res_str(r), (long) dura, IDLE_MS);
    CHECK(r == BPVM_SD_OK && dura >= IDLE_MS, m);
    CHECK(info.bloques == 63963136u && info.version == 2 && info.alta_cap == 1
          && strcmp(info.producto, "SU32G") == 0 && info.serie == 0x12345678u,
          "…y se identifica: SU32G, 63.963.136 bloques, v2, direcciona por bloque, su serie");
    snprintf(m, sizeof m, "MIENTRAS ARRANCA, la pantalla tiene bus: %d turnos, y lo más que espera por uno, %ld ms "
             "(la TF de antes la dejaba %d ms fuera)", g_turnos[1], (long) g_peor[1], IDLE_MS);
    CHECK(g_turnos[1] >= 3 && g_peor[1] < IDLE_MS / 3, m);

    /* ── 2. LA ESCRITURA: mientras la tarjeta graba ── */
    uint8_t blk[512], vuelta[512];
    for (i = 0; i < 512; i++) blk[i] = (uint8_t) (i * 13 + 7);
    g_fase = 2;
    t0 = bpvm_platform_now_ms();
    r = bpvm_sd_escribir_bloque(&p, &info, 1234, blk);
    dura = bpvm_platform_now_ms() - t0;
    g_fase = 0;
    snprintf(m, sizeof m, "la TF escribe un bloque (%s, %ld ms: la tarjeta tarda %d en grabarlo)",
             bpvm_sd_res_str(r), (long) dura, GRABA_MS);
    CHECK(r == BPVM_SD_OK && dura >= GRABA_MS, m);
    snprintf(m, sizeof m, "MIENTRAS GRABA, la pantalla tiene bus: %d turnos, y lo más que espera por uno, %ld ms "
             "(la TF de antes la dejaba %d ms fuera)", g_turnos[2], (long) g_peor[2], GRABA_MS);
    CHECK(g_turnos[2] >= 3 && g_peor[2] < GRABA_MS / 3, m);
    r = bpvm_sd_leer_bloque(&p, &info, 1234, vuelta);
    CHECK(r == BPVM_SD_OK && memcmp(blk, vuelta, 512) == 0, "…y lo relee igual, byte a byte");

    g_sigue = 0;
    pthread_join(hp, NULL);
    snprintf(m, sizeof m, "la pantalla no perdió ningún turno (%d fallos%s%s)", g_fallos_pant,
             g_err[0] ? " — " : "", g_err);
    CHECK(g_fallos_pant == 0, m);
    CHECK(g_nivel[CS_PANT] == 1 && g_nivel[CS_TF] == 1 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2),
          "al acabar, los dos CS arriba y el bus libre");

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
