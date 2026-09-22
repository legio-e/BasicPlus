/* lvgl_pack.c — LA ENTRADA del pack de la GUI (V7/H6 P3): LVGL + gui.c + el
 * driver del panel, compilados aparte, grabados en la zona de packs y ejecutados
 * XIP. La VM no sabe nada de esto: sólo `publica()`/`busca()` y la tabla que
 * gui_pack_publics.c construye de bpvm_gui.h (generada). Es el modelo de
 * `sqlite_pack.c`, con dos diferencias que están medidas (V7_IDEAS §H6):
 *
 *   · LVGL no pide malloc a la BIOS: trae su TLSF sobre un pool ESTÁTICO
 *     (LV_MEM_SIZE en .bss, la perilla del build). El bloque de RAM que el
 *     cargador le da es [.data | .bss] y nada más; `arena()` queda para SQLite.
 *   · No usa coma flotante, ni ficheros salvo los PNG/fuentes que pida un
 *     programa (por la BIOS), ni tiempo salvo el reloj de la HAL (el tick).
 *
 * Devuelve 0 si arrancó, o el paso que falló (los mismos códigos que el mini
 * para los cuatro primeros; 5 = publicar; cada paso se cuenta por `log`). */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "gui_pack_api.h"
#include "lvgl.h"
#include <stdint.h>

/* La tabla que presta el firmware: la única ventana del pack al mundo. La ven
 * el pegamento (gui_pack_glue.c: ficheros, log) y el driver (panel_c5.c: la HAL). */
const bpvm_bios_t* g_bios = 0;

/* La tabla que ESTE pack publica (gui_pack_publics.c, GENERADA de bpvm_gui.h). */
extern const bpvm_pack_api_t bpvm_gui_api;

static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

int32_t bp_pack_init(const bpvm_bios_t* bios)
{
    if (bios == 0)                          return 1;
    if (bios->magic   != BPVM_BIOS_MAGIC)   return 2;
    if (bios->version <  BPVM_BIOS_VERSION) return 3;   /* la tabla crece por el final */
    if (bios->log     == 0)                 return 4;
    g_bios = bios;
    di("gui: pack vivo (LVGL " LVGL_VERSION_INFO ")");

    /* LVGL entero, sin display todavía: el display lo monta gui.c en el primer
     * Gui.screen() (lvgl_ensure_init -> bpvm_gui_disp_init, que es panel_c5.c).
     * lv_init es idempotente en v9, así que esa segunda llamada no molesta. */
    lv_init();
    di("gui: lv_init hecho (TLSF sobre el pool estatico del pack)");

    int r = bios->publica(BPVM_GUI_MARCA, &bpvm_gui_api);
    if (r != 0) {
        di(r == -3 ? "gui: 'LVGL' ya estaba publicada (¿el pack cargado dos veces?)"
         : r == -4 ? "gui: la BIOS RECHAZO la tabla (la reja): ¿gui_pack_publics.c no esta al dia?"
         : r == -2 ? "gui: no queda hueco para publicar 'LVGL'"
                   : "gui: publica('LVGL') fallo");
        return 5;
    }
    di("gui: API publicada como 'LVGL'");
    return 0;
}
