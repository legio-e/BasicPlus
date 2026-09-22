/* gui_pack_api.h — GENERADO por scripts/gen_gui_pack.py de include/bpvm_gui.h.
 * NO EDITAR: la cabecera es el origen; esto se regenera. V7/H6 P2.
 *
 * La TABLA con la que el pack de la GUI sirve a la VM (un puntero por función de
 * bpvm_gui.h que cruza la frontera) y los NOMBRES bajo los que las publica en su
 * bpvm_pack_api_t (marca 'LVGL'). Los trampolines de gui_pack.c la rellenan por
 * nombre en el primer uso; el pack la construye en gui_pack_publics.c. */
#ifndef GUI_PACK_API_H
#define GUI_PACK_API_H
#include <stddef.h>
#include <stdint.h>

#define BPVM_GUI_MARCA    0x4C56474Cu   /* 'LVGL' */
#define BPVM_GUI_API_VER  1u            /* sube si cambia la FIRMA de algo de abajo */
#define BPVM_GUI_N_FUNCS  88

typedef struct bpvm_gui_tabla {
    int (*parent_alive)(int parent);
    int (*screen_active)(void);
    int (*create_obj)(int parent);
    int (*create_label)(int parent);
    int (*create_button)(int parent);
    int (*create_checkbox)(int parent);
    int (*create_switch)(int parent);
    int (*create_slider)(int parent);
    int (*create_bar)(int parent);
    int (*create_spinbox)(int parent);
    int (*create_led)(int parent);
    int (*create_dropdown)(int parent);
    int (*create_textarea)(int parent);
    void (*set_options)(int handle, const char* opts);
    const char* (*get_text)(int handle);
    int (*create_list)(int parent);
    int (*create_keyboard)(int parent);
    void (*keyboard_set_textarea)(int handle, int ta_handle);
    int (*create_msgbox)(int parent);
    void (*set_buttons)(int handle, const char* labels);
    int (*create_tabview)(int parent);
    int (*tabview_add_tab)(int handle, const char* name);
    int (*create_table)(int parent);
    int (*create_chart)(int parent);
    void (*chart_set_points)(int handle, int n);
    int (*chart_add_series)(int handle, int rgb);
    void (*chart_push)(int handle, int series, int value);
    void (*chart_set_value)(int handle, int series, int idx, int value);
    void (*chart_set_type)(int handle, int type);
    void (*table_set_grid)(int handle, int rows, int cols);
    void (*table_set_cell)(int handle, int row, int col, const char* text);
    const char* (*table_get_cell)(int handle, int row, int col);
    int (*image_new)(void);
    int (*image_load_file)(int id, const char* path);
    int (*image_width)(int id);
    int (*image_height)(int id);
    int (*create_imageview)(int parent);
    void (*imageview_set_image)(int view, int img);
    void (*imageview_refresh)(int view);
    void (*set_font_size)(int handle, int px);
    int (*get_font_size)(int handle);
    int (*load_font)(const char* path);
    void (*set_rotation)(int deg);
    void (*textarea_set_readonly)(int handle, int ro);
    int (*textarea_get_readonly)(int handle);
    void (*set_text)(int handle, const char* s);
    void (*set_width)(int handle, int w);
    void (*set_height)(int handle, int h);
    void (*align)(int handle, int a, int dx, int dy);
    void (*set_x)(int handle, int x);
    int (*get_x)(int handle);
    void (*set_y)(int handle, int y);
    int (*get_y)(int handle);
    int (*get_width)(int handle);
    int (*get_height)(int handle);
    void (*set_scroll_dir)(int handle, int dir);
    int (*get_scroll_dir)(int handle);
    int (*get_align)(int handle);
    int (*get_align_dx)(int handle);
    int (*get_align_dy)(int handle);
    int (*get_auth_width)(int handle);
    int (*get_auth_height)(int handle);
    int (*shot)(const char* path);
    void (*refresh)(int handle);
    void (*set_checked)(int handle, int v);
    int (*get_checked)(int handle);
    void (*set_value)(int handle, int v);
    int (*get_value)(int handle);
    void (*set_range)(int handle, int mn, int mx);
    void (*set_bg_color)(int handle, uint32_t rgb);
    void (*set_text_color)(int handle, uint32_t rgb);
    void (*set_font)(int handle, int font_id);
    void (*clean)(int handle);
    void (*delete)(int handle);
    void (*reset)(void);
    void (*bind_click)(int handle, uint32_t objptr);
    void (*inject_click)(uint32_t objptr);
    void (*inject_change)(uint32_t objptr);
    uint32_t (*next_event)(int* kind_out);
    void (*visit_roots)(void (*visit)(void* ctx, uint32_t objptr), void* ctx);
    size_t (*dump_tree)(char** out);
    uint32_t (*lvgl_pump)(void);
    int (*lvgl_window_open)(void);
    uint32_t (*disp_pump)(void);
    int (*disp_is_open)(void);
    void (*disp_set_rotation)(int deg);
    int (*disp_native_size)(int* w, int* h);
    void (*dump_free)(char* p);   /* pack: libera lo que dump_tree reservó en SU arena */
} bpvm_gui_tabla_t;

/* En el MISMO orden que la struct: el resolvedor recorre las dos a la par. */
static const char* const BPVM_GUI_NOMBRES[BPVM_GUI_N_FUNCS] = {
    "parent_alive",
    "screen_active",
    "create_obj",
    "create_label",
    "create_button",
    "create_checkbox",
    "create_switch",
    "create_slider",
    "create_bar",
    "create_spinbox",
    "create_led",
    "create_dropdown",
    "create_textarea",
    "set_options",
    "get_text",
    "create_list",
    "create_keyboard",
    "keyboard_set_textarea",
    "create_msgbox",
    "set_buttons",
    "create_tabview",
    "tabview_add_tab",
    "create_table",
    "create_chart",
    "chart_set_points",
    "chart_add_series",
    "chart_push",
    "chart_set_value",
    "chart_set_type",
    "table_set_grid",
    "table_set_cell",
    "table_get_cell",
    "image_new",
    "image_load_file",
    "image_width",
    "image_height",
    "create_imageview",
    "imageview_set_image",
    "imageview_refresh",
    "set_font_size",
    "get_font_size",
    "load_font",
    "set_rotation",
    "textarea_set_readonly",
    "textarea_get_readonly",
    "set_text",
    "set_width",
    "set_height",
    "align",
    "set_x",
    "get_x",
    "set_y",
    "get_y",
    "get_width",
    "get_height",
    "set_scroll_dir",
    "get_scroll_dir",
    "get_align",
    "get_align_dx",
    "get_align_dy",
    "get_auth_width",
    "get_auth_height",
    "shot",
    "refresh",
    "set_checked",
    "get_checked",
    "set_value",
    "get_value",
    "set_range",
    "set_bg_color",
    "set_text_color",
    "set_font",
    "clean",
    "delete",
    "reset",
    "bind_click",
    "inject_click",
    "inject_change",
    "next_event",
    "visit_roots",
    "dump_tree",
    "lvgl_pump",
    "lvgl_window_open",
    "disp_pump",
    "disp_is_open",
    "disp_set_rotation",
    "disp_native_size",
    "dump_free",
};

#endif /* GUI_PACK_API_H */
