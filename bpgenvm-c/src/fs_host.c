/*
 * fs_host.c — backend de file I/O sobre libc, para la VM C en HOST.
 *
 * HOST-ONLY: no se linka en el firmware (el device registra su propio backend
 * sobre fs_get/fs_put). Lo registra test/main.c con bpvm_fs_register_host().
 * Sin sandbox: el path se usa tal cual contra el FS del host (paridad con la
 * VM-Java cuando no hay workdir configurado).
 */
#include "bpvm_fs.h"
#include <stdio.h>
#include <string.h>

static int host_stat(const char* path, uint32_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    fclose(f);
    if (sz < 0) return -1;
    if (size) *size = (uint32_t) sz;
    return 0;
}

static long host_read(const char* path, uint8_t* dst, uint32_t cap) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(dst, 1, (size_t) cap, f);
    fclose(f);
    return (long) n;
}

static int host_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    FILE* f = fopen(path, append ? "ab" : "wb");
    if (!f) return -1;
    if (len > 0 && data) {
        if (fwrite(data, 1, (size_t) len, f) != (size_t) len) { fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

/* #240 — borrado y rename para el logger (rotación y limpieza). */
static int host_remove(const char* path) {
    return (remove(path) == 0) ? 0 : -1;
}

static int host_rename(const char* from, const char* to) {
    /* Semántica de la VM-Java (REPLACE_EXISTING): en Windows rename() falla
     * si el destino existe — lo borramos antes (en POSIX rename ya
     * sobreescribe y el remove previo es inocuo). */
    FILE* f = fopen(to, "rb");
    if (f) { fclose(f); remove(to); }
    return (rename(from, to) == 0) ? 0 : -1;
}

/* #240 (2ª pasada) — resto de IO.bp en host. fs_host.c es HOST-ONLY, así que
 * podemos salirnos de C99 puro: _mkdir/_stat en Windows, mkdir/stat POSIX. */
#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define HOST_MKDIR(p)  _mkdir(p)
#define HOST_RMDIR(p)  _rmdir(p)
typedef struct _stat host_stat_t;
#define HOST_STATFN(p, st) _stat((p), (st))
#define HOST_ISDIR(m)  (((m) & _S_IFDIR) != 0)
#else
#include <sys/stat.h>
#include <unistd.h>
#define HOST_MKDIR(p)  mkdir((p), 0777)
#define HOST_RMDIR(p)  rmdir(p)
typedef struct stat host_stat_t;
#define HOST_STATFN(p, st) stat((p), (st))
#define HOST_ISDIR(m)  S_ISDIR(m)
#endif

/* mkdir recursivo (crea intermedios; ok si ya existe — semántica VM-Java
 * Files.createDirectories). */
static int host_mkdir(const char* path) {
    char buf[512];
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(buf)) return -1;
    memcpy(buf, path, n + 1);
    for (size_t i = 1; i < n; i++) {
        if (buf[i] == '/' || buf[i] == '\\') {
            char c = buf[i];
            buf[i] = '\0';
            HOST_MKDIR(buf);   /* ignora fallo: puede existir o ser "C:" */
            buf[i] = c;
        }
    }
    if (HOST_MKDIR(buf) == 0) return 0;
    /* ya existía como directorio → ok */
    host_stat_t st;
    if (HOST_STATFN(buf, &st) == 0 && HOST_ISDIR(st.st_mode)) return 0;
    return -1;
}

static int host_rmdir(const char* path) {
    return (HOST_RMDIR(path) == 0) ? 0 : -1;   /* falla si no vacío — como Java */
}

static int host_copy(const char* from, const char* to) {
    FILE* fi = fopen(from, "rb");
    if (!fi) return -1;
    FILE* fo = fopen(to, "wb");   /* sobreescribe (REPLACE_EXISTING) */
    if (!fo) { fclose(fi); return -1; }
    char buf[4096];
    size_t n;
    int rc = 0;
    while ((n = fread(buf, 1, sizeof(buf), fi)) > 0) {
        if (fwrite(buf, 1, n, fo) != n) { rc = -1; break; }
    }
    if (ferror(fi)) rc = -1;
    fclose(fi);
    fclose(fo);
    return rc;
}

static int host_isdir(const char* path) {
    host_stat_t st;
    return (HOST_STATFN(path, &st) == 0 && HOST_ISDIR(st.st_mode)) ? 1 : 0;
}

static long long host_mtime_ms(const char* path) {
    host_stat_t st;
    if (HOST_STATFN(path, &st) != 0) return -1;
    return (long long) st.st_mtime * 1000LL;
}

static const bpvm_fs_backend_t s_host_fs = {
    .stat     = host_stat,
    .read     = host_read,
    .write    = host_write,
    .remove   = host_remove,
    .rename   = host_rename,
    .mkdir    = host_mkdir,
    .rmdir    = host_rmdir,
    .copy     = host_copy,
    .isdir    = host_isdir,
    .mtime_ms = host_mtime_ms,
};

void bpvm_fs_register_host(void) {
    bpvm_fs_set_backend(&s_host_fs);
}
