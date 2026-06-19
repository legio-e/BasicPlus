/*
 * bpvm_gui.h — backend GUI (modelo de comportamiento) de la VM-C, V3 / H4.1.
 *
 * Espejo del GuiBackend de miVM: cada widget BP es un `handle` (int) que mapea
 * a un nodo con (type, text, w, h, align, dx, dy, parent, objptr). El volcado
 * dump_tree produce el MISMO texto byte-a-byte que miVM → paridad de
 * comportamiento (que se mide sobre el árbol del UI, NO sobre píxeles).
 *
 * H4.1 es SÓLO el modelo: basta para la paridad y no arrastra dependencias.
 * El render real (LVGL + SDL en host) cuelga de este modelo en H4.2. Los
 * builtins __gui* (builtins.c) delegan aquí; el upcall onClick lo dispara el
 * case GUI_RUN via bpvm_call_bp_from_builtin (interp.c).
 */
#ifndef BPVM_GUI_H
#define BPVM_GUI_H

#include <stddef.h>
#include <stdint.h>

/* Creación / pantalla raíz. Devuelven el handle (>0), 0 si overflow. */
int  bpvm_gui_screen_active(void);
int  bpvm_gui_create_obj(int parent);
int  bpvm_gui_create_label(int parent);
int  bpvm_gui_create_button(int parent);

/* Configuración. Texto + geometría/anclaje afectan al dumpTree (modelo). Color y
 * fuente son render-only: no-op en el modelo (no tocan el dump) y, bajo BPVM_LVGL,
 * aplican estilo al lv_obj. */
void bpvm_gui_set_text(int handle, const char* s);
void bpvm_gui_set_width(int handle, int w);
void bpvm_gui_set_height(int handle, int h);
void bpvm_gui_align(int handle, int a, int dx, int dy);
/* H6 — geometría explícita (x,y), scroll (opt-in), refresh. Geometría/scroll
 * afectan al dump (modelo); refresh es render-only. */
void bpvm_gui_set_x(int handle, int x);
int  bpvm_gui_get_x(int handle);
void bpvm_gui_set_y(int handle, int y);
int  bpvm_gui_get_y(int handle);
int  bpvm_gui_get_width(int handle);
int  bpvm_gui_get_height(int handle);
void bpvm_gui_set_scroll_dir(int handle, int dir);
int  bpvm_gui_get_scroll_dir(int handle);
void bpvm_gui_refresh(int handle);
void bpvm_gui_set_bg_color(int handle, uint32_t rgb);
void bpvm_gui_set_text_color(int handle, uint32_t rgb);
void bpvm_gui_set_font(int handle, int font_id);
void bpvm_gui_clean(int handle);
void bpvm_gui_delete(int handle);

/* Eventos. bind asocia el objeto BP dueño del widget; inject encola un clic
 * sintético (diagnóstico / pruebas headless); next saca el siguiente objptr
 * (0 = cola vacía) — lo drena el bombeo de GUI_RUN. */
void     bpvm_gui_bind_click(int handle, uint32_t objptr);
void     bpvm_gui_inject_click(uint32_t objptr);
uint32_t bpvm_gui_next_click(void);

/* Vuelca el árbol de widgets a un buffer recién malloc'd (el caller hace free).
 * Devuelve la longitud en bytes (sin el '\0'). Byte-idéntico a miVM. */
size_t bpvm_gui_dump_tree(char** out);

#ifdef BPVM_LVGL
/* H4.2 — render real (LVGL v9). pump = una iteración del lazo (lv_timer_handler
 * + delay); window_open = false cuando el usuario cierra. */
void bpvm_gui_lvgl_pump(void);
int  bpvm_gui_lvgl_window_open(void);

/* H5.1 — backend de display (lo provee la PLATAFORMA; gui.c lo llama tras
 * lv_init()): host = SDL (src/gui_display_sdl.c), micro = LTDC (port/). El resto
 * del render —creación de widgets— es portable y vive en gui.c. */
void bpvm_gui_disp_init(int w, int h);   /* tick + display + (host) input/cierre */
void bpvm_gui_disp_pump(void);           /* lv_timer_handler + ceder CPU */
int  bpvm_gui_disp_is_open(void);        /* host: ventana abierta; micro: 1 si corre */
#endif

#endif /* BPVM_GUI_H */
