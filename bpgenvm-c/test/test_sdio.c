/*
 * test_sdio.c — V5/H6: la línea `sd` del ENV en su variante SDIO.
 *
 * QUÉ SE PRUEBA Y POR QUÉ. La cintura del P4 no se puede compilar aquí (hace
 * falta `idf.py`), así que la parte que SÍ es portable se ejerce a fondo: la
 * decodificación del texto. Es donde viven los fallos mudos — una clave que se
 * come a otra por ser su prefijo deja un pin a un valor plausible y equivocado,
 * y eso no se ve hasta que la tarjeta no contesta en la placa.
 *
 *   make test-sdio
 */
#include "bpvm_blk_sdmmc.h"

#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* La línea real de la Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, con los pines
 * del esquema y el GPIO45 de la alimentación. */
#define LINEA_P4 "clk:43,cmd:44,d0:39,d1:40,d2:41,d3:42,pwr:45"

int main(void)
{
    printf("== V5/H6: la entrada 'sd' en variante SDIO ==\n\n");

    /* ── La placa de verdad ──────────────────────────────────────────────── */
    {
        bpvm_sdio_pines_t p;
        char motivo[120];
        int r = bpvm_sdio_pines_parse(LINEA_P4, &p, motivo, sizeof motivo);
        CHECK(r == 0,                        "P4: la linea del esquema se acepta");
        CHECK(p.clk == 43 && p.cmd == 44,    "P4: clk 43, cmd 44");
        CHECK(p.d0 == 39 && p.d1 == 40 && p.d2 == 41 && p.d3 == 42,
                                             "P4: d0-d3 = 39,40,41,42");
        CHECK(p.ancho == 4,                  "P4: con las cuatro lineas, 4 bits");
        CHECK(p.pwr == 45,                   "P4: la alimentacion es el GPIO45");
        CHECK(p.pwr_activo_alto == 0,        "P4: activo BAJO por defecto (medido en placa)");
        CHECK(p.slot == 0 && p.khz == 0,     "P4: slot y reloj, a su valor por defecto");
    }

    /* ── El orden da igual, que es para lo que están las etiquetas ───────── */
    {
        bpvm_sdio_pines_t a, b;
        bpvm_sdio_pines_parse(LINEA_P4, &a, 0, 0);
        bpvm_sdio_pines_parse("pwr:45,d3:42,cmd:44,d1:40,clk:43,d2:41,d0:39", &b, 0, 0);
        CHECK(memcmp(&a, &b, sizeof a) == 0, "el ORDEN de las etiquetas no importa");
    }

    /* ── 1 bit: legítimo, y sin lineas a medias ──────────────────────────── */
    {
        bpvm_sdio_pines_t p;
        char motivo[120];
        CHECK(bpvm_sdio_pines_parse("clk:43,cmd:44,d0:39", &p, motivo, sizeof motivo) == 0
              && p.ancho == 1,
                                             "sin d1/d2/d3 -> 1 bit, y se acepta");

        /* Tres de las cuatro es el caso peligroso: a 4 bits con una linea sin
         * cablear NO falla al montar, falla al leer un bloque. Se rechaza. */
        int r = bpvm_sdio_pines_parse("clk:43,cmd:44,d0:39,d1:40,d2:41", &p,
                                      motivo, sizeof motivo);
        CHECK(r == -1,                       "lineas de datos A MEDIAS -> rechazo");
        CHECK(strstr(motivo, "d1, d2 y d3") != NULL,
                                             "y el motivo dice como arreglarlo");
    }

    /* ── Los obligatorios, y que el motivo diga CUAL falta ───────────────── */
    {
        bpvm_sdio_pines_t p;
        char motivo[120];
        bpvm_sdio_pines_parse("cmd:44,d0:39", &p, motivo, sizeof motivo);
        CHECK(strstr(motivo, "clk") != NULL, "falta clk -> el motivo lo NOMBRA");
        bpvm_sdio_pines_parse("clk:43,d0:39", &p, motivo, sizeof motivo);
        CHECK(strstr(motivo, "cmd") != NULL, "falta cmd -> el motivo lo NOMBRA");
        bpvm_sdio_pines_parse("clk:43,cmd:44", &p, motivo, sizeof motivo);
        CHECK(strstr(motivo, "d0") != NULL,  "falta d0 -> el motivo lo NOMBRA");
    }

    /* ── ⚠️ El prefijo que se come a la clave larga ──────────────────────
     * `pwr` es prefijo de `pwralto`. Si la tabla las mirara al reves, `pwr`
     * casaria primero, `alto:1` se leeria como valor no numerico... o peor,
     * en otra combinacion, dejaria un pin plausible y equivocado. */
    {
        bpvm_sdio_pines_t p;
        char motivo[120];
        int r = bpvm_sdio_pines_parse("clk:43,cmd:44,d0:39,pwr:45,pwralto:1",
                                      &p, motivo, sizeof motivo);
        CHECK(r == 0 && p.pwr == 45 && p.pwr_activo_alto == 1,
                                             "'pwralto' NO lo captura 'pwr'");
    }

    /* ── Tolerancia: lo desconocido se ignora, y eso permite UNA linea para
     *    las dos placas (el firmware de cada una coge sus etiquetas) ─────── */
    {
        bpvm_sdio_pines_t p;
        int r = bpvm_sdio_pines_parse(
            "sck:34,mosi:35,miso:36,cs:39,cd:40,clk:43,cmd:44,d0:39,d1:40,d2:41,d3:42,pwr:45",
            &p, 0, 0);
        CHECK(r == 0 && p.clk == 43 && p.ancho == 4 && p.pwr == 45,
                                             "una linea MIXTA (SPI+SDIO) se lee bien");
    }

    /* ── Lo que no debe reventar ─────────────────────────────────────────── */
    {
        bpvm_sdio_pines_t p;
        char motivo[120];
        CHECK(bpvm_sdio_pines_parse(NULL, &p, motivo, sizeof motivo) == -1,
                                             "NULL -> rechazo, sin reventar");
        CHECK(bpvm_sdio_pines_parse("", &p, motivo, sizeof motivo) == -1,
                                             "cadena vacia -> rechazo");
        CHECK(bpvm_sdio_pines_parse("clk:43,cmd:44,d0:", &p, motivo, sizeof motivo) == -1,
                                             "valor sin digitos -> rechazo");
        CHECK(bpvm_sdio_pines_parse(LINEA_P4, NULL, motivo, sizeof motivo) == -1,
                                             "destino NULL -> rechazo");
        /* Y sin sitio donde escribir el motivo, tampoco. */
        CHECK(bpvm_sdio_pines_parse("clk:43", &p, 0, 0) == -1,
                                             "sin buffer de motivo, sigue contestando");
    }

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
