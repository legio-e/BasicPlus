/* gui_pack_stubs.c — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.
 * NO EDITAR. V7/H6 P2: un DOBLE del pack de la GUI para test_guipack.c: cada
 * función con la firma de verdad, que cuenta la llamada y devuelve un valor marcado
 * (7, "stub", 240x320…). Así la costura se prueba en el PC sin pack ni LVGL. */
#include "bpvm_gui.h"
#include "gui_pack_api.h"
#include "bpvm_pack_api.h"
#include <stdlib.h>
#include <string.h>
int g_stub_llamadas = 0;
int g_stub_dump_free = 0;
static int stub_parent_alive(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_screen_active(void) { g_stub_llamadas++;  return 7; }
static int stub_create_obj(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_label(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_button(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_checkbox(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_switch(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_slider(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_bar(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_spinbox(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_led(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_dropdown(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_textarea(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static void stub_set_options(int handle, const char* opts) { g_stub_llamadas++; (void) handle; (void) opts; }
static const char* stub_get_text(int handle) { g_stub_llamadas++; (void) handle; return "stub"; }
static int stub_create_list(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_keyboard(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static void stub_keyboard_set_textarea(int handle, int ta_handle) { g_stub_llamadas++; (void) handle; (void) ta_handle; }
static int stub_create_msgbox(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static void stub_set_buttons(int handle, const char* labels) { g_stub_llamadas++; (void) handle; (void) labels; }
static int stub_create_tabview(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_tabview_add_tab(int handle, const char* name) { g_stub_llamadas++; (void) handle; (void) name; return 7; }
static int stub_create_table(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static int stub_create_chart(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static void stub_chart_set_points(int handle, int n) { g_stub_llamadas++; (void) handle; (void) n; }
static int stub_chart_add_series(int handle, int rgb) { g_stub_llamadas++; (void) handle; (void) rgb; return 7; }
static void stub_chart_push(int handle, int series, int value) { g_stub_llamadas++; (void) handle; (void) series; (void) value; }
static void stub_chart_set_value(int handle, int series, int idx, int value) { g_stub_llamadas++; (void) handle; (void) series; (void) idx; (void) value; }
static void stub_chart_set_type(int handle, int type) { g_stub_llamadas++; (void) handle; (void) type; }
static void stub_table_set_grid(int handle, int rows, int cols) { g_stub_llamadas++; (void) handle; (void) rows; (void) cols; }
static void stub_table_set_cell(int handle, int row, int col, const char* text) { g_stub_llamadas++; (void) handle; (void) row; (void) col; (void) text; }
static const char* stub_table_get_cell(int handle, int row, int col) { g_stub_llamadas++; (void) handle; (void) row; (void) col; return "stub"; }
static int stub_image_new(void) { g_stub_llamadas++;  return 7; }
static int stub_image_load_file(int id, const char* path) { g_stub_llamadas++; (void) id; (void) path; return 7; }
static int stub_image_width(int id) { g_stub_llamadas++; (void) id; return 7; }
static int stub_image_height(int id) { g_stub_llamadas++; (void) id; return 7; }
static int stub_create_imageview(int parent) { g_stub_llamadas++; (void) parent; return 7; }
static void stub_imageview_set_image(int view, int img) { g_stub_llamadas++; (void) view; (void) img; }
static void stub_imageview_refresh(int view) { g_stub_llamadas++; (void) view; }
static void stub_set_font_size(int handle, int px) { g_stub_llamadas++; (void) handle; (void) px; }
static int stub_get_font_size(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_load_font(const char* path) { g_stub_llamadas++; (void) path; return 7; }
static void stub_set_rotation(int deg) { g_stub_llamadas++; (void) deg; }
static void stub_textarea_set_readonly(int handle, int ro) { g_stub_llamadas++; (void) handle; (void) ro; }
static int stub_textarea_get_readonly(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static void stub_set_text(int handle, const char* s) { g_stub_llamadas++; (void) handle; (void) s; }
static void stub_set_width(int handle, int w) { g_stub_llamadas++; (void) handle; (void) w; }
static void stub_set_height(int handle, int h) { g_stub_llamadas++; (void) handle; (void) h; }
static void stub_align(int handle, int a, int dx, int dy) { g_stub_llamadas++; (void) handle; (void) a; (void) dx; (void) dy; }
static void stub_set_x(int handle, int x) { g_stub_llamadas++; (void) handle; (void) x; }
static int stub_get_x(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static void stub_set_y(int handle, int y) { g_stub_llamadas++; (void) handle; (void) y; }
static int stub_get_y(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_width(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_height(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static void stub_set_scroll_dir(int handle, int dir) { g_stub_llamadas++; (void) handle; (void) dir; }
static int stub_get_scroll_dir(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_align(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_align_dx(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_align_dy(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_auth_width(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_get_auth_height(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static int stub_shot(const char* path) { g_stub_llamadas++; (void) path; return 7; }
static void stub_refresh(int handle) { g_stub_llamadas++; (void) handle; }
static void stub_set_checked(int handle, int v) { g_stub_llamadas++; (void) handle; (void) v; }
static int stub_get_checked(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static void stub_set_value(int handle, int v) { g_stub_llamadas++; (void) handle; (void) v; }
static int stub_get_value(int handle) { g_stub_llamadas++; (void) handle; return 7; }
static void stub_set_range(int handle, int mn, int mx) { g_stub_llamadas++; (void) handle; (void) mn; (void) mx; }
static void stub_set_bg_color(int handle, uint32_t rgb) { g_stub_llamadas++; (void) handle; (void) rgb; }
static void stub_set_text_color(int handle, uint32_t rgb) { g_stub_llamadas++; (void) handle; (void) rgb; }
static void stub_set_font(int handle, int font_id) { g_stub_llamadas++; (void) handle; (void) font_id; }
static void stub_clean(int handle) { g_stub_llamadas++; (void) handle; }
static void stub_delete(int handle) { g_stub_llamadas++; (void) handle; }
static void stub_reset(void) { g_stub_llamadas++;  }
static void stub_bind_click(int handle, uint32_t objptr) { g_stub_llamadas++; (void) handle; (void) objptr; }
static void stub_inject_click(uint32_t objptr) { g_stub_llamadas++; (void) objptr; }
static void stub_inject_change(uint32_t objptr) { g_stub_llamadas++; (void) objptr; }
static uint32_t stub_next_event(int* kind_out) { g_stub_llamadas++; (void) kind_out; return 7; }
static void stub_visit_roots(void (*visit)(void* ctx, uint32_t objptr), void* ctx) { g_stub_llamadas++; visit(ctx, 42u); }
static size_t stub_dump_tree(char** out) { g_stub_llamadas++; { const char* d = "stub-dump"; char* b = (char*) malloc(strlen(d) + 1); strcpy(b, d); *out = b; return strlen(d); } }
static uint32_t stub_lvgl_pump(void) { g_stub_llamadas++;  return 7; }
static int stub_lvgl_window_open(void) { g_stub_llamadas++;  return 7; }
static uint32_t stub_disp_pump(void) { g_stub_llamadas++;  return 7; }
static int stub_disp_is_open(void) { g_stub_llamadas++;  return 7; }
static void stub_disp_set_rotation(int deg) { g_stub_llamadas++; (void) deg; }
static int stub_disp_native_size(int* w, int* h) { g_stub_llamadas++; *w = 240; *h = 320; return 1; }
static void stub_dump_free(char* p) { g_stub_dump_free++; free(p); }

const bpvm_pack_sym_t bpvm_gui_stub_syms[BPVM_GUI_N_FUNCS] = {
    { "parent_alive", (bpvm_pack_fn_t) stub_parent_alive },
    { "screen_active", (bpvm_pack_fn_t) stub_screen_active },
    { "create_obj", (bpvm_pack_fn_t) stub_create_obj },
    { "create_label", (bpvm_pack_fn_t) stub_create_label },
    { "create_button", (bpvm_pack_fn_t) stub_create_button },
    { "create_checkbox", (bpvm_pack_fn_t) stub_create_checkbox },
    { "create_switch", (bpvm_pack_fn_t) stub_create_switch },
    { "create_slider", (bpvm_pack_fn_t) stub_create_slider },
    { "create_bar", (bpvm_pack_fn_t) stub_create_bar },
    { "create_spinbox", (bpvm_pack_fn_t) stub_create_spinbox },
    { "create_led", (bpvm_pack_fn_t) stub_create_led },
    { "create_dropdown", (bpvm_pack_fn_t) stub_create_dropdown },
    { "create_textarea", (bpvm_pack_fn_t) stub_create_textarea },
    { "set_options", (bpvm_pack_fn_t) stub_set_options },
    { "get_text", (bpvm_pack_fn_t) stub_get_text },
    { "create_list", (bpvm_pack_fn_t) stub_create_list },
    { "create_keyboard", (bpvm_pack_fn_t) stub_create_keyboard },
    { "keyboard_set_textarea", (bpvm_pack_fn_t) stub_keyboard_set_textarea },
    { "create_msgbox", (bpvm_pack_fn_t) stub_create_msgbox },
    { "set_buttons", (bpvm_pack_fn_t) stub_set_buttons },
    { "create_tabview", (bpvm_pack_fn_t) stub_create_tabview },
    { "tabview_add_tab", (bpvm_pack_fn_t) stub_tabview_add_tab },
    { "create_table", (bpvm_pack_fn_t) stub_create_table },
    { "create_chart", (bpvm_pack_fn_t) stub_create_chart },
    { "chart_set_points", (bpvm_pack_fn_t) stub_chart_set_points },
    { "chart_add_series", (bpvm_pack_fn_t) stub_chart_add_series },
    { "chart_push", (bpvm_pack_fn_t) stub_chart_push },
    { "chart_set_value", (bpvm_pack_fn_t) stub_chart_set_value },
    { "chart_set_type", (bpvm_pack_fn_t) stub_chart_set_type },
    { "table_set_grid", (bpvm_pack_fn_t) stub_table_set_grid },
    { "table_set_cell", (bpvm_pack_fn_t) stub_table_set_cell },
    { "table_get_cell", (bpvm_pack_fn_t) stub_table_get_cell },
    { "image_new", (bpvm_pack_fn_t) stub_image_new },
    { "image_load_file", (bpvm_pack_fn_t) stub_image_load_file },
    { "image_width", (bpvm_pack_fn_t) stub_image_width },
    { "image_height", (bpvm_pack_fn_t) stub_image_height },
    { "create_imageview", (bpvm_pack_fn_t) stub_create_imageview },
    { "imageview_set_image", (bpvm_pack_fn_t) stub_imageview_set_image },
    { "imageview_refresh", (bpvm_pack_fn_t) stub_imageview_refresh },
    { "set_font_size", (bpvm_pack_fn_t) stub_set_font_size },
    { "get_font_size", (bpvm_pack_fn_t) stub_get_font_size },
    { "load_font", (bpvm_pack_fn_t) stub_load_font },
    { "set_rotation", (bpvm_pack_fn_t) stub_set_rotation },
    { "textarea_set_readonly", (bpvm_pack_fn_t) stub_textarea_set_readonly },
    { "textarea_get_readonly", (bpvm_pack_fn_t) stub_textarea_get_readonly },
    { "set_text", (bpvm_pack_fn_t) stub_set_text },
    { "set_width", (bpvm_pack_fn_t) stub_set_width },
    { "set_height", (bpvm_pack_fn_t) stub_set_height },
    { "align", (bpvm_pack_fn_t) stub_align },
    { "set_x", (bpvm_pack_fn_t) stub_set_x },
    { "get_x", (bpvm_pack_fn_t) stub_get_x },
    { "set_y", (bpvm_pack_fn_t) stub_set_y },
    { "get_y", (bpvm_pack_fn_t) stub_get_y },
    { "get_width", (bpvm_pack_fn_t) stub_get_width },
    { "get_height", (bpvm_pack_fn_t) stub_get_height },
    { "set_scroll_dir", (bpvm_pack_fn_t) stub_set_scroll_dir },
    { "get_scroll_dir", (bpvm_pack_fn_t) stub_get_scroll_dir },
    { "get_align", (bpvm_pack_fn_t) stub_get_align },
    { "get_align_dx", (bpvm_pack_fn_t) stub_get_align_dx },
    { "get_align_dy", (bpvm_pack_fn_t) stub_get_align_dy },
    { "get_auth_width", (bpvm_pack_fn_t) stub_get_auth_width },
    { "get_auth_height", (bpvm_pack_fn_t) stub_get_auth_height },
    { "shot", (bpvm_pack_fn_t) stub_shot },
    { "refresh", (bpvm_pack_fn_t) stub_refresh },
    { "set_checked", (bpvm_pack_fn_t) stub_set_checked },
    { "get_checked", (bpvm_pack_fn_t) stub_get_checked },
    { "set_value", (bpvm_pack_fn_t) stub_set_value },
    { "get_value", (bpvm_pack_fn_t) stub_get_value },
    { "set_range", (bpvm_pack_fn_t) stub_set_range },
    { "set_bg_color", (bpvm_pack_fn_t) stub_set_bg_color },
    { "set_text_color", (bpvm_pack_fn_t) stub_set_text_color },
    { "set_font", (bpvm_pack_fn_t) stub_set_font },
    { "clean", (bpvm_pack_fn_t) stub_clean },
    { "delete", (bpvm_pack_fn_t) stub_delete },
    { "reset", (bpvm_pack_fn_t) stub_reset },
    { "bind_click", (bpvm_pack_fn_t) stub_bind_click },
    { "inject_click", (bpvm_pack_fn_t) stub_inject_click },
    { "inject_change", (bpvm_pack_fn_t) stub_inject_change },
    { "next_event", (bpvm_pack_fn_t) stub_next_event },
    { "visit_roots", (bpvm_pack_fn_t) stub_visit_roots },
    { "dump_tree", (bpvm_pack_fn_t) stub_dump_tree },
    { "lvgl_pump", (bpvm_pack_fn_t) stub_lvgl_pump },
    { "lvgl_window_open", (bpvm_pack_fn_t) stub_lvgl_window_open },
    { "disp_pump", (bpvm_pack_fn_t) stub_disp_pump },
    { "disp_is_open", (bpvm_pack_fn_t) stub_disp_is_open },
    { "disp_set_rotation", (bpvm_pack_fn_t) stub_disp_set_rotation },
    { "disp_native_size", (bpvm_pack_fn_t) stub_disp_native_size },
    { "dump_free", (bpvm_pack_fn_t) stub_dump_free },
};
const bpvm_pack_api_t bpvm_gui_api_stubs = { BPVM_GUI_MARCA, BPVM_GUI_API_VER, BPVM_GUI_N_FUNCS, bpvm_gui_stub_syms };
