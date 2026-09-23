/*
 * test_arbitro.c — V7/H6.c G4.a2: EL ÁRBITRO DE LOS BUSES, con DOS HILOS de verdad.
 *
 * Dos hilos del SO machacan dos dispositivos del mismo SPI 2 —la pantalla (CS 10,
 * 40 MHz) y la TF (CS 23, 20 MHz), la topología de la C5— contra una cintura de
 * mentira que GRABA cada trozo que pasa por el cable con el nivel de los dos CS y la
 * velocidad del bus EN ESE INSTANTE. Cada turno manda tres trozos, y cada
 * transferencia tarda (duerme) para que los hilos se crucen de verdad. La cinta
 * tiene que salir limpia:
 *
 *   · cada trozo con SU CS abajo y el del otro arriba;
 *   · a SU velocidad (la del turno, no la del último que ajustó);
 *   · los tres trozos de un turno SEGUIDOS: nadie se mete en medio;
 *   · y todos los trozos, ni uno perdido.
 *
 * Y LA INANICIÓN, aparte y a tiempo: un «glotón» (la pantalla volcando) encadena
 * turnos sin respirar y otro (la TF) pide UNO. Con la cortesía del árbitro la TF
 * entra en cuanto la pantalla suelta; sin ella, quien suelta y vuelve a pedir gana
 * siempre —ya está corriendo; el otro tiene que despertar— y la TF espera a que al
 * glotón le apetezca parar. Salió de verdad aquí el 23-sep: la caza de los dos hilos
 * no la protegía (pasaba igual sin la cortesía), así que va con su propia medida.
 *
 * EL CONTROL (`make test-arbitro-control`): el mismo test con el árbitro apagado
 * (-DBPVM_SIN_ARBITRO en bpvm_bus.c) TIENE que fallar. Si pasara, esta prueba no
 * probaría nada: los hilos no se estarían cruzando.
 *
 *   make test-arbitro
 */
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

#define TURNOS   25          /* por hilo */
#define TROZOS    3          /* por turno */
#define HZ_PANT  40000000
#define HZ_TF    20000000

/* ── la cintura de mentira: GRABA ── */
typedef struct { unsigned char dev, turno, trozo; int hz, cs10, cs23; } trozo_t;
static trozo_t g_cinta[2 * TURNOS * TROZOS + 64];
static int     g_n = 0;
static pthread_mutex_t g_cinta_m = PTHREAD_MUTEX_INITIALIZER;   /* protege la CINTA, no el bus */
static volatile int g_hz = 0;
static volatile int g_nivel[40];

static int  f_abrir(int bus, int sck, int mosi, int miso) { (void) bus; (void) sck; (void) mosi; (void) miso; return 0; }
static void f_ajustar(int bus, int hz, int modo) { (void) bus; (void) modo; g_hz = hz; }
static void f_cerrar(int bus) { (void) bus; }
static void f_init(int bus, int a, int b, int c, int hz, int m) { (void) bus; (void) a; (void) b; (void) c; (void) hz; (void) m; }
static int  f_w(int bus, const uint8_t* d, size_t n) {
    (void) bus;
    pthread_mutex_lock(&g_cinta_m);
    if (g_n < (int) (sizeof g_cinta / sizeof g_cinta[0])) {
        trozo_t* t = &g_cinta[g_n++];
        t->dev = d[0]; t->turno = d[1]; t->trozo = d[2];
        t->hz = g_hz; t->cs10 = g_nivel[10]; t->cs23 = g_nivel[23];
    }
    pthread_mutex_unlock(&g_cinta_m);
    bpvm_platform_thread_sleep_ms(1);          /* el cable tarda: que los hilos se crucen */
    return (int) n;
}
static int  f_r(int bus, uint8_t* d, size_t n) { (void) bus; (void) d; return (int) n; }
static int  f_t(int bus, const uint8_t* t, uint8_t* r, size_t n) { (void) r; return f_w(bus, t, n); }
static const bpvm_spi_backend_t SPI_CINTA = { f_init, f_w, f_r, f_t, f_abrir, f_ajustar, f_cerrar };

static void fg_init(int pin, int mode) { (void) pin; (void) mode; }
static void fg_pull(int pin, int m) { (void) pin; (void) m; }
static void fg_write(int pin, int v) { if (pin >= 0 && pin < 40) g_nivel[pin] = v ? 1 : 0; }
static int  fg_read(int pin) { return (pin >= 0 && pin < 40) ? g_nivel[pin] : 0; }
static const bpvm_gpio_backend_t GPIO_FALSO = { fg_init, fg_pull, fg_write, fg_read };

/* ── los dos usuarios ── */
typedef struct { unsigned char dev; int cs, hz, quien, fallos; char err[160]; } usuario_t;

static void* machaca(void* arg) {
    usuario_t* u = (usuario_t*) arg;
    int t, k;
    for (t = 0; t < TURNOS; t++) {
        if (bpvm_spi_tomar(2, u->cs, u->hz, 0, u->quien, u->err, sizeof u->err) != 0) { u->fallos++; continue; }
        for (k = 0; k < TROZOS; k++) {
            uint8_t b[3];
            b[0] = u->dev; b[1] = (uint8_t) t; b[2] = (uint8_t) k;
            if (bpvm_spi_xfer(2, b, NULL, 3) != 3) u->fallos++;
        }
        bpvm_spi_dejar(2);
    }
    return NULL;
}

/* ── la inanición ── */
static volatile int g_gloton_sigue = 1, g_gloton_turnos = 0;
static void* gloton(void* arg) {
    char err[160];
    (void) arg;
    while (g_gloton_sigue) {
        if (bpvm_spi_tomar(2, 10, HZ_PANT, 0, BPVM_BUS_PACK, err, sizeof err) != 0) continue;
        bpvm_platform_thread_sleep_ms(20);     /* un turno de volcado; y al soltar, a por otro */
        bpvm_spi_dejar(2);
        g_gloton_turnos++;
    }
    return NULL;
}

int main(void) {
    static uint16_t pines[40];
    static const uint8_t spis[1] = { 2 };
    static bpvm_caps_t caps;
    usuario_t pant, tf;
    pthread_t h1, h2;
    char e[160], m[256];
    int i, malos_cs = 0, malos_hz = 0, cortados = 0, n_p = 0, n_t = 0, primero = -1;

    for (i = 0; i < 40; i++) { pines[i] = BPVM_PIN_DIGITAL; g_nivel[i] = 1; }
    memset(&caps, 0, sizeof caps);
    caps.micro = "micro de prueba"; caps.spi = spis; caps.n_spi = 1; caps.pin = pines; caps.n_pin = 40;
    bpvm_caps_set(&caps);
    bpvm_spi_set_backend(&SPI_CINTA);
    bpvm_gpio_set_backend(&GPIO_FALSO);
    bpvm_bus_arbitro_init();

    printf("== V7/H6.c G4.a2: el árbitro, con dos hilos sobre el mismo SPI 2 ==\n");
    CHECK(bpvm_spi_abrir(2, 6, 7, 8, HZ_PANT, 0, BPVM_BUS_PACK, NULL, e, sizeof e) == 0 &&
          bpvm_spi_abrir(2, -1, -1, -1, HZ_TF, 0, BPVM_BUS_FIRMWARE, NULL, e, sizeof e) == 0,
          "la pantalla (un pack) y la TF (el firmware) comparten el SPI 2");

    memset(&pant, 0, sizeof pant); pant.dev = 'P'; pant.cs = 10; pant.hz = HZ_PANT; pant.quien = BPVM_BUS_PACK;
    memset(&tf, 0, sizeof tf);     tf.dev = 'T';   tf.cs = 23;   tf.hz = HZ_TF;     tf.quien = BPVM_BUS_FIRMWARE;
    pthread_create(&h1, NULL, machaca, &pant);
    pthread_create(&h2, NULL, machaca, &tf);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    for (i = 0; i < g_n; i++) {
        const trozo_t* t = &g_cinta[i];
        int suyo_abajo = (t->dev == 'P') ? (t->cs10 == 0 && t->cs23 == 1) : (t->cs23 == 0 && t->cs10 == 1);
        int suya_hz    = (t->dev == 'P') ? (t->hz == HZ_PANT) : (t->hz == HZ_TF);
        if (t->dev == 'P') n_p++; else n_t++;
        if (!suyo_abajo) malos_cs++;
        if (!suya_hz) malos_hz++;
        /* los trozos de un turno, seguidos: el siguiente del mismo turno va justo detrás */
        if (t->trozo < TROZOS - 1 &&
            (i + 1 >= g_n || g_cinta[i + 1].dev != t->dev || g_cinta[i + 1].turno != t->turno || g_cinta[i + 1].trozo != t->trozo + 1))
            cortados++;
        if (primero < 0 && (!suyo_abajo || !suya_hz)) primero = i;
    }
    snprintf(m, sizeof m, "ningún turno falló (pantalla %d, TF %d fallos%s%s)", pant.fallos, tf.fallos,
             pant.err[0] ? " — " : "", pant.err[0] ? pant.err : tf.err);
    CHECK(pant.fallos == 0 && tf.fallos == 0, m);
    snprintf(m, sizeof m, "todos los trozos en la cinta: %d de la pantalla y %d de la TF (de %d cada uno)", n_p, n_t, TURNOS * TROZOS);
    CHECK(n_p == TURNOS * TROZOS && n_t == TURNOS * TROZOS, m);
    snprintf(m, sizeof m, "cada trozo con SU CS abajo y el del otro arriba (%d mal)", malos_cs);
    CHECK(malos_cs == 0, m);
    snprintf(m, sizeof m, "cada trozo a SU velocidad: 40 MHz la pantalla, 20 MHz la TF (%d mal)", malos_hz);
    CHECK(malos_hz == 0, m);
    snprintf(m, sizeof m, "los tres trozos de cada turno, SEGUIDOS: nadie se mete en medio (%d cortes)", cortados);
    CHECK(cortados == 0, m);
    if (primero >= 0)
        printf("        el primer trozo malo, el %d: %c turno %d trozo %d a %d Hz, CS10=%d CS23=%d\n", primero,
               g_cinta[primero].dev, g_cinta[primero].turno, g_cinta[primero].trozo, g_cinta[primero].hz,
               g_cinta[primero].cs10, g_cinta[primero].cs23);
    CHECK(g_nivel[10] == 1 && g_nivel[23] == 1 && !bpvm_bus_tomado(BPVM_BUS_SPI, 2),
          "al acabar, los dos CS arriba y el bus libre");

    /* ── 2. LA INANICIÓN ──
     * Cinco intentos, y cuenta el PEOR: sin la cortesía, que la TF se quede fuera es
     * probable pero no seguro (medido: la cazaba un intento de cada dos), y cinco dejan
     * poco hueco para escaparse. Con ella, cada intento está acotado POR CONSTRUCCIÓN (el
     * turno en curso y, como mucho, el anterior), así que no hay falsos fallos. */
    {
        pthread_t hg;
        char err[160];
        int64_t t0, espera, peor_ms = 0;
        int r, antes, durante, peor_turnos = 0, fallos = 0, k;
        err[0] = 0;
        pthread_create(&hg, NULL, gloton, NULL);
        for (k = 0; k < 5; k++) {
            antes = g_gloton_turnos;
            while (g_gloton_turnos < antes + 2) bpvm_platform_thread_sleep_ms(5);   /* que esté encadenando */
            antes = g_gloton_turnos;
            t0 = bpvm_platform_now_ms();
            r = bpvm_spi_tomar(2, 23, HZ_TF, 0, BPVM_BUS_FIRMWARE, err, sizeof err);
            espera = bpvm_platform_now_ms() - t0;
            durante = g_gloton_turnos - antes;      /* contados al ENTRAR, no al acabar */
            if (r == 0) bpvm_spi_dejar(2); else fallos++;
            if (espera > peor_ms) peor_ms = espera;
            if (durante > peor_turnos) peor_turnos = durante;
        }
        g_gloton_sigue = 0;
        pthread_join(hg, NULL);
        snprintf(m, sizeof m, "INANICIÓN: la TF pide un turno entre los de un glotón, cinco veces, y entra en cuanto suelta "
                 "(el peor: %ld ms y %d turnos del glotón; topes: 200 ms y 2 turnos, el suyo y el anterior)%s%s",
                 (long) peor_ms, peor_turnos, err[0] ? " — " : "", err);
        CHECK(fallos == 0 && peor_ms < 200 && peor_turnos <= 2, m);
    }

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
