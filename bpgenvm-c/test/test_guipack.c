/*
 * test_guipack.c — V7/H6 P2: la COSTURA entre la VM y el pack de la GUI, en el PC.
 *
 * Lo que se prueba no es la GUI (eso es gui.c, con su paridad) sino la frontera:
 * que sin pack la VM lo diga CON NOMBRE y no salte a ninguna parte; que con una
 * tabla publicada bajo 'LVGL' cada trampolín llegue a SU función (por nombre, no
 * por posición); que la memoria que el pack reserva se copie y se le devuelva
 * con su `dump_free` (dos alocadores, dos dueños); y que un símbolo que falte o
 * una versión distinta se digan, en vez de dejar un puntero a NULL esperando.
 *
 * Se enlaza con gui_pack.c (los trampolines GENERADOS), bpvm_bios.c (el
 * registro) y gui_pack_stubs.c (un doble del pack, GENERADO también). Sin la VM
 * entera: bpvm_malloc/free/diag son dobles de aquí.
 *
 *   make test-guipack
 */
#include "bpvm_gui.h"
#include "gui_pack_api.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* Dobles de la VM: el alocador de LA VM (distinto del del doble del pack, que usa
 * malloc a pelo — aquí también, pero contado aparte para ver quién libera qué). */
static int g_vm_mallocs = 0, g_vm_frees = 0;
#include "bpvm_alloc.h"   /* bpvm_malloc es un macro sobre bpvm_alloc_raw: el doble es ESE */
void* bpvm_alloc_raw(size_t n, int zero, bpvm_alloc_kind_t k, const char* file, int line) {
    (void) k; (void) file; (void) line; g_vm_mallocs++; return zero ? calloc(1, n) : malloc(n);
}
void  bpvm_free(void* p)    { if (p) g_vm_frees++; free(p); }
static char g_diag[256];
void bpvm_diag(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(g_diag, sizeof g_diag, fmt, ap); va_end(ap); }

extern const bpvm_pack_api_t bpvm_gui_api_stubs;
extern const bpvm_pack_sym_t bpvm_gui_stub_syms[];
extern int g_stub_llamadas, g_stub_dump_free;

/* V7/H6.c G2 — un DRIVER DE PANTALLA de mentira. La VM sólo mira que 'PANT' esté
 * publicada (el driver lo usa el pack de LVGL, no la VM); y con él se prueba
 * también el resolvedor del contrato (bpvm_pant.h), que es de la cabecera. */
#include "bpvm_pant.h"
static int  pd_init(void) { return 0; }
static int  pd_size(int* w, int* h) { *w = 240; *h = 320; return 1; }
static void pd_flush(int a, int b, int c, int d, const uint8_t* p) { (void) a; (void) b; (void) c; (void) d; (void) p; }
static int  pd_formato(void) { return BPVM_PANT_RGB565_BE; }
static const bpvm_pack_sym_t PANT_SYMS[] = {
    { "init", (bpvm_pack_fn_t) pd_init }, { "size", (bpvm_pack_fn_t) pd_size }, { "flush", (bpvm_pack_fn_t) pd_flush } };
static const bpvm_pack_api_t PANT_DOBLE = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, 3u, PANT_SYMS };
static const bpvm_pack_sym_t PANT_SYMS_FMT[] = {
    { "init", (bpvm_pack_fn_t) pd_init }, { "size", (bpvm_pack_fn_t) pd_size },
    { "flush", (bpvm_pack_fn_t) pd_flush }, { "formato", (bpvm_pack_fn_t) pd_formato } };
static const bpvm_pack_api_t PANT_CON_FORMATO = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, 4u, PANT_SYMS_FMT };
static const bpvm_pack_sym_t PANT_SYMS_SIN_FLUSH[] = {
    { "init", (bpvm_pack_fn_t) pd_init }, { "size", (bpvm_pack_fn_t) pd_size } };
static const bpvm_pack_api_t PANT_SIN_FLUSH = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, 2u, PANT_SYMS_SIN_FLUSH };
/* V7/H6.c G3 — un driver que se CONFIGURA y dice si su config vale (`estado`). */
static const char* g_pd_estado = 0;
static const char* pd_estado(void) { return g_pd_estado; }
static const bpvm_pack_sym_t PANT_SYMS_EST[] = {
    { "init", (bpvm_pack_fn_t) pd_init }, { "size", (bpvm_pack_fn_t) pd_size },
    { "flush", (bpvm_pack_fn_t) pd_flush }, { "estado", (bpvm_pack_fn_t) pd_estado } };
static const bpvm_pack_api_t PANT_CON_ESTADO = { BPVM_PANT_MARCA, BPVM_PANT_API_VER, 4u, PANT_SYMS_EST };

static int  g_visitas = 0; static uint32_t g_visto = 0;
static void visitante(void* ctx, uint32_t objptr) { (void) ctx; g_visitas++; g_visto = objptr; }

int main(void) {
    printf("== la costura VM <-> pack de la GUI (V7/H6 P2) ==\n");

    /* ── 1. SIN PACK: se dice, con nombre, y nada salta ── */
    {
        bpvm_bios_packs_reset(); bpvm_gui_pack_reset();
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 0, "sin pack -> disponible() = 0");
        CHECK(m && strstr(m, "no tiene el pack 'LVGL'") != 0, "y el motivo lo dice: no tiene el pack 'LVGL'");
        CHECK(strstr(g_diag, "LVGL") != 0, "y queda en el diag de la VM");
        CHECK(bpvm_gui_create_label(1) == 0, "create_label sin pack -> 0 (no salta a ninguna parte)");
        CHECK(bpvm_gui_get_text(1) == 0, "get_text sin pack -> NULL");
        bpvm_gui_set_text(1, "x");                        /* void: no revienta */
        char* d = (char*) 1; size_t n = bpvm_gui_dump_tree(&d);
        CHECK(n == 0 && d == 0, "dump_tree sin pack -> 0 bytes y NULL");
        CHECK(g_stub_llamadas == 0, "y el doble no recibio NADA");
    }

    /* ── 1b. V7/H6.c G2: el pack de LVGL SIN driver de pantalla ('PANT') ──
     * El pack de LVGL ya no trae driver: si nadie ha publicado 'PANT', la GUI
     * no pinta en ninguna parte, y eso tiene que ser un error CON NOMBRE antes
     * del primer screen, no una pantalla negra. */
    {
        bpvm_bios_packs_reset(); bpvm_gui_pack_reset();
        CHECK(bpvm_bios_publica(BPVM_GUI_MARCA, &bpvm_gui_api_stubs) == 0, "el pack 'LVGL' publicado, SIN driver de pantalla");
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 0, "LVGL sin 'PANT' -> disponible() = 0");
        CHECK(m && strstr(m, "driver de pantalla") != 0 && strstr(m, "'PANT'") != 0,
              "y el motivo lo dice: ningun driver de pantalla ('PANT')");
        CHECK(bpvm_bios_publica(BPVM_PANT_MARCA, &PANT_DOBLE) == 0, "se graba el driver: 'PANT' publicado");
        CHECK(bpvm_gui_disponible(&m) == 1, "y con el driver, disponible() = 1 (sin reiniciar nada)");
    }

    /* ── 1c. V7/H6.c G3: un driver que se CONFIGURA dice si su config vale ──
     * El genérico lee la placa del ENV: una errata en `lcd=` tiene que llegar al
     * programa con nombre, no como pantalla negra. Y sin guardarlo: arreglado el
     * ENV, el siguiente intento ya pasa. */
    {
        bpvm_bios_packs_reset(); bpvm_gui_pack_reset();
        CHECK(bpvm_bios_publica(BPVM_GUI_MARCA, &bpvm_gui_api_stubs) == 0 &&
              bpvm_bios_publica(BPVM_PANT_MARCA, &PANT_CON_ESTADO) == 0, "LVGL + un driver con 'estado'");
        const char* m = 0;
        g_pd_estado = "ENV lcd: falta 'dc'";
        CHECK(bpvm_gui_disponible(&m) == 0, "su config no vale -> disponible() = 0");
        CHECK(m && strcmp(m, "pantalla: ENV lcd: falta 'dc'") == 0, "y el motivo es el del DRIVER: «pantalla: ENV lcd: falta 'dc'»");
        g_pd_estado = 0;
        CHECK(bpvm_gui_disponible(&m) == 1, "arreglado el ENV (estado = NULL) -> disponible() = 1, sin reiniciar");
        g_pd_estado = "";
        CHECK(bpvm_gui_disponible(&m) == 1, "y un estado vacio cuenta como listo");
        bpvm_pant_t p;
        CHECK(bpvm_pant_resolver(&PANT_CON_ESTADO, &p) == 0 && p.estado == pd_estado, "el resolvedor recoge 'estado'");
        CHECK(bpvm_pant_resolver(&PANT_DOBLE, &p) == 0 && p.estado == 0, "y sin publicarlo queda a NULL (= siempre listo)");
    }

    /* ── 2. CON PACK: cada trampolin llega a SU funcion, por nombre ── */
    {
        bpvm_gui_pack_reset();
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 1, "con pack (y su driver) -> disponible() = 1");
        CHECK(bpvm_gui_create_label(3) == 7, "create_label -> el 7 del doble");
        CHECK(bpvm_gui_screen_active() == 7, "screen_active -> 7");
        CHECK(strcmp(bpvm_gui_get_text(1), "stub") == 0, "get_text -> \"stub\" (const char* cruza)");
        CHECK(bpvm_gui_next_event(0) == 7u, "next_event -> 7 (uint32_t cruza)");
        int w = 0, h = 0;
        CHECK(bpvm_gui_disp_native_size(&w, &h) == 1 && w == 240 && h == 320, "disp_native_size -> 240x320 por punteros de salida");
        bpvm_gui_visit_roots(visitante, 0);
        CHECK(g_visitas == 1 && g_visto == 42u, "visit_roots: el pack llama al visitante de la VM (callback cruza)");
        int antes = g_stub_llamadas;
        bpvm_gui_set_text(2, "hola"); bpvm_gui_align(2, 4, 0, 0); bpvm_gui_delete(2);
        CHECK(g_stub_llamadas == antes + 3, "tres void seguidos: tres llamadas al doble");
    }

    /* ── 3. dump_tree: DOS alocadores, DOS duenos ── */
    {
        int vm_m = g_vm_mallocs, df = g_stub_dump_free;
        char* out = 0; size_t n = bpvm_gui_dump_tree(&out);
        CHECK(n == 9 && out && strcmp(out, "stub-dump") == 0, "dump_tree -> la copia con el texto del pack (9 bytes)");
        CHECK(g_vm_mallocs == vm_m + 1, "la copia sale del alocador de LA VM (el que va a liberar builtins.c)");
        CHECK(g_stub_dump_free == df + 1, "y el original vuelve al pack por dump_free (su arena, su free)");
        bpvm_free(out);
    }

    /* ── 4. LE FALTA UN SIMBOLO: se dice cual ── */
    {
        bpvm_bios_packs_reset(); bpvm_gui_pack_reset();
        static bpvm_pack_api_t corta;
        corta = bpvm_gui_api_stubs; corta.n = BPVM_GUI_N_FUNCS - 1;   /* sin el ultimo: dump_free */
        CHECK(bpvm_bios_publica(BPVM_GUI_MARCA, &corta) == 0, "una tabla mas corta se publica (es valida en si)");
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 0, "pero la GUI no esta disponible");
        CHECK(m && strstr(m, "le falta 'dump_free'") != 0, "y el motivo NOMBRA el simbolo que falta");
        CHECK(bpvm_gui_create_label(1) == 0, "y los trampolines no llaman a una tabla a medias");
    }

    /* ── 5. OTRA VERSION de API: se dice ── */
    {
        bpvm_bios_packs_reset(); bpvm_gui_pack_reset();
        static bpvm_pack_api_t vieja;
        vieja = bpvm_gui_api_stubs; vieja.version = BPVM_GUI_API_VER + 1;
        CHECK(bpvm_bios_publica(BPVM_GUI_MARCA, &vieja) == 0, "se publica (la reja no juzga la version)");
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 0, "pero no vale para esta VM");
        CHECK(m && strstr(m, "version") != 0 && strstr(m, "v2") != 0, "y el motivo dice la version (v2 frente a v1)");
    }

    /* ── 6. La tabla y los nombres van A LA PAR (el generador los saca del mismo sitio) ── */
    /* ── V7/H6.c G2: el resolvedor del contrato 'PANT' ── */
    {
        bpvm_pant_t p;
        CHECK(bpvm_pant_resolver(&PANT_DOBLE, &p) == 0, "'PANT' con init/size/flush -> vale");
        CHECK(p.rotate == 0 && p.touch == 0 && p.formato == 0,
              "y las opcionales que no publica quedan a NULL (sin rotar, sin tactil, RGB565)");
        CHECK(bpvm_pant_resolver(&PANT_CON_FORMATO, &p) == 0 && p.formato && p.formato() == BPVM_PANT_RGB565_BE,
              "un driver que publica 'formato' -> se resuelve y dice RGB565 girado");
        const char* mal = bpvm_pant_resolver(&PANT_SIN_FLUSH, &p);
        CHECK(mal && strstr(mal, "'flush'") != 0, "sin 'flush' -> no vale, y el motivo NOMBRA lo que falta");
        CHECK(bpvm_pant_resolver(&bpvm_gui_api_stubs, &p) != 0,
              "una tabla de OTRA marca (la de 'LVGL') -> no vale como driver");
        CHECK(bpvm_pant_bytes_por_pixel(BPVM_PANT_RGB888) == 3 && bpvm_pant_bytes_por_pixel(BPVM_PANT_RGB565_BE) == 2,
              "bytes por pixel: RGB888 = 3, RGB565 (girado o no) = 2");
    }

    CHECK(sizeof(bpvm_gui_tabla_t) == BPVM_GUI_N_FUNCS * sizeof(void (*)(void)),
          "la struct tiene exactamente BPVM_GUI_N_FUNCS punteros (sin huecos ni relleno)");
    CHECK(strcmp(BPVM_GUI_NOMBRES[BPVM_GUI_N_FUNCS - 1], "dump_free") == 0, "el ultimo nombre es dump_free");

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
