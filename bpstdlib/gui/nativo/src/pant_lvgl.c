/* pant_lvgl.c — LVGL sobre 'PANT': el display del pack de la GUI, SIN driver de
 * pantalla dentro (V7/H6.c G2).
 *
 * Hasta G2 el pack de LVGL llevaba compilado su driver (`panel_c5.c`, o el nulo
 * para el P4), elegido al construirlo: un pack por placa, y el driver —2 KB de
 * los 715— viajando dentro de lo que no cambia nunca. Ahora el driver es un pack
 * aparte que publica 'PANT' (`bpvm_pant.h`), y este fichero es todo lo que LVGL
 * necesita para usarlo. Implementa el contrato de display de `bpvm_gui.h`
 * (`bpvm_gui_disp_*`) que `gui.c` ya llamaba, así que `gui.c` no se entera.
 *
 * ─── LO QUE VIVE AQUÍ, Y POR QUÉ AQUÍ ───
 *
 *   · el display de LVGL, sus dos búferes de dibujo y el formato de píxel: son
 *     de LVGL. El driver pide el formato (`formato`) y aquí se pinta en él, o
 *     se giran los bytes con la rutina de LVGL, antes de pasarle la zona;
 *   · el táctil como `lv_indev`: LVGL lo lee; el driver sólo dice dónde tocan;
 *   · el reloj de LVGL (`lv_tick`): sale de la HAL de la BIOS.
 *
 * ─── EL DRIVER SE BUSCA EN EL PRIMER USO ───
 *
 * `bpvm_gui_disp_native_size` (que `gui.c` llama ANTES de crear el screen) o
 * `bpvm_gui_disp_init`, lo que llegue antes. No al cargar el pack: así el orden
 * en que se graben los dos packs da igual. Si no hay driver lo dice con nombre;
 * y la VM, antes de dejar que un programa cree su screen, comprueba lo mismo
 * (`bpvm_gui_disponible`), así que el programa recibe un error y no una
 * pantalla negra. Lo que queda aquí —un display que no pinta— es sólo para que
 * LVGL no se caiga si alguien llegara sin pasar por esa puerta.
 */
#include "bpvm_bios.h"
#include "bpvm_gui.h"
#include "bpvm_pant.h"
#include "lvgl.h"
#include "src/draw/sw/lv_draw_sw.h"    /* lv_draw_sw_rgb565_swap */
#include <stdint.h>

extern const bpvm_bios_t* g_bios;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

/* Los búferes de dibujo, en BYTES: LVGL saca de aquí cuántas líneas caben (a
 * 240 px en RGB565 son las 24 líneas de siempre; en RGB888 o en una pantalla
 * más ancha, menos). Estáticos: van en la .bss del pack, como antes en el
 * driver. La perilla es de build_lvgl.sh. */
#ifndef BPVM_PANT_DRAW_BYTES
#define BPVM_PANT_DRAW_BYTES 11520
#endif
/* V7/H6.b — ¿UNO o DOS? El flush de este pack es SÍNCRONO (al volver del driver la franja ya salió), así
 * que el segundo búfer no adelanta nada: LVGL no dibuja en él mientras el primero se envía. Son 11,5 KB
 * pagados sin uso, y en el pack para pantallas pequeñas (GuiMini) va uno (-DBPVM_PANT_BUFS=1). El doble
 * búfer con sentido —el envío asíncrono— queda para cuando lo elija el usuario y haya memoria (Eduardo,
 * 25-sep; V7_IDEAS §H6.14.8). El pack de siempre sigue con dos: no se cambia lo que no hace falta. */
#ifndef BPVM_PANT_BUFS
#define BPVM_PANT_BUFS 2
#endif
static uint8_t s_buf1[BPVM_PANT_DRAW_BYTES] __attribute__((aligned(16)));
#if BPVM_PANT_BUFS >= 2
static uint8_t s_buf2[BPVM_PANT_DRAW_BYTES] __attribute__((aligned(16)));
#define PANT_BUF2 s_buf2
#else
#define PANT_BUF2 0
#endif

static bpvm_pant_t   s_p;
static int           s_estado  = 0;    /* 0 sin buscar · 1 resuelto · -1 no hay / no vale */
static int           s_formato = BPVM_PANT_RGB565;
static int           s_w = 0, s_h = 0; /* el tamaño físico, a 0° */
static lv_display_t* s_disp  = 0;
static lv_indev_t*   s_touch = 0;
static int           s_open  = 0;

static int resolver(void) {
    if (s_estado != 0) return s_estado > 0;
    s_estado = -1;
    if (!g_bios) return 0;
    const bpvm_pack_api_t* api = (const bpvm_pack_api_t*) g_bios->busca(BPVM_PANT_MARCA);
    if (api == 0) {
        di("gui: NO HAY DRIVER DE PANTALLA — nadie ha publicado 'PANT': graba el pack del driver de esta placa");
        return 0;
    }
    const char* mal = bpvm_pant_resolver(api, &s_p);
    if (mal) {
        char b[120];
        lv_snprintf(b, sizeof b, "gui: el driver de pantalla ('PANT') no vale: %s", mal);
        di(b);
        return 0;
    }
    s_formato = s_p.formato ? s_p.formato() : BPVM_PANT_RGB565;
    if (s_formato != BPVM_PANT_RGB565 && s_formato != BPVM_PANT_RGB565_BE && s_formato != BPVM_PANT_RGB888) {
        char b[96];
        lv_snprintf(b, sizeof b, "gui: el driver pide un formato de pixel que no existe (%d)", s_formato);
        di(b);
        return 0;
    }
    if (!s_p.size(&s_w, &s_h) || s_w <= 0 || s_h <= 0) {
        di("gui: el driver de pantalla no sabe su tamano");
        return 0;
    }
    s_estado = 1;
    return 1;
}

static uint32_t pant_tick(void) { return g_bios->now_ms(); }

/* El flush de LVGL: lo que el driver pidió, y la zona. Los bytes se giran AQUÍ,
 * con la rutina de LVGL, si el driver quiere RGB565 al revés (SPI). */
static void pant_flush(lv_display_t* disp, const lv_area_t* a, uint8_t* px) {
    if (s_formato == BPVM_PANT_RGB565_BE)
        lv_draw_sw_rgb565_swap(px, (uint32_t) ((a->x2 - a->x1 + 1) * (a->y2 - a->y1 + 1)));
    s_p.flush(a->x1, a->y1, a->x2, a->y2, px);
    lv_display_flush_ready(disp);               /* síncrono: al volver del driver ya está */
}

/* Un display que NO pinta, para que LVGL no se caiga sin driver (ver arriba). */
static void mudo_flush(lv_display_t* disp, const lv_area_t* a, uint8_t* px) {
    (void) a; (void) px;
    lv_display_flush_ready(disp);
}

static void pant_touch(lv_indev_t* indev, lv_indev_data_t* data) {
    (void) indev;
    int x = 0, y = 0;
    if (s_p.touch && s_p.touch(&x, &y) == 1) {
        data->point.x = (int32_t) x;
        data->point.y = (int32_t) y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
    }
}

int bpvm_gui_disp_native_size(int* w, int* h) {
    if (!resolver()) return 0;
    *w = s_w; *h = s_h;
    return 1;
}

void bpvm_gui_disp_init(int w, int h) {
    /* V7 #644: con driver manda el tamaño del panel (s_w×s_h, el que gui.c ya pidió por disp_native_size); sin él
     * —la pantalla muda—, el que pasa gui.c. */
    if (!g_bios) return;
    lv_tick_set_cb(pant_tick);

    if (!resolver()) {
        s_disp = lv_display_create(w > 0 ? w : 240, h > 0 ? h : 320);
        lv_display_set_flush_cb(s_disp, mudo_flush);
        lv_display_set_buffers(s_disp, s_buf1, 0, BPVM_PANT_DRAW_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
        di("gui: display MUDO (sin driver de pantalla): el modelo funciona, no se pinta nada");
        return;
    }

    /* MIGA DE PAN, como en el driver de la C5: el arranque de una pantalla es
     * donde una placa se reinicia sin decir nada. */
    di("gui: arrancando el driver de pantalla ('PANT')");
    int r = s_p.init();
    if (r != 0) {
        char b[80];
        lv_snprintf(b, sizeof b, "gui: el driver de pantalla no arranco (init = %d)", r);
        di(b);
    }

    s_disp = lv_display_create(s_w, s_h);
    lv_display_set_color_format(s_disp, s_formato == BPVM_PANT_RGB888 ? LV_COLOR_FORMAT_RGB888
                                                                      : LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, pant_flush);
    lv_display_set_buffers(s_disp, s_buf1, PANT_BUF2, BPVM_PANT_DRAW_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    if (s_p.touch) {
        s_touch = lv_indev_create();
        lv_indev_set_type(s_touch, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(s_touch, pant_touch);
    }
    s_open = (r == 0);
    {
        char b[112];
        lv_snprintf(b, sizeof b, "gui: pantalla %dx%d por 'PANT', formato %s%s%s",
                    s_w, s_h,
                    s_formato == BPVM_PANT_RGB888 ? "RGB888" : s_formato == BPVM_PANT_RGB565_BE ? "RGB565 girado" : "RGB565",
                    s_p.touch ? ", tactil" : ", sin tactil",
                    s_p.rotate ? ", gira" : "");
        di(b);
    }
}

uint32_t bpvm_gui_disp_pump(void) {
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > BPVM_GUI_OCIO_MAX_MS) idle_ms = BPVM_GUI_OCIO_MAX_MS;
    return idle_ms;
}

int bpvm_gui_disp_is_open(void) { return s_open; }

void bpvm_gui_disp_set_rotation(int deg) {
    if (!s_open || !s_disp) return;
    if (!s_p.rotate || s_p.rotate(deg) != 0) {
        di("gui: el driver de pantalla no sabe girar — la orientacion se queda como estaba");
        return;
    }
    /* Con 90/270 el driver direcciona ancho y alto al revés: LVGL tiene que pintar eso. */
    if (deg == 90 || deg == 270) lv_display_set_resolution(s_disp, s_h, s_w);
    else                         lv_display_set_resolution(s_disp, s_w, s_h);
    lv_obj_invalidate(lv_screen_active());
}
