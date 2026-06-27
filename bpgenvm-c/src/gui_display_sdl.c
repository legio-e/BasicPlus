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
#include <stdio.h>
#include "lvgl.h"
#include "bpvm_gui.h"

static volatile int g_window_closed = 0;
static volatile int g_shot_requested = 0;   /* F12 → captura de pantalla (docs) */

/* Watch de eventos SDL: marca el cierre de ventana SIN consumir el evento (LVGL
 * sigue recibiendo el ratón). Evita depender de internals de lv_sdl. */
static int SDLCALL lvgl_watch(void* ud, SDL_Event* e) {
    (void) ud;
    if (e->type == SDL_QUIT ||
        (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_CLOSE))
        g_window_closed = 1;
    /* Captura de pantalla para documentación: F12 marca la petición; el snapshot
     * real lo hace el pump (hilo del lv_timer_handler) para no reentrar LVGL
     * desde el hilo de eventos SDL. */
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_F12)
        g_shot_requested = 1;
    return 1;   /* 1 = no filtrar (el evento sigue su curso) */
}

/* Escribe un BMP 24-bit (BGR, bottom-up, filas a múltiplo de 4) desde un buffer
 * ARGB8888 de LVGL (en memoria: byte[0]=B,[1]=G,[2]=R,[3]=A). Sin dependencias. */
static int write_bmp24(const char* path, const uint8_t* px, int w, int h, int stride) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    int row = w * 3;
    int pad = (4 - (row % 4)) % 4;
    unsigned img  = (unsigned)((row + pad) * h);
    unsigned fsz  = 54u + img;
    unsigned char fh[14] = { 'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0 };
    fh[2]=(unsigned char)(fsz); fh[3]=(unsigned char)(fsz>>8); fh[4]=(unsigned char)(fsz>>16); fh[5]=(unsigned char)(fsz>>24);
    unsigned char ih[40] = {0};
    ih[0]=40;
    ih[4]=(unsigned char)w;  ih[5]=(unsigned char)(w>>8);  ih[6]=(unsigned char)(w>>16);  ih[7]=(unsigned char)(w>>24);
    ih[8]=(unsigned char)h;  ih[9]=(unsigned char)(h>>8);  ih[10]=(unsigned char)(h>>16); ih[11]=(unsigned char)(h>>24);
    ih[12]=1; ih[14]=24;   /* 1 plano, 24 bpp, sin compresión */
    fwrite(fh,1,14,f); fwrite(ih,1,40,f);
    unsigned char z[3] = {0,0,0};
    for (int y = h - 1; y >= 0; y--) {                 /* bottom-up */
        const uint8_t* r = px + (size_t) y * stride;
        for (int x = 0; x < w; x++) {
            const uint8_t* p = r + (size_t) x * 4;     /* ARGB8888: B,G,R,A */
            unsigned char bgr[3] = { p[0], p[1], p[2] };
            fwrite(bgr,1,3,f);
        }
        if (pad) fwrite(z,1,(size_t)pad,f);
    }
    fclose(f);
    return 0;
}

/* Snapshot de la pantalla activa → BMP numerado en el cwd. Lo llama el pump. */
static void bpvm_gui_take_screenshot(void) {
    lv_draw_buf_t* snap = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
    if (!snap) { printf("[gui] captura: lv_snapshot_take fallo\n"); fflush(stdout); return; }
    static int n = 0;
    char path[64];
    snprintf(path, sizeof(path), "bp_shot_%04d.bmp", ++n);
    int rc = write_bmp24(path, snap->data, (int) snap->header.w, (int) snap->header.h,
                         (int) snap->header.stride);
    if (rc == 0) printf("[gui] captura guardada: %s (%dx%d)\n", path,
                        (int) snap->header.w, (int) snap->header.h);
    else         printf("[gui] captura: error escribiendo %s\n", path);
    fflush(stdout);
    lv_draw_buf_destroy(snap);
}

void bpvm_gui_disp_init(int w, int h) {
    SDL_SetMainReady();
    lv_tick_set_cb(SDL_GetTicks);
    lv_sdl_window_create(w, h);
    lv_sdl_mouse_create();
    SDL_AddEventWatch(lvgl_watch, NULL);
}

void bpvm_gui_disp_pump(void) {
    lv_timer_handler();
    if (g_shot_requested) { g_shot_requested = 0; bpvm_gui_take_screenshot(); }
    SDL_Delay(16);
}

int  bpvm_gui_disp_is_open(void) { return !g_window_closed; }

#endif /* BPVM_LVGL */
