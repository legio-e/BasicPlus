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

/* #310 — overlay de lectura (ver bpvm_fs.h). Se consulta antes del backend. */
static bpvm_fs_ov_stat_fn g_ov_stat = NULL;
static bpvm_fs_ov_read_fn g_ov_read = NULL;
static void*              g_ov_user = NULL;

void bpvm_fs_set_overlay(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd, void* user) {
    g_ov_stat = st; g_ov_read = rd; g_ov_user = user;
}

/* 1 = el overlay reclama este path (y deja el tamaño en *size). */
static int overlay_claims(const char* path, uint32_t* size) {
    return g_ov_stat && g_ov_stat(g_ov_user, path, size) == 0;
}

int bpvm_fs_stat(const char* path, uint32_t* size) {
    uint32_t osz = 0;
    if (overlay_claims(path, &osz)) { if (size) *size = osz; return 0; }
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->stat) return be->stat(path, size);
    return -1;
}

long bpvm_fs_read(const char* path, uint8_t* dst, uint32_t cap) {
    uint32_t osz = 0;
    if (overlay_claims(path, &osz) && g_ov_read)
        return g_ov_read(g_ov_user, path, 0, dst, cap);
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

/* H13 hallazgo 13(a) — DOS fallos distintos que antes valian lo mismo (-1):
 *   -1  la cintura SI implementa fechas, pero esta no se pudo leer.
 *   -2  esta cintura NO IMPLEMENTA la fecha (mtime_ms = NULL).
 *
 * OJO CON EL MATIZ, que lo puso Eduardo: -2 NO significa "littlefs no guarda
 * fechas". Significa "aqui no esta implementado" — que es lo unico cierto y lo
 * unico que seguira siendo cierto cuando MANANA una familia si lo implemente.
 * Hornear la causa concreta en el mensaje lo deja caducado el dia que cambie.
 *
 * Es la forma de PROTOCOLO para cualquier verbo de la fachada que una cintura no
 * cubra: no se contesta exito, no se inventa un valor y no se echa la culpa a
 * una implementacion — se dice "no implementado en esta plataforma". */
long long bpvm_fs_mtime_ms(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    if (!be) return -1;
    if (!be->mtime_ms) return -2;      /* no soportado por esta cintura */
    return be->mtime_ms(path);
}

/* H2·B1.3 — listado (wire LS+CRC / explorer del IDE). */
int bpvm_fs_en_raiz(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    /* El backend raíz es el del prefijo vacío. Si no hay ninguno registrado,
     * la respuesta honesta es "sí": no hay montaje que pueda atenderlo. */
    for (int i = 0; i < g_mount_count; i++) {
        if (g_mounts[i].prefix[0] == '\0') return be == g_mounts[i].be;
    }
    return 1;
}

/*
 * Los MONTAJES que cuelgan DIRECTAMENTE de `path`, emitidos como directorios.
 *
 * Hacen falta porque un punto de montaje no es un directorio de nadie: no está
 * en littlefs (ahí no existe) ni en el volumen montado (ahí es la raíz). Vive
 * en ESTA tabla y en ningún otro sitio. Sin esto, `/sd` se puede recorrer
 * escribiendo la ruta a mano pero no aparece al listar `/` — existe y no se ve,
 * que es la peor de las dos formas de no funcionar.
 *
 * "Directamente" = un solo tramo por debajo: listando `/` sale `sd`, pero un
 * hipotético `/mnt/sd` sólo saldría al listar `/mnt`.
 */
static void emite_montajes_hijos(const char* path,
                                 void (*cb)(const char*, int, uint32_t, void*),
                                 void* user) {
    size_t plen = strlen(path);
    /* La raíz se escribe "/" o "" según quién pregunte; las dos son la raíz. */
    if (plen == 1 && path[0] == '/') plen = 0;
    for (int i = 0; i < g_mount_count; i++) {
        const char* p = g_mounts[i].prefix;
        if (p[0] == '\0') continue;                     /* el backend raíz     */
        if (strcmp(p, path) == 0) continue;             /* el montaje es ESTE  */
        if (strncmp(p, path, plen) != 0) continue;      /* cuelga de otro sitio */
        /* El '/' de aquí no es cosmético: sin él, listar "/m" daría por hijo
         * suyo el montaje "/mnt/sd" — el prefijo coincide sin caer en frontera. */
        const char* resto = p + plen;
        if (resto[0] != '/' || resto[1] == '\0') continue;
        if (strchr(resto + 1, '/') != NULL) continue;   /* nieto, no hijo      */
        cb(resto + 1, 1 /* es directorio */, 0, user);
    }
}

int bpvm_fs_list(const char* path,
                 void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                 void* user) {
    const bpvm_fs_backend_t* be = route(path);
    int r = (be && be->list) ? be->list(path, cb, user) : -1;
    /* Aunque el backend haya fallado: que no se pueda leer un volumen no borra
     * los que cuelgan de él. */
    if (cb && path) emite_montajes_hijos(path, cb, user);
    return r;
}

/* #305 — lectura parcial. Sin backend que la implemente devuelve -1: el llamante
 * decide si degrada al camino de fichero entero o falla. */
/* V5/H2 — las gemelas de escritura de read_at. Como `write` y `remove`, NO
 * consultan el overlay de packs: ése es de sólo lectura y lo que se escribe va
 * al FS de verdad. */
long bpvm_fs_write_at(const char* path, uint32_t off,
                      const uint8_t* data, uint32_t len) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->write_at) return be->write_at(path, off, data, len);
    return -1;
}

int bpvm_fs_truncate(const char* path, uint32_t size) {
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->truncate) return be->truncate(path, size);
    return -1;
}

long bpvm_fs_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    uint32_t osz = 0;
    if (overlay_claims(path, &osz) && g_ov_read)
        return g_ov_read(g_ov_user, path, off, dst, cap);
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
