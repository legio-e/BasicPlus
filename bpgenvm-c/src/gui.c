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

/* Cola circular de clics (objptrs): la alimentan __guiClick (sintético) y, bajo
 * LVGL, el callback de clic real; la drena el bombeo de GUI_RUN. */
static uint32_t g_clicks[GUI_MAX_NODES];
static int      g_click_head = 0, g_click_tail = 0;

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

void bpvm_gui_set_text(int handle, const char* s) {
    gui_node* n = node_for(handle); if (!n) return;
    free(n->text); n->text = NULL;
    if (s) { size_t L = strlen(s); n->text = (char*) malloc(L + 1); if (n->text) memcpy(n->text, s, L + 1); }
#ifdef BPVM_LVGL
    if (n->lv && strcmp(n->type, "label") == 0) lv_label_set_text(n->lv, s ? s : "");
#endif
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
    if (n->lv)
        lv_obj_add_event_cb(n->lv, lvgl_click_cb, LV_EVENT_CLICKED, (void*) (intptr_t) objptr);
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

/* ---- Cola de clics ---- */
void bpvm_gui_inject_click(uint32_t objptr) {
    int nt = (g_click_tail + 1) % GUI_MAX_NODES;
    if (nt == g_click_head) return;
    g_clicks[g_click_tail] = objptr; g_click_tail = nt;
}
uint32_t bpvm_gui_next_click(void) {
    if (g_click_head == g_click_tail) return 0;
    uint32_t v = g_clicks[g_click_head];
    g_click_head = (g_click_head + 1) % GUI_MAX_NODES;
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
