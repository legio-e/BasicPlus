/*
 * test_sdsc.c — el camino SDSC (baja capacidad) SIN una tarjeta SDSC.
 *
 * POR QUÉ EXISTE. La ficha H2-P5 decía que faltaba probar SDSC y que hacía
 * falta una tarjeta de ≤2 GB. Eduardo, 17-ago: *«no tengo tarjetas de 2G ni
 * voy a tener, están obsoletas»*. Y tiene razón — pero lo que da miedo de
 * SDSC no es la tarjeta, es UNA CUENTA:
 *
 *     uint32_t arg = info->alta_cap ? lba : (lba * 512u);
 *
 * En SDHC/SDXC el argumento de CMD17/CMD24 es el BLOQUE; en SDSC es el BYTE.
 * Equivocarse ahí **no da error**: lee o escribe otro sitio, ×512 o ÷512.
 * Y esa cuenta es aritmética pura sobre un dato que viene del OCR, así que se
 * puede forzar en el PC. Lo único que tocaba el hardware eran dos funciones de
 * plataforma —`bpvm_spi_transfer` y `bpvm_gpio_write`—, y aquí las ponemos
 * nosotros, igual que `test_blk.c` hace con el reloj.
 *
 * LO QUE ESTO PRUEBA Y LO QUE NO. Prueba la parte que corrompe en silencio: el
 * argumento que sale por el bus. NO prueba las rarezas eléctricas y de arranque
 * de una tarjeta SDSC real (no responde a CMD8, negociación distinta) — eso
 * sigue sin poderse medir sin una, y así queda dicho en la ficha. Pero el
 * decodificador del CSD v1 ya estaba cubierto en `test_sd.c`, así que con esto
 * el camino SDSC queda probado en todo lo que es nuestro.
 *
 * EL CONTROL. Cada caso va con su gemelo de alta capacidad. Un test que sólo
 * mirase SDSC podría estar midiendo cualquier cosa: lo que da sentido al 1024
 * es que el MISMO lba dé 2 con `alta_cap = 1`.
 *
 *   make test-sdsc
 */
#include "bpvm_sd.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Tapón del reloj: lo pide el enlazador y enlazar la plataforma de verdad
 * arrastraría media VM. Avanza solo para que ningún plazo se dispare. */
static int64_t g_ms = 0;
int64_t bpvm_platform_now_ms(void) { return g_ms++; }

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* ─────────────────────────────────────────────────────────────────────────
 * El doble del bus SPI.
 *
 * No imita una tarjeta entera: imita lo justo para que el driver llegue hasta
 * el final del comando y nosotros veamos el argumento. Un doble más amable que
 * el original sería una trampa —contestar que sí a todo esconde fallos— así
 * que éste sigue la MISMA secuencia que el protocolo real: hueco, comando,
 * cuatro bytes de argumento, CRC, y sólo entonces el R1.
 */
enum { LIBRE, ARG, R1, TOKEN, DATOS, ACEPTA, OCUPADA };

static struct {
    int      estado;
    int      n;              /* bytes vistos dentro del estado actual        */
    uint8_t  cmd;            /* el último comando reconocido                 */
    uint32_t arg;            /* SU ARGUMENTO — esto es lo que venimos a ver  */
    int      capturado;      /* cuántos comandos completos hemos visto       */
} bus;

static void bus_reset(void) { memset(&bus, 0, sizeof bus); bus.estado = LIBRE; }

/* Fin de un comando: vuelve a la escucha SIN borrar lo capturado. Tenerlo
 * separado de `bus_reset` no es cosmético — la primera versión llamaba a
 * `bus_reset` aquí y se comía el `arg` justo despues de leerlo, así que TODO
 * salía a 0 y en rojo. Lo cazó la comprobación del instrumento, que por eso
 * va antes que las medidas. */
static void fin_de_comando(void) { bus.estado = LIBRE; bus.n = 0; }

/* El resto de la plataforma que el enlazador pide. No se usan en este test
 * —no arrancamos ninguna tarjeta, entramos directos a leer/escribir— pero
 * tienen que existir. `gpio_read` a 1 = «no hay tarjeta insertada» según el
 * convenio de card-detect activo bajo; da igual, nadie lo consulta aquí. */
void bpvm_gpio_write(int pin, int valor)  { (void) pin; (void) valor; }
void bpvm_gpio_init (int pin, int modo)   { (void) pin; (void) modo; }
void bpvm_gpio_pull (int pin, int modo)   { (void) pin; (void) modo; }
int  bpvm_gpio_read (int pin)             { (void) pin; return 1; }
void bpvm_spi_init  (int bus, uint32_t hz){ (void) bus; (void) hz; }

void bpvm_spi_transfer(int b, const uint8_t* tx, uint8_t* rx, int n)
{
    (void) b;
    for (int i = 0; i < n; i++) {
        uint8_t t = tx[i];
        uint8_t r = 0xFF;                    /* línea en reposo               */

        switch (bus.estado) {
        case LIBRE:
            /* Un comando en SPI empieza por 01xxxxxx. El 0xFF del hueco no lo
             * es (11111111), así que esto no confunde relleno con comando. */
            if ((t & 0xC0) == 0x40) {
                bus.cmd = (uint8_t) (t & 0x3F);
                bus.arg = 0; bus.n = 0; bus.estado = ARG;
            }
            break;

        case ARG:
            bus.arg = (bus.arg << 8) | t;
            if (++bus.n == 4) { bus.n = 0; bus.estado = R1; }
            break;

        case R1:
            /* El primer byte tras el argumento es el CRC; el siguiente ya es
             * la ventana del R1. Contestamos "aceptado" (0x00) en cuanto toca. */
            if (bus.n++ == 0) break;
            r = 0x00;
            bus.capturado++;
            bus.n = 0;
            bus.estado = (bus.cmd == 17) ? TOKEN : (bus.cmd == 24 ? ACEPTA : LIBRE);
            break;

        case TOKEN:
            r = 0xFE;                        /* token de dato                 */
            bus.n = 0; bus.estado = DATOS;
            break;

        case DATOS:
            r = 0x5A;                        /* relleno + los 2 de CRC16      */
            if (++bus.n == 512 + 2) fin_de_comando();
            break;

        case ACEPTA:
            /* Escritura: el driver manda hueco + token + 512 + CRC y LUEGO lee
             * la respuesta de datos. Dejamos pasar todo eso y contestamos 0x05
             * (aceptado) en el byte que toca. */
            if (bus.n++ < 1 + 1 + 512 + 2) break;
            r = 0x05;
            bus.estado = OCUPADA; bus.n = 0;
            break;

        case OCUPADA:
            /* Ocupada grabando = MISO a 0. Un solo byte y suelta: si nos
             * quedáramos a 0 el driver giraría hasta agotar su plazo. */
            r = (bus.n++ == 0) ? 0x00 : 0xFF;
            if (bus.n > 1) fin_de_comando();
            break;
        }
        if (rx) rx[i] = r;
    }
}

/* ───────────────────────────────────────────────────────────────────────── */

static uint32_t arg_de_lectura(uint8_t alta_cap, uint32_t lba)
{
    bpvm_sd_pines_t p = { 0, 34, 35, 36, 39, 40, 1 };
    bpvm_sd_info_t info; memset(&info, 0, sizeof info);
    info.alta_cap = alta_cap;
    uint8_t dst[512];

    bus_reset();
    bpvm_sd_res_t r = bpvm_sd_leer_bloque(&p, &info, lba, dst);
    if (r != BPVM_SD_OK) { printf("  (lectura devolvio %s)\n", bpvm_sd_res_str(r)); }
    return bus.arg;
}

static uint32_t arg_de_escritura(uint8_t alta_cap, uint32_t lba)
{
    bpvm_sd_pines_t p = { 0, 34, 35, 36, 39, 40, 1 };
    bpvm_sd_info_t info; memset(&info, 0, sizeof info);
    info.alta_cap = alta_cap;
    uint8_t src[512]; memset(src, 0x11, sizeof src);

    bus_reset();
    bpvm_sd_res_t r = bpvm_sd_escribir_bloque(&p, &info, lba, src);
    if (r != BPVM_SD_OK) { printf("  (escritura devolvio %s)\n", bpvm_sd_res_str(r)); }
    return bus.arg;
}

int main(void)
{
    printf("== H2-P5: el camino SDSC, forzado sin tarjeta ==\n\n");

    /* Antes de creerse nada: que el doble funcione. Si el driver no llega a
     * mandar el comando, `arg` se quedaría a 0 y TODO saldría verde por el
     * motivo equivocado. */
    printf("-- el instrumento\n");
    bus_reset();
    (void) arg_de_lectura(1, 7);
    CHECK(bus.capturado == 1,  "el doble ve UN comando completo en una lectura");
    CHECK(bus.cmd == 17,       "y lo reconoce como CMD17 (READ_SINGLE)");

    printf("\n-- lectura: el argumento de CMD17\n");
    CHECK(arg_de_lectura(1, 2) == 2u,
          "alta capacidad (SDHC/SDXC): lba 2 -> arg 2, el BLOQUE");
    CHECK(arg_de_lectura(0, 2) == 1024u,
          "baja capacidad (SDSC):      lba 2 -> arg 1024, el BYTE (2 x 512)");

    /* Un lba grande: es donde un desbordamiento se manifestaría. 8.388.608 x
     * 512 = 4.294.967.296, que NO cabe en 32 bits y da la vuelta a 0 — o sea
     * que una SDSC no puede direccionar tan lejos, pero la cuenta tiene que
     * ser la que es, no una que "arregle" el caso por su cuenta. */
    CHECK(arg_de_lectura(0, 4096) == 2097152u,
          "SDSC: lba 4096 -> arg 2.097.152 (sigue siendo x512)");

    printf("\n-- escritura: el argumento de CMD24\n");
    CHECK(arg_de_escritura(1, 3) == 3u,
          "alta capacidad: lba 3 -> arg 3");
    CHECK(arg_de_escritura(0, 3) == 1536u,
          "baja capacidad: lba 3 -> arg 1536 (3 x 512)");

    /* El bloque 0 es el mismo en las dos, y por eso NO sirve de prueba: es el
     * unico lba donde un driver roto acierta por casualidad. Se comprueba para
     * dejar dicho que la coincidencia es esperada, no un fallo del test. */
    printf("\n-- el bloque 0, donde las dos coinciden\n");
    CHECK(arg_de_lectura(1, 0) == 0u && arg_de_lectura(0, 0) == 0u,
          "lba 0 -> arg 0 en las dos (por eso arrancar no distingue el fallo)");

    printf("\n%s\n", g_fail == 0 ? "TODO OK" : "HAY FALLOS");
    return g_fail == 0 ? 0 : 1;
}
