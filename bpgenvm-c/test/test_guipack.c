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

    /* ── 2. CON PACK: cada trampolin llega a SU funcion, por nombre ── */
    {
        bpvm_gui_pack_reset();
        CHECK(bpvm_bios_publica(BPVM_GUI_MARCA, &bpvm_gui_api_stubs) == 0, "publicar el doble bajo 'LVGL' -> 0 (la reja lo acepta)");
        const char* m = 0;
        CHECK(bpvm_gui_disponible(&m) == 1, "con pack -> disponible() = 1");
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
    CHECK(sizeof(bpvm_gui_tabla_t) == BPVM_GUI_N_FUNCS * sizeof(void (*)(void)),
          "la struct tiene exactamente BPVM_GUI_N_FUNCS punteros (sin huecos ni relleno)");
    CHECK(strcmp(BPVM_GUI_NOMBRES[BPVM_GUI_N_FUNCS - 1], "dump_free") == 0, "el ultimo nombre es dump_free");

    printf("\n[status=%s]\n", g_fail ? "FAIL" : "OK");
    return g_fail ? 1 : 0;
}
