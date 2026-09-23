/* driver_nulo.c — el DRIVER DE PANTALLA que no pinta (V7/H6.c G2; hasta entonces
 * `panel_nulo.c`, compilado DENTRO del pack de LVGL). Publica 'PANT' con un
 * `flush` que no manda nada a ninguna parte.
 *
 * ─── PARA QUÉ ───
 *
 * Idea de Eduardo (22-sep): *«¿se podría probar en la P4 sin desactivar el LVGL
 * actual?»*. El P4 no tiene la pantalla de la C5; con este driver el pack de
 * LVGL se carga y se ejercita igual —su TLSF, su `.rodata`, `gui.c` dentro, la
 * tabla de ~87 símbolos resuelta por nombre, el MODELO, que es sobre lo que se
 * mide la paridad con miVM— y CONVIVE con el LVGL de la imagen, que sigue
 * pintando en su MIPI-DSI. Lo que no prueba es el hardware: eso es la C5.
 *
 * ─── Y ES LA PLANTILLA ───
 *
 * Es el driver de pantalla más pequeño que cumple el contrato (`bpvm_pant.h`):
 * las tres funciones obligatorias —init, size, flush— y la entrada que las
 * publica. Quien escriba el driver de SU pantalla puede empezar copiando éste:
 * todo lo demás (rotate, touch, formato) es opcional y se añade publicándolo.
 *
 * Se elige GRABÁNDOLO, no en runtime: un panel mudo porque su chip no contesta
 * y un driver que no pinta A PROPÓSITO son cosas distintas, y confundirlas es
 * exactamente el fallo silencioso que la casa no admite — por eso su `init`
 * lo dice en el log. */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_pant.h"
#include <stdint.h>

const bpvm_bios_t* g_bios = 0;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

#ifndef BPVM_NULO_W
#define BPVM_NULO_W 240      /* el tamaño de la C5: así el modelo (y el dump) sale igual */
#endif
#ifndef BPVM_NULO_H
#define BPVM_NULO_H 320
#endif

/* Cuenta los volcados. Contar NO es adorno: es la diferencia entre «LVGL dibujó
 * y no había dónde ponerlo» y «LVGL no llegó a dibujar». */
static volatile uint32_t s_flushes = 0;

static int nulo_init(void) {
    di("driver NULO: no pinta (prueba del pack de LVGL sin la pantalla de su placa)");
    return 0;
}
static int nulo_size(int* w, int* h) { *w = BPVM_NULO_W; *h = BPVM_NULO_H; return 1; }
static void nulo_flush(int x1, int y1, int x2, int y2, const uint8_t* px) {
    (void) x1; (void) y1; (void) x2; (void) y2; (void) px;
    s_flushes++;
}

static const bpvm_pack_sym_t s_syms[] = {
    { "init",  (bpvm_pack_fn_t) nulo_init  },
    { "size",  (bpvm_pack_fn_t) nulo_size  },
    { "flush", (bpvm_pack_fn_t) nulo_flush },
};
static const bpvm_pack_api_t s_api = {
    BPVM_PANT_MARCA, BPVM_PANT_API_VER,
    (uint32_t) (sizeof s_syms / sizeof s_syms[0]), s_syms
};

int32_t bp_pack_init(const bpvm_bios_t* bios)
{
    if (bios == 0)                        return 1;
    if (bios->magic != BPVM_BIOS_MAGIC)   return 2;
    if (bios->version < 4u)               return 3;   /* sólo log y publica: el punto de encuentro es de la v4 */
    if (bios->log == 0)                   return 4;
    g_bios = bios;
    if (bios->publica(BPVM_PANT_MARCA, &s_api) != 0) {
        di("driver NULO: publica('PANT') fallo (¿otro driver de pantalla grabado?)");
        return 5;
    }
    di("driver NULO: publicado como 'PANT' (240x320, no pinta)");
    return 0;
}
