/*
 * gui_smoke.c — smoke de toolchain para H4.2: comprueba que LVGL v9 + SDL2
 * compilan y enlazan con este MinGW (w64devkit) y que abre una ventana. NO
 * toca la VM; solo valida el cimiento de build antes de integrar LVGL en gui.c.
 *
 *   make gui-smoke
 */
#define SDL_MAIN_HANDLED          /* conservamos nuestro main(); sin -lSDL2main */
#include <SDL2/SDL.h>
#include "lvgl.h"
#include <stdio.h>

int main(void) {
    SDL_SetMainReady();
    lv_init();
    lv_tick_set_cb(SDL_GetTicks);                 /* fuente de tiempo (ms) */
    lv_display_t* disp  = lv_sdl_window_create(480, 320);
    lv_indev_t*   mouse = lv_sdl_mouse_create();
    (void) disp; (void) mouse;

    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "BasicPlus LVGL OK");
    lv_obj_center(label);

    for (int i = 0; i < 60; i++) {                /* ~1 s de frames */
        lv_timer_handler();
        SDL_Delay(16);
    }
    printf("LVGL+SDL smoke OK\n");
    return 0;
}
