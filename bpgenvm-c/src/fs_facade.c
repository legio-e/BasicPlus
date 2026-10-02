/*
 * fs_facade.c — despacho de file I/O al backend de plataforma (patrón gpio.c).
 *
 * Sin backend registrado, las operaciones fallan limpio (-1 / 0) y el builtin
 * lanza un RuntimeError BP. Cada plataforma registra su backend: el host con
 * libc (fs_host.c), el firmware sobre su FS (fs_get/fs_put).
 */
#include "bpvm_fs.h"
#include "crc32.h"        /* #305 — bpvm_fs_crc32 por trozos */
#include "bpvm_log.h"     /* #628 — la línea del fallo la escribe quien guarda el motivo */
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
    /* V7/H3 — `/sd` es un prefijo RESERVADO (decisión del 23-ago, hecha hoy): sin
     * tarjeta montada, una ruta bajo /sd FALLA en vez de caer en silencio a la
     * flash interna. Antes, `writeFile("/sd/log.txt")` sin tarjeta escribía en
     * littlefs un fichero que nadie encontraba luego en la SD. */
    if (best_len == 0 && strncmp(path, BPVM_FS_PREFIJO_SD, sizeof(BPVM_FS_PREFIJO_SD) - 1) == 0) {
        char c = path[sizeof(BPVM_FS_PREFIJO_SD) - 1];
        if (c == '/' || c == '\0') return NULL;
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

static int fallback_locate(const char* nombre, char* out, size_t cap);   /* V7/H2 (#493) */

const char* bpvm_fs_resolve(const char* path, char* out, size_t outsz) {
    if (!path || outsz == 0) { if (outsz) out[0] = '\0'; return out; }
    if (path[0] == '/') {                             /* absoluto → tal cual */
        snprintf(out, outsz, "%s", path);
        return out;
    }
    /* #362 — un path CON ESQUEMA ("algo:loquesea") es una dirección exacta, no
     * una ruta relativa que haya que ir buscando: se deja intacta. La fachada
     * no necesita saber QUÉ esquemas hay —eso es de quien los sirve—, sólo que
     * a una dirección no se le pone delante el base-dir.
     *
     * Sin esto, "pack:Iconos/logo.png" se probaría primero como
     * "<basedir>/pack:Iconos/logo.png"; quien resuelve se queda con el
     * basename, el prefijo se perdería por el camino y el recurso saldría del
     * pack que tocara — lo contrario de lo que se pidió. De regalo, en el host
     * un "C:/..." deja de tratarse como relativo. */
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') break;           /* ya es ruta: no hay esquema */
        if (*p == ':') { snprintf(out, outsz, "%s", path); return out; }
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
    /* (4) [V7/H2, #493] el RESPALDO (la zona de packs), a propósito y el
     * último: si algún pack tiene un fichero con ese nombre, devuelve su ruta
     * cualificada (`pack:<Pack>/<path>`), que es la única a la que el respaldo
     * contesta desde hoy. Antes esto no hacía falta porque la zona reclamaba
     * `<basedir>/<path>` en el paso (1) — y por ese mismo disfraz tapaba a /lib
     * y mataba el XIP. */
    if (fallback_locate(path, out, outsz)) return out;
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

/* #362 — respaldo de lectura: el espejo del overlay, por DEBAJO del backend. */
static bpvm_fs_ov_stat_fn   g_fb_stat   = NULL;
static bpvm_fs_ov_read_fn   g_fb_read   = NULL;
static bpvm_fs_ov_locate_fn g_fb_locate = NULL;    /* V7/H2 (#493) */
static void*                g_fb_user   = NULL;

void bpvm_fs_set_fallback(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd,
                          bpvm_fs_ov_locate_fn locate, void* user) {
    g_fb_stat = st; g_fb_read = rd; g_fb_locate = locate; g_fb_user = user;
}

/* 1 = el respaldo tiene un fichero con ese nombre y deja su ruta CUALIFICADA en
 * out. Es la pregunta «¿alguien lo tiene?», separada de «¿es tuya esta ruta?». */
static int fallback_locate(const char* nombre, char* out, size_t cap) {
    return g_fb_locate && g_fb_locate(g_fb_user, nombre, out, cap) == 0;
}

/* ¿El backend de esta ruta sabe leer en secuencia? (V7/H2, #493) */
static int be_rs_op_ok(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    return be && be->read_stream;
}

/* 1 = el overlay reclama este path (y deja el tamaño en *size). */
static int overlay_claims(const char* path, uint32_t* size) {
    return g_ov_stat && g_ov_stat(g_ov_user, path, size) == 0;
}

/* Ídem para el respaldo. Mismo contrato: 0 = es suyo. */
static int fallback_claims(const char* path, uint32_t* size) {
    return g_fb_stat && g_fb_stat(g_fb_user, path, size) == 0;
}

/* ── #628 — EL MOTIVO DEL ÚLTIMO FALLO (el contrato, en bpvm_fs.h) ──────────
 * Lo anota el que falla: el motor, con `bpvm_fs_anota_fallo`, o la fachada. Los
 * textos son fijos (literales): se guardan por puntero y el log los lee después. */
static bpvm_fs_fail_t s_fallo       = BPVM_FS_FAIL_OTHER;
static int            s_fallo_crudo = 0;
static const char*    s_fallo_motor = "fs";
static const char*    s_fallo_texto = "sin fallos";
static uint32_t       s_fallo_n     = 0;   /* cuántos van anotados: si no se mueve, nadie dijo nada */

void bpvm_fs_anota_fallo(const char* motor, int crudo, bpvm_fs_fail_t clase, const char* texto) {
    s_fallo_motor = motor ? motor : "fs";
    s_fallo_crudo = crudo;
    s_fallo       = clase;
    s_fallo_texto = texto ? texto : "";
    s_fallo_n++;
}

bpvm_fs_fail_t bpvm_fs_last_fail(void) { return s_fallo; }

void bpvm_fs_log_fail(const char* op, const char* path, unsigned long size,
                      unsigned long libre, unsigned long total) {
    log_printf("fs: %s '%s' (%lu B) FALLO %s=%d %s  [libre %lu/%lu B]",
               op, path, size, s_fallo_motor, s_fallo_crudo, s_fallo_texto, libre, total);
}

/* Un fallo de la propia fachada. -1, para el `return`. */
static int fs_falla(bpvm_fs_fail_t clase, const char* texto) {
    bpvm_fs_anota_fallo("fs", 0, clase, texto);
    return -1;
}

/* LA GARANTÍA: cada fallo lleva SU motivo. `n0` es la cuenta al empezar; si la
 * operación falla y nadie anotó nada entretanto, anota la fachada. Así un fallo
 * nunca hereda el motivo de otro, y un camino que se olvide de anotar sale «sin
 * detalle», no con un motivo falso. Un éxito no escribe nada. */
#define SIN_DETALLE "el motor no dice por que"
static long con_motivo(long r, uint32_t n0, bpvm_fs_fail_t clase, const char* texto) {
    if (r < 0 && s_fallo_n == n0) bpvm_fs_anota_fallo("fs", 0, clase, texto);
    return r;
}
static long sin_motivo(long r, uint32_t n0) { return con_motivo(r, n0, BPVM_FS_FAIL_OTHER, SIN_DETALLE); }

/* Sin motor que atienda la ruta (p. ej. /sd sin tarjeta), o con motor y sin esa operación. */
static const char* falta(const bpvm_fs_backend_t* be) {
    return be ? "esa unidad no lo hace" : "ninguna unidad atiende esa ruta";
}

int bpvm_fs_stat(const char* path, uint32_t* size) {
    uint32_t osz = 0, n0 = s_fallo_n;
    if (overlay_claims(path, &osz)) { if (size) *size = osz; return 0; }
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->stat && be->stat(path, size) == 0) return 0;
    if (fallback_claims(path, &osz)) { if (size) *size = osz; return 0; }
    return (int) con_motivo(-1, n0, BPVM_FS_FAIL_OTHER, (be && be->stat) ? SIN_DETALLE : falta(be));
}

long bpvm_fs_read(const char* path, uint8_t* dst, uint32_t cap) {
    uint32_t osz = 0, n0 = s_fallo_n;
    if (overlay_claims(path, &osz) && g_ov_read)
        return sin_motivo(g_ov_read(g_ov_user, path, 0, dst, cap), n0);
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->read) {
        long n = be->read(path, dst, cap);
        if (n >= 0) return n;
    }
    if (fallback_claims(path, &osz) && g_fb_read)
        return sin_motivo(g_fb_read(g_fb_user, path, 0, dst, cap), n0);
    return con_motivo(-1, n0, BPVM_FS_FAIL_OTHER, (be && be->read) ? SIN_DETALLE : falta(be));
}

int bpvm_fs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->write) return (int) sin_motivo(be->write(path, data, len, append), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

int bpvm_fs_exists(const char* path) {
    uint32_t sz = 0;
    return (bpvm_fs_stat(path, &sz) == 0) ? 1 : 0;
}

/* #240 — ops opcionales del backend (logger: rotación y limpieza). */
int bpvm_fs_remove(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->remove) return (int) sin_motivo(be->remove(path), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

int bpvm_fs_rename(const char* from, const char* to) {
    /* B1.3: entre montajes distintos → -1 (mover cross-motor = fase B). */
    const bpvm_fs_backend_t* be = route(from);
    if (be != route(to)) return fs_falla(BPVM_FS_FAIL_OTHER, "entre unidades distintas no se mueve ni se copia");
    uint32_t n0 = s_fallo_n;
    if (be && be->rename) return (int) sin_motivo(be->rename(from, to), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

/* #240 (2ª pasada) — resto de IO.bp. */
int bpvm_fs_mkdir(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->mkdir) return (int) sin_motivo(be->mkdir(path), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

int bpvm_fs_rmdir(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->rmdir) return (int) sin_motivo(be->rmdir(path), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

int bpvm_fs_copy(const char* from, const char* to) {
    /* B1.3: entre montajes distintos → -1 (copiar cross-motor = fase B,
     * trivial vía read+write cuando haga falta). */
    const bpvm_fs_backend_t* be = route(from);
    if (be != route(to)) return fs_falla(BPVM_FS_FAIL_OTHER, "entre unidades distintas no se mueve ni se copia");
    uint32_t n0 = s_fallo_n;
    if (be && be->copy) return (int) sin_motivo(be->copy(from, to), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
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

/* ── #461 — CONTAR LOS FICHEROS, sin guardar sus nombres ─────────────────────
 *
 * Sustituye los TRES recorridos planos que tenian Pico, ESP32 y STM32 (`fs_list`
 * + `dir_snapshot_t`), que desde que `LIST` vive en el comun (`U3.23`) ya solo
 * servian para esto: devolver un numero.
 *
 * Y guardaban 96 nombres de 64 caracteres para ello — en el ESP32, `static`, o
 * sea 6 KB de `.bss` PERMANENTES para contar. Contar no necesita los nombres:
 * el callback corre bajo el cerrojo del FS y ahi incrementar es seguro (lo que
 * no lo era, y por eso existia el snapshot, era EMITIR al wire). Lo unico que
 * hay que recordar son los DIRECTORIOS por visitar, porque descender desde
 * dentro del callback reentraria el cerrojo.
 *
 * Esa cola va con los nombres PEGADOS en un pool, no en ranuras de tamano fijo:
 * los paths reales de una placa miden 13,3 de media (medido sobre los `.mod` de `/lib`)
 * y reservar 64 por ranura desperdiciaba el 78 %. Criterio de Eduardo: «si la
 * mayoria son de 16 caracteres y puede haber una de 128, ponerlas todas al
 * tamano mayor es un derroche de RAM que no tenemos».
 *
 * Va en la PILA (352 B) y no en `.bss`: se usa durante la cuenta y punto. */
#define CNT_MAX_DIRS  16
#define CNT_POOL      320

typedef struct {
    int      ficheros;
    char*    pool;
    uint16_t off[CNT_MAX_DIRS];
    int      n;          /* directorios encolados            */
    int      usado;      /* bytes gastados del pool           */
    int      omitidos;   /* los que no cupieron: por ranuras O por pool */
    const char* dir;     /* el directorio en curso            */
} bpvm_cnt_ctx_t;

static void bpvm_cnt_cb(const char* name, int is_dir, uint32_t size, void* user) {
    bpvm_cnt_ctx_t* c = (bpvm_cnt_ctx_t*) user;
    (void) size;
    if (!is_dir) { c->ficheros++; return; }

    /* Un subdirectorio: se apunta para visitarlo despues. */
    int raiz = (c->dir[1] == '\0');
    int len  = (int) strlen(c->dir) + (raiz ? 0 : 1) + (int) strlen(name) + 1;
    if (c->n >= CNT_MAX_DIRS || c->usado + len > CNT_POOL) {
        /* #425 — un directorio sin recorrer TAMBIEN falta, y las dos razones
         * (sin ranura / sin pool) cuentan igual. Un total incompleto que no lo
         * diga es peor que un tope bajo. */
        c->omitidos++;
        return;
    }
    snprintf(c->pool + c->usado, (size_t)(CNT_POOL - c->usado), "%s%s%s",
             c->dir, raiz ? "" : "/", name);
    c->off[c->n++] = (uint16_t) c->usado;
    c->usado += len;
}

int bpvm_fs_count_files(void) {
    char pool[CNT_POOL];
    bpvm_cnt_ctx_t c;
    memset(&c, 0, sizeof c);
    c.pool = pool;

    snprintf(pool, sizeof pool, "/");
    c.off[c.n++] = 0;
    c.usado = 2;

    for (int i = 0; i < c.n; i++) {
        char dir[CNT_POOL];
        snprintf(dir, sizeof dir, "%s", pool + c.off[i]);  /* copia: sin aliasing */
        c.dir = dir;
        (void) bpvm_fs_list(dir, bpvm_cnt_cb, &c);   /* un volumen caido no borra el resto */
    }
    /* Deja rastro de que falta algo. Era `bpvm_fs_log_fail`, que escribe el motivo del ÚLTIMO fallo del FS —nada que
     * ver con esto— y ataba la fachada a littlefs: test-crc, sin enlazar desde el 31-ago (#628). */
    if (c.omitidos > 0)
        log_printf("fs: count '/': %d carpeta(s) sin recorrer (la cola de %d se lleno): el total es un minimo",
                   c.omitidos, CNT_MAX_DIRS);
    return c.ficheros;
}

int bpvm_fs_list(const char* path,
                 void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                 void* user) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    int r = (be && be->list) ? (int) sin_motivo(be->list(path, cb, user), n0)
                             : fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
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
    uint32_t n0 = s_fallo_n;
    if (be && be->write_at) return sin_motivo(be->write_at(path, off, data, len), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

int bpvm_fs_truncate(const char* path, uint32_t size) {
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->truncate) return (int) sin_motivo(be->truncate(path, size), n0);
    return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));
}

long bpvm_fs_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap) {
    uint32_t osz = 0, n0 = s_fallo_n;
    if (overlay_claims(path, &osz) && g_ov_read)
        return sin_motivo(g_ov_read(g_ov_user, path, off, dst, cap), n0);
    const bpvm_fs_backend_t* be = route(path);
    if (be && be->read_at) {
        long n = be->read_at(path, off, dst, cap);
        if (n >= 0) return n;
    }
    /* EL FALLBACK, que aquí FALTABA (16-ago). `stat` y `read` lo consultan
     * desde V5/H4 y esto no, así que un fichero de la ZONA DE PACKS existía
     * para `stat` —con su tamaño— y no se podía leer por trozos.
     *
     * No es teórico y costó una tarde: con el SQLite.pack grabado en la P4, el
     * resolutor probaba `/app/SQLite.mod`, el `stat` decía que sí (8325 B, los
     * del pack, aunque en `/app` no hubiera nada), y la carga —que va por
     * `read_at` desde #305— moría con `IO error`. De propina, el firmware
     * avisaba de que «el FS eclipsa al del pack» cuando no había ningún fichero
     * en el FS: el que reclamaba el `stat` era el pack mismo.
     *
     * La regla que se restablece es la única que hace usable una fachada: **si
     * `stat` dice que un fichero existe, se tiene que poder leer, por trozos o
     * entero**. Que dos de las tres funciones consultaran una capa y la tercera
     * no era la incoherencia; `read_at` llegó en #305 y el fallback en H4, y
     * nunca se juntaron. */
    if (fallback_claims(path, &osz) && g_fb_read)
        return sin_motivo(g_fb_read(g_fb_user, path, off, dst, cap), n0);
    return con_motivo(-1, n0, BPVM_FS_FAIL_OTHER, (be && be->read_at) ? SIN_DETALLE : falta(be));
}

/* #305 — CRC-32 de un fichero POR TROZOS. El buffer es de pila y pequeño a
 * propósito: es lo que sustituye al scratch del tamaño del fichero mayor. 256 B
 * es el mismo tamaño que ya usa littlefs para sus buffers de lectura, así que no
 * introduce un número nuevo que cuadrar. */
/* #453 — LA MISMA HISTORIA QUE `crc32`, con el segundo llamador.
 *
 * Cuando se arreglo el CRC (#398/#424) se anadio esa op al backend porque era
 * el caso cronometrado; el OTRO bucle de `read_at` -- el `GET` del wire -- se
 * quedo igual. Medido el 30-ago en un S3: un fichero de 9 KB (36 trozos) se
 * abre, y uno de 120 KB (472 trozos) NO -- se pasa de los 10 s de timeout del
 * IDE. Mismo codigo y misma placa: solo cambia el tamano.
 *
 * Si el backend sabe leer en secuencia, que lo haga el (UNA apertura). Si no,
 * el bucle de siempre: mas lento, pero correcto -- y es lo que ya habia. */
long bpvm_fs_read_stream(const char* path, bpvm_fs_chunk_cb cb, void* user) {
    if (!cb) return fs_falla(BPVM_FS_FAIL_OTHER, "argumentos no validos");
    /* [V7/H2, #493] OVERLAY Y RESPALDO TAMBIÉN AQUÍ. Esta op y `crc32` iban
     * directas al backend si éste las traía, y en littlefs (placa y sim) las
     * trae: un fichero de la zona de packs tenía `stat` pero el `GET` del wire
     * mandaba la cabecera bulk y luego NADA, y `STAT{crc}` daba -1. Medido en
     * el sim el 15-sep. La regla es la misma que restableció `read_at` el
     * 16-ago: si `stat` dice que existe, se tiene que poder leer, como sea. */
    uint32_t osz = 0;
    if (!overlay_claims(path, &osz) && be_rs_op_ok(path)) {
        const bpvm_fs_backend_t* be_rs = route(path);
        /* Si el backend TIENE el fichero, su read_stream manda, con su error
         * incluido: repetir por el bucle un fichero que falló a medias mandaría
         * los primeros trozos dos veces (la revisión lo señaló). Al bucle sólo
         * se cae cuando el backend no lo tiene (entonces es del respaldo). */
        uint32_t bsz = 0;
        if (be_rs->stat && be_rs->stat(path, &bsz) == 0) {
            uint32_t n0 = s_fallo_n;
            return sin_motivo(be_rs->read_stream(path, cb, user), n0);
        }
    }

    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) return -1;
    uint8_t  buf[256];
    uint32_t off = 0;
    while (off < size) {
        uint32_t n0 = s_fallo_n;
        long n = bpvm_fs_read_at(path, off, buf, sizeof buf);
        if (n <= 0)                          /* incluye "backend sin read_at" (ya anotado) */
            return con_motivo(-1, n0, BPVM_FS_FAIL_IO, "el fichero se acabo antes de tiempo");
        if (cb(buf, (uint32_t) n, user) != 0) break;   /* el llamador aborta */
        off += (uint32_t) n;
    }
    return (long) off;
}

int bpvm_fs_crc32(const char* path, uint32_t* crc_out) {
    /* #398 — si el backend sabe hacerlo, que lo haga ÉL: abre el fichero una
     * vez y lo lee en secuencia. El bucle de abajo, que va por `read_at`, paga
     * una apertura + un seek desde el principio POR CADA 256 B — medido en la
     * P4: el 99 % del tiempo del listado del árbol, 5432 aperturas para 1,3 MB.
     * Se queda como camino de respaldo para backends sin `crc32`, y como
     * ORÁCULO del nuevo en `test/test_fscrc.c`: los dos tienen que dar el mismo
     * número, que además es el de `java.util.zip.CRC32`. */
    uint32_t osz = 0;
    if (!overlay_claims(path, &osz)) {      /* V7/H2 (#493): ver read_stream */
        const bpvm_fs_backend_t* be_crc = route(path);
        if (be_crc && be_crc->crc32 && be_crc->crc32(path, crc_out) == 0) return 0;
    }

    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0) return -1;
    uint32_t st = BPVM_CRC32_INIT;
    uint8_t  buf[256];
    uint32_t off = 0;
    while (off < size) {
        uint32_t n0 = s_fallo_n;
        long n = bpvm_fs_read_at(path, off, buf, sizeof buf);
        if (n <= 0)                          /* incluye "backend sin read_at" (ya anotado) */
            return (int) con_motivo(-1, n0, BPVM_FS_FAIL_IO, "el fichero se acabo antes de tiempo");
        st = bpvm_crc32_update(st, buf, (size_t) n);
        off += (uint32_t) n;
    }
    if (crc_out) *crc_out = bpvm_crc32_final(st);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V7/H3 (#491) — EL CURSOR. Ver el contrato en bpvm_fs.h.
 *
 * La fachada guarda en la cabecera lo que es suyo (posición, tamaño, modo, de
 * dónde sale el fichero) y le deja al motor su estado, opaco, detrás. Para un
 * pack (overlay o respaldo) o un motor sin cursor no hay estado: se lee con
 * `read_at` a la posición que lleva la cabecera —un pack es XIP, y un motor sin
 * `fopen` es el de siempre, sólo que ahora la posición la lleva el llamador.
 * ═══════════════════════════════════════════════════════════════════════════ */
static bpvm_fs_file_t* fh_de(void* blk) {
    bpvm_fs_file_t* h = (bpvm_fs_file_t*) blk;
    return (h && h->magic == BPVM_FS_FILE_MAGIC) ? h : NULL;
}
static void* fh_st(bpvm_fs_file_t* h)   { return (uint8_t*) h + h->st_off; }
static const char* fh_path(bpvm_fs_file_t* h) { return (const char*) h + h->path_off; }

/* El estado del motor se alinea a 8 CONTANDO desde la dirección real del
 * bloque (un byte[] de la VM va alineado a 4): por eso hay 8 de holgura. */
static uint32_t fh_st_off(const void* blk) {
    uintptr_t base = (uintptr_t) blk + sizeof(bpvm_fs_file_t);
    uintptr_t al = (base + 7u) & ~(uintptr_t) 7u;
    return (uint32_t) (al - (uintptr_t) blk);
}
static uint32_t be_state_size(const char* path) {
    const bpvm_fs_backend_t* be = route(path);
    return (be && be->fopen) ? be->file_state_size : 0u;
}

uint32_t bpvm_fs_file_block_size(const char* path) {
    if (!path) return 0;
    return (uint32_t) sizeof(bpvm_fs_file_t) + 8u + be_state_size(path) + (uint32_t) strlen(path) + 1u;
}

int bpvm_fs_fopen(void* blk, uint32_t cap, const char* path, int mode) {
    if (!blk || !path || mode < BPVM_FS_O_READ || mode > BPVM_FS_O_RW)
        return fs_falla(BPVM_FS_FAIL_OTHER, "argumentos no validos");
    if (fh_de(blk))                                  /* ya tiene un fichero: ciérralo antes */
        return fs_falla(BPVM_FS_FAIL_OTHER, "el bloque ya tiene un fichero abierto");
    uint32_t need = bpvm_fs_file_block_size(path);
    if (cap < need || need > 0xFFFFu) return fs_falla(BPVM_FS_FAIL_TOO_BIG, "el bloque del fichero no cabe");
    bpvm_fs_file_t* h = (bpvm_fs_file_t*) blk;
    memset(h, 0, sizeof *h);
    h->st_off   = (uint16_t) fh_st_off(blk);
    h->path_off = (uint16_t) (h->st_off + be_state_size(path));
    memcpy((uint8_t*) blk + h->path_off, path, strlen(path) + 1u);
    h->mode = (uint8_t) mode;

    uint32_t osz = 0;
    /* 1. El pack en ejecución va primero, como en `read` (#310); sólo lectura. */
    if (mode == BPVM_FS_O_READ && overlay_claims(path, &osz) && g_ov_read) {
        h->kind = 2; h->size = osz; h->magic = BPVM_FS_FILE_MAGIC; return 0;
    }
    /* 2. El motor, con su cursor de verdad. */
    const bpvm_fs_backend_t* be = route(path);
    uint32_t n0 = s_fallo_n;
    if (be && be->fopen && be->file_state_size) {
        if (be->fopen(fh_st(h), path, mode) == 0) {
            long sz = be->fsize ? be->fsize(fh_st(h)) : -1;
            if (sz < 0) { be->fclose(fh_st(h)); return (int) sin_motivo(-1, n0); }
            h->be = be; h->kind = 1; h->size = (uint32_t) sz;
            if (mode == BPVM_FS_O_APPEND) {
                if (be->fseek(fh_st(h), h->size) != 0) { be->fclose(fh_st(h)); return (int) sin_motivo(-1, n0); }
                h->pos = h->size;
            }
            h->magic = BPVM_FS_FILE_MAGIC; return 0;
        }
        if (mode != BPVM_FS_O_READ) return (int) sin_motivo(-1, n0);   /* escribir: sólo el motor sabe */
    } else if (mode != BPVM_FS_O_READ) {
        return fs_falla(BPVM_FS_FAIL_OTHER, falta(be));                /* sin motor con cursor no se escribe */
    }
    /* 3. Lectura sin cursor del motor: `read_at` a la posición de la cabecera. */
    if (be && be->read_at && be->stat && be->stat(path, &osz) == 0) {
        h->be = be; h->kind = 4; h->size = osz; h->magic = BPVM_FS_FILE_MAGIC; return 0;
    }
    /* 4. La zona de packs grabados, la última (#362, #493). */
    if (fallback_claims(path, &osz) && g_fb_read) {
        h->kind = 3; h->size = osz; h->magic = BPVM_FS_FILE_MAGIC; return 0;
    }
    return (int) con_motivo(-1, n0, BPVM_FS_FAIL_NOT_FOUND, "no esta ni en la unidad ni en los packs");
}

int bpvm_fs_fclose(void* blk) {
    bpvm_fs_file_t* h = fh_de(blk);
    if (!h) return 0;                                /* idempotente: cerrar lo cerrado no es error */
    int r = 0;
    if (h->kind == 1 && h->be && h->be->fclose) {
        uint32_t n0 = s_fallo_n;
        r = (int) sin_motivo(h->be->fclose(fh_st(h)), n0);
    }
    h->magic = 0; h->kind = 0; h->be = NULL;
    return r;
}

/* La lectura "a offsets" de los que no tienen cursor propio (kind 2/3/4). */
static long fh_read_at(bpvm_fs_file_t* h, uint8_t* dst, uint32_t n) {
    if (h->pos >= h->size) return 0;
    if (n > h->size - h->pos) n = h->size - h->pos;
    switch (h->kind) {
        case 2: return g_ov_read ? g_ov_read(g_ov_user, fh_path(h), h->pos, dst, n) : -1;
        case 3: return g_fb_read ? g_fb_read(g_fb_user, fh_path(h), h->pos, dst, n) : -1;
        case 4: return h->be->read_at(fh_path(h), h->pos, dst, n);
        default: return -1;
    }
}

long bpvm_fs_fread(void* blk, uint8_t* dst, uint32_t n) {
    bpvm_fs_file_t* h = fh_de(blk);
    if (!h) return fs_falla(BPVM_FS_FAIL_OTHER, "el fichero no esta abierto");
    if (!dst && n) return fs_falla(BPVM_FS_FAIL_OTHER, "argumentos no validos");
    if (n == 0) return 0;
    uint32_t n0 = s_fallo_n;
    long r = sin_motivo((h->kind == 1) ? h->be->fread(fh_st(h), dst, n) : fh_read_at(h, dst, n), n0);
    if (r > 0) h->pos += (uint32_t) r;
    return r;
}

long bpvm_fs_fwrite(void* blk, const uint8_t* src, uint32_t n) {
    bpvm_fs_file_t* h = fh_de(blk);
    if (!h) return fs_falla(BPVM_FS_FAIL_OTHER, "el fichero no esta abierto");
    if (h->kind != 1 || h->mode == BPVM_FS_O_READ) return fs_falla(BPVM_FS_FAIL_OTHER, "el fichero es de solo lectura");
    if (!src && n) return fs_falla(BPVM_FS_FAIL_OTHER, "argumentos no validos");
    if (n == 0) return 0;
    uint32_t n0 = s_fallo_n;
    long r = h->be->fwrite(fh_st(h), src, n);
    if (r < 0) return sin_motivo(-1, n0);
    if ((uint32_t) r != n) return con_motivo(-1, n0, BPVM_FS_FAIL_NO_SPACE, "escritura corta: el medio se ha llenado");
    h->pos += n;
    if (h->pos > h->size) h->size = h->pos;
    return r;
}

int bpvm_fs_fseek(void* blk, uint32_t pos) {
    bpvm_fs_file_t* h = fh_de(blk);
    if (!h) return fs_falla(BPVM_FS_FAIL_OTHER, "el fichero no esta abierto");
    if (pos > h->size) return fs_falla(BPVM_FS_FAIL_OTHER, "fuera del fichero");   /* fuera de [0, size]: no hay huecos */
    uint32_t n0 = s_fallo_n;
    if (h->kind == 1 && h->be->fseek(fh_st(h), pos) != 0) return (int) sin_motivo(-1, n0);
    h->pos = pos;
    return 0;
}

long bpvm_fs_ftell(void* blk) { bpvm_fs_file_t* h = fh_de(blk); return h ? (long) h->pos : -1; }
long bpvm_fs_fsize(void* blk) { bpvm_fs_file_t* h = fh_de(blk); return h ? (long) h->size : -1; }
int  bpvm_fs_fisopen(void* blk) { return fh_de(blk) ? 1 : 0; }
const char* bpvm_fs_fpath(void* blk) { bpvm_fs_file_t* h = fh_de(blk); return h ? fh_path(h) : ""; }

int bpvm_fs_fflush(void* blk) {
    bpvm_fs_file_t* h = fh_de(blk);
    if (!h) return fs_falla(BPVM_FS_FAIL_OTHER, "el fichero no esta abierto");
    if (h->kind != 1 || h->mode == BPVM_FS_O_READ || !h->be->fsync) return 0;
    uint32_t n0 = s_fallo_n;
    return (int) sin_motivo(h->be->fsync(fh_st(h)), n0);
}
