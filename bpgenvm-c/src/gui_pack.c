/* gui_pack.c — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.
 * NO EDITAR. V7/H6 P2: la GUI de la VM cuando LVGL NO va dentro de la imagen.
 *
 * Cada función de bpvm_gui.h es aquí un TRAMPOLÍN a la tabla que publica el pack de
 * la GUI bajo la marca 'LVGL' (bpvm_pack_api_t con los nombres de gui_pack_api.h).
 * La tabla se resuelve UNA vez, en el primer uso, POR NOMBRE (bpvm_pack_api_find):
 * un símbolo que falte se dice, y una versión de API distinta también. Sin pack,
 * `bpvm_gui_disponible()` devuelve 0 con el motivo y el builtin __guiScreenActive
 * lanza un RuntimeError CON NOMBRE; los demás trampolines, si se llaman sin tabla,
 * no hacen nada y devuelven 0/NULL (nunca saltan a ninguna parte).
 *
 * Se compila con -DBPVM_GUI_PACK en vez de gui.c; en el host y en las imágenes con
 * LVGL dentro no existe: ahí bpvm_gui_* son las funciones de gui.c. */
#include "bpvm_gui.h"
#include "gui_pack_api.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_alloc.h"
#include "bpvm.h"
#include <string.h>
#include <stdio.h>

static bpvm_gui_tabla_t s_t;
static int  s_estado = 0;          /* 0 = sin resolver · 1 = resuelta · -1 = no hay / no vale */
static char s_motivo[128];

static int resolver(void) {
    if (s_estado != 0) return s_estado > 0;
    const bpvm_pack_api_t* api = (const bpvm_pack_api_t*) bpvm_bios_busca(BPVM_GUI_MARCA);
    if (api == 0) {
        snprintf(s_motivo, sizeof s_motivo, "esta placa no tiene el pack 'LVGL' grabado (la GUI llega por el pack)");
        s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;
    }
    const char* mal = bpvm_pack_api_verify(api, BPVM_GUI_MARCA, BPVM_GUI_API_VER);
    if (mal) {
        snprintf(s_motivo, sizeof s_motivo, "el pack 'LVGL' no vale: %s (API v%u, se esperaba v%u)", mal, (unsigned) api->version, (unsigned) BPVM_GUI_API_VER);
        s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;
    }
    bpvm_pack_fn_t* ranuras = (bpvm_pack_fn_t*) (void*) &s_t;   /* la struct son punteros a función, en orden */
    for (int i = 0; i < BPVM_GUI_N_FUNCS; i++) {
        bpvm_pack_fn_t f = bpvm_pack_api_find(api, BPVM_GUI_NOMBRES[i]);
        if (f == 0) {
            snprintf(s_motivo, sizeof s_motivo, "al pack 'LVGL' le falta '%s'", BPVM_GUI_NOMBRES[i]);
            s_estado = -1; bpvm_diag("GUI: %s", s_motivo); return 0;
        }
        ranuras[i] = f;
    }
    s_estado = 1;
    bpvm_diag("GUI: pack 'LVGL' resuelto (%d funciones, API v%u)", (int) BPVM_GUI_N_FUNCS, (unsigned) api->version);
    return 1;
}

int bpvm_gui_disponible(const char** motivo) {
    int ok = resolver();
    if (motivo) *motivo = ok ? "" : s_motivo;
    return ok;
}

/* Para las pruebas de host y el simulador: olvidar la tabla (otro pack, otra vez). */
void bpvm_gui_pack_reset(void) { s_estado = 0; memset(&s_t, 0, sizeof s_t); }

int bpvm_gui_parent_alive(int parent) { return resolver() ? s_t.parent_alive(parent) : 0; }
int bpvm_gui_screen_active(void) { return resolver() ? s_t.screen_active() : 0; }
int bpvm_gui_create_obj(int parent) { return resolver() ? s_t.create_obj(parent) : 0; }
int bpvm_gui_create_label(int parent) { return resolver() ? s_t.create_label(parent) : 0; }
int bpvm_gui_create_button(int parent) { return resolver() ? s_t.create_button(parent) : 0; }
int bpvm_gui_create_checkbox(int parent) { return resolver() ? s_t.create_checkbox(parent) : 0; }
int bpvm_gui_create_switch(int parent) { return resolver() ? s_t.create_switch(parent) : 0; }
int bpvm_gui_create_slider(int parent) { return resolver() ? s_t.create_slider(parent) : 0; }
int bpvm_gui_create_bar(int parent) { return resolver() ? s_t.create_bar(parent) : 0; }
int bpvm_gui_create_spinbox(int parent) { return resolver() ? s_t.create_spinbox(parent) : 0; }
int bpvm_gui_create_led(int parent) { return resolver() ? s_t.create_led(parent) : 0; }
int bpvm_gui_create_dropdown(int parent) { return resolver() ? s_t.create_dropdown(parent) : 0; }
int bpvm_gui_create_textarea(int parent) { return resolver() ? s_t.create_textarea(parent) : 0; }
void bpvm_gui_set_options(int handle, const char* opts) { if (resolver()) s_t.set_options(handle, opts); }
const char* bpvm_gui_get_text(int handle) { return resolver() ? s_t.get_text(handle) : 0; }
int bpvm_gui_create_list(int parent) { return resolver() ? s_t.create_list(parent) : 0; }
int bpvm_gui_create_keyboard(int parent) { return resolver() ? s_t.create_keyboard(parent) : 0; }
void bpvm_gui_keyboard_set_textarea(int handle, int ta_handle) { if (resolver()) s_t.keyboard_set_textarea(handle, ta_handle); }
int bpvm_gui_create_msgbox(int parent) { return resolver() ? s_t.create_msgbox(parent) : 0; }
void bpvm_gui_set_buttons(int handle, const char* labels) { if (resolver()) s_t.set_buttons(handle, labels); }
int bpvm_gui_create_tabview(int parent) { return resolver() ? s_t.create_tabview(parent) : 0; }
int bpvm_gui_tabview_add_tab(int handle, const char* name) { return resolver() ? s_t.tabview_add_tab(handle, name) : 0; }
int bpvm_gui_create_table(int parent) { return resolver() ? s_t.create_table(parent) : 0; }
int bpvm_gui_create_chart(int parent) { return resolver() ? s_t.create_chart(parent) : 0; }
void bpvm_gui_chart_set_points(int handle, int n) { if (resolver()) s_t.chart_set_points(handle, n); }
int bpvm_gui_chart_add_series(int handle, int rgb) { return resolver() ? s_t.chart_add_series(handle, rgb) : 0; }
void bpvm_gui_chart_push(int handle, int series, int value) { if (resolver()) s_t.chart_push(handle, series, value); }
void bpvm_gui_chart_set_value(int handle, int series, int idx, int value) { if (resolver()) s_t.chart_set_value(handle, series, idx, value); }
void bpvm_gui_chart_set_type(int handle, int type) { if (resolver()) s_t.chart_set_type(handle, type); }
void bpvm_gui_table_set_grid(int handle, int rows, int cols) { if (resolver()) s_t.table_set_grid(handle, rows, cols); }
void bpvm_gui_table_set_cell(int handle, int row, int col, const char* text) { if (resolver()) s_t.table_set_cell(handle, row, col, text); }
const char* bpvm_gui_table_get_cell(int handle, int row, int col) { return resolver() ? s_t.table_get_cell(handle, row, col) : 0; }
int bpvm_gui_image_new(void) { return resolver() ? s_t.image_new() : 0; }
int bpvm_gui_image_load_file(int id, const char* path) { return resolver() ? s_t.image_load_file(id, path) : 0; }
int bpvm_gui_image_width(int id) { return resolver() ? s_t.image_width(id) : 0; }
int bpvm_gui_image_height(int id) { return resolver() ? s_t.image_height(id) : 0; }
int bpvm_gui_create_imageview(int parent) { return resolver() ? s_t.create_imageview(parent) : 0; }
void bpvm_gui_imageview_set_image(int view, int img) { if (resolver()) s_t.imageview_set_image(view, img); }
void bpvm_gui_imageview_refresh(int view) { if (resolver()) s_t.imageview_refresh(view); }
void bpvm_gui_set_font_size(int handle, int px) { if (resolver()) s_t.set_font_size(handle, px); }
int bpvm_gui_get_font_size(int handle) { return resolver() ? s_t.get_font_size(handle) : 0; }
int bpvm_gui_load_font(const char* path) { return resolver() ? s_t.load_font(path) : 0; }
void bpvm_gui_set_rotation(int deg) { if (resolver()) s_t.set_rotation(deg); }
void bpvm_gui_textarea_set_readonly(int handle, int ro) { if (resolver()) s_t.textarea_set_readonly(handle, ro); }
int bpvm_gui_textarea_get_readonly(int handle) { return resolver() ? s_t.textarea_get_readonly(handle) : 0; }
void bpvm_gui_set_text(int handle, const char* s) { if (resolver()) s_t.set_text(handle, s); }
void bpvm_gui_set_width(int handle, int w) { if (resolver()) s_t.set_width(handle, w); }
void bpvm_gui_set_height(int handle, int h) { if (resolver()) s_t.set_height(handle, h); }
void bpvm_gui_align(int handle, int a, int dx, int dy) { if (resolver()) s_t.align(handle, a, dx, dy); }
void bpvm_gui_set_x(int handle, int x) { if (resolver()) s_t.set_x(handle, x); }
int bpvm_gui_get_x(int handle) { return resolver() ? s_t.get_x(handle) : 0; }
void bpvm_gui_set_y(int handle, int y) { if (resolver()) s_t.set_y(handle, y); }
int bpvm_gui_get_y(int handle) { return resolver() ? s_t.get_y(handle) : 0; }
int bpvm_gui_get_width(int handle) { return resolver() ? s_t.get_width(handle) : 0; }
int bpvm_gui_get_height(int handle) { return resolver() ? s_t.get_height(handle) : 0; }
void bpvm_gui_set_scroll_dir(int handle, int dir) { if (resolver()) s_t.set_scroll_dir(handle, dir); }
int bpvm_gui_get_scroll_dir(int handle) { return resolver() ? s_t.get_scroll_dir(handle) : 0; }
int bpvm_gui_get_align(int handle) { return resolver() ? s_t.get_align(handle) : 0; }
int bpvm_gui_get_align_dx(int handle) { return resolver() ? s_t.get_align_dx(handle) : 0; }
int bpvm_gui_get_align_dy(int handle) { return resolver() ? s_t.get_align_dy(handle) : 0; }
int bpvm_gui_get_auth_width(int handle) { return resolver() ? s_t.get_auth_width(handle) : 0; }
int bpvm_gui_get_auth_height(int handle) { return resolver() ? s_t.get_auth_height(handle) : 0; }
int bpvm_gui_shot(const char* path) { return resolver() ? s_t.shot(path) : 0; }
void bpvm_gui_refresh(int handle) { if (resolver()) s_t.refresh(handle); }
void bpvm_gui_set_checked(int handle, int v) { if (resolver()) s_t.set_checked(handle, v); }
int bpvm_gui_get_checked(int handle) { return resolver() ? s_t.get_checked(handle) : 0; }
void bpvm_gui_set_value(int handle, int v) { if (resolver()) s_t.set_value(handle, v); }
int bpvm_gui_get_value(int handle) { return resolver() ? s_t.get_value(handle) : 0; }
void bpvm_gui_set_range(int handle, int mn, int mx) { if (resolver()) s_t.set_range(handle, mn, mx); }
void bpvm_gui_set_bg_color(int handle, uint32_t rgb) { if (resolver()) s_t.set_bg_color(handle, rgb); }
void bpvm_gui_set_text_color(int handle, uint32_t rgb) { if (resolver()) s_t.set_text_color(handle, rgb); }
void bpvm_gui_set_font(int handle, int font_id) { if (resolver()) s_t.set_font(handle, font_id); }
void bpvm_gui_clean(int handle) { if (resolver()) s_t.clean(handle); }
void bpvm_gui_delete(int handle) { if (resolver()) s_t.delete(handle); }
void bpvm_gui_reset(void) { if (resolver()) s_t.reset(); }
void bpvm_gui_bind_click(int handle, uint32_t objptr) { if (resolver()) s_t.bind_click(handle, objptr); }
void bpvm_gui_inject_click(uint32_t objptr) { if (resolver()) s_t.inject_click(objptr); }
void bpvm_gui_inject_change(uint32_t objptr) { if (resolver()) s_t.inject_change(objptr); }
uint32_t bpvm_gui_next_event(int* kind_out) { return resolver() ? s_t.next_event(kind_out) : 0; }
void bpvm_gui_visit_roots(void (*visit)(void* ctx, uint32_t objptr), void* ctx) { if (resolver()) s_t.visit_roots(visit, ctx); }
size_t bpvm_gui_dump_tree(char** out) {
    /* la memoria la reserva el PACK en su arena: se copia a la de la VM (que es lo
     * que el que llama va a liberar) y se le devuelve la suya con dump_free. */
    if (!resolver()) { if (out) *out = 0; return 0; }
    char* suyo = 0; size_t n = s_t.dump_tree(&suyo);
    char* mio = (char*) bpvm_malloc(n + 1);
    if (mio) { if (suyo && n) memcpy(mio, suyo, n); mio[n] = 0; }
    if (suyo) s_t.dump_free(suyo);
    if (out) *out = mio;
    return mio ? n : 0;
}
uint32_t bpvm_gui_lvgl_pump(void) { return resolver() ? s_t.lvgl_pump() : 0; }
int bpvm_gui_lvgl_window_open(void) { return resolver() ? s_t.lvgl_window_open() : 0; }
uint32_t bpvm_gui_disp_pump(void) { return resolver() ? s_t.disp_pump() : 0; }
int bpvm_gui_disp_is_open(void) { return resolver() ? s_t.disp_is_open() : 0; }
void bpvm_gui_disp_set_rotation(int deg) { if (resolver()) s_t.disp_set_rotation(deg); }
int bpvm_gui_disp_native_size(int* w, int* h) { return resolver() ? s_t.disp_native_size(w, h) : 0; }

/* Las del host/simulador no cruzan: en un micro no hay ventana ni --screen=. */
void bpvm_gui_set_screen_size(int w, int h) { (void) w; (void) h; }
#ifdef BPVM_GUI_RENDER
void bpvm_gui_disp_set_headless(int on) { (void) on; }
void bpvm_gui_disp_set_exit_on_close(int on) { (void) on; }
void bpvm_gui_disp_set_title(const char* title) { (void) title; }
#endif
