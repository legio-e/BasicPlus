/*
 * gui_display_sdl.c — backend de display LVGL para el HOST (ventana SDL2), V3/H5.1.
 *
 * Implementa el contrato bpvm_gui_disp_* (declarado en bpvm_gui.h) que gui.c
 * llama tras lv_init(). El render de widgets (lv_label/lv_button/align/…) es
 * PORTABLE y vive en gui.c; lo único atado a la plataforma —ventana, tick,
 * input, cierre— es esto. En el micro el backend equivalente es LTDC (port/).
 *
 * Todo el fichero va bajo el guard BPVM_LVGL → sin el flag es una unidad de
 * compilación vacía (inofensiva si el build lo incluye sin LVGL).
 */
#ifdef BPVM_LVGL

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "bpvm_gui.h"

static volatile int g_window_closed = 0;

/* Watch de eventos SDL: marca el cierre de ventana SIN consumir el evento (LVGL
 * sigue recibiendo el ratón). Evita depender de internals de lv_sdl. */
static int SDLCALL lvgl_watch(void* ud, SDL_Event* e) {
    (void) ud;
    if (e->type == SDL_QUIT ||
        (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_CLOSE))
        g_window_closed = 1;
    return 1;   /* 1 = no filtrar (el evento sigue su curso) */
}

void bpvm_gui_disp_init(int w, int h) {
    SDL_SetMainReady();
    lv_tick_set_cb(SDL_GetTicks);
    lv_sdl_window_create(w, h);
    lv_sdl_mouse_create();
    SDL_AddEventWatch(lvgl_watch, NULL);
}

void bpvm_gui_disp_pump(void)  { lv_timer_handler(); SDL_Delay(16); }

int  bpvm_gui_disp_is_open(void) { return !g_window_closed; }

#endif /* BPVM_LVGL */
