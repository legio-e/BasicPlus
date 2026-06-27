/*
 * fs_facade.c — despacho de file I/O al backend de plataforma (patrón gpio.c).
 *
 * Sin backend registrado, las operaciones fallan limpio (-1 / 0) y el builtin
 * lanza un RuntimeError BP. Cada plataforma registra su backend: el host con
 * libc (fs_host.c), el firmware sobre su FS (fs_get/fs_put).
 */
#include "bpvm_fs.h"
#include <string.h>
#include <stdio.h>

static const bpvm_fs_backend_t* g_fs = NULL;

void bpvm_fs_set_backend(const bpvm_fs_backend_t* backend) {
    g_fs = backend;
}

/* ── H19-F1 — base-dir por ejecución ─────────────────────────────────────── */
static char g_basedir[256] = "";   /* "" = sin proyecto (modo plano) */

void bpvm_fs_set_basedir(const char* dir) {
    if (!dir || !dir[0]) { g_basedir[0] = '\0'; return; }
    size_t n = strlen(dir);
    while (n > 1 && dir[n - 1] == '/') n--;          /* quita '/' final(es) */
    if (n >= sizeof(g_basedir)) n = sizeof(g_basedir) - 1;
    memcpy(g_basedir, dir, n);
    g_basedir[n] = '\0';
}

const char* bpvm_fs_basedir(void) { return g_basedir; }

void bpvm_fs_set_basedir_from_module(const char* modpath) {
    /* "/app/<proj>/entry.mod" → "/app/<proj>" ; cualquier otra cosa → plano. */
    if (modpath && strncmp(modpath, "/app/", 5) == 0) {
        const char* rest  = modpath + 5;             /* "<proj>/entry.mod" */
        const char* slash = strchr(rest, '/');       /* 1er '/' tras <proj> */
        if (slash && slash > rest) {
            char dir[256];
            size_t n = (size_t)(slash - modpath);    /* "/app/<proj>" */
            if (n >= sizeof(dir)) n = sizeof(dir) - 1;
            memcpy(dir, modpath, n);
            dir[n] = '\0';
            bpvm_fs_set_basedir(dir);
            return;
        }
    }
    bpvm_fs_set_basedir(NULL);                        /* plano */
}

const char* bpvm_fs_resolve(const char* path, char* out, size_t outsz) {
    if (!path || outsz == 0) { if (outsz) out[0] = '\0'; return out; }
    if (path[0] == '/') {                             /* absoluto → tal cual */
        snprintf(out, outsz, "%s", path);
        return out;
    }
    uint32_t sz = 0;
    if (g_basedir[0]) {                               /* (1) proyecto */
        snprintf(out, outsz, "%s/%s", g_basedir, path);
        if (bpvm_fs_stat(out, &sz) == 0) return out;
    }
    snprintf(out, outsz, "%s", path);                 /* (2) tal cual (cwd/literal) */
    if (bpvm_fs_stat(out, &sz) == 0) return out;
    snprintf(out, outsz, "/app/%s", path);            /* (3) modo plano */
    if (bpvm_fs_stat(out, &sz) == 0) return out;
    /* nada existe → mejor candidata para el error (proyecto si lo hay). */
    if (g_basedir[0]) snprintf(out, outsz, "%s/%s", g_basedir, path);
    return out;
}

int bpvm_fs_stat(const char* path, uint32_t* size) {
    if (g_fs && g_fs->stat) return g_fs->stat(path, size);
    return -1;
}

long bpvm_fs_read(const char* path, uint8_t* dst, uint32_t cap) {
    if (g_fs && g_fs->read) return g_fs->read(path, dst, cap);
    return -1;
}

int bpvm_fs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    if (g_fs && g_fs->write) return g_fs->write(path, data, len, append);
    return -1;
}

int bpvm_fs_exists(const char* path) {
    uint32_t sz = 0;
    return (bpvm_fs_stat(path, &sz) == 0) ? 1 : 0;
}

/* #240 — ops opcionales del backend (logger: rotación y limpieza). */
int bpvm_fs_remove(const char* path) {
    if (g_fs && g_fs->remove) return g_fs->remove(path);
    return -1;
}

int bpvm_fs_rename(const char* from, const char* to) {
    if (g_fs && g_fs->rename) return g_fs->rename(from, to);
    return -1;
}

/* #240 (2ª pasada) — resto de IO.bp. */
int bpvm_fs_mkdir(const char* path) {
    if (g_fs && g_fs->mkdir) return g_fs->mkdir(path);
    return -1;
}

int bpvm_fs_rmdir(const char* path) {
    if (g_fs && g_fs->rmdir) return g_fs->rmdir(path);
    return -1;
}

int bpvm_fs_copy(const char* from, const char* to) {
    if (g_fs && g_fs->copy) return g_fs->copy(from, to);
    return -1;
}

int bpvm_fs_isdir(const char* path) {
    if (g_fs && g_fs->isdir) return g_fs->isdir(path);
    return 0;
}

long long bpvm_fs_mtime_ms(const char* path) {
    if (g_fs && g_fs->mtime_ms) return g_fs->mtime_ms(path);
    return -1;
}
