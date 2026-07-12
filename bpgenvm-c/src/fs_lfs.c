/*
 * fs_lfs.c — backend littlefs de la fachada bpvm_fs (H2 fase A · B1.1).
 *
 * PORTABLE: opera sobre UN lfs_t montado por bpvm_fs_lfs_attach(); el
 * block-device lo aporta el llamante (host: filebd en fs_lfs_host.c; micro:
 * la cintura de flash de cada familia, B2/B3). No sabe de dónde vienen los
 * bloques — esa es la gracia (§2 del plan, docs/V4_IDEAS.md).
 *
 * SEMÁNTICA = espejo de fs_host.c (el contrato de la fachada, paridad):
 *   stat    → -1 en directorios (host: fopen("rb") falla en dirs)
 *   write   → trunc/append, crea el fichero pero NO los padres (como fopen)
 *   remove  → SOLO ficheros (host Windows: remove() falla en dirs; rmdir aparte)
 *   rename  → REPLACE_EXISTING sobre fichero destino (lfs_rename ya lo hace)
 *   mkdir   → recursivo (crea intermedios), ok si ya existe
 *   rmdir   → SOLO dirs, falla si no vacío
 *   copy    → sobreescribe destino, por trozos (sin buffers grandes)
 *   mtime   → NULL (littlefs no tiene timestamps → el builtin lanza
 *             "no soportado" limpio, igual que el device de hoy)
 *
 * DURABILIDAD (charla Eduardo 12-jul): cada op es open→…→close y el close de
 * littlefs COMMITTEA → todo write/append cuya llamada retornó está en flash;
 * un cuelgue pierde como mucho la op en curso, jamás corrompe (B0.3). El
 * flush explícito solo tendrá sentido con la futura API de streaming.
 *
 * Concurrencia: las ops serializan con un lock grueso en B1.4 (littlefs no es
 * reentrante). Hoy el default de la VM es 1 worker → sin carrera.
 */
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"
#include "bpvm_platform.h"
#include <string.h>

static lfs_t s_lfs;
static int   s_mounted = 0;

/* B1.4 — LOCK GRUESO del FS (requisito multitarea, Eduardo 12-jul): littlefs
 * NO es reentrante y tiene ≥2 clientes (Threads BP vía builtins + la comm
 * task del wire cuando el repl migre a la fachada en B2). El lock envuelve
 * la OPERACIÓN COMPLETA de la fachada (no cada llamada lfs_* suelta) →
 * cada op es ATÓMICA: un append de log jamás se entrelaza con otro y las
 * secuencias internas stat+remove no tienen TOCTOU. El FS es camino frío
 * (ops raras, ~ms) e inherentemente serial → lock global correcto (§5.1 de
 * la propuesta; hooks LFS_THREADSAFE serían por-llamada, más débiles).
 * Mutex de la cintura de plataforma: pthread en host, FreeRTOS en el micro.
 * Verificado rojo→verde con test_fs_lfs_mt (sin lock: assertion pcache de
 * littlefs con 4 threads; con lock: 0 corrupciones). */
static bpvm_platform_mutex_handle_t s_fs_lock;
static int s_fs_lock_ready = 0;

static void fs_lock(void)   { if (s_fs_lock_ready) bpvm_platform_mutex_lock(&s_fs_lock); }
static void fs_unlock(void) { if (s_fs_lock_ready) bpvm_platform_mutex_unlock(&s_fs_lock); }

/* ¿el path existe y de qué tipo? 0=no existe, 1=fichero, 2=dir (size out) */
static int kind_of(const char* path, uint32_t* size) {
    struct lfs_info info;
    if (lfs_stat(&s_lfs, path, &info) < 0) return 0;
    if (size) *size = (uint32_t) info.size;
    return (info.type == LFS_TYPE_DIR) ? 2 : 1;
}

static int be_stat_impl(const char* path, uint32_t* size) {
    return (kind_of(path, size) == 1) ? 0 : -1;   /* dirs → -1, como host */
}

static long be_read_impl(const char* path, uint8_t* dst, uint32_t cap) {
    lfs_file_t f;
    if (lfs_file_open(&s_lfs, &f, path, LFS_O_RDONLY) < 0) return -1;
    lfs_ssize_t n = lfs_file_read(&s_lfs, &f, dst, cap);
    lfs_file_close(&s_lfs, &f);
    return (n < 0) ? -1 : (long) n;
}

static int be_write_impl(const char* path, const uint8_t* data, uint32_t len, int append) {
    lfs_file_t f;
    int flags = LFS_O_WRONLY | LFS_O_CREAT | (append ? LFS_O_APPEND : LFS_O_TRUNC);
    if (lfs_file_open(&s_lfs, &f, path, flags) < 0) return -1;
    int rc = 0;
    if (len > 0 && data) {
        lfs_ssize_t n = lfs_file_write(&s_lfs, &f, data, len);
        if (n < 0 || (uint32_t) n != len) rc = -1;
    }
    /* el close COMMITTEA (durabilidad-por-llamada; ver cabecera) */
    if (lfs_file_close(&s_lfs, &f) < 0) rc = -1;
    return rc;
}

static int be_remove_impl(const char* path) {
    if (kind_of(path, NULL) != 1) return -1;      /* solo ficheros, como host */
    return (lfs_remove(&s_lfs, path) < 0) ? -1 : 0;
}

static int be_rename_impl(const char* from, const char* to) {
    /* lfs_rename ya sobreescribe un fichero destino (REPLACE_EXISTING). */
    return (lfs_rename(&s_lfs, from, to) < 0) ? -1 : 0;
}

/* mkdir recursivo (crea intermedios; ok si ya existe) — espejo de host_mkdir. */
static int be_mkdir_impl(const char* path) {
    char buf[512];
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(buf)) return -1;
    memcpy(buf, path, n + 1);
    for (size_t i = 1; i < n; i++) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            lfs_mkdir(&s_lfs, buf);   /* ignora fallo: puede existir */
            buf[i] = '/';
        }
    }
    int e = lfs_mkdir(&s_lfs, buf);
    if (e == 0 || e == LFS_ERR_EXIST) {
        return (kind_of(buf, NULL) == 2) ? 0 : -1;   /* EXIST como fichero → -1 */
    }
    return -1;
}

static int be_rmdir_impl(const char* path) {
    if (kind_of(path, NULL) != 2) return -1;      /* solo dirs, como host */
    return (lfs_remove(&s_lfs, path) < 0) ? -1 : 0;   /* NOTEMPTY → -1 */
}

static int be_copy_impl(const char* from, const char* to) {
    lfs_file_t fi, fo;
    if (lfs_file_open(&s_lfs, &fi, from, LFS_O_RDONLY) < 0) return -1;
    if (lfs_file_open(&s_lfs, &fo, to, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
        lfs_file_close(&s_lfs, &fi);
        return -1;
    }
    uint8_t buf[256];
    int rc = 0;
    for (;;) {
        lfs_ssize_t n = lfs_file_read(&s_lfs, &fi, buf, sizeof(buf));
        if (n < 0) { rc = -1; break; }
        if (n == 0) break;
        if (lfs_file_write(&s_lfs, &fo, buf, (lfs_size_t) n) != n) { rc = -1; break; }
    }
    lfs_file_close(&s_lfs, &fi);
    if (lfs_file_close(&s_lfs, &fo) < 0) rc = -1;
    return rc;
}

static int be_isdir_impl(const char* path) {
    return (kind_of(path, NULL) == 2) ? 1 : 0;
}

/* B1.3 — listado (wire LS+CRC / explorer del IDE): cb por entrada, sin
 * '.' ni '..'. OJO: el cb corre BAJO el lock del FS (op atómica) → no debe
 * re-entrar en la fachada (el llamante acumula y postprocesa fuera). */
static int be_list_impl(const char* path,
                        void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                        void* user) {
    lfs_dir_t d;
    struct lfs_info info;
    if (lfs_dir_open(&s_lfs, &d, path) < 0) return -1;
    while (lfs_dir_read(&s_lfs, &d, &info) > 0) {
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) continue;
        cb(info.name, (info.type == LFS_TYPE_DIR) ? 1 : 0, (uint32_t) info.size, user);
    }
    lfs_dir_close(&s_lfs, &d);
    return 0;
}

/* B1.4 — wrappers con el lock grueso: cada op de la fachada = una sección
 * crítica completa. kind_of y las _impl NUNCA toman el lock (se llaman solo
 * desde aquí) → sin recursión. */
static int be_stat(const char* path, uint32_t* size) {
    fs_lock(); int r = be_stat_impl(path, size); fs_unlock(); return r;
}
static long be_read(const char* path, uint8_t* dst, uint32_t cap) {
    fs_lock(); long r = be_read_impl(path, dst, cap); fs_unlock(); return r;
}
static int be_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    fs_lock(); int r = be_write_impl(path, data, len, append); fs_unlock(); return r;
}
static int be_remove(const char* path) {
    fs_lock(); int r = be_remove_impl(path); fs_unlock(); return r;
}
static int be_rename(const char* from, const char* to) {
    fs_lock(); int r = be_rename_impl(from, to); fs_unlock(); return r;
}
static int be_mkdir(const char* path) {
    fs_lock(); int r = be_mkdir_impl(path); fs_unlock(); return r;
}
static int be_rmdir(const char* path) {
    fs_lock(); int r = be_rmdir_impl(path); fs_unlock(); return r;
}
static int be_copy(const char* from, const char* to) {
    fs_lock(); int r = be_copy_impl(from, to); fs_unlock(); return r;
}
static int be_isdir(const char* path) {
    fs_lock(); int r = be_isdir_impl(path); fs_unlock(); return r;
}
static int be_list(const char* path,
                   void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                   void* user) {
    fs_lock(); int r = be_list_impl(path, cb, user); fs_unlock(); return r;
}

static const bpvm_fs_backend_t s_lfs_backend = {
    .stat     = be_stat,
    .read     = be_read,
    .write    = be_write,
    .remove   = be_remove,
    .rename   = be_rename,
    .mkdir    = be_mkdir,
    .rmdir    = be_rmdir,
    .copy     = be_copy,
    .isdir    = be_isdir,
    .mtime_ms = NULL,   /* littlefs no tiene timestamps → "no soportado" limpio */
    .list     = be_list,   /* B1.3 */
};

int bpvm_fs_lfs_attach(const struct lfs_config* cfg, int format_if_needed) {
    if (s_mounted) return -1;
    /* B1.4 — el lock nace ANTES del mount (una vez; sobrevive re-attaches). */
    if (!s_fs_lock_ready) {
        if (bpvm_platform_mutex_init(&s_fs_lock) != 0) return -1;
        s_fs_lock_ready = 1;
    }
    fs_lock();
    int e = lfs_mount(&s_lfs, cfg);
    if (e < 0 && format_if_needed) {
        /* primer arranque / bump de formato → reformateo limpio (§7 del plan) */
        e = lfs_format(&s_lfs, cfg);
        if (e == 0) e = lfs_mount(&s_lfs, cfg);
    }
    if (e == 0) s_mounted = 1;
    fs_unlock();
    if (e < 0) return -1;
    bpvm_fs_set_backend(&s_lfs_backend);
    return 0;
}

void bpvm_fs_lfs_detach(void) {
    if (!s_mounted) return;
    fs_lock();
    lfs_unmount(&s_lfs);
    s_mounted = 0;
    fs_unlock();
    bpvm_fs_set_backend(NULL);   /* las ops vuelven a "fallo limpio" */
}
