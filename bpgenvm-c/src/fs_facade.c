/*
 * fs_facade.c — despacho de file I/O al backend de plataforma (patrón gpio.c).
 *
 * Sin backend registrado, las operaciones fallan limpio (-1 / 0) y el builtin
 * lanza un RuntimeError BP. Cada plataforma registra su backend: el host con
 * libc (fs_host.c), el firmware sobre su FS (fs_get/fs_put).
 */
#include "bpvm_fs.h"

static const bpvm_fs_backend_t* g_fs = NULL;

void bpvm_fs_set_backend(const bpvm_fs_backend_t* backend) {
    g_fs = backend;
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
