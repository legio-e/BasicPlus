/*
 * fs_facade.c — despacho de file I/O al backend de plataforma (patrón gpio.c).
 *
 * Sin backend registrado, las operaciones fallan limpio (-1 / 0) y el builtin
 * lanza un RuntimeError BP. Cada plataforma registra su backend: el host con
 * libc (fs_host.c), el firmware sobre su FS (fs_get/fs_put).
 */
#include "bpvm_fs.h"
#include "crc32.h"        /* #305 — bpvm_fs_crc32 por trozos */
#include <string.h>
#include <stdio.h>

/* ── H2·B1.3 — tabla de MONTAJES (andamiaje multi-motor, fase B lista) ─────
 * Entrada 0 = backend RAÍZ (prefijo ""); las demás, prefijos tipo "/sd".
 * route() elige el prefijo coincidente MÁS LARGO (un prefijo coincide si el
 * path empieza por él y sigue '/' o fin). Con solo la raíz montada, el ruteo
 * es transparente = comportamiento de siempre. */
#define BPVM_FS_MAX_MOUNTS 4

typedef struct {
    char prefix[32];                 /* "" = raíz */
    const bpvm_fs_backend_t* be;
} fs_mount_t;

static fs_mount_t g_mounts[BPVM_FS_MAX_MOUNTS];
static int        g_mount_count = 0;

void bpvm_fs_set_backend(const bpvm_fs_backend_t* backend) {
    /* backend raíz nuevo → borra los montajes (arranque limpio / tests) */
    g_mount_count = 0;
    if (backend) {
        g_mounts[0].prefix[0] = '\0';
        g_mounts[0].be = backend;
        g_mount_count = 1;
    }
}

int bpvm_fs_mount(const char* prefix, const bpvm_fs_backend_t* backend) {
    if (!prefix || prefix[0] != '/' || !backend) return -1;
    size_t n = strlen(prefix);
    if (n >= sizeof(g_mounts[0].prefix)) return -1;
    for (int i = 0; i < g_mount_count; i++) {          /* re-montar = actualizar */
        if (strcmp(g_mounts[i].prefix, prefix) == 0) { g_mounts[i].be = backend; return 0; }
    }
    if (g_mount_count >= BPVM_FS_MAX_MOUNTS) return -1;
    memcpy(g_mounts[g_mount_count].prefix, prefix, n + 1);
    g_mounts[g_mount_count].be = backend;
    g_mount_count++;
    return 0;
}

/* backend que atiende `path` (prefijo coincidente más largo; raíz = fallback) */
static const bpvm_fs_backend_t* route(const char* path) {
    const bpvm_fs_backend_t* best = NULL;
    size_t best_len = 0;
    int have_best = 0;
    if (!path) path = "";
    for (int i = 0; i < g_mount_count; i++) {
        const char* p = g_mounts[i].prefix;
        size_t n = strlen(p);
        if (n == 0) {                                   /* raíz: match universal */
            if (!have_best) { best = g_mounts[i].be; best_len = 0; have_best = 1; }
            continue;
        }
        if (strncmp(path, p, n) == 0 && (path[n] == '/' || path[n] == '\0')) {
            if (!have_best || n > best_len) { best = g_mounts[i].be; best_len = n; have_best = 1; }
        }
    }
    return have_best ? best : NULL;
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

static char g_mainmod[256] = "";   /* ruta COMPLETA del módulo principal (entry) */

void bpvm_fs_set_main_module_path(const char* path) {
    if (!path || !path[0]) { g_mainmod[0] = '\0'; return; }
    size_t n = strlen(path);
    if (n >= sizeof(g_mainmod)) n = sizeof(g_mainmod) - 1;
    memcpy(g_mainmod, path, n);
    g_mainmod[n] = '\0';
}

const char* bpvm_fs_main_module_path(void) { return g_mainmod; }

void bpvm_fs_set_basedir_from_module(const char* modpath) {
    bpvm_fs_set_main_module_path(modpath);   /* guarda la ruta completa del entry */
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
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->stat) return be->stat(path, size);
    return -1;
}

long bpvm_fs_read(const char* path, uint8_t* dst, uint32_t cap) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->read) return be->read(path, dst, cap);
    return -1;
}

int bpvm_fs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->write) return be->write(path, data, len, append);
    return -1;
}

int bpvm_fs_exists(const char* path) {
    uint32_t sz = 0;
    return (bpvm_fs_stat(path, &sz) == 0) ? 1 : 0;
}

/* #240 — ops opcionales del backend (logger: rotación y limpieza). */
int bpvm_fs_remove(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->remove) return be->remove(path);
    return -1;
}

int bpvm_fs_rename(const char* from, const char* to) {
    /* B1.3: entre montajes distintos → -1 (mover cross-motor = fase B). */
    const bpvm_fs_backend_t* be = route(from);
    if (be != route(to)) return -1;
    if (be && be->rename) return be->rename(from, to);
    return -1;
}

/* #240 (2ª pasada) — resto de IO.bp. */
int bpvm_fs_mkdir(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->mkdir) return be->mkdir(path);
    return -1;
}

int bpvm_fs_rmdir(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->rmdir) return be->rmdir(path);
    return -1;
}

int bpvm_fs_copy(const char* from, const char* to) {
    /* B1.3: entre montajes distintos → -1 (copiar cross-motor = fase B,
     * trivial vía read+write cuando haga falta). */
    const bpvm_fs_backend_t* be = route(from);
    if (be != route(to)) return -1;
    if (be && be->copy) return be->copy(from, to);
    return -1;
}

int bpvm_fs_isdir(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->isdir) return be->isdir(path);
    return 0;
}

long long bpvm_fs_mtime_ms(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->mtime_ms) return be->mtime_ms(path);
    return -1;
}

/* H2·B1.3 — listado (wire LS+CRC / explorer del IDE). */
int bpvm_fs_list(const char* path,
                 void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                 void* user) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->list) return be->list(path, cb, user);
    return -1;
}

/* #305 — lectura parcial. Sin backend que la implemente devuelve -1: el llamante
 * decide si degrada al camino de fichero entero o falla. */
long bpvm_fs_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->read_at) return be->read_at(path, off, dst, cap);
    return -1;
}

/* #305 — CRC-32 de un fichero POR TROZOS. El buffer es de pila y pequeño a
 * propósito: es lo que sustituye al scratch del tamaño del fichero mayor. 256 B
 * es el mismo tamaño que ya usa littlefs para sus buffers de lectura, así que no
 * introduce un número nuevo que cuadrar. */
int bpvm_fs_crc32(const char* path, uint32_t* crc_out) {
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) return -1;
    uint32_t st = BPVM_CRC32_INIT;
    uint8_t  buf[256];
    uint32_t off = 0;
    while (off < size) {
        long n = bpvm_fs_read_at(path, off, buf, sizeof buf);
        if (n <= 0) return -1;               /* incluye "backend sin read_at" */
        st = bpvm_crc32_update(st, buf, (size_t) n);
        off += (uint32_t) n;
    }
    if (crc_out) *crc_out = bpvm_crc32_final(st);
    return 0;
}
