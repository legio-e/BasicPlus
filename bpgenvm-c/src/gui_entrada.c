/*
 * gui_entrada.c — V8/H2, paso 4 (10-nov): EL TECLADO Y EL RATÓN EN LA GUI (V8_IDEAS §E1, «El paso 4»). LVGL lee el teclado
 * y el ratón del SO él mismo, en el hilo de la GUI, como lee la táctil: sin un Keyboards.Keyboard ni un Mice.Mouse de BP
 * en medio (un clic es apretar y soltar, y el ratón del programa no avisa al soltar: LVGL pregunta).
 *
 *   - EL TECLADO: un dispositivo de LVGL de tipo teclado, que lee su lector del teclado del SO (bpvm_teclado_lector_*),
 *     con un GRUPO por defecto: cada control que se cree entra solo, el que tiene el foco recibe las teclas, Tab y
 *     Mayús+Tab pasan al siguiente y al anterior, y Enter pulsa el botón. El grupo se crea cuando hay un teclado
 *     conectado, con los controles que ya haya: sin teclado, LVGL no le da el foco a nadie (un campo de texto enseñaría
 *     su cursor desde el arranque) y la GUI se ve como siempre.
 *   - EL RATÓN: un segundo puntero, con su flecha (lv_indev_set_cursor), que lee su lector del ratón del SO
 *     (bpvm_raton_lector_*): dónde está y si el izquierdo está apretado (un clic corto, entre dos lecturas, no se
 *     pierde); y la rueda, al objeto que se desplace bajo la flecha. La flecha, sólo con un ratón conectado.
 *   - EN EL PC (`pc`), la táctil es el mismo ratón: con «Ratón simulado» conectado, la táctil se apaga y el cursor del
 *     sistema se esconde en la ventana; si no, cada clic contaría dos veces. En una placa son dos aparatos y van los dos.
 *
 * El ratón del SO da los puntos de la pantalla tal como se ve; LVGL quiere los del panel (los gira él, como los de la
 * táctil): a_panel deshace la cuenta de indev_pointer_proc (lv_indev.c). En el PC no hace nada: allí LVGL no gira, la
 * ventana gira con la pantalla (#813).
 *
 * Lo usa la ventana de SDL (gui_display_sdl.c: la VM-C y el micro simulado). EN LAS PLACAS (el paso 5, la pieza 3,
 * 12-nov), el pack de la GUI: el mismo fichero, que pant_lvgl.c arranca al crear la pantalla si la imagen publica
 * 'ENTR' (bpvm_entr.h); lo que aquí se le pide al teclado y al ratón del SO, allí lo sirve entrada_pack.c por esa tabla.
 * Y en la placa, además, el tamaño de la pantalla tal como se ve, al ratón del SO (pantalla_al_raton).
 */
#ifdef BPVM_LVGL
#include "gui_entrada.h"
#include "bpvm_teclado.h"
#include "bpvm_raton.h"
#include "bpvm_bt.h"
#include "bpvm_internal.h"   /* bpvm_diag */

#include <string.h>

static lv_indev_t* s_teclado = NULL;
static lv_indev_t* s_raton   = NULL;
static lv_indev_t* s_tactil  = NULL;
static lv_group_t* s_grupo   = NULL;
static lv_obj_t*   s_flecha_obj = NULL;
static void      (*s_cursor_sistema)(int visible) = NULL;
static int         s_pc = 0;
static int         s_lt = -1, s_lr = -1;   /* los lectores del teclado y del ratón del SO */
static int         s_con_raton = 0;        /* ¿se ve la flecha? */
static uint32_t    s_pend = 0;             /* la tecla de LVGL apretada en la lectura anterior: ésta la suelta */

/* ── La flecha: 12 × 19, con la punta arriba a la izquierda, que es donde está el ratón. X negro, . blanco. ─────────── */

#define FLECHA_W 12
#define FLECHA_H 19
static const char* const FLECHA[FLECHA_H] = {
    "X           ", "XX          ", "X.X         ", "X..X        ", "X...X       ", "X....X      ", "X.....X     ",
    "X......X    ", "X.......X   ", "X........X  ", "X.........X ", "X......XXXXX", "X...X..X    ", "X..XX..X    ",
    "X.X  X..X   ", "XX   X..X   ", "X     X..X  ", "      X..X  ", "       XX   ",
};
static uint32_t       s_flecha_px[FLECHA_W * FLECHA_H];   /* ARGB8888: 0xAARRGGBB */
static lv_image_dsc_t s_flecha;

static void prepara_flecha(void) {
    int x, y;
    for (y = 0; y < FLECHA_H; y++)
        for (x = 0; x < FLECHA_W; x++) {
            char c = FLECHA[y][x];
            s_flecha_px[y * FLECHA_W + x] = (c == 'X') ? 0xFF000000u : (c == '.') ? 0xFFFFFFFFu : 0x00000000u;
        }
    memset(&s_flecha, 0, sizeof s_flecha);
    s_flecha.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_flecha.header.cf = LV_COLOR_FORMAT_ARGB8888;
    s_flecha.header.w = FLECHA_W;
    s_flecha.header.h = FLECHA_H;
    s_flecha.header.stride = FLECHA_W * 4;
    s_flecha.data_size = sizeof s_flecha_px;
    s_flecha.data = (const uint8_t*) s_flecha_px;
}

/* ── El teclado ───────────────────────────────────────────────────────────────────────────────────────────────────── */

/* Un carácter, como lo quiere LVGL (lv_textarea_add_char): sus bytes UTF-8 en un entero, el primero en el de menos
 * peso. */
static uint32_t utf8_lvgl(int32_t cp) {
    uint8_t b[4] = { 0, 0, 0, 0 };
    if (cp < 0x80) return (uint32_t) cp;
    if (cp < 0x800) {
        b[0] = (uint8_t) (0xC0 | (cp >> 6));
        b[1] = (uint8_t) (0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        b[0] = (uint8_t) (0xE0 | (cp >> 12));
        b[1] = (uint8_t) (0x80 | ((cp >> 6) & 0x3F));
        b[2] = (uint8_t) (0x80 | (cp & 0x3F));
    } else {
        b[0] = (uint8_t) (0xF0 | (cp >> 18));
        b[1] = (uint8_t) (0x80 | ((cp >> 12) & 0x3F));
        b[2] = (uint8_t) (0x80 | ((cp >> 6) & 0x3F));
        b[3] = (uint8_t) (0x80 | (cp & 0x3F));
    }
    return (uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24);
}

/* Una tecla del SO (el int32: el código HID abajo, los bits de estado arriba; y su carácter), en una de LVGL; 0 si LVGL
 * no la usa (F1, Ctrl+S...: ésas, para scr.onKey). */
static uint32_t tecla_lvgl(int32_t k, int32_t cp) {
    int shift = (k & (int32_t) (BPVM_TECLA_SHIFT_IZQ | BPVM_TECLA_SHIFT_DER)) != 0;
    switch (k & (int32_t) BPVM_TECLA_CODIGO) {
        case 40: case 88: return LV_KEY_ENTER;              /* Enter, y el del teclado numérico */
        case 41: return LV_KEY_ESC;
        case 42: return LV_KEY_BACKSPACE;
        case 43: return shift ? LV_KEY_PREV : LV_KEY_NEXT;  /* Tab y Mayús+Tab: el control siguiente y el anterior */
        case 76: return LV_KEY_DEL;
        case 74: return LV_KEY_HOME;
        case 77: return LV_KEY_END;
        case 79: return LV_KEY_RIGHT;
        case 80: return LV_KEY_LEFT;
        case 81: return LV_KEY_DOWN;
        case 82: return LV_KEY_UP;
        default: break;
    }
    if (cp >= 32 && cp != 127) return utf8_lvgl(cp);       /* un carácter: al campo con el foco */
    return 0;
}

static lv_obj_tree_walk_res_t al_grupo(lv_obj_t* o, void* ud) {
    (void) ud;
    if (lv_obj_is_group_def(o)) lv_group_add_obj(s_grupo, o);
    return LV_OBJ_TREE_WALK_NEXT;
}

/* El grupo, la primera vez que hay un teclado conectado: por defecto (lo que se cree después entra solo) y con los
 * controles que ya haya en la pantalla. */
static void crea_grupo(void) {
    s_grupo = lv_group_create();
    lv_group_set_default(s_grupo);
    lv_indev_set_group(s_teclado, s_grupo);
    lv_obj_tree_walk(lv_screen_active(), al_grupo, NULL);
}

/* LVGL lo lee en cada pasada: cada tecla, apretada en una lectura y suelta en la siguiente, todas las que haya. */
static void leer_teclado(lv_indev_t* indev, lv_indev_data_t* data) {
    int32_t k, cp;
    (void) indev;
    if (!s_grupo && bpvm_bt_hay_teclado()) crea_grupo();
    if (s_pend) {
        data->key = s_pend;
        data->state = LV_INDEV_STATE_RELEASED;
        s_pend = 0;
        data->continue_reading = true;   /* y otra lectura, por si hay más */
        return;
    }
    while (bpvm_teclado_lector_lee(s_lt, &k, &cp)) {
        uint32_t lk = tecla_lvgl(k, cp);
        if (lk == 0) continue;
        data->key = lk;
        data->state = LV_INDEV_STATE_PRESSED;
        s_pend = lk;
        data->continue_reading = true;
        return;
    }
    data->state = LV_INDEV_STATE_RELEASED;   /* nada nuevo (la tecla, la última: la pone LVGL) */
}

/* ── El ratón ─────────────────────────────────────────────────────────────────────────────────────────────────────── */

/* Del punto de la pantalla tal como se ve al del panel: lo contrario de indev_pointer_proc (lv_indev.c), que lo vuelve a
 * girar, como el de la táctil. Su cuenta usa el tamaño del panel SIN girar (disp->hor_res, ver_res); los getters lo dan
 * girado (también el «physical», medido: con 90° los clics del ratón caían fuera del botón), así que se deshace. */
static void a_panel(int32_t* x, int32_t* y) {
    lv_display_t* d = lv_display_get_default();
    int32_t w, h, lx = *x, ly = *y;
    lv_display_rotation_t rot;
    if (!d) return;
    rot = lv_display_get_rotation(d);
    if (rot == LV_DISPLAY_ROTATION_90 || rot == LV_DISPLAY_ROTATION_270) {
        w = lv_display_get_vertical_resolution(d);
        h = lv_display_get_horizontal_resolution(d);
    } else {
        w = lv_display_get_horizontal_resolution(d);
        h = lv_display_get_vertical_resolution(d);
    }
    switch (rot) {
        case LV_DISPLAY_ROTATION_90:  *x = ly;         *y = h - lx - 1; break;
        case LV_DISPLAY_ROTATION_180: *x = w - lx - 1; *y = h - ly - 1; break;
        case LV_DISPLAY_ROTATION_270: *x = w - ly - 1; *y = lx;         break;
        default: break;
    }
}

/* La rueda: el primer objeto bajo la flecha, o un padre suyo, que se pueda desplazar hacia ese lado (+ hacia arriba). */
#define RUEDA_PASO 40
static void desplaza(int32_t x, int32_t y, int32_t muescas) {
    lv_point_t p;
    lv_obj_t* o;
    p.x = x;
    p.y = y;
    for (o = lv_indev_search_obj(lv_screen_active(), &p); o; o = lv_obj_get_parent(o)) {
        if (!lv_obj_has_flag(o, LV_OBJ_FLAG_SCROLLABLE) || !(lv_obj_get_scroll_dir(o) & LV_DIR_VER)) continue;
        if ((muescas > 0 && lv_obj_get_scroll_top(o) > 0) || (muescas < 0 && lv_obj_get_scroll_bottom(o) > 0)) {
            lv_obj_scroll_by_bounded(o, 0, muescas * RUEDA_PASO, LV_ANIM_OFF);
            return;
        }
    }
}

/* Se conecta o se desconecta un ratón: la flecha; y en el PC, la táctil y el cursor del sistema, al revés. */
static void cambia_raton(int con) {
    s_con_raton = con;
    if (s_flecha_obj) {
        if (con) lv_obj_remove_flag(s_flecha_obj, LV_OBJ_FLAG_HIDDEN);
        else     lv_obj_add_flag(s_flecha_obj, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_pc && s_tactil) lv_indev_enable(s_tactil, !con);
    if (s_cursor_sistema) s_cursor_sistema(!con);
}

/* V8/H2, paso 5 (la pieza 3, 12-nov) — EN UNA PLACA, el tamaño de la pantalla tal como se ve, al ratón del SO
 * (bpvm_raton_pantalla), para que no se salga: allí gira el driver y LVGL sólo cambia su resolución (pant_lvgl.c), así
 * que la de LVGL es la que se ve. Al empezar y cada vez que cambia (un giro). En el PC no: la ventana pone el punto ella
 * misma (bpvm_raton_pc). */
static void pantalla_al_raton(void) {
    static int32_t s_w = -1, s_h = -1;
    lv_display_t* d = lv_display_get_default();
    int32_t w, h;
    if (s_pc || !d) return;
    w = lv_display_get_horizontal_resolution(d);
    h = lv_display_get_vertical_resolution(d);
    if (w == s_w && h == s_h) return;
    s_w = w;
    s_h = h;
    bpvm_raton_pantalla(w, h);
}

static void leer_raton(lv_indev_t* indev, lv_indev_data_t* data) {
    int32_t x, y, muescas;
    int izq;
    int con = bpvm_bt_hay_raton();
    (void) indev;
    pantalla_al_raton();
    if (con != s_con_raton) cambia_raton(con);
    if (!con || !bpvm_raton_lector_lee(s_lr, &x, &y, &izq, &muescas)) {
        data->state = LV_INDEV_STATE_RELEASED;   /* sin ratón: suelto y donde estaba (el punto lo pone LVGL) */
        return;
    }
    if (muescas != 0) desplaza(x, y, muescas);
    a_panel(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state = izq ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void bpvm_gui_entrada_tactil(lv_indev_t* tactil) {
    lv_indev_t* i;
    lv_indev_t* primer_puntero = NULL;
    lv_indev_t* primer_teclado = NULL;
    s_tactil = tactil;
    if (s_con_raton && s_pc && s_tactil) lv_indev_enable(s_tactil, false);
    /* Lo que van a encontrar los manejadores de SDL de LVGL: el primero de cada tipo (gui_entrada.h). */
    for (i = lv_indev_get_next(NULL); i; i = lv_indev_get_next(i)) {
        if (!primer_puntero && lv_indev_get_type(i) == LV_INDEV_TYPE_POINTER) primer_puntero = i;
        if (!primer_teclado && lv_indev_get_type(i) == LV_INDEV_TYPE_KEYPAD) primer_teclado = i;
    }
    if (primer_puntero == s_raton || primer_teclado == s_teclado)
        bpvm_diag("[gui] AVISO: LVGL encuentra antes el %s de la GUI que el de SDL: el de SDL no recibira nada",
                  primer_puntero == s_raton ? "raton" : "teclado");
}

void bpvm_gui_entrada_init(int pc, void (*cursor_sistema)(int visible)) {
    s_pc = pc;
    s_cursor_sistema = cursor_sistema;
    s_lt = bpvm_teclado_lector_abre();
    s_lr = bpvm_raton_lector_abre();

    s_teclado = lv_indev_create();
    lv_indev_set_type(s_teclado, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_teclado, leer_teclado);

    prepara_flecha();
    s_raton = lv_indev_create();
    lv_indev_set_type(s_raton, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_raton, leer_raton);
    s_flecha_obj = lv_image_create(lv_layer_sys());
    lv_image_set_src(s_flecha_obj, &s_flecha);
    lv_obj_add_flag(s_flecha_obj, LV_OBJ_FLAG_HIDDEN);
    lv_indev_set_cursor(s_raton, s_flecha_obj);
    pantalla_al_raton();   /* en una placa: el ratón, dentro de esta pantalla desde ya */
}

#endif /* BPVM_LVGL */
