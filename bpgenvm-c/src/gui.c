/*
 * gui.c — backend GUI de la VM-C, V3 / H4.
 *
 * H4.1: MODELO de comportamiento (espejo del GuiBackend de miVM). Cada widget BP
 * es un handle → nodo (type, text, w, h, align, dx, dy, parent, objptr). dump_tree
 * produce el MISMO texto byte-a-byte que miVM → paridad (sobre el árbol, no
 * píxeles). Es la espina dorsal; siempre presente con BPVM_GUI.
 *
 * H4.2: bajo BPVM_LVGL, además, cada op del modelo crea/actualiza su lv_obj
 * (render real con LVGL v9 + ventana SDL en host). El modelo sigue siendo la
 * fuente de verdad del dump_tree → la paridad NO depende de LVGL. Sin BPVM_LVGL
 * (build modelo-only / headless / arnés), todo lo de LVGL desaparece.
 */
#include "bpvm_gui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef BPVM_LVGL
#include "lvgl.h"
#include <stdint.h>     /* intptr_t (lvgl_click_cb) */
/* El backend de display (ventana/tick/cierre) lo provee la plataforma vía
 * bpvm_gui_disp_* — host = SDL (src/gui_display_sdl.c), micro = LTDC (port/).
 * Aquí no se toca SDL: el render de widgets es portable. */
#endif

#define GUI_MAX_NODES 512
#define GUI_SCREEN_W  480
#define GUI_SCREEN_H  320

typedef struct {
    int         used;       /* 0 = libre/muerto */
    int         handle;
    const char* type;       /* "screen" | "panel" | "label" | "button" */
    int         parent;     /* handle del padre (0 = raíz) */
    int         w, h;       /* -1 = auto */
    int         x, y;       /* posición explícita (cuando pos_set) */
    int         pos_set;    /* 1 → x,y mandan; 0 → align manda */
    int         align, dx, dy;
    int         scroll;     /* ScrollDir: 0=NONE 1=HOR 2=VER 3=BOTH (default NONE) */
    int         has_value;  /* 1 en value-widgets (checkbox, switch, slider, bar, ...) */
    int         value;      /* estado del value-widget (checkbox/switch: 0/1; slider/bar: entero) */
    int         rmin, rmax; /* rango de value-widgets enteros (clamp); default 0..100 */
    int         trows, tcols; /* table: dimensiones de la rejilla */
    char**      cells;      /* table: celdas row-major (trows*tcols), cada una malloc o NULL */
    char*       text;       /* malloc; NULL/"" = sin texto */
    uint32_t    objptr;     /* objeto BP dueño (bind_click), 0 = ninguno */
#ifdef BPVM_LVGL
    lv_obj_t*   lv;         /* widget LVGL asociado (NULL si aún sin crear) */
#endif
} gui_node;

static gui_node g_nodes[GUI_MAX_NODES];
static int      g_node_count = 0;
static int      g_screen = 0;
static int      g_next_handle = 1;

/* Cola circular de eventos {objptr, kind}: la alimentan __guiClick/__guiChange
 * (sintéticos) y, bajo LVGL, los callbacks reales; la drena el bombeo de GUI_RUN.
 * kind: 0=CLICK 1=CHANGE (paridad con GuiBackend.KIND_* de miVM). */
static uint32_t g_ev_obj[GUI_MAX_NODES];
static int      g_ev_kind[GUI_MAX_NODES];
static int      g_ev_head = 0, g_ev_tail = 0;

static gui_node* node_for(int handle) {
    if (handle <= 0) return NULL;
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].handle == handle) return &g_nodes[i];
    return NULL;
}

static int create_node(const char* type, int parent) {
    if (g_node_count >= GUI_MAX_NODES) return 0;
    gui_node* n = &g_nodes[g_node_count++];
    n->used = 1; n->handle = g_next_handle++; n->type = type; n->parent = parent;
    n->w = -1; n->h = -1; n->x = 0; n->y = 0; n->pos_set = 0;
    n->align = 0; n->dx = 0; n->dy = 0; n->scroll = 0;
    n->has_value = 0; n->value = 0; n->rmin = 0; n->rmax = 100;
    n->trows = 0; n->tcols = 0; n->cells = NULL;
    n->text = NULL; n->objptr = 0;
#ifdef BPVM_LVGL
    n->lv = NULL;
#endif
    return n->handle;
}

/* ===================== Render LVGL (solo BPVM_LVGL) ===================== */
#ifdef BPVM_LVGL

/* Nuestro Gui.Align (0..8) → constante LVGL (por NOMBRE; el orden numérico de
 * LV_ALIGN_* difiere del nuestro). */
static const lv_align_t ALIGN_MAP[9] = {
    LV_ALIGN_TOP_LEFT,    LV_ALIGN_TOP_MID,    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_LEFT_MID,    LV_ALIGN_CENTER,     LV_ALIGN_RIGHT_MID,
    LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_MID, LV_ALIGN_BOTTOM_RIGHT
};

static int g_lvgl_inited = 0;

static void lvgl_ensure_init(void) {
    if (g_lvgl_inited) return;
    g_lvgl_inited = 1;
    lv_init();                                       /* core LVGL (portable) */
    bpvm_gui_disp_init(GUI_SCREEN_W, GUI_SCREEN_H);  /* tick + display + input (plataforma) */
}

/* Callback de clic LVGL: encola el objptr del objeto BP dueño (user_data). NO
 * ejecuta BP — eso lo hace el worker en GUI_RUN (modelo de interrupción). */
static void lvgl_click_cb(lv_event_t* e) {
    uint32_t objptr = (uint32_t) (intptr_t) lv_event_get_user_data(e);
    bpvm_gui_inject_click(objptr);
}

/* Callback de cambio de valor (checkbox/slider/...): refleja el estado del
 * widget en el modelo (n->value) y encola un CHANGE. Solo la interacción del
 * usuario lo dispara; los cambios programáticos (lv_obj_add_state desde
 * set_checked) NO emiten VALUE_CHANGED en LVGL — igual que la supresión de
 * miVM, así onChange se comporta idéntico en las dos VMs. */
static void lvgl_change_cb(lv_event_t* e) {
    uint32_t objptr = (uint32_t) (intptr_t) lv_event_get_user_data(e);
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].objptr == objptr && g_nodes[i].lv) {
            if (strcmp(g_nodes[i].type, "slider") == 0)
                g_nodes[i].value = lv_slider_get_value(g_nodes[i].lv);
            else if (strcmp(g_nodes[i].type, "spinbox") == 0)
                g_nodes[i].value = lv_spinbox_get_value(g_nodes[i].lv);
            else if (strcmp(g_nodes[i].type, "dropdown") == 0)
                g_nodes[i].value = (int) lv_dropdown_get_selected(g_nodes[i].lv);
            else if (strcmp(g_nodes[i].type, "textarea") == 0) {
                const char* t = lv_textarea_get_text(g_nodes[i].lv);   /* refleja el contenido en el modelo */
                free(g_nodes[i].text); g_nodes[i].text = NULL;
                if (t) { size_t L = strlen(t); g_nodes[i].text = (char*) malloc(L + 1); if (g_nodes[i].text) memcpy(g_nodes[i].text, t, L + 1); }
            }
            else if (strcmp(g_nodes[i].type, "tabview") == 0)
                g_nodes[i].value = (int) lv_tabview_get_tab_active(g_nodes[i].lv);
            else  /* checkbox, toggle: estado CHECKED (0/1) */
                g_nodes[i].value = lv_obj_has_state(g_nodes[i].lv, LV_STATE_CHECKED) ? 1 : 0;
            break;
        }
    bpvm_gui_inject_change(objptr);
}

/* Click en un botón de un lv_list: fija el índice seleccionado en el modelo de la
 * LISTA (user_data = objptr de la lista) y encola un CHANGE sobre ella. */
static void lvgl_list_btn_cb(lv_event_t* e) {
    uint32_t list_objptr = (uint32_t) (intptr_t) lv_event_get_user_data(e);
    lv_obj_t* btn = (lv_obj_t*) lv_event_get_target(e);
    int idx = (int) lv_obj_get_index(btn);
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].objptr == list_objptr) { g_nodes[i].value = idx; break; }
    bpvm_gui_inject_change(list_objptr);
}

static lv_obj_t* parent_lv(int parent) {
    gui_node* p = node_for(parent);
    if (p && p->lv) return p->lv;
    return lv_screen_active();
}

void bpvm_gui_lvgl_pump(void)        { bpvm_gui_disp_pump(); }
int  bpvm_gui_lvgl_window_open(void) { return g_lvgl_inited && bpvm_gui_disp_is_open(); }

#endif /* BPVM_LVGL */

/* ===================== API del modelo (siempre) ======================= */

int bpvm_gui_screen_active(void) {
    if (g_screen != 0) return g_screen;
    int h = create_node("screen", 0);
    gui_node* n = node_for(h);
    if (n) { n->w = GUI_SCREEN_W; n->h = GUI_SCREEN_H; }
    g_screen = h;
#ifdef BPVM_LVGL
    lvgl_ensure_init();
    if (n) n->lv = lv_screen_active();
#endif
    return h;
}

int bpvm_gui_create_obj(int parent) {
    int h = create_node("panel", parent);
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_obj_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_label(int parent) {
    int h = create_node("label", parent);
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_label_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_button(int parent) {
    int h = create_node("button", parent);
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_button_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_checkbox(int parent) {
    int h = create_node("checkbox", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) n->lv = lv_checkbox_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_switch(int parent) {
    int h = create_node("toggle", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) n->lv = lv_switch_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_slider(int parent) {
    int h = create_node("slider", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) { n->lv = lv_slider_create(parent_lv(parent)); lv_slider_set_range(n->lv, n->rmin, n->rmax); }
#endif
    return h;
}
int bpvm_gui_create_bar(int parent) {
    int h = create_node("bar", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) { n->lv = lv_bar_create(parent_lv(parent)); lv_bar_set_range(n->lv, n->rmin, n->rmax); }
#endif
    return h;
}
int bpvm_gui_create_spinbox(int parent) {
    int h = create_node("spinbox", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) { n->lv = lv_spinbox_create(parent_lv(parent)); lv_spinbox_set_range(n->lv, n->rmin, n->rmax); }
#endif
    return h;
}
int bpvm_gui_create_led(int parent) {
    int h = create_node("led", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;
#ifdef BPVM_LVGL
    if (n) n->lv = lv_led_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_dropdown(int parent) {
    int h = create_node("dropdown", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;   /* value = índice seleccionado */
#ifdef BPVM_LVGL
    if (n) n->lv = lv_dropdown_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_textarea(int parent) {
    int h = create_node("textarea", parent);   /* texto editable: NO value-widget (sin val=) */
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_textarea_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_list(int parent) {
    int h = create_node("list", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;   /* value = índice seleccionado */
#ifdef BPVM_LVGL
    if (n) n->lv = lv_list_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_keyboard(int parent) {
    int h = create_node("keyboard", parent);   /* sin estado en el modelo (se ata a un textarea) */
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_keyboard_create(parent_lv(parent));
#endif
    return h;
}
void bpvm_gui_keyboard_set_textarea(int handle, int ta_handle) {
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle); gui_node* ta = node_for(ta_handle);
    if (n && n->lv && ta && ta->lv) lv_keyboard_set_textarea(n->lv, ta->lv);
#else
    (void) handle; (void) ta_handle;   /* render-only: el teclado físico edita el textarea */
#endif
}
int bpvm_gui_create_msgbox(int parent) {
    int h = create_node("msgbox", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;   /* value = botón pulsado */
#ifdef BPVM_LVGL
    if (n) n->lv = lv_msgbox_create(parent_lv(parent));
#endif
    return h;
}
int bpvm_gui_create_tabview(int parent) {
    int h = create_node("tabview", parent);
    gui_node* n = node_for(h); if (n) n->has_value = 1;   /* value = pestaña activa */
#ifdef BPVM_LVGL
    if (n) n->lv = lv_tabview_create(parent_lv(parent));
#endif
    return h;
}
/* añade una pestaña; la página es un nodo hijo (nombre en text) cuyo handle se
 * devuelve para envolverlo en un TabPage (BP) y usarlo como parent. */
int bpvm_gui_tabview_add_tab(int handle, const char* name) {
    int h = create_node("tabpage", handle);
    gui_node* n = node_for(h); if (!n) return h;
    if (name) { size_t L = strlen(name); n->text = (char*) malloc(L + 1); if (n->text) memcpy(n->text, name, L + 1); }
#ifdef BPVM_LVGL
    gui_node* tv = node_for(handle);
    if (tv && tv->lv) n->lv = lv_tabview_add_tab(tv->lv, name ? name : "");
#endif
    return h;
}
/* ---- Table — rejilla de celdas de texto (filas×columnas). ---- */
int bpvm_gui_create_table(int parent) {
    int h = create_node("table", parent);
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_table_create(parent_lv(parent));
#endif
    return h;
}
void bpvm_gui_table_set_grid(int handle, int rows, int cols) {
    gui_node* n = node_for(handle); if (!n) return;
    if (rows < 0) rows = 0;
    if (cols < 0) cols = 0;
    int total = rows * cols; if (total < 1) total = 1;
    char** nc = (char**) calloc((size_t) total, sizeof(char*));
    if (!nc) return;
    if (n->cells) {   /* conserva las celdas que sigan en rango y libera el resto */
        for (int r = 0; r < n->trows; r++) {
            for (int c = 0; c < n->tcols; c++) {
                char* old = n->cells[r * n->tcols + c];
                if (r < rows && c < cols) nc[r * cols + c] = old;
                else free(old);
            }
        }
        free(n->cells);
    }
    n->trows = rows; n->tcols = cols; n->cells = nc;
#ifdef BPVM_LVGL
    if (n->lv) { lv_table_set_row_cnt(n->lv, rows); lv_table_set_col_cnt(n->lv, cols); }
#endif
}
void bpvm_gui_table_set_cell(int handle, int row, int col, const char* text) {
    gui_node* n = node_for(handle); if (!n || !n->cells) return;
    if (row < 0 || row >= n->trows || col < 0 || col >= n->tcols) return;
    int idx = row * n->tcols + col;
    free(n->cells[idx]);
    size_t L = text ? strlen(text) : 0;
    n->cells[idx] = (char*) malloc(L + 1);
    if (n->cells[idx]) { if (text) memcpy(n->cells[idx], text, L); n->cells[idx][L] = '\0'; }
#ifdef BPVM_LVGL
    if (n->lv) lv_table_set_cell_value(n->lv, row, col, text ? text : "");
#endif
}
const char* bpvm_gui_table_get_cell(int handle, int row, int col) {
    gui_node* n = node_for(handle); if (!n || !n->cells) return "";
    if (row < 0 || row >= n->trows || col < 0 || col >= n->tcols) return "";
    const char* s = n->cells[row * n->tcols + col];
    return s ? s : "";
}
/* botones del msgbox \n-sep: render-only (LVGL crea el footer); en el modelo no-op. */
void bpvm_gui_set_buttons(int handle, const char* labels) {
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle); if (!n || !n->lv) return;
    if (strcmp(n->type, "msgbox") == 0 && labels && *labels) {
        const char* p = labels;
        while (*p) {
            const char* nl = strchr(p, '\n');
            size_t len = nl ? (size_t)(nl - p) : strlen(p);
            char lbl[64]; if (len >= sizeof(lbl)) len = sizeof(lbl) - 1;
            memcpy(lbl, p, len); lbl[len] = '\0';
            lv_obj_t* btn = lv_msgbox_add_footer_button(n->lv, lbl);
            lv_obj_add_event_cb(btn, lvgl_list_btn_cb, LV_EVENT_CLICKED, (void*) (intptr_t) n->objptr);
            if (!nl) break;
            p = nl + 1;
        }
    }
#else
    (void) handle; (void) labels;
#endif
}

void bpvm_gui_set_text(int handle, const char* s) {
    gui_node* n = node_for(handle); if (!n) return;
    free(n->text); n->text = NULL;
    if (s) { size_t L = strlen(s); n->text = (char*) malloc(L + 1); if (n->text) memcpy(n->text, s, L + 1); }
#ifdef BPVM_LVGL
    if (n->lv && strcmp(n->type, "label") == 0)    lv_label_set_text(n->lv, s ? s : "");
    if (n->lv && strcmp(n->type, "checkbox") == 0) lv_checkbox_set_text(n->lv, s ? s : "");
    if (n->lv && strcmp(n->type, "textarea") == 0) lv_textarea_set_text(n->lv, s ? s : "");
    if (n->lv && strcmp(n->type, "msgbox") == 0)   lv_msgbox_add_text(n->lv, s ? s : "");
#endif
}
/* dropdown: opciones \n-separadas (guardadas en n->text, igual que LVGL). */
void bpvm_gui_set_options(int handle, const char* opts) {
    gui_node* n = node_for(handle); if (!n) return;
    free(n->text); n->text = NULL;
    if (opts) { size_t L = strlen(opts); n->text = (char*) malloc(L + 1); if (n->text) memcpy(n->text, opts, L + 1); }
#ifdef BPVM_LVGL
    if (n->lv && strcmp(n->type, "dropdown") == 0) lv_dropdown_set_options(n->lv, opts ? opts : "");
    else if (n->lv && strcmp(n->type, "list") == 0) {
        lv_obj_clean(n->lv);   /* quita los botones previos */
        if (opts && *opts) {
            const char* p = opts;
            while (*p) {
                const char* nl = strchr(p, '\n');
                size_t len = nl ? (size_t)(nl - p) : strlen(p);
                char item[128]; if (len >= sizeof(item)) len = sizeof(item) - 1;
                memcpy(item, p, len); item[len] = '\0';
                lv_obj_t* btn = lv_list_add_button(n->lv, NULL, item);
                lv_obj_add_event_cb(btn, lvgl_list_btn_cb, LV_EVENT_CLICKED, (void*) (intptr_t) n->objptr);
                if (!nl) break;
                p = nl + 1;
            }
        }
    }
#endif
}
const char* bpvm_gui_get_text(int handle) {
    gui_node* n = node_for(handle); return (n && n->text) ? n->text : "";
}
void bpvm_gui_set_width(int handle, int w) {
    gui_node* n = node_for(handle); if (!n) return;
    n->w = w;
#ifdef BPVM_LVGL
    if (n->lv && w >= 0) lv_obj_set_width(n->lv, w);
#endif
}
void bpvm_gui_set_height(int handle, int h) {
    gui_node* n = node_for(handle); if (!n) return;
    n->h = h;
#ifdef BPVM_LVGL
    if (n->lv && h >= 0) lv_obj_set_height(n->lv, h);
#endif
}
void bpvm_gui_align(int handle, int a, int dx, int dy) {
    gui_node* n = node_for(handle); if (!n) return;
    n->align = a; n->dx = dx; n->dy = dy; n->pos_set = 0;
#ifdef BPVM_LVGL
    if (n->lv && a >= 0 && a < 9) lv_obj_align(n->lv, ALIGN_MAP[a], dx, dy);
#endif
}

/* H6 — geometría explícita (x,y) + scroll (opt-in) + refresh. El backend es la
 * verdad; el dump usa la geometría AUTORADA (pos explícita o align), nunca los
 * píxeles computados (que difieren por backend para tamaños auto). */
void bpvm_gui_set_x(int handle, int x) {
    gui_node* n = node_for(handle); if (!n) return;
    n->x = x; n->pos_set = 1;
#ifdef BPVM_LVGL
    if (n->lv) lv_obj_set_x(n->lv, x);
#endif
}
int bpvm_gui_get_x(int handle) {
    gui_node* n = node_for(handle); if (!n) return 0;
    if (n->pos_set) return n->x;
#ifdef BPVM_LVGL
    if (n->lv) return lv_obj_get_x(n->lv);
#endif
    return 0;
}
void bpvm_gui_set_y(int handle, int y) {
    gui_node* n = node_for(handle); if (!n) return;
    n->y = y; n->pos_set = 1;
#ifdef BPVM_LVGL
    if (n->lv) lv_obj_set_y(n->lv, y);
#endif
}
int bpvm_gui_get_y(int handle) {
    gui_node* n = node_for(handle); if (!n) return 0;
    if (n->pos_set) return n->y;
#ifdef BPVM_LVGL
    if (n->lv) return lv_obj_get_y(n->lv);
#endif
    return 0;
}
int bpvm_gui_get_width(int handle) {
    gui_node* n = node_for(handle); if (!n) return 0;
    if (n->w >= 0) return n->w;
#ifdef BPVM_LVGL
    if (n->lv) return lv_obj_get_width(n->lv);
#endif
    return 0;
}
int bpvm_gui_get_height(int handle) {
    gui_node* n = node_for(handle); if (!n) return 0;
    if (n->h >= 0) return n->h;
#ifdef BPVM_LVGL
    if (n->lv) return lv_obj_get_height(n->lv);
#endif
    return 0;
}
void bpvm_gui_set_scroll_dir(int handle, int dir) {
    gui_node* n = node_for(handle); if (!n) return;
    n->scroll = dir;
#ifdef BPVM_LVGL
    if (n->lv) {
        lv_dir_t d = (dir == 1) ? LV_DIR_HOR : (dir == 2) ? LV_DIR_VER
                   : (dir == 3) ? LV_DIR_ALL : LV_DIR_NONE;
        lv_obj_set_scroll_dir(n->lv, d);
        lv_obj_set_scrollbar_mode(n->lv, dir != 0 ? LV_SCROLLBAR_MODE_AUTO : LV_SCROLLBAR_MODE_OFF);
    }
#endif
}
int bpvm_gui_get_scroll_dir(int handle) {
    gui_node* n = node_for(handle); return n ? n->scroll : 0;
}
void bpvm_gui_refresh(int handle) {
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle); if (n && n->lv) lv_obj_invalidate(n->lv);
#else
    (void) handle;
#endif
}

/* H6 value-widgets (checkbox por ahora): el estado vive en n->value (modelo =
 * verdad). El set programático NO emite onChange (LVGL no dispara VALUE_CHANGED
 * en lv_obj_add_state) — paridad con la supresión de miVM. */
void bpvm_gui_set_checked(int handle, int v) {
    gui_node* n = node_for(handle); if (!n) return;
    n->value = v ? 1 : 0;
#ifdef BPVM_LVGL
    if (n->lv) {
        if (v) lv_obj_add_state(n->lv, LV_STATE_CHECKED);
        else   lv_obj_remove_state(n->lv, LV_STATE_CHECKED);
    }
#endif
}
int bpvm_gui_get_checked(int handle) {
    gui_node* n = node_for(handle); return n ? (n->value != 0) : 0;
}

/* Value-widgets enteros (slider/bar): n->value clampado a [rmin,rmax] (igual en
 * las 2 VMs → el dump coincide). set programático no emite onChange. */
void bpvm_gui_set_value(int handle, int v) {
    gui_node* n = node_for(handle); if (!n) return;
    int cv = v < n->rmin ? n->rmin : (v > n->rmax ? n->rmax : v);
    n->value = cv;
#ifdef BPVM_LVGL
    if (n->lv) {
        if (strcmp(n->type, "slider") == 0)       lv_slider_set_value(n->lv, cv, LV_ANIM_OFF);
        else if (strcmp(n->type, "bar") == 0)     lv_bar_set_value(n->lv, cv, LV_ANIM_OFF);
        else if (strcmp(n->type, "spinbox") == 0) lv_spinbox_set_value(n->lv, cv);
        else if (strcmp(n->type, "led") == 0)     { if (cv) lv_led_on(n->lv); else lv_led_off(n->lv); }
        else if (strcmp(n->type, "dropdown") == 0) lv_dropdown_set_selected(n->lv, (uint16_t) cv);
        else if (strcmp(n->type, "tabview") == 0)  lv_tabview_set_active(n->lv, (uint32_t) cv, LV_ANIM_OFF);
    }
#endif
}
int bpvm_gui_get_value(int handle) {
    gui_node* n = node_for(handle); return n ? n->value : 0;
}
void bpvm_gui_set_range(int handle, int mn, int mx) {
    gui_node* n = node_for(handle); if (!n) return;
    n->rmin = mn; n->rmax = mx;
    if (n->value < mn) n->value = mn; else if (n->value > mx) n->value = mx;   /* re-clampa */
#ifdef BPVM_LVGL
    if (n->lv) {
        if (strcmp(n->type, "slider") == 0) { lv_slider_set_range(n->lv, mn, mx); lv_slider_set_value(n->lv, n->value, LV_ANIM_OFF); }
        else if (strcmp(n->type, "bar") == 0) { lv_bar_set_range(n->lv, mn, mx); lv_bar_set_value(n->lv, n->value, LV_ANIM_OFF); }
        else if (strcmp(n->type, "spinbox") == 0) { lv_spinbox_set_range(n->lv, mn, mx); lv_spinbox_set_value(n->lv, n->value); }
    }
#endif
}

/* Color/fuente: NO afectan al dump (render-only) → no-op en modelo-only; bajo
 * LVGL aplican estilo al lv_obj. */
void bpvm_gui_set_bg_color(int handle, uint32_t rgb) {
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle);
    if (n && n->lv) {
        lv_obj_set_style_bg_color(n->lv, lv_color_hex(rgb & 0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(n->lv, LV_OPA_COVER, 0);
    }
#else
    (void) handle; (void) rgb;
#endif
}
void bpvm_gui_set_text_color(int handle, uint32_t rgb) {
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle);
    if (n && n->lv) lv_obj_set_style_text_color(n->lv, lv_color_hex(rgb & 0xFFFFFF), 0);
#else
    (void) handle; (void) rgb;
#endif
}
void bpvm_gui_set_font(int handle, int font_id) {
    /* Fuente por tamaño llegará en una 2ª tanda; no afecta al dump. */
    (void) handle; (void) font_id;
}

void bpvm_gui_bind_click(int handle, uint32_t objptr) {
    gui_node* n = node_for(handle); if (!n) return;
    n->objptr = objptr;
#ifdef BPVM_LVGL
    if (n->lv) {
        if (n->has_value || strcmp(n->type, "textarea") == 0)  /* value-widget o texto: CHANGE */
            lv_obj_add_event_cb(n->lv, lvgl_change_cb, LV_EVENT_VALUE_CHANGED, (void*) (intptr_t) objptr);
        else
            lv_obj_add_event_cb(n->lv, lvgl_click_cb, LV_EVENT_CLICKED, (void*) (intptr_t) objptr);
    }
#endif
}
void bpvm_gui_clean(int handle) {
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].parent == handle) g_nodes[i].used = 0;
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle); if (n && n->lv) lv_obj_clean(n->lv);
#endif
}
void bpvm_gui_delete(int handle) {
    gui_node* n = node_for(handle); if (!n) return;
    n->used = 0;
#ifdef BPVM_LVGL
    if (n->lv) { lv_obj_delete(n->lv); n->lv = NULL; }
#endif
}

/* ---- Cola de eventos {objptr, kind} ---- */
static void ev_push(uint32_t objptr, int kind) {
    int nt = (g_ev_tail + 1) % GUI_MAX_NODES;
    if (nt == g_ev_head) return;          /* cola llena: descarta (como offer() de miVM) */
    g_ev_obj[g_ev_tail] = objptr; g_ev_kind[g_ev_tail] = kind; g_ev_tail = nt;
}
void bpvm_gui_inject_click(uint32_t objptr)  { ev_push(objptr, 0); }
void bpvm_gui_inject_change(uint32_t objptr) { ev_push(objptr, 1); }
uint32_t bpvm_gui_next_event(int* kind_out) {
    if (g_ev_head == g_ev_tail) { if (kind_out) *kind_out = 0; return 0; }
    uint32_t v = g_ev_obj[g_ev_head];
    if (kind_out) *kind_out = g_ev_kind[g_ev_head];
    g_ev_head = (g_ev_head + 1) % GUI_MAX_NODES;
    return v;
}

/* ---- dump_tree — byte-idéntico a GuiBackend.dumpTree de miVM ---- */
static void buf_append(char** buf, size_t* len, size_t* cap, const char* s, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t nc = *cap ? *cap : 256;
        while (nc < *len + n + 1) nc *= 2;
        *buf = (char*) realloc(*buf, nc); *cap = nc;
    }
    memcpy(*buf + *len, s, n); *len += n; (*buf)[*len] = '\0';
}
static void dump_node(char** buf, size_t* len, size_t* cap, int handle, int depth) {
    gui_node* n = node_for(handle); if (!n) return;
    for (int i = 0; i < depth; i++) buf_append(buf, len, cap, "  ", 2);
    buf_append(buf, len, cap, n->type, strlen(n->type));
    if (n->text && n->text[0]) {
        buf_append(buf, len, cap, " \"", 2);
        buf_append(buf, len, cap, n->text, strlen(n->text));
        buf_append(buf, len, cap, "\"", 1);
    }
    char tmp[96];
    int k = snprintf(tmp, sizeof(tmp), " [%dx%d", n->w, n->h);
    if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    if (n->pos_set) k = snprintf(tmp, sizeof(tmp), " pos=%d,%d", n->x, n->y);
    else            k = snprintf(tmp, sizeof(tmp), " align=%d +%d,%d", n->align, n->dx, n->dy);
    if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    if (n->scroll != 0) {
        k = snprintf(tmp, sizeof(tmp), " scroll=%d", n->scroll);
        if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    }
    if (n->has_value) {
        k = snprintf(tmp, sizeof(tmp), " val=%d", n->value);
        if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    }
    if (n->cells) {   /* table: dimensiones + celdas row-major (|-sep) */
        k = snprintf(tmp, sizeof(tmp), " grid=%dx%d \"", n->trows, n->tcols);
        if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
        int total = n->trows * n->tcols;
        for (int i = 0; i < total; i++) {
            if (i > 0) buf_append(buf, len, cap, "|", 1);
            const char* cv = n->cells[i] ? n->cells[i] : "";
            buf_append(buf, len, cap, cv, strlen(cv));
        }
        buf_append(buf, len, cap, "\"", 1);
    }
    buf_append(buf, len, cap, "]\n", 2);
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].parent == handle)
            dump_node(buf, len, cap, g_nodes[i].handle, depth + 1);
}
size_t bpvm_gui_dump_tree(char** out) {
    char* buf = NULL; size_t len = 0, cap = 0;
    if (g_screen != 0) dump_node(&buf, &len, &cap, g_screen, 0);
    if (!buf) { buf = (char*) malloc(1); if (buf) buf[0] = '\0'; }
    *out = buf;
    return len;
}
