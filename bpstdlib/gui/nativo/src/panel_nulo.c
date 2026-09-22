/* panel_nulo.c — el panel que NO PINTA (V7/H6 P3): un display de LVGL con un
 * flush que no manda nada a ninguna parte.
 *
 * ─── PARA QUÉ, Y POR QUÉ NO ES UN JUGUETE ───
 *
 * Idea de Eduardo (22-sep): *«¿se podría probar en la P4 sin desactivar el LVGL
 * actual?»*. El pack de la GUI trae el panel de la C5 (ST7789 + CH32 + CST3530),
 * que en un P4 no existe; pero de todo lo que hay que probar, el panel es la
 * ÚLTIMA pieza y la única que de verdad pide esa placa. Con este backend el pack
 * se puede cargar y ejercitar HOY en el P4 —que es la que está en la mesa— y lo
 * que se prueba no es poco:
 *
 *   · el `.npk` de 760 KB se realoja y se salta (2774 relocalizaciones)
 *   · LVGL arranca DENTRO de un pack: su TLSF sobre el pool estático, su stdlib
 *     interna, y su `.rodata` —las fuentes— realojada
 *   · `gui.c`, el MISMO fichero de la imagen, compilado con BPVM_EN_PACK, con el
 *     pegamento sirviéndole memoria, log y ficheros desde la tabla BIOS
 *   · la tabla de ~87 símbolos publicada bajo 'LVGL' y resuelta POR NOMBRE
 *   · el MODELO: crear widgets, textos, geometría — que es justamente sobre lo
 *     que se mide la paridad con miVM (el árbol, no los píxeles)
 *
 * Lo que NO prueba, y hay que decirlo: el panel, el táctil y el tiempo de flush.
 * Eso es la C5 y no hay atajo.
 *
 * Y hay una segunda cosa que sí prueba, y es la que preguntaba Eduardo: que el
 * pack CONVIVE con el LVGL de la imagen. Son dos instancias independientes —cada
 * una con sus globales, la de la imagen en su `.bss`, la del pack en su bloque de
 * RAM— y la del pack sólo se mueve cuando alguien la llama. La imagen sigue
 * pintando en su panel MIPI-DSI como si nada.
 *
 * Se elige en el BUILD (`build_lvgl.sh <destino> nulo`), no en runtime: un panel
 * que se queda mudo porque su chip no contesta y un panel que no pinta A PROPÓSITO
 * son cosas distintas, y confundirlas es exactamente el fallo silencioso que la
 * casa no admite. */
#include "bpvm_bios.h"
#include "bpvm_gui.h"
#include "lvgl.h"
#include <stdint.h>

extern const bpvm_bios_t* g_bios;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

#ifndef BPVM_PACK_NULO_W
#define BPVM_PACK_NULO_W 240      /* el tamaño de la C5: así el modelo (y el dump) sale igual */
#endif
#ifndef BPVM_PACK_NULO_H
#define BPVM_PACK_NULO_H 320
#endif
#define DRAW_LINES  8
#define DRAW_BYTES  (BPVM_PACK_NULO_W * DRAW_LINES * 2)

static uint8_t       s_buf[DRAW_BYTES];
static lv_display_t* s_disp = 0;
static int           s_open = 0;
static uint32_t      s_flushes = 0;

static uint32_t nulo_tick(void) { return g_bios->now_ms(); }

/* Cuenta y da por bueno. Contar NO es adorno: es la diferencia entre «LVGL
 * dibujó y no había dónde ponerlo» y «LVGL no llegó a dibujar». */
static void nulo_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    (void) area; (void) px_map;
    s_flushes++;
    lv_display_flush_ready(disp);
}

void bpvm_gui_disp_init(int w, int h) {
    (void) w; (void) h;
    if (!g_bios) return;
    lv_tick_set_cb(nulo_tick);
    s_disp = lv_display_create(BPVM_PACK_NULO_W, BPVM_PACK_NULO_H);
    lv_display_set_flush_cb(s_disp, nulo_flush);
    lv_display_set_buffers(s_disp, s_buf, NULL, DRAW_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    s_open = 1;
    di("gui: panel NULO (no pinta: prueba del pack sin la placa de su pantalla)");
}

uint32_t bpvm_gui_disp_pump(void) {
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > BPVM_GUI_OCIO_MAX_MS) idle_ms = BPVM_GUI_OCIO_MAX_MS;
    return idle_ms;
}
int  bpvm_gui_disp_is_open(void) { return s_open; }
int  bpvm_gui_disp_native_size(int* w, int* h) { *w = BPVM_PACK_NULO_W; *h = BPVM_PACK_NULO_H; return 1; }
void bpvm_gui_disp_set_rotation(int deg) { (void) deg; }

/* Lo que el panel nulo sí puede contar. No cruza la tabla de la GUI (no está en
 * bpvm_gui.h): lo lee la entrada del pack para dejarlo en el log. */
uint32_t bpvm_pack_nulo_flushes(void) { return s_flushes; }
