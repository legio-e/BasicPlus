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
#include "bpvm_fs.h"     /* lectura de PNG portable (host libc + device RAM-FS) */
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

/* Aspecto lógico del screen del MODELO (paridad con screenW/H de miVM): setRotation
 * con 90/270 lo intercambia — funcione antes o después de crear el screen, igual que
 * GuiBackend.setRotation en la VM Java. */
static int g_screen_w0 = GUI_SCREEN_W;
static int g_screen_h0 = GUI_SCREEN_H;

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
    int         img_asset;  /* imageview: id del asset Image asignado (0 = ninguno) */
    int         rendered_version; /* imageview: versión del asset ya renderizada (version-stamp) */
    int         reloads;    /* imageview: nº de recargas reales (prueba la optimización) */
    int         font_size;  /* tamaño de fuente en px (0 = por defecto); el dump lo refleja */
    int         readonly;   /* textarea: solo lectura (sin cursor, no editable) */
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

/* Registro de assets Image (bitmap cargado de archivo). NO son nodos: viven
 * aparte y se comparten entre ImageView. En host el modelo solo guarda ruta +
 * dimensiones (del header PNG); el decode real (lodepng) es del device. */
#define GUI_MAX_IMAGES 64
typedef struct {
    int   used;
    char* path;     /* malloc; NULL/"" = sin ruta */
    int   w, h;     /* dimensiones intrínsecas del PNG (0 si no cargado) */
    int   loaded;
    int   version;  /* sube en cada load_file: el ImageView recarga si cambió */
#ifdef BPVM_LVGL
    uint8_t*       png_data;  /* fichero PNG completo en RAM (malloc) para el decoder */
    lv_image_dsc_t dsc;       /* descriptor LVGL (src VARIABLE) → lv_image_set_src */
#endif
} gui_image;
static gui_image g_images[GUI_MAX_IMAGES];
static int       g_image_count = 0;

static gui_image* image_for(int id) {
    if (id <= 0 || id > g_image_count) return NULL;
    gui_image* a = &g_images[id - 1];
    return a->used ? a : NULL;
}

static gui_node* node_for(int handle) {
    if (handle <= 0) return NULL;
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].used && g_nodes[i].handle == handle) return &g_nodes[i];
    return NULL;
}

/* ¿es `parent` un contenedor vivo? Un widget (todo lo que no sea la pantalla
 * raíz) se crea SIEMPRE bajo un contenedor existente; crear con un handle
 * 0/no-vivo es el error "widget sin contenedor" que lanza el builtin. La misma
 * regla y el mismo mensaje viven en miVM (GuiBackend) -> paridad. */
int bpvm_gui_parent_alive(int parent) {
    return node_for(parent) != NULL;
}

static int create_node(const char* type, int parent) {
    if (g_node_count >= GUI_MAX_NODES) return 0;
    gui_node* n = &g_nodes[g_node_count++];
    n->used = 1; n->handle = g_next_handle++; n->type = type; n->parent = parent;
    n->w = -1; n->h = -1; n->x = 0; n->y = 0; n->pos_set = 0;
    n->align = 0; n->dx = 0; n->dy = 0; n->scroll = 0;
    n->has_value = 0; n->value = 0; n->rmin = 0; n->rmax = 100;
    n->trows = 0; n->tcols = 0; n->cells = NULL;
    n->img_asset = 0; n->rendered_version = 0; n->reloads = 0;
    n->font_size = 0; n->readonly = 0;
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
    lv_lodepng_init();                               /* decoder PNG (image / Gui.Image) */
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
        if (g_nodes[i].used && g_nodes[i].objptr == list_objptr) {
            g_nodes[i].value = idx;
            /* msgbox: pulsar un botón del footer CIERRA el diálogo (UX estándar;
             * el onChange ya recibió 'result' = idx). La LISTA no se cierra: ahí
             * el clic es selección de ítem. close_async = borrado diferido, seguro
             * desde el propio callback del botón (que se destruye con el msgbox). */
            if (strcmp(g_nodes[i].type, "msgbox") == 0 && g_nodes[i].lv) {
                lv_msgbox_close_async(g_nodes[i].lv);
                g_nodes[i].lv = NULL;
            }
            break;
        }
    bpvm_gui_inject_change(list_objptr);
}

static lv_obj_t* parent_lv(int parent) {
    gui_node* p = node_for(parent);
    if (p && p->lv) return p->lv;
    return lv_screen_active();
}

/* Mapea px → fuente Montserrat disponible (catálogo de lv_conf.h). Sin match
 * exacto → la default (14). Mantener sincronizado con LV_FONT_MONTSERRAT_* del conf. */
static const lv_font_t* font_for_px(int px) {
    switch (px) {
        case 12: return &lv_font_montserrat_12;
        case 14: return &lv_font_montserrat_14;
        case 16: return &lv_font_montserrat_16;
        case 18: return &lv_font_montserrat_18;
        case 20: return &lv_font_montserrat_20;
        case 24: return &lv_font_montserrat_24;
        case 28: return &lv_font_montserrat_28;
        case 32: return &lv_font_montserrat_32;
        case 36: return &lv_font_montserrat_36;
        case 40: return &lv_font_montserrat_40;
        case 48: return &lv_font_montserrat_48;
        default: return &lv_font_montserrat_14;
    }
}

void bpvm_gui_lvgl_pump(void)        { bpvm_gui_disp_pump(); }
int  bpvm_gui_lvgl_window_open(void) { return g_lvgl_inited && bpvm_gui_disp_is_open(); }

#endif /* BPVM_LVGL */

/* ===================== API del modelo (siempre) ======================= */

int bpvm_gui_screen_active(void) {
    if (g_screen != 0) return g_screen;
    int h = create_node("screen", 0);
    gui_node* n = node_for(h);
    if (n) { n->w = g_screen_w0; n->h = g_screen_h0; }
    g_screen = h;
#ifdef BPVM_LVGL
    lvgl_ensure_init();
    if (n) n->lv = lv_screen_active();
#endif
    return h;
}

/* Orientación del display: 0/90/180/270 (grados); otros valores se IGNORAN (misma
 * regla que miVM → paridad). El giro real lo hace el display (costura disp_set_rotation:
 * en el P4-ws lo implementa su flush, en el host SDL el propio driver LVGL; el táctil se
 * transforma solo en lv_indev). El MODELO solo refleja el aspecto en el screen. */
static int g_rotation = 0;

void bpvm_gui_set_rotation(int deg) {
    if (deg != 0 && deg != 90 && deg != 180 && deg != 270) return;
    int was90 = (g_rotation == 90 || g_rotation == 270);
    int is90  = (deg == 90  || deg == 270);
    g_rotation = deg;
    if (was90 != is90) {
        int t = g_screen_w0; g_screen_w0 = g_screen_h0; g_screen_h0 = t;
        gui_node* n = (g_screen != 0) ? node_for(g_screen) : NULL;
        if (n) { n->w = g_screen_w0; n->h = g_screen_h0; }
    }
#ifdef BPVM_LVGL
    lvgl_ensure_init();                  /* rotar implica display: init si aún no lo está */
    bpvm_gui_disp_set_rotation(deg);
#endif
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
    if (n) {
        n->lv = lv_led_create(parent_lv(parent));
        /* lv_led pinta un "glow" (shadow escalado por el brillo, ver lv_led.c);
         * shadow_width=0 en el estilo lo anula (0*brillo=0) → LED de borde nítido. */
        lv_obj_set_style_shadow_width(n->lv, 0, 0);
    }
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
/* ---- Image — asset (bitmap) separado del control que lo muestra. ---- */
int bpvm_gui_image_new(void) {
    if (g_image_count >= GUI_MAX_IMAGES) return 0;
    int id = g_image_count + 1;          /* id 1-based = índice+1 */
    gui_image* a = &g_images[g_image_count++];
    a->used = 1; a->path = NULL; a->w = 0; a->h = 0; a->loaded = 0; a->version = 0;
#ifdef BPVM_LVGL
    a->png_data = NULL;
    lv_memzero(&a->dsc, sizeof(a->dsc));
#endif
    return id;
}
/* Carga un PNG: saca width/height del header IHDR (sin decoder). Devuelve 1 si
 * la cabecera PNG es válida, 0 si no (no encontrado / no es PNG). */
int bpvm_gui_image_load_file(int id, const char* path) {
    gui_image* a = image_for(id); if (!a) return 0;
    free(a->path);
    size_t L = path ? strlen(path) : 0;
    a->path = (char*) malloc(L + 1);
    if (a->path) { if (path) memcpy(a->path, path, L); a->path[L] = '\0'; }
    a->w = 0; a->h = 0; a->loaded = 0;
    /* H19-F1 — resuelve relativo al base-dir del proyecto (si lo hay), luego
     * cwd/literal, luego /app (modo plano). Los absolutos no se tocan. El dump
     * guarda la ruta pedida (path), no la resuelta. */
    char alt[300];
    const char* rpath = bpvm_fs_resolve(path ? path : "", alt, sizeof(alt));
    /* Lee el header por el FS facade (portable): en host = libc, en placa = RAM-FS.
     * Solo el IHDR (24 bytes) para sacar dimensiones; el decode de píxeles (lodepng)
     * es aparte. */
    unsigned char b[24];
    long n = bpvm_fs_read(rpath, b, sizeof(b));
    if (n >= 24
        && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G'
        && b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A) {
        a->w = (int) (((uint32_t) b[16] << 24) | ((uint32_t) b[17] << 16)
                    | ((uint32_t) b[18] << 8)  |  (uint32_t) b[19]);
        a->h = (int) (((uint32_t) b[20] << 24) | ((uint32_t) b[21] << 16)
                    | ((uint32_t) b[22] << 8)  |  (uint32_t) b[23]);
        a->loaded = 1;
    }
#ifdef BPVM_LVGL
    /* Para renderizar: lee el PNG ENTERO a RAM y monta un lv_image_dsc_t (src
     * VARIABLE) que el decoder lodepng decodifica al asignarlo con set_src. */
    free(a->png_data); a->png_data = NULL;
    lv_memzero(&a->dsc, sizeof(a->dsc));
    if (a->loaded) {
        uint32_t sz = 0;
        if (bpvm_fs_stat(rpath, &sz) == 0 && sz > 0
            && (a->png_data = (uint8_t*) malloc(sz)) != NULL
            && bpvm_fs_read(rpath, a->png_data, sz) == (long) sz) {
            a->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            a->dsc.header.cf    = LV_COLOR_FORMAT_RAW;   /* encoded: el decoder resuelve el formato real */
            a->dsc.header.w     = (uint32_t) a->w;
            a->dsc.header.h     = (uint32_t) a->h;
            a->dsc.data         = a->png_data;
            a->dsc.data_size    = sz;
        } else {
            free(a->png_data); a->png_data = NULL;
            a->loaded = 0;   /* no se pudo leer entero → no renderizable */
        }
    }
#endif
    a->version++;   /* (re)carga: el asset cambió → los ImageView lo recargarán */
    return a->loaded;
}
int bpvm_gui_image_width(int id)  { gui_image* a = image_for(id); return a ? a->w : 0; }
int bpvm_gui_image_height(int id) { gui_image* a = image_for(id); return a ? a->h : 0; }
int bpvm_gui_create_imageview(int parent) {
    int h = create_node("imageview", parent);
#ifdef BPVM_LVGL
    gui_node* n = node_for(h); if (n) n->lv = lv_image_create(parent_lv(parent));
#endif
    return h;
}
void bpvm_gui_imageview_set_image(int view, int img) {
    gui_node* n = node_for(view); if (!n) return;
    if (n->img_asset != img) {   /* cambia de imagen → forzar recarga en el próximo refresh */
        n->img_asset = img;
        n->rendered_version = 0;
    }
    /* El src real LVGL (lv_image_set_src con un lv_image_dsc_t decodificado por
     * lodepng) es trabajo del lote de dispositivo; en host el modelo (img_asset)
     * es la verdad del dump. */
}
/* refresh: recarga solo si el asset cambió desde la última vez (version-stamp).
 * Si no cambió, no hace nada (no re-decodifica). reloads cuenta las recargas
 * reales — es el observable que prueba la optimización en el dump. */
void bpvm_gui_imageview_refresh(int view) {
    gui_node* n = node_for(view); if (!n) return;
    gui_image* a = image_for(n->img_asset);
    int cur = a ? a->version : 0;
    if (cur != n->rendered_version) {
        n->rendered_version = cur;
        n->reloads++;
#ifdef BPVM_LVGL
        /* recarga real: el decoder lodepng decodifica el PNG del descriptor. */
        if (n->lv && a && a->loaded) lv_image_set_src(n->lv, &a->dsc);
#endif
    }
}
/* ---- Fuente: tamaño de texto por componente (catálogo). El modelo guarda el px
 *      pedido (lo refleja el dump); bajo LVGL aplica la fuente del catálogo. ---- */
void bpvm_gui_set_font_size(int handle, int px) {
    gui_node* n = node_for(handle); if (!n) return;
    n->font_size = px;
#ifdef BPVM_LVGL
    if (n->lv && px > 0) lv_obj_set_style_text_font(n->lv, font_for_px(px), 0);
#endif
}
int bpvm_gui_get_font_size(int handle) {
    gui_node* n = node_for(handle);
    return n ? n->font_size : 0;
}
/* ---- Textarea read-only: sin cursor, no editable (display de solo lectura). ---- */
void bpvm_gui_textarea_set_readonly(int handle, int ro) {
    gui_node* n = node_for(handle); if (!n) return;
    n->readonly = ro ? 1 : 0;
#ifdef BPVM_LVGL
    if (n->lv) {
        if (n->readonly) {
            lv_obj_remove_flag(n->lv, LV_OBJ_FLAG_CLICKABLE);          /* no foco → no edita */
            lv_obj_set_style_opa(n->lv, LV_OPA_TRANSP, LV_PART_CURSOR); /* oculta el cursor */
        } else {
            lv_obj_add_flag(n->lv, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_opa(n->lv, LV_OPA_COVER, LV_PART_CURSOR);
        }
    }
#endif
}
int bpvm_gui_textarea_get_readonly(int handle) {
    gui_node* n = node_for(handle);
    return n ? n->readonly : 0;
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
/* ---- Fuentes cargadas en runtime (.bin LVGL binfont, vía loadFont). El id
 *      (1-based) se asigna SIEMPRE (con o sin LVGL) para que la VM-C y la miVM
 *      devuelvan ids idénticos (paridad dual-VM). Solo bajo LVGL se materializa el
 *      lv_font_t; sin LVGL es un contador. No se resetea entre runs, igual que
 *      g_node_count/g_image_count (estado de proceso). ---- */
#define GUI_MAX_FONTS 32
static int g_font_count = 0;
#ifdef BPVM_LVGL
static lv_font_t* g_loaded_fonts[GUI_MAX_FONTS];
#endif

int bpvm_gui_load_font(const char* path) {
    if (g_font_count >= GUI_MAX_FONTS) return 0;      /* registro lleno */
    int id = ++g_font_count;                           /* 1-based; idéntico en ambas VMs */
#ifdef BPVM_LVGL
    g_loaded_fonts[id - 1] = NULL;
    /* H19-F1 — resuelve relativo al base-dir del proyecto (si lo hay), luego
     * cwd/literal, luego /app (modo plano). Los absolutos no se tocan. */
    char alt[300];
    const char* rpath = bpvm_fs_resolve(path ? path : "", alt, sizeof(alt));
    uint32_t sz = 0;
    if (bpvm_fs_stat(rpath, &sz) == 0 && sz > 0) {
        uint8_t* buf = (uint8_t*) malloc(sz);
        if (buf && bpvm_fs_read(rpath, buf, sz) == (long) sz)
            g_loaded_fonts[id - 1] = lv_binfont_create_from_buffer(buf, sz);
        free(buf);     /* el lv_font_t copia lo que necesita; el buffer ya no hace falta */
    }
    if (!g_loaded_fonts[id - 1]) {
        printf("[gui] loadFont('%s'): no se pudo cargar (id %d queda sin fuente)\n",
               path ? path : "", id);
        fflush(stdout);
    }
#else
    (void) path;
#endif
    return id;
}

void bpvm_gui_set_font(int handle, int font_id) {
    /* font_id = id devuelto por loadFont (1-based). Aplica la fuente al render del
     * componente; NO afecta al dump (paridad con la miVM). id fuera de rango o sin
     * fuente cargada = no-op. */
#ifdef BPVM_LVGL
    gui_node* n = node_for(handle);
    if (n && n->lv && font_id >= 1 && font_id <= g_font_count) {
        lv_font_t* f = g_loaded_fonts[font_id - 1];
        if (f) lv_obj_set_style_text_font(n->lv, f, 0);
    }
#else
    (void) handle; (void) font_id;
#endif
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
    if (strcmp(n->type, "imageview") == 0) {   /* imageview: asset asignado (ruta + dims) + recargas */
        if (n->img_asset != 0) {
            gui_image* a = image_for(n->img_asset);
            const char* p = (a && a->path) ? a->path : "";
            int iw = a ? a->w : 0, ih = a ? a->h : 0;
            buf_append(buf, len, cap, " img=\"", 6);
            buf_append(buf, len, cap, p, strlen(p));
            k = snprintf(tmp, sizeof(tmp), "\" %dx%d", iw, ih);
            if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
        } else {
            buf_append(buf, len, cap, " img=<none>", 11);
        }
        k = snprintf(tmp, sizeof(tmp), " reloads=%d", n->reloads);
        if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    }
    if (n->font_size != 0) {
        k = snprintf(tmp, sizeof(tmp), " font=%d", n->font_size);
        if (k > 0) buf_append(buf, len, cap, tmp, (size_t) k);
    }
    if (n->readonly) buf_append(buf, len, cap, " ro", 3);
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
