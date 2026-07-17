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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "bpvm_gui.h"
#include "crc32.h"

static volatile int g_window_closed = 0;
static volatile int g_shot_requested = 0;   /* F12 → captura de pantalla (docs) */

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
    uint8_t* tmp = (uint8_t*) malloc(4 + len);
    if (!tmp) return -1;
    memcpy(tmp, type, 4);
    if (len) memcpy(tmp + 4, data, len);
    int ok = (fwrite(tmp, 1, 4 + len, f) == 4 + len);
    uint32_t crc = bpvm_crc32(tmp, 4 + len);
    free(tmp);
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
    uint8_t* raw = (uint8_t*) malloc(rawlen);
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
    uint8_t* z = (uint8_t*) malloc(2 + nblocks * 5 + rawlen + 4);
    if (!z) { free(raw); return -1; }
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
    free(raw);

    FILE* f = fopen(path, "wb");
    if (!f) { free(z); return -1; }
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
    free(z);
    return rc;
}

/* Snapshot de la pantalla activa → PNG numerado en el cwd. Lo llama el pump. */
static void bpvm_gui_take_screenshot(void) {
    lv_draw_buf_t* snap = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
    if (!snap) { printf("[gui] captura: lv_snapshot_take fallo\n"); fflush(stdout); return; }
    static int n = 0;
    char path[64];
    snprintf(path, sizeof(path), "bp_shot_%04d.png", ++n);
    int rc = write_png24(path, snap->data, (int) snap->header.w, (int) snap->header.h,
                         (int) snap->header.stride);
    if (rc == 0) printf("[gui] captura guardada: %s (%dx%d)\n", path,
                        (int) snap->header.w, (int) snap->header.h);
    else         printf("[gui] captura: error escribiendo %s\n", path);
    fflush(stdout);
    lv_draw_buf_destroy(snap);
}

void bpvm_gui_disp_init(int w, int h) {
    SDL_SetMainReady();
    lv_tick_set_cb(SDL_GetTicks);
    lv_sdl_window_create(w, h);
    lv_sdl_mouse_create();
    SDL_AddEventWatch(lvgl_watch, NULL);
}

void bpvm_gui_disp_pump(void) {
    lv_timer_handler();
    /* BPVM_GUI_SHOT_MS: se lee en el primer pump (aquí SDL ya está arrancado, así
     * que SDL_GetTicks() vale) y la cuenta arranca desde el primer frame, que es
     * el instante que le importa a quien pide la captura. */
    if (!g_shot_env_read) {
        g_shot_env_read = 1;
        const char* e = getenv("BPVM_GUI_SHOT_MS");
        if (e && *e) {
            g_shot_after_ms = atoi(e);
            if (g_shot_after_ms < 0) g_shot_after_ms = 0;
            g_shot_deadline = SDL_GetTicks() + (uint32_t) g_shot_after_ms;
            printf("[gui] BPVM_GUI_SHOT_MS=%d → captura automática y cierre\n",
                   g_shot_after_ms);
            fflush(stdout);
        }
    }
    if (g_shot_after_ms >= 0 && SDL_GetTicks() >= g_shot_deadline) {
        g_shot_after_ms = -1;      /* una sola vez */
        g_shot_requested = 1;
    }
    if (g_shot_requested) {
        g_shot_requested = 0;
        bpvm_gui_take_screenshot();
        /* Si la captura la pidió el entorno, cerramos: el que lanza esto es un
         * script, y si no salimos se queda colgado esperando a que un humano
         * cierre la ventana. F12 (petición manual) no cierra: ahí SÍ hay alguien
         * delante que quiere seguir mirando. */
        if (g_shot_env_read && getenv("BPVM_GUI_SHOT_MS")) g_window_closed = 1;
    }
    SDL_Delay(16);
}

int  bpvm_gui_disp_is_open(void) { return !g_window_closed; }

/* Rotación en runtime (Gui.setRotation): el driver SDL de LVGL gira en su PROPIO flush
 * (lv_sdl_window.c) — la ventana (el "panel físico") se queda w×h y el contenido rota
 * dentro, igual que en placa. deg llega validado de gui.c. */
void bpvm_gui_disp_set_rotation(int deg)
{
    lv_display_t *d = lv_display_get_default();
    if (d == NULL) return;
    switch (deg) {
        case 0:   lv_display_set_rotation(d, LV_DISPLAY_ROTATION_0);   break;
        case 90:  lv_display_set_rotation(d, LV_DISPLAY_ROTATION_90);  break;
        case 180: lv_display_set_rotation(d, LV_DISPLAY_ROTATION_180); break;
        case 270: lv_display_set_rotation(d, LV_DISPLAY_ROTATION_270); break;
        default:  break;
    }
}

#endif /* BPVM_LVGL */
