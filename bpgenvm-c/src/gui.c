/*
 * gui.c — backend GUI (modelo de comportamiento) de la VM-C, V3 / H4.1.
 *
 * Espejo del GuiBackend de miVM. SÓLO el modelo: nodos con (type, text, w, h,
 * align, dx, dy, parent, objptr) + dump_tree byte-idéntico a miVM. Sin LVGL/SDL
 * (H4.2). Estado en estáticos: la VM-C de host es de proceso único por ejecución
 * (cada `bpgenvm-c file.mod` es un proceso fresco → estáticos a cero). Para el
 * modo daemon (runModule repetido) habría que resetear entre runs — TODO H4.x.
 */
#include "bpvm_gui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GUI_MAX_NODES 512

typedef struct {
    int         used;       /* 0 = libre/muerto (delete/clean) */
    int         handle;
    const char* type;       /* "screen" | "panel" | "label" | "button" */
    int         parent;     /* handle del padre (0 = raíz) */
    int         w, h;       /* -1 = auto (tamaño preferido) */
    int         align, dx, dy;
    char*       text;       /* malloc; NULL/"" = sin texto */
    uint32_t    objptr;     /* objeto BP dueño (bind_click), 0 = ninguno */
} gui_node;

static gui_node g_nodes[GUI_MAX_NODES];
static int      g_node_count = 0;        /* slots usados del array (orden de creación) */
static int      g_screen = 0;            /* handle de la pantalla raíz */
static int      g_next_handle = 1;

/* Tamaño por defecto de la pantalla; debe coincidir con miVM (screenW/screenH). */
#define GUI_SCREEN_W 480
#define GUI_SCREEN_H 320

/* Cola circular de clics sintéticos (objptrs). La drena GUI_RUN. */
static uint32_t g_clicks[GUI_MAX_NODES];
static int      g_click_head = 0, g_click_tail = 0;

static gui_node* node_for(int handle) {
    if (handle <= 0) return NULL;
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].handle == handle) return &g_nodes[i];
    return NULL;
}

static int create_node(const char* type, int parent) {
    if (g_node_count >= GUI_MAX_NODES) return 0;   /* overflow → handle inválido */
    gui_node* n = &g_nodes[g_node_count++];
    n->used = 1; n->handle = g_next_handle++; n->type = type; n->parent = parent;
    n->w = -1; n->h = -1; n->align = 0; n->dx = 0; n->dy = 0;
    n->text = NULL; n->objptr = 0;
    return n->handle;
}

int bpvm_gui_screen_active(void) {
    if (g_screen != 0) return g_screen;
    int h = create_node("screen", 0);
    gui_node* n = node_for(h);
    if (n) { n->w = GUI_SCREEN_W; n->h = GUI_SCREEN_H; }
    g_screen = h;
    return h;
}

int bpvm_gui_create_obj(int parent)    { return create_node("panel",  parent); }
int bpvm_gui_create_label(int parent)  { return create_node("label",  parent); }
int bpvm_gui_create_button(int parent) { return create_node("button", parent); }

void bpvm_gui_set_text(int handle, const char* s) {
    gui_node* n = node_for(handle); if (!n) return;
    free(n->text); n->text = NULL;
    if (s) {
        size_t L = strlen(s);
        n->text = (char*) malloc(L + 1);
        if (n->text) memcpy(n->text, s, L + 1);
    }
}
void bpvm_gui_set_width(int handle, int w)  { gui_node* n = node_for(handle); if (n) n->w = w; }
void bpvm_gui_set_height(int handle, int h) { gui_node* n = node_for(handle); if (n) n->h = h; }
void bpvm_gui_align(int handle, int a, int dx, int dy) {
    gui_node* n = node_for(handle); if (!n) return;
    n->align = a; n->dx = dx; n->dy = dy;
}
void bpvm_gui_bind_click(int handle, uint32_t objptr) {
    gui_node* n = node_for(handle); if (n) n->objptr = objptr;
}
void bpvm_gui_clean(int handle) {
    /* marca muertos los hijos directos (sus subárboles quedan inalcanzables en
     * el dump, igual que miVM: removeAll + children.clear()). */
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].parent == handle) g_nodes[i].used = 0;
}
void bpvm_gui_delete(int handle) {
    gui_node* n = node_for(handle); if (n) n->used = 0;
}

/* ---- Cola de clics ---- */
void bpvm_gui_inject_click(uint32_t objptr) {
    int nt = (g_click_tail + 1) % GUI_MAX_NODES;
    if (nt == g_click_head) return;   /* cola llena → descarta */
    g_clicks[g_click_tail] = objptr; g_click_tail = nt;
}
uint32_t bpvm_gui_next_click(void) {
    if (g_click_head == g_click_tail) return 0;   /* vacía */
    uint32_t v = g_clicks[g_click_head];
    g_click_head = (g_click_head + 1) % GUI_MAX_NODES;
    return v;
}

/* ---- dump_tree — DEBE ser byte-idéntico a GuiBackend.dumpTree de miVM ----
 *   <indent×depth: "  "><type>[ "<text>"] [<w>x<h> align=<a> +<dx>,<dy>]\n
 */
static void buf_append(char** buf, size_t* len, size_t* cap, const char* s, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t nc = *cap ? *cap : 256;
        while (nc < *len + n + 1) nc *= 2;
        *buf = (char*) realloc(*buf, nc);
        *cap = nc;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
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
    int k = snprintf(tmp, sizeof(tmp), " [%dx%d align=%d +%d,%d]\n",
                     n->w, n->h, n->align, n->dx, n->dy);
    if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    /* hijos en orden de creación (= orden del array) — igual que la lista
     * children de miVM, que se llena en el mismo orden. */
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
