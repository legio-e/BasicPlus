/* gui_pack_glue.c — lo que gui.c y LVGL PIDEN de la VM, servido desde la BIOS
 * (V7/H6 P3). gui.c es el MISMO fichero que corre en el host y en las imágenes
 * con LVGL dentro; lo único que cambia en el pack es quién está detrás de estos
 * seis nombres:
 *
 *   bpvm_alloc_raw / bpvm_realloc_at / bpvm_free   -> el TLSF de LVGL (su pool)
 *   bpvm_diag                                      -> bios->log (formateado con lv_vsnprintf)
 *   bpvm_fs_stat / read / write / remove / exists / isdir / resolve
 *                                                  -> las ranuras de ficheros de la BIOS
 *   __assert_func (newlib) y el assert de LVGL     -> el log, y se sigue
 *
 * Medido (nm -u de liblvgl.a, 22-sep): con la stdlib interna LVGL sólo deja
 * abiertos libgcc y assert; gui.c deja estos. Nada de malloc de la BIOS. */
#include "bpvm_bios.h"
#include "bpvm_alloc.h"
#include "bpvm_fs.h"
#include "lvgl.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

extern const bpvm_bios_t* g_bios;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

/* ── memoria: el pool de LVGL ─────────────────────────────────────────────── */
void* bpvm_alloc_raw(size_t n, int zero, bpvm_alloc_kind_t k, const char* file, int line) {
    (void) k; (void) file; (void) line;
    void* p = zero ? lv_malloc_zeroed(n) : lv_malloc(n);
    if (p == 0) di("gui: SIN MEMORIA en el pool del pack (sube BPVM_PACK_LV_MEM_KB)");
    return p;
}
void* bpvm_realloc_at(void* p, size_t n, const char* file, int line) { (void) file; (void) line; return lv_realloc(p, n); }
void  bpvm_free(void* p) { lv_free(p); }

/* ── la voz: bpvm_diag -> log de la BIOS ──────────────────────────────────── */
void bpvm_diag(const char* fmt, ...) {
    char buf[200];
    va_list ap; va_start(ap, fmt);
    lv_vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    di(buf);
}

/* ── ficheros: por descriptor de la BIOS, con la forma por camino de la fachada ── */
int bpvm_fs_exists(const char* path) { return (g_bios && path) ? g_bios->existe(path) : 0; }
int bpvm_fs_isdir(const char* path)  { (void) path; return 0; }   /* la BIOS no distingue: un directorio no es un PNG */
int bpvm_fs_stat(const char* path, uint32_t* size) {
    if (!g_bios || !path) return -1;
    int fd = g_bios->abrir(path, 0);
    if (fd < 0) return -1;
    long t = g_bios->tamano(fd);
    g_bios->cerrar(fd);
    if (t < 0) return -1;
    if (size) *size = (uint32_t) t;
    return 0;
}
long bpvm_fs_read(const char* path, uint8_t* dst, uint32_t cap) {
    if (!g_bios || !path) return -1;
    int fd = g_bios->abrir(path, 0);
    if (fd < 0) return -1;
    long t = g_bios->tamano(fd);
    long n = -1;
    if (t >= 0) n = g_bios->leer(fd, 0, dst, (uint32_t) ((uint32_t) t < cap ? (uint32_t) t : cap));
    g_bios->cerrar(fd);
    return n;
}
int bpvm_fs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    if (!g_bios || !path) return -1;
    int fd = g_bios->abrir(path, 1);
    if (fd < 0) return -1;
    uint32_t desde = 0;
    if (append) { long t = g_bios->tamano(fd); desde = (t > 0) ? (uint32_t) t : 0; }
    else        { g_bios->truncar(fd, 0); }
    long n = g_bios->escribir(fd, desde, data, len);
    g_bios->cerrar(fd);
    return (n == (long) len) ? 0 : -1;
}
int bpvm_fs_remove(const char* path) { return (g_bios && path) ? g_bios->borrar(path) : -1; }
/* El camino ya llega RESUELTO desde la VM (builtins.c lo resuelve antes de cruzar,
 * como hace con los recursos del pack); aquí sólo se devuelve tal cual. */
const char* bpvm_fs_resolve(const char* path, char* out, size_t outsz) { (void) out; (void) outsz; return path; }

/* ── la libc que gui.c y el propio gcc piden, sobre la BIOS (como sqlite_pack.c) ──
 * gcc emite llamadas a memcpy/memset para copiar structs aunque se compile con
 * -fno-builtin -ffreestanding; el resto lo llama gui.c a mano. snprintf y
 * printf van por el sprintf interno de LVGL: printf (un aviso de loadFont) al log. */
#include <stddef.h>
void*  memcpy (void* d, const void* s, size_t n)       { return g_bios->memcpy(d, s, n); }
void*  memmove(void* d, const void* s, size_t n)       { return g_bios->memmove(d, s, n); }
void*  memset (void* d, int c, size_t n)               { return g_bios->memset(d, c, n); }
int    memcmp (const void* a, const void* b, size_t n) { return g_bios->memcmp(a, b, n); }
size_t strlen (const char* s)                          { return g_bios->strlen(s); }
int    strcmp (const char* a, const char* b)           { return g_bios->strcmp(a, b); }
int    strncmp(const char* a, const char* b, size_t n) { return g_bios->strncmp(a, b, n); }
char*  strchr (const char* s, int c)                   { return g_bios->strchr(s, c); }
int snprintf(char* buf, size_t n, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = lv_vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
int printf(const char* fmt, ...) {
    char buf[200];
    va_list ap; va_start(ap, fmt);
    int r = lv_vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    di(buf);
    return r;
}

/* ── asserts: se cuentan, no se cuelga ────────────────────────────────────── */
void bpvm_pack_lv_assert(const char* fichero, int linea) {
    char buf[160];
    lv_snprintf(buf, sizeof buf, "gui: LV_ASSERT en %s:%d", fichero ? fichero : "?", linea);
    di(buf);
}
void __assert_func(const char* file, int line, const char* func, const char* expr) {
    char buf[200];
    lv_snprintf(buf, sizeof buf, "gui: assert(%s) en %s:%d (%s)", expr ? expr : "?", file ? file : "?", line, func ? func : "?");
    di(buf);
    for (;;) { }   /* newlib lo declara noreturn: aquí se para, ya está dicho por qué */
}
