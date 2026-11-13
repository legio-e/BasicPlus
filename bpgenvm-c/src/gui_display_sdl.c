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
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include "bpvm_out.h"     /* V8 #755: BPVM_EOL, el salto de la consola del PC */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "bpvm_gui.h"
#include "bpvm_gui_host.h"   /* V7 #646: los verbos de la ventana, sólo del PC */
#include "bpvm_teclado.h"    /* V8/H2, paso 2: las teclas de la ventana, al teclado del SO */
#include "bpvm_raton.h"      /* V8/H2, paso 3: y su ratón, al ratón del SO */
#include "gui_entrada.h"     /* V8/H2, paso 4: el teclado y el ratón del SO, en LVGL */
#include "crc32.h"

static volatile int g_window_closed = 0;
static volatile int g_shot_requested = 0;   /* F12 → captura de pantalla (docs) */

/* V8/H2, paso 3 — el ratón de la ventana: los botones, como quedan (los bits HID), para el ratón del SO. */
static uint32_t g_raton_botones = 0;

/* V7/H12, LVGL con dos runs — LA VENTANA, SIEMPRE DESDE EL HILO QUE LA CREÓ. LVGL lee los eventos de SDL dentro de
 * lv_timer_handler, y en Windows la cola de mensajes es del hilo que creó la ventana: bombeada desde otro, deja de
 * responder. Con dos runs la GUI sólo la toca el run 0 (interp.c, el viaje); si algo la bombea desde otro hilo, que lo
 * diga (la primera vez) y lo cuente. */
static SDL_threadID  g_hilo_ventana   = 0;
static unsigned long g_bombeos_ajenos = 0;

/* Disparo NO interactivo de la captura: BPVM_GUI_SHOT_MS=<ms>.
 *
 * F12 exige un humano delante, y eso deja fuera dos casos que importan: que la
 * captura entre en una batería de tests, y que pueda mirarla quien depura sin
 * tener las manos en el teclado (p.ej. Claude, que ve PNG). Con la variable de
 * entorno: espera <ms> desde el primer pump, captura, y CIERRA — así el proceso
 * termina solo y se puede lanzar desde un script sin que se quede colgado
 * esperando a que alguien cierre la ventana.
 *
 * Va por entorno y no por flag de la CLI a propósito: test/main.c no sabe nada
 * del GUI (ni una guarda BPVM_LVGL), así que un flag obligaría a acoplarlos. El
 * backend se lee su propia variable y nadie más se entera. Funciona además se
 * lance como se lance (incluido desde el IDE).
 */
static int      g_shot_after_ms  = -1;      /* -1 = off */
static uint32_t g_shot_deadline  = 0;
static int      g_shot_env_read  = 0;

/* --- Guion de entrada: BPVM_GUI_SCRIPT=<fichero> --------------------------
 * Da MANOS a quien ya tiene ojos (la captura PNG). Un comando por linea:
 *
 *   wait <ms>       espera
 *   click <x> <y>   clic izquierdo en coordenadas de PANTALLA
 *   teclea <texto>  V8/H2, paso 2: el texto, tecla a tecla (pulsar, su carácter, soltar), como un teclado de EE. UU.:
 *                   las letras, los números y el espacio con su tecla; lo demás de ASCII, sólo su carácter
 *   tecla <código>  una tecla por su código HID, sin carácter (Enter es 40, Escape 41, la flecha arriba 82)
 *   mueve <x> <y>   V8/H2, paso 3: el ratón, a ese punto de la ventana (con los botones que tenga apretados el guion)
 *   pulsa <x> <y> [b]   aprieta un botón ahí, sin soltarlo: b, el número de SDL (1 izquierdo, el de por defecto; 2
 *                   central; 3 derecho; 4 y 5, los laterales). Para que LVGL lo vea como un arrastre, un `wait` antes de
 *                   moverlo: LVGL lee el ratón una vez por pasada
 *   suelta <x> <y> [b]  lo suelta ahí
 *   rueda <d>       la rueda, d muescas (+ hacia arriba), donde esté el ratón
 *   shot            captura PNG (bp_shot_NNNN.png)
 *   quit            cierra la ventana (termina el programa)
 *
 * Lineas vacias y las que empiezan por '#' se ignoran.
 *
 * El clic se inyecta como EVENTO SDL de raton, NO llamando a bpvm_gui_inject_click:
 * asi entra por el camino de verdad —indev de LVGL, su hit-testing, su callback—
 * y no aparece una ruta paralela que se pueda pudrir sin que nadie se entere.
 * LVGL filtra los eventos por windowID (lv_sdl_mouse_handler), de ahi g_win_id.
 *
 * Por que un guion y no el wire: el wire es binario v1 y lo comparten el IDE y
 * los 3 firmwares, asi que tocarle el protocolo pide recompilarlos y verificar
 * en placa. El guion no toca nada de eso, es DETERMINISTA (vale para una bateria
 * de golden images) y se prueba entero en host. El wire es el paso 2 — y es el
 * que dara ojos+manos sobre la PLACA, que es donde de verdad hacen falta.
 */
static uint32_t g_win_id         = 0;   /* ventana SDL de LVGL (para inyectar) */
static char*    g_script_buf     = NULL;
static char**   g_script         = NULL;
static int      g_script_n       = 0;
static int      g_script_i       = 0;
static uint32_t g_script_gate    = 0;   /* no avanzar hasta este tick */
static uint32_t g_click_up_at    = 0;   /* soltar el boton en este tick */
static int      g_click_x = 0, g_click_y = 0;

static void mouse_event(uint32_t type, int x, int y) {
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = type;
    if (type == SDL_MOUSEMOTION) {
        e.motion.windowID = g_win_id; e.motion.x = x; e.motion.y = y;
        e.motion.state = (type == SDL_MOUSEMOTION) ? 0 : SDL_BUTTON_LMASK;
    } else {
        e.button.windowID = g_win_id; e.button.x = x; e.button.y = y;
        e.button.button = SDL_BUTTON_LEFT;
        e.button.state  = (type == SDL_MOUSEBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
        e.button.clicks = 1;
    }
    SDL_PushEvent(&e);
}

/* V8/H2, paso 2 — una tecla del guion, como la manda SDL: pulsarla, su carácter (si lo da) y soltarla. Entra por el mismo
 * vigilante que las de verdad (lvgl_watch), y de ahí al teclado del SO, si hay un teclado conectado. */
static void guion_tecla(int sc, unsigned mod, const char* texto) {
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_KEYDOWN;
    e.key.windowID = g_win_id;
    e.key.state = SDL_PRESSED;
    e.key.keysym.scancode = (SDL_Scancode) sc;
    e.key.keysym.sym = SDL_GetKeyFromScancode((SDL_Scancode) sc);
    e.key.keysym.mod = (Uint16) mod;
    SDL_PushEvent(&e);
    if (texto && *texto) {
        SDL_memset(&e, 0, sizeof(e));
        e.type = SDL_TEXTINPUT;
        e.text.windowID = g_win_id;
        snprintf(e.text.text, sizeof(e.text.text), "%s", texto);
        SDL_PushEvent(&e);
    }
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_KEYUP;
    e.key.windowID = g_win_id;
    e.key.state = SDL_RELEASED;
    e.key.keysym.scancode = (SDL_Scancode) sc;
    e.key.keysym.sym = SDL_GetKeyFromScancode((SDL_Scancode) sc);
    e.key.keysym.mod = (Uint16) mod;
    SDL_PushEvent(&e);
}

static void guion_teclea(const char* s) {
    for (; *s; s++) {
        unsigned char c = (unsigned char) *s;
        char t[2];
        int sc = 0;
        unsigned mod = 0;
        if (c >= 0x80) continue;   /* sólo ASCII */
        t[0] = (char) c; t[1] = '\0';
        if (c >= 'a' && c <= 'z') sc = 4 + (c - 'a');
        else if (c >= 'A' && c <= 'Z') { sc = 4 + (c - 'A'); mod = KMOD_LSHIFT; }
        else if (c >= '1' && c <= '9') sc = 30 + (c - '1');
        else if (c == '0') sc = 39;
        else if (c == ' ') sc = 44;
        if (sc != 0) { guion_tecla(sc, mod, t); continue; }
        {   /* sin tecla en el guion: sólo su carácter, como uno que compone el sistema */
            SDL_Event e;
            SDL_memset(&e, 0, sizeof(e));
            e.type = SDL_TEXTINPUT;
            e.text.windowID = g_win_id;
            snprintf(e.text.text, sizeof(e.text.text), "%s", t);
            SDL_PushEvent(&e);
        }
    }
}

/* V8/H2, paso 3 — el ratón del guion, como lo manda SDL: entra por el mismo vigilante que el de verdad (lvgl_watch), y
 * de ahí al ratón del SO, si hay un ratón conectado; LVGL, como táctil, lo ve igual que el clic. `g_guion_botones`, los
 * que tiene apretados el guion (la máscara de SDL), para los movimientos. */
static uint32_t g_guion_botones = 0;

static void guion_raton(uint32_t type, int x, int y, int boton) {
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = type;
    if (type == SDL_MOUSEMOTION) {
        e.motion.windowID = g_win_id; e.motion.x = x; e.motion.y = y;
        e.motion.state = g_guion_botones;
    } else if (type == SDL_MOUSEWHEEL) {
        e.wheel.windowID = g_win_id; e.wheel.y = boton;   /* aquí, las muescas */
        e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    } else {
        e.button.windowID = g_win_id; e.button.x = x; e.button.y = y;
        e.button.button = (Uint8) boton;
        e.button.state  = (type == SDL_MOUSEBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
        e.button.clicks = 1;
    }
    SDL_PushEvent(&e);
}

static void script_load(void) {
    const char* path = getenv("BPVM_GUI_SCRIPT");
    if (!path || !*path) return;
    FILE* f = fopen(path, "rb");
    if (!f) { printf("[gui] guion: no se puede abrir %s" BPVM_EOL, path); fflush(stdout); return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return; }
    /* #814: el guion es de la ventana, no del programa (dura lo que ella): memoria de la plataforma. Con la del programa,
     * el fin de RUN avisaba de una fuga que no era suya. */
    g_script_buf = (char*) bpvm_malloc_os((size_t) n + 1);
    if (!g_script_buf) { fclose(f); return; }
    size_t rd = fread(g_script_buf, 1, (size_t) n, f);
    fclose(f);
    g_script_buf[rd] = '\0';
    /* Trocear en lineas (in-place) y quedarnos con las utiles. */
    g_script = (char**) bpvm_malloc_os(sizeof(char*) * (rd + 1));
    if (!g_script) return;
    char* p = g_script_buf;
    while (*p) {
        char* line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = '\0';
        while (*line == ' ' || *line == '\t') line++;
        size_t L = strlen(line);
        while (L && (line[L-1] == '\r' || line[L-1] == ' ')) line[--L] = '\0';
        if (L && line[0] != '#') g_script[g_script_n++] = line;
    }
    printf("[gui] guion %s: %d comandos" BPVM_EOL, path, g_script_n);
    fflush(stdout);
}

/* Un paso del guion por pump. Respeta el gate (waits y el soltar del clic). */
static void script_step(void) {
    if (!g_script || g_script_i >= g_script_n) return;
    if (SDL_GetTicks() < g_script_gate) return;
    if (g_click_up_at) return;                 /* clic a medias: esperar al UP */
    const char* line = g_script[g_script_i++];
    int x, y, ms;
    if (sscanf(line, "wait %d", &ms) == 1) {
        g_script_gate = SDL_GetTicks() + (uint32_t) ms;
    } else if (sscanf(line, "click %d %d", &x, &y) == 2) {
        mouse_event(SDL_MOUSEMOTION, x, y);
        mouse_event(SDL_MOUSEBUTTONDOWN, x, y);
        g_click_x = x; g_click_y = y;
        g_click_up_at = SDL_GetTicks() + 60;   /* el UP va en un pump posterior:
                                                * LVGL lee el indev en cada
                                                * lv_timer_handler, y press+release
                                                * en la misma pasada se pierde. */
        printf("[gui] guion: click %d,%d" BPVM_EOL, x, y); fflush(stdout);
    } else if (strncmp(line, "teclea ", 7) == 0) {   /* V8/H2, paso 2 */
        guion_teclea(line + 7);
        printf("[gui] guion: teclea '%s'" BPVM_EOL, line + 7); fflush(stdout);
    } else if (sscanf(line, "tecla %d", &x) == 1) {
        guion_tecla(x, 0, NULL);
        printf("[gui] guion: tecla %d" BPVM_EOL, x); fflush(stdout);
    } else if (sscanf(line, "mueve %d %d", &x, &y) == 2) {   /* V8/H2, paso 3: el ratón */
        guion_raton(SDL_MOUSEMOTION, x, y, 0);
        printf("[gui] guion: mueve %d,%d" BPVM_EOL, x, y); fflush(stdout);
    } else if (strncmp(line, "pulsa ", 6) == 0 || strncmp(line, "suelta ", 7) == 0) {
        int pulsa = line[0] == 'p', b = SDL_BUTTON_LEFT;
        int n = sscanf(line + (pulsa ? 6 : 7), "%d %d %d", &x, &y, &b);
        if (n < 2 || b < 1 || b > 5) {
            printf("[gui] guion: comando no reconocido: '%s'" BPVM_EOL, line); fflush(stdout);
        } else {
            guion_raton(SDL_MOUSEMOTION, x, y, 0);   /* llegar ahí, con los botones de antes */
            if (pulsa) g_guion_botones |= SDL_BUTTON((uint32_t) b);
            else       g_guion_botones &= ~SDL_BUTTON((uint32_t) b);
            guion_raton(pulsa ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP, x, y, b);
            printf("[gui] guion: %s %d,%d (%d)" BPVM_EOL, pulsa ? "pulsa" : "suelta", x, y, b); fflush(stdout);
        }
    } else if (sscanf(line, "rueda %d", &x) == 1) {
        guion_raton(SDL_MOUSEWHEEL, 0, 0, x);
        printf("[gui] guion: rueda %d" BPVM_EOL, x); fflush(stdout);
    } else if (strcmp(line, "shot") == 0) {
        g_shot_requested = 1;
    } else if (strcmp(line, "quit") == 0) {
        g_window_closed = 1;
    } else {
        printf("[gui] guion: comando no reconocido: '%s'" BPVM_EOL, line); fflush(stdout);
    }
}

/* V8/H2, paso 2 (9-nov) — el estado de SDL, en los bits del teclado del SO: el byte de modificadores del estándar HID y
 * los bloqueos (bpvm_teclado.h). El código de la tecla ya lo es: el `scancode` de SDL sigue el estándar HID. */
static uint32_t estado_sdl(unsigned m) {
    uint32_t e = 0;
    if (m & KMOD_LCTRL)  e |= BPVM_TECLA_CTRL_IZQ;
    if (m & KMOD_LSHIFT) e |= BPVM_TECLA_SHIFT_IZQ;
    if (m & KMOD_LALT)   e |= BPVM_TECLA_ALT;
    if (m & KMOD_LGUI)   e |= BPVM_TECLA_WIN_IZQ;
    if (m & KMOD_RCTRL)  e |= BPVM_TECLA_CTRL_DER;
    if (m & KMOD_RSHIFT) e |= BPVM_TECLA_SHIFT_DER;
    if (m & (KMOD_RALT | KMOD_MODE)) e |= BPVM_TECLA_ALTGR;
    if (m & KMOD_RGUI)   e |= BPVM_TECLA_WIN_DER;
    if (m & KMOD_CAPS)   e |= BPVM_TECLA_BLOQ_MAYUS;
    if (m & KMOD_NUM)    e |= BPVM_TECLA_BLOQ_NUM;
    if (m & 0x8000u)     e |= BPVM_TECLA_BLOQ_DESPL;   /* KMOD_SCROLL, desde SDL 2.0.18 */
    return e;
}

/* El siguiente carácter de un texto UTF-8 (lo que da SDL_TEXTINPUT), como código Unicode; avanza `*s`. */
static int32_t sig_utf8(const unsigned char** s) {
    const unsigned char* p = *s;
    int32_t cp;
    int n, i;
    if (p[0] < 0x80u)                 { cp = p[0];         n = 1; }
    else if ((p[0] & 0xE0u) == 0xC0u) { cp = p[0] & 0x1F; n = 2; }
    else if ((p[0] & 0xF0u) == 0xE0u) { cp = p[0] & 0x0F; n = 3; }
    else if ((p[0] & 0xF8u) == 0xF0u) { cp = p[0] & 0x07; n = 4; }
    else { *s = p + 1; return 0xFFFD; }
    for (i = 1; i < n; i++) {
        if ((p[i] & 0xC0u) != 0x80u) { *s = p + i; return 0xFFFD; }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    *s = p + n;
    return cp;
}

/* V8/H2, paso 3 — los botones de SDL, en los bits del informe HID (BPVM_RATON_*): el número de un botón, y la máscara de
 * los que están apretados. */
static uint32_t boton_hid(int b) {
    switch (b) {
        case SDL_BUTTON_LEFT:   return BPVM_RATON_IZQ;
        case SDL_BUTTON_RIGHT:  return BPVM_RATON_DER;
        case SDL_BUTTON_MIDDLE: return BPVM_RATON_CENTRAL;
        case SDL_BUTTON_X1:     return BPVM_RATON_ATRAS;
        case SDL_BUTTON_X2:     return BPVM_RATON_ADELANTE;
        default:                return 0;
    }
}
static uint32_t botones_hid(uint32_t m) {
    uint32_t b = 0;
    if (m & SDL_BUTTON_LMASK)  b |= BPVM_RATON_IZQ;
    if (m & SDL_BUTTON_RMASK)  b |= BPVM_RATON_DER;
    if (m & SDL_BUTTON_MMASK)  b |= BPVM_RATON_CENTRAL;
    if (m & SDL_BUTTON_X1MASK) b |= BPVM_RATON_ATRAS;
    if (m & SDL_BUTTON_X2MASK) b |= BPVM_RATON_ADELANTE;
    return b;
}

/* El punto de la ventana, en la pantalla, y dentro de ella: arrastrando fuera, el borde. #813: la ventana es la pantalla
 * tal como se ve (gira con ella: bpvm_gui_disp_set_rotation), así que no hay cuenta que hacer, y su tamaño es el de la
 * pantalla de LVGL. */
static void en_pantalla(int wx, int wy, int32_t* x, int32_t* y) {
    lv_display_t* d = lv_display_get_default();
    const int w = d ? (int) lv_display_get_horizontal_resolution(d) : 0;
    const int h = d ? (int) lv_display_get_vertical_resolution(d) : 0;
    if (wx >= w) wx = w - 1;
    if (wx < 0) wx = 0;
    if (wy >= h) wy = h - 1;
    if (wy < 0) wy = 0;
    *x = wx;
    *y = wy;
}

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
    /* V8/H2, paso 2 — las teclas de la ventana, al teclado del SO (él mira que haya un teclado conectado): la pulsación,
     * sin las repeticiones; su carácter llega aparte, en SDL_TEXTINPUT; y al soltarla, por si no dio ninguno. */
    if (e->type == SDL_KEYDOWN && !e->key.repeat)
        bpvm_teclado_pc_pulsada((int32_t) e->key.keysym.scancode, estado_sdl((unsigned) e->key.keysym.mod));
    else if (e->type == SDL_KEYUP)
        bpvm_teclado_pc_soltada((int32_t) e->key.keysym.scancode);
    else if (e->type == SDL_TEXTINPUT) {
        const unsigned char* s = (const unsigned char*) e->text.text;
        uint32_t est = estado_sdl((unsigned) SDL_GetModState());
        while (*s) bpvm_teclado_pc_texto(sig_utf8(&s), est);
    }
    /* V8/H2, paso 3 — el ratón de la ventana, al ratón del SO (él mira que haya un ratón conectado): moverse, los botones
     * y la rueda, con el punto en la pantalla. Como táctil, LVGL lo sigue viendo igual. Lo que SDL fabrica de una
     * pantalla táctil no es un ratón. */
    if (lv_display_get_default() != NULL) {   /* al cerrar la ventana, LVGL borra la pantalla */
        int32_t x, y;
        if (e->type == SDL_MOUSEMOTION && e->motion.which != SDL_TOUCH_MOUSEID) {
            g_raton_botones = botones_hid(e->motion.state);
            en_pantalla(e->motion.x, e->motion.y, &x, &y);
            bpvm_raton_pc(x, y, g_raton_botones, 0);
        } else if ((e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)
                   && e->button.which != SDL_TOUCH_MOUSEID) {
            if (e->type == SDL_MOUSEBUTTONDOWN) g_raton_botones |= boton_hid(e->button.button);
            else                                g_raton_botones &= ~boton_hid(e->button.button);
            en_pantalla(e->button.x, e->button.y, &x, &y);
            bpvm_raton_pc(x, y, g_raton_botones, 0);
        } else if (e->type == SDL_MOUSEWHEEL && e->wheel.which != SDL_TOUCH_MOUSEID) {
            int32_t d = e->wheel.y;   /* + hacia arriba, como el informe HID */
            if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) d = -d;
            if (d != 0) bpvm_raton_pc(bpvm_raton_x(), bpvm_raton_y(), g_raton_botones, d);   /* donde esté */
        }
    }
    return 1;   /* 1 = no filtrar (el evento sigue su curso) */
}

/* --- PNG ------------------------------------------------------------------
 * Mismo espiritu que write_bmp24: sin dependencias. PNG parece pedir zlib, pero
 * NO: el formato admite bloques deflate STORED (sin comprimir), asi que basta
 * con envolverlos. Y de los dos checksums que pide, uno ya lo teniamos —
 * bpvm_crc32 es EXACTAMENTE el CRC-32 de PNG (poly 0xEDB88320) — y el otro, el
 * Adler-32, son cinco lineas. Total: cero libs nuevas.
 *
 * Por que PNG si ya habia BMP: el BMP lo lee cualquier visor... pero no Claude,
 * que solo abre PNG/JPG. Y el objetivo de esto es justamente que la captura la
 * pueda MIRAR quien esta depurando, sea humano o no.
 *
 * El fichero sale grande (no se comprime nada), pero es un PNG valido de verdad
 * y estas capturas son de diagnostico, no de distribucion.
 */
static uint32_t adler32(const uint8_t* d, size_t n) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < n; i++) { a = (a + d[i]) % 65521u; b = (b + a) % 65521u; }
    return (b << 16) | a;
}

static void be32w(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t) v;
}

/* Un chunk PNG: len(4 BE) + tipo(4) + datos + crc(4 BE). El CRC cubre tipo+datos
 * (no la longitud), asi que hay que tenerlos contiguos para calcularlo. */
static int png_chunk(FILE* f, const char* type, const uint8_t* data, size_t len) {
    uint8_t n[4];
    be32w(n, (uint32_t) len);
    if (fwrite(n, 1, 4, f) != 4) return -1;
    uint8_t* tmp = (uint8_t*) bpvm_malloc(4 + len);
    if (!tmp) return -1;
    memcpy(tmp, type, 4);
    if (len) memcpy(tmp + 4, data, len);
    int ok = (fwrite(tmp, 1, 4 + len, f) == 4 + len);
    uint32_t crc = bpvm_crc32(tmp, 4 + len);
    bpvm_free(tmp);
    if (!ok) return -1;
    uint8_t c[4];
    be32w(c, crc);
    return (fwrite(c, 1, 4, f) == 4) ? 0 : -1;
}

/* Escribe un PNG RGB de 8 bits desde un buffer ARGB8888 de LVGL
 * (en memoria: byte[0]=B,[1]=G,[2]=R,[3]=A). PNG va top-down, al reves que BMP. */
static int write_png24(const char* path, const uint8_t* px, int w, int h, int stride) {
    if (w <= 0 || h <= 0) return -1;

    /* Datos crudos: por fila, un byte de filtro (0=None) + w pixeles RGB. */
    size_t rowlen = 1 + (size_t) w * 3;
    size_t rawlen = rowlen * (size_t) h;
    uint8_t* raw = (uint8_t*) bpvm_malloc(rawlen);
    if (!raw) return -1;
    for (int y = 0; y < h; y++) {
        uint8_t* o = raw + (size_t) y * rowlen;
        *o++ = 0;                                    /* filtro None */
        const uint8_t* r = px + (size_t) y * stride;
        for (int x = 0; x < w; x++) {
            const uint8_t* p = r + (size_t) x * 4;   /* B,G,R,A en memoria */
            *o++ = p[2]; *o++ = p[1]; *o++ = p[0];   /* PNG los quiere R,G,B */
        }
    }

    /* Stream zlib: cabecera + bloques stored (<=65535 cada uno) + adler32. */
    size_t nblocks = (rawlen + 65534) / 65535;
    uint8_t* z = (uint8_t*) bpvm_malloc(2 + nblocks * 5 + rawlen + 4);
    if (!z) { bpvm_free(raw); return -1; }
    size_t zi = 0, off = 0;
    z[zi++] = 0x78; z[zi++] = 0x01;        /* CM=deflate/32K + FLG (0x7801 %31==0) */
    do {
        size_t n = rawlen - off;
        if (n > 65535) n = 65535;
        uint16_t ln = (uint16_t) n, nl = (uint16_t) ~ln;
        z[zi++] = (off + n >= rawlen) ? 1 : 0;       /* BFINAL, BTYPE=00 (stored) */
        z[zi++] = (uint8_t) ln; z[zi++] = (uint8_t)(ln >> 8);   /* LEN  (LE) */
        z[zi++] = (uint8_t) nl; z[zi++] = (uint8_t)(nl >> 8);   /* NLEN (LE) */
        memcpy(z + zi, raw + off, n);
        zi += n; off += n;
    } while (off < rawlen);
    be32w(z + zi, adler32(raw, rawlen)); zi += 4;
    bpvm_free(raw);

    FILE* f = fopen(path, "wb");
    if (!f) { bpvm_free(z); return -1; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    uint8_t ihdr[13];
    be32w(ihdr, (uint32_t) w); be32w(ihdr + 4, (uint32_t) h);
    ihdr[8] = 8;    /* 8 bits por canal */
    ihdr[9] = 2;    /* color type 2 = truecolor RGB */
    ihdr[10] = 0;   /* compresion: deflate (la unica que define PNG) */
    ihdr[11] = 0;   /* filtro: adaptativo */
    ihdr[12] = 0;   /* sin entrelazar */
    int rc = 0;
    if (fwrite(sig, 1, 8, f) != 8)          rc = -1;
    if (!rc) rc = png_chunk(f, "IHDR", ihdr, sizeof(ihdr));
    if (!rc) rc = png_chunk(f, "IDAT", z, zi);
    if (!rc) rc = png_chunk(f, "IEND", NULL, 0);
    fclose(f);
    bpvm_free(z);
    return rc;
}

/* V8/H2, paso 4 (10-nov) — LO DE ENCIMA DE LA PANTALLA, SOBRE LA CAPTURA: lv_snapshot_take dibuja la pantalla y sus hijos,
 * no las capas de encima, donde están un Msgbox (la de arriba) y la flecha del ratón (la del sistema). Sin esto, la
 * captura no enseñaba lo que se ve en la ventana (la flecha, medido con el guion del ratón). Las dos, ARGB8888 sin
 * premultiplicar: se mezcla cada punto con su alfa. */
static void compone_capa(lv_draw_buf_t* snap, lv_obj_t* capa) {
    lv_draw_buf_t* c;
    uint32_t x, y, w, h;
    if (!capa) return;
    c = lv_snapshot_take(capa, LV_COLOR_FORMAT_ARGB8888);
    if (!c) return;
    w = (uint32_t) snap->header.w;
    h = (uint32_t) snap->header.h;
    if ((uint32_t) c->header.w == w && (uint32_t) c->header.h == h) {
        for (y = 0; y < h; y++) {
            uint8_t* d = snap->data + y * (uint32_t) snap->header.stride;
            const uint8_t* s = c->data + y * (uint32_t) c->header.stride;
            for (x = 0; x < w; x++, d += 4, s += 4) {
                unsigned a = s[3];
                if (a == 0) continue;
                d[0] = (uint8_t) ((s[0] * a + d[0] * (255u - a)) / 255u);
                d[1] = (uint8_t) ((s[1] * a + d[1] * (255u - a)) / 255u);
                d[2] = (uint8_t) ((s[2] * a + d[2] * (255u - a)) / 255u);
            }
        }
    }
    lv_draw_buf_destroy(c);
}

/* Snapshot de la pantalla activa → PNG numerado en el cwd. Lo llama el pump. */
static void bpvm_gui_take_screenshot(void) {
    lv_draw_buf_t* snap = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
    if (!snap) { printf("[gui] captura: lv_snapshot_take fallo" BPVM_EOL); fflush(stdout); return; }
    compone_capa(snap, lv_layer_top());   /* V8/H2, paso 4: y lo de encima (un Msgbox, la flecha del ratón) */
    compone_capa(snap, lv_layer_sys());
    static int n = 0;
    char path[64];
    snprintf(path, sizeof(path), "bp_shot_%04d.png", ++n);
    int rc = write_png24(path, snap->data, (int) snap->header.w, (int) snap->header.h,
                         (int) snap->header.stride);
    if (rc == 0) printf("[gui] captura guardada: %s (%dx%d)" BPVM_EOL, path,
                        (int) snap->header.w, (int) snap->header.h);
    else         printf("[gui] captura: error escribiendo %s" BPVM_EOL, path);
    fflush(stdout);
    lv_draw_buf_destroy(snap);
}

/* H10 — micro simulado SIN pantalla. LVGL SIGUE montándose (los ~50 sitios que
 * llaman lv_* dan por hecho que hay display; saltarse lv_init sería sembrar
 * NULLs), pero contra un display fuera de pantalla cuyo flush no escribe en
 * ningún sitio. Resultado: los widgets se crean, el modelo y el dumpTree son
 * correctos, no aparece ventana, y disp_is_open() = 0 → Gui.run() vuelve en
 * seguida en vez de girar esperando un cierre que nunca llega. */
static int g_headless = 0;
void bpvm_gui_disp_set_headless(int on) { g_headless = on ? 1 : 0; }

/* F1 de V6 (12-sep) — CERRAR LA VENTANA = APAGAR EL MICRO SIMULADO. El driver SDL de
 * LVGL borra el display al cerrar la ventana (lv_sdl_window.c: SDL_WINDOWEVENT_CLOSE
 * → lv_display_delete; SDL_QUIT → SDL_Quit + lv_deinit) y aqui `g_window_closed` se
 * queda a 1 para siempre: el simulador seguia vivo y escuchando, y CADA programa
 * grafico posterior terminaba al instante con EXITED OK sin pintar nada — un «OK»
 * que engañaba (lo vio el verificador del ZIP). Reabrir seria reinicializar LVGL,
 * el display, el raton y el gancho de captura a medias de un proceso; mas honesto y
 * mas corto: el simulador TERMINA, y el IDE dice «el simulador no esta en marcha —
 * pulsa Arrancar». Solo el simulador lo pide (bpvm_sim.c); la VM-C de linea de
 * comandos sigue como estaba (su programa continua tras run() con la ventana cerrada). */
static int g_exit_on_close = 0;
void bpvm_gui_disp_set_exit_on_close(int on) { g_exit_on_close = on ? 1 : 0; }

static void headless_flush(lv_display_t* d, const lv_area_t* a, uint8_t* px) {
    (void) a; (void) px;
    lv_display_flush_ready(d);
}

static void disp_init_headless(int w, int h) {
    lv_tick_set_cb(SDL_GetTicks);   /* el tick sirve igual; SDL no abre nada por sí solo */
    lv_display_t* d = lv_display_create(w, h);
    /* Buffer parcial de 10 líneas: LVGL renderiza por trozos y lo tira. Es de la pantalla y dura lo que ella (#814):
     * memoria de la plataforma, no del programa, o el fin de RUN avisa de una fuga que no es suya. */
    static uint8_t* buf = NULL;
    size_t bytes = (size_t) w * 10u * 4u;   /* ARGB8888, el formato por defecto */
    bpvm_free(buf);
    buf = (uint8_t*) bpvm_malloc_os(bytes);
    if (!buf) { printf("[gui] headless: sin memoria para el buffer de dibujo" BPVM_EOL); return; }
    lv_display_set_flush_cb(d, headless_flush);
    lv_display_set_buffers(d, buf, NULL, (uint32_t) bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    g_window_closed = 1;            /* no hay ventana que cerrar: "cerrada" desde el minuto 0 */
    printf("[gui] sin pantalla (%dx%d fuera de pantalla)" BPVM_EOL, w, h);
    fflush(stdout);
}

/* #322 — el título por defecto de LVGL es "LVGL Simulator": nombra la librería
 * que dibuja, no lo que está corriendo. Quien arranca el display dice quién es;
 * el micro simulado se identifica como tal para no confundirlo con una placa. */
static char g_title[64] = "BasicPlus";

void bpvm_gui_disp_set_title(const char* title) {
    if (!title || !*title) return;
    snprintf(g_title, sizeof g_title, "%s", title);
}

/* V8/H2, paso 4 — el cursor del sistema en la ventana: escondido mientras se ve la flecha de la GUI (gui_entrada.c). */
static void cursor_sdl(int visible) { SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE); }

void bpvm_gui_disp_init(int w, int h) {
    if (g_headless) { disp_init_headless(w, h); return; }
    SDL_SetMainReady();
    lv_tick_set_cb(SDL_GetTicks);
    g_hilo_ventana = SDL_ThreadID();   /* V7/H12: el único que la bombea */
    lv_display_t* d = lv_sdl_window_create(w, h);
    lv_sdl_window_set_title(d, g_title);
    /* V8/H2, paso 4: el teclado y el ratón del SO, en LVGL (gui_entrada.c). En el PC la táctil es el mismo ratón: con
     * «Ratón simulado» conectado se apaga, y el cursor del sistema se esconde en la ventana.
     * ⚠️ LOS NUESTROS, ANTES QUE LOS DE SDL, Y EL TECLADO DE SDL, APAGADO: los manejadores de SDL de LVGL cogen el primer
     * dispositivo de cada tipo de su lista, que empieza por el más nuevo, y dan por hecho que es el suyo. Con el nuestro
     * más nuevo, el de las teclas leía unos datos que el nuestro no tiene y reventaba (strlen de un nulo, medido con gdb
     * al teclear), y el del ratón se habría quedado con nuestro puntero, sin táctil. Así encuentran los suyos: la táctil y
     * un teclado de SDL que no se lee nunca. */
    bpvm_gui_entrada_init(1, cursor_sdl);
    lv_indev_t* tactil = lv_sdl_mouse_create();   /* el ratón del PC, como táctil */
    lv_indev_t* teclado_sdl = lv_sdl_keyboard_create();
    if (teclado_sdl) lv_indev_enable(teclado_sdl, false);
    bpvm_gui_entrada_tactil(tactil);
    SDL_AddEventWatch(lvgl_watch, NULL);
    /* windowID de la ventana que acaba de crear LVGL: hace falta para inyectar
     * eventos de ratón sintéticos (BPVM_GUI_SCRIPT). lv_sdl_mouse_handler filtra
     * por event->button.windowID, así que un evento sin él se descarta y el clic
     * se pierde EN SILENCIO. */
    SDL_Renderer* r = (SDL_Renderer*) lv_sdl_window_get_renderer(d);
    SDL_Window* win = r ? SDL_RenderGetWindow(r) : NULL;
    if (win) g_win_id = SDL_GetWindowID(win);
    else { printf("[gui] aviso: sin windowID → los clics del guión no llegarán" BPVM_EOL); fflush(stdout); }
}

uint32_t bpvm_gui_disp_pump(void) {
    if (g_hilo_ventana != 0 && SDL_ThreadID() != g_hilo_ventana && g_bombeos_ajenos++ == 0) {
        printf("[gui] AVISO: la ventana, bombeada desde OTRO hilo que el que la creo (no debe pasar: con dos runs, "
               "la GUI es del run 0)\n");
        fflush(stdout);
    }
    uint32_t idle_ms = lv_timer_handler();
    /* BPVM_GUI_SHOT_MS: se lee en el primer pump (aquí SDL ya está arrancado, así
     * que SDL_GetTicks() vale) y la cuenta arranca desde el primer frame, que es
     * el instante que le importa a quien pide la captura. */
    if (!g_shot_env_read) {
        g_shot_env_read = 1;
        script_load();
        const char* e = getenv("BPVM_GUI_SHOT_MS");
        if (e && *e) {
            g_shot_after_ms = atoi(e);
            if (g_shot_after_ms < 0) g_shot_after_ms = 0;
            g_shot_deadline = SDL_GetTicks() + (uint32_t) g_shot_after_ms;
            printf("[gui] BPVM_GUI_SHOT_MS=%d → captura automática y cierre" BPVM_EOL,
                   g_shot_after_ms);
            fflush(stdout);
        }
    }
    if (g_shot_after_ms >= 0 && SDL_GetTicks() >= g_shot_deadline) {
        g_shot_after_ms = -1;      /* una sola vez */
        g_shot_requested = 1;
    }
    /* Soltar el botón del clic del guión: va en un pump POSTERIOR al press
     * porque LVGL lee el indev una vez por lv_timer_handler — press+release en
     * la misma pasada se pierde. */
    if (g_click_up_at && SDL_GetTicks() >= g_click_up_at) {
        g_click_up_at = 0;
        mouse_event(SDL_MOUSEBUTTONUP, g_click_x, g_click_y);
    }
    script_step();
    if (g_shot_requested) {
        g_shot_requested = 0;
        bpvm_gui_take_screenshot();
        /* Si la captura la pidió el entorno, cerramos: el que lanza esto es un
         * script, y si no salimos se queda colgado esperando a que un humano
         * cierre la ventana. F12 (petición manual) no cierra: ahí SÍ hay alguien
         * delante que quiere seguir mirando. */
        if (g_shot_env_read && getenv("BPVM_GUI_SHOT_MS")) g_window_closed = 1;
    }
    if (g_window_closed && g_exit_on_close && !g_headless) {
        printf("[gui] ventana cerrada: el micro simulado termina (en el IDE, pulsa Arrancar para otro)" BPVM_EOL);
        fflush(stdout);
        exit(0);
    }
    /* #462 - antes aqui habia un SDL_Delay(16) INCONDICIONAL, que ademas tiraba
     * lo que lv_timer_handler acababa de contestar. Y no dormia el hilo BP del
     * GUI: dormia la tarea de SO sobre la que corren TODOS los hilos BP. Ahora
     * el ocio se devuelve y lo duerme el lazo BP. */
    if (idle_ms > BPVM_GUI_OCIO_MAX_MS) idle_ms = BPVM_GUI_OCIO_MAX_MS;
    return idle_ms;
}

int  bpvm_gui_disp_is_open(void) { return !g_window_closed; }

/* V6/P2.3 — en host la ventana no tiene tamaño propio: manda --screen= o el default
 * del modelo (480×320). Devolver 0 es decir «no sé»; gui.c no toca nada. */
int bpvm_gui_disp_native_size(int* w, int* h) { (void) w; (void) h; return 0; }

/* Rotación en runtime (Gui.setRotation). #813 (11-nov) — EN EL PC, LA VENTANA GIRA CON LA PANTALLA. En una placa, la
 * imagen se gira porque el panel se ha girado de verdad, para verlo así (Eduardo: «La pantalla la giras físicamente
 * porque te interesa verla en esa posición»): en el PC, la ventana se pone como ese panel girado y la imagen se ve
 * derecha, como en la VM-Java. Con 180°, como sin girar. Así que LVGL no gira nada: la pantalla toma el tamaño que ya
 * tiene el modelo (gui.c lo cambia antes de llamar aquí), y el driver de SDL de LVGL pone la ventana de ese tamaño; sin
 * ventana, igual. La táctil, el ratón y el guion, en la pantalla tal como se ve, sin cuentas.
 * Hasta el 11-nov giraba LVGL, y salía mal por dos cosas: su driver de SDL sólo gira el dibujo en el modo PARTIAL, y la
 * ventana va en DIRECT (rayas y colores pálidos, sin imagen); y si el programa giraba antes de crear la pantalla, la
 * ventana ya se creaba con el tamaño girado y LVGL la giraba otra vez (el modelo, 320×480; LVGL, 480×320). */
void bpvm_gui_disp_set_rotation(int deg)
{
    lv_display_t* d = lv_display_get_default();
    int w = 0, h = 0;
    (void) deg;   /* lo que importa, el tamaño, ya está en el modelo */
    if (d == NULL) return;
    bpvm_gui_get_screen_size(&w, &h);
    if (w > 0 && h > 0) lv_display_set_resolution(d, w, h);
}

#endif /* BPVM_LVGL */
