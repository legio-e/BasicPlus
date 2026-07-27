/*
 * fs_lfs_esp32.c — H2·B3.a: littlefs en el ESP32 (S3 Xtensa + P4 RISC-V, MISMO
 * fichero; el P4 lo compila desde esp32p4/). Reemplaza al stopgap fs_ram.c
 * (mirror en RAM sobre la partición "bpfs").
 *
 * Mucho más simple que la cintura del RP2350 (fs_lfs_pico.c):
 *  - NO hay descriptor de particiones: la tabla `esp_partition` YA lo es.
 *    La partición de datos "bpfs" (partitions.csv) define offset y tamaño;
 *    la cintura lee `s_part->size` en runtime → S3 (256K) y P4 (~10M/26M)
 *    con el MISMO código.
 *  - NO hay gimnasia XIP/lock de flash: `esp_partition_{read,write,erase_range}`
 *    son los 3 primitivos que littlefs necesita, 1:1, y esp-idf hace el acceso
 *    a flash de forma segura. La multitarea la cubre el LOCK GRUESO de la
 *    fachada (B1.4) — los DOS clientes (builtins BP + comm task) pasan por él.
 *
 * El SHIM del API legado fs.h (fs_get/put/delete/list/stats) es IDÉNTICO al del
 * RP2350: todo son llamadas `bpvm_fs_*` a la fachada (backend-agnóstico). La
 * migración de los llamadores a la fachada directa + retirar el scratch = B2 v2.
 */
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"
#include "log.h"          /* #329 — el porqué de un fallo de FS, al log persistente */

#include "esp_partition.h"

#include <string.h>
#include <stdio.h>

/* ───────────────────────── cintura block-device (esp_partition) ─────────── */

#define ESP_FS_BLOCK_SIZE   4096u          /* sector de borrado de la flash SPI */

static const esp_partition_t* s_part = NULL;   /* particion vendor "bpdata" (FS+Packs) */
static uint32_t s_fs_offset = 0;   /* H9: inicio del sub-rango FS DENTRO de bpdata (relativo) */

static int esp_bd_read(const struct lfs_config* c, lfs_block_t block,
                       lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    return (esp_partition_read(s_part, s_fs_offset + block * ESP_FS_BLOCK_SIZE + off,
                               buffer, size) == ESP_OK) ? 0 : LFS_ERR_IO;
}

static int esp_bd_prog(const struct lfs_config* c, lfs_block_t block,
                       lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    return (esp_partition_write(s_part, s_fs_offset + block * ESP_FS_BLOCK_SIZE + off,
                                buffer, size) == ESP_OK) ? 0 : LFS_ERR_IO;
}

static int esp_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    return (esp_partition_erase_range(s_part, s_fs_offset + block * ESP_FS_BLOCK_SIZE,
                                      ESP_FS_BLOCK_SIZE) == ESP_OK) ? 0 : LFS_ERR_IO;
}

static int esp_bd_sync(const struct lfs_config* c) {
    (void) c;   /* esp_partition_write es síncrono */
    return 0;
}

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en fs_lfs.c).
 * cache_size DEBE ser BPVM_FS_LFS_CACHE (attach lo valida). */
static uint8_t s_read_buf[256];
static uint8_t s_prog_buf[256];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));

static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* ───────────────────────── shim del API legado fs.h ──────────────────────
 * H11 — AQUÍ VIVÍA `fs_get` Y SU ESPEJO DE 64 KB, y ya no existen.
 *
 * El shim heredaba de fs_ram.c un contrato imposible de cumplir barato: devolver
 * un PUNTERO a los bytes, "válido hasta el siguiente put/delete". Con littlefs
 * los ficheros no están mapeados en memoria, así que sostener ese puntero
 * obligaba a copiar el fichero ENTERO a un estático de 64 KB — reservados en
 * dram0_0_seg para siempre, se usaran o no. Era el mayor estático del firmware.
 *
 * Todos sus llamantes se pasaron a la fachada portable, que trabaja por trozos:
 *   - ¿existe? / ¿tamaño?      → bpvm_fs_stat            (STAT, esp32_mods)
 *   - leer un cacho            → bpvm_fs_read_at         (GET, autorun)
 *   - cargar un módulo         → bpvm_load_mod_stream    (RUN: main, deps)
 *   - leer a un destino propio → bpvm_fs_read            (.mdn → RAM ejecutable)
 *   - CRC                      → bpvm_fs_crc32           (LS)
 * Mismo camino que cerró #305 en el Pico, donde el espejo era de 128 KB. */

fs_status_t fs_init_at(uint32_t fs_offset, uint32_t fs_size) {
    /* H9: la zona de datos es la particion vendor "bpdata"; la REGION FS es su
     * sub-rango [fs_offset, fs_offset+fs_size). El tamaño lo define el env
     * (bpvm_part, un mando); aqui solo se monta. Antes montaba "bpfs" entera. */
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      ESP_PARTITION_SUBTYPE_ANY, "bpdata");
    if (!s_part) {
        log_printf("fs: NO hay particion 'bpdata' (tabla vieja sin reflashear?)");
        return FS_ERR_BAD_FLASH;            /* sin particion de datos → sin FS */
    }
    /* #329 — el tamaño de bpdata al log: si la placa arrastra una tabla vieja
     * (el S3 tenía una de 2 MB con 16 MB de flash), aquí se ve de un vistazo en
     * vez de deducirlo de un "FS lleno". */
    log_printf("fs: bpdata @0x%X size=%u KB | region FS +0x%X size=%u KB",
               (unsigned) s_part->address, (unsigned) (s_part->size / 1024u),
               (unsigned) fs_offset, (unsigned) (fs_size / 1024u));
    if (fs_size == 0 || (uint64_t) fs_offset + fs_size > (uint64_t) s_part->size) {
        log_printf("fs: la region FS NO CABE en bpdata → sin FS");
        return FS_ERR_BAD_FLASH;            /* region fuera de bpdata */
    }
    s_fs_offset = fs_offset;

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = esp_bd_read;
    s_cfg.prog  = esp_bd_prog;
    s_cfg.erase = esp_bd_erase;
    s_cfg.sync  = esp_bd_sync;
    s_cfg.read_size      = 256;
    s_cfg.prog_size      = 256;
    s_cfg.block_size     = ESP_FS_BLOCK_SIZE;                 /* 4096 */
    s_cfg.block_count    = fs_size / ESP_FS_BLOCK_SIZE;
    s_cfg.cache_size     = 256;                              /* == BPVM_FS_LFS_CACHE */
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) {   /* formatea si no monta */
        int e = bpvm_fs_lfs_last_err();
        log_printf("fs: littlefs NO monta ni formatea (lfs=%d %s)",
                   e, bpvm_fs_lfs_err_str(e));
        log_flush();
        return FS_ERR_BAD_FLASH;
    }
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    /* Igual que el Pico: cuántos ficheros hay y cuánto ocupan. Es la línea que
     * dice de un golpe si la stdlib está o falta, y si el volumen va lleno. */
    log_printf("fs: %d ficheros, %u/%u bytes usados",
               fs_file_count(), (unsigned) fs_used_bytes(), (unsigned) fs_total_bytes());
    log_flush();
    return FS_OK;
}

void fs_format_ram(void) {
    /* legado "vaciar" → reformateo EN CALIENTE del volumen littlefs */
    if (bpvm_fs_lfs_format() == 0) {
        bpvm_fs_mkdir("/sys");
        bpvm_fs_mkdir("/lib");
        bpvm_fs_mkdir("/app");
    }
}

fs_status_t fs_save_to_flash(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada) → no-op */
    return FS_OK;
}

/* fs_ram.c exponía autosave (mirror→flash diferido); con littlefs es moot
 * (cada close committea). Stubs para no romper los llamadores (esp32_mods.c). */
void fs_autosave_suspend(void) { }
void fs_autosave_resume(int save_now) { (void) save_now; }


/* fs_put crea los dirs padre (el FS viejo era PLANO; littlefs necesita /lib
 * antes de escribir /lib/x). */
static void ensure_parent_dirs(const char* name) {
    const char* last = strrchr(name, '/');
    if (!last || last == name) return;
    char dir[128];
    size_t n = (size_t)(last - name);
    if (n >= sizeof(dir)) return;
    memcpy(dir, name, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);
}

/* #329 — el fallo de escritura DEJA DE MENTIR. Antes cualquier retorno != 0 de
 * la fachada se traducía a FS_ERR_NO_SPACE y el IDE decía "FS lleno" con el FS
 * al 1% — un mensaje falso manda la depuración por el barranco equivocado y
 * cuesta tardes. Ahora se lee el lfs_error real y además se deja en el log
 * persistente, que sobrevive al reset y se saca con LOG_DUMP. */
static fs_status_t map_lfs_err(const char* op, const char* name, uint32_t size) {
    int e = bpvm_fs_lfs_last_err();
    log_printf("fs: %s '%s' (%u B) FALLO lfs=%d %s  [libre %u/%u B]",
               op, name, (unsigned) size, e, bpvm_fs_lfs_err_str(e),
               (unsigned) fs_free_bytes(), (unsigned) fs_total_bytes());
    log_flush();
    switch (e) {
        case LFS_ERR_NOSPC:        return FS_ERR_NO_SPACE;
        case LFS_ERR_FBIG:         return FS_ERR_TOO_BIG;
        case LFS_ERR_NAMETOOLONG:  return FS_ERR_NAME_TOO_LONG;
        case LFS_ERR_EXIST:        return FS_ERR_EXISTS;
        case LFS_ERR_NOENT:        return FS_ERR_NOT_FOUND;
        case LFS_ERR_IO:
        case LFS_ERR_CORRUPT:      return FS_ERR_BAD_FLASH;
        default:                   return FS_ERR_INVALID;
    }
}

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (strlen(name) >= 128) return FS_ERR_NAME_TOO_LONG;
    ensure_parent_dirs(name);
    if (bpvm_fs_write(name, data, size, 0) != 0) return map_lfs_err("put", name, size);
    return FS_OK;
}

/* #294 streaming PUT — apende un trozo al fichero. El PUT_BEGIN del wire lo
 * crea/trunca (+dirs) con fs_put(name,NULL,0); cada PUT_DATA llama aquí. Permite
 * subir ficheros mayores que el buffer del wire sin buferizarlos enteros. */
fs_status_t fs_put_append(const char* name, const uint8_t* data, uint32_t size) {
    if (strlen(name) >= 128) return FS_ERR_NAME_TOO_LONG;
    if (size == 0) return FS_OK;
    if (bpvm_fs_write(name, data, size, 1) != 0) return map_lfs_err("append", name, size);
    return FS_OK;
}

fs_status_t fs_delete(const char* name) {
    if (!bpvm_fs_exists(name)) return FS_ERR_NOT_FOUND;
    return (bpvm_fs_remove(name) == 0) ? FS_OK : FS_ERR_INVALID;
}

/* ── fs_list: BFS emitiendo paths PLANOS como el FS viejo. El cb de la fachada
 * corre BAJO el lock → SOLO acumula; el descenso y la emisión van FUERA. ── */
#define LIST_MAX_DIRS     16
#define LIST_MAX_ENTRIES  32
#define LIST_NAME_MAX     64

typedef struct {
    char     names[LIST_MAX_ENTRIES][LIST_NAME_MAX];
    uint32_t sizes[LIST_MAX_ENTRIES];
    uint8_t  isdir[LIST_MAX_ENTRIES];
    int      n;
} dir_snapshot_t;

static void snap_cb(const char* name, int is_dir, uint32_t size, void* user) {
    dir_snapshot_t* s = (dir_snapshot_t*) user;
    if (s->n >= LIST_MAX_ENTRIES) return;
    snprintf(s->names[s->n], LIST_NAME_MAX, "%s", name);
    s->sizes[s->n] = size;
    s->isdir[s->n] = (uint8_t) is_dir;
    s->n++;
}

int fs_list(fs_list_cb_t cb, void* user) {
    static char pending[LIST_MAX_DIRS][LIST_NAME_MAX];
    static dir_snapshot_t snap;
    int head = 0, tail = 0;
    snprintf(pending[tail++], LIST_NAME_MAX, "/");

    while (head < tail) {
        /* Copia a un LOCAL: `dir` apuntando dentro de `pending` + snprintf a otro
         * slot de `pending` = solape aparente que GCC-15 marca (-Werror=restrict),
         * aunque head≠tail. El local desacopla lectura de escritura. */
        char dir[LIST_NAME_MAX];
        snprintf(dir, sizeof(dir), "%s", pending[head++]);
        snap.n = 0;
        if (bpvm_fs_list(dir, snap_cb, &snap) != 0) continue;
        for (int i = 0; i < snap.n; i++) {
            char full[LIST_NAME_MAX];
            int is_root = (dir[1] == '\0');
            if (snap.isdir[i]) {
                if (tail < LIST_MAX_DIRS) {
                    snprintf(pending[tail], LIST_NAME_MAX, "%s%s%s",
                             dir, is_root ? "" : "/", snap.names[i]);
                    tail++;
                }
                continue;
            }
            if (is_root) snprintf(full, sizeof(full), "%s", snap.names[i]);
            else         snprintf(full, sizeof(full), "%s/%s", dir, snap.names[i]);
            if (cb(full, snap.sizes[i], user) != 0) return 1;
        }
    }
    return 0;
}

uint32_t fs_total_bytes(void) {
    uint32_t t = 0; bpvm_fs_lfs_stats(&t, NULL); return t;
}
uint32_t fs_used_bytes(void) {
    uint32_t u = 0; bpvm_fs_lfs_stats(NULL, &u); return u;
}
uint32_t fs_free_bytes(void) {
    uint32_t t = 0, u = 0; bpvm_fs_lfs_stats(&t, &u); return (t > u) ? (t - u) : 0;
}

static int count_cb(const char* name, uint32_t size, void* user) {
    (void) name; (void) size; (*(int*) user)++; return 0;
}
int fs_file_count(void) { int n = 0; fs_list(count_cb, &n); return n; }

const char* fs_status_str(fs_status_t s) {
    switch (s) {
        case FS_OK:                return "OK";
        case FS_ERR_NOT_FOUND:     return "no encontrado";
        case FS_ERR_EXISTS:        return "ya existe";
        case FS_ERR_NO_SPACE:      return "sin espacio";
        case FS_ERR_NAME_TOO_LONG: return "nombre demasiado largo";
        case FS_ERR_TOO_BIG:       return "fichero demasiado grande";
        case FS_ERR_TABLE_FULL:    return "tabla llena";
        case FS_ERR_BAD_FLASH:     return "flash corrupta";
        case FS_ERR_INVALID:       return "operacion invalida";
        default:                   return "error desconocido";
    }
}

void fs_register_bpvm(void) {
    /* no-op: bpvm_fs_lfs_attach (en fs_init) ya registró el backend en la
     * fachada — los builtins van DIRECTOS a littlefs (lock grueso de B1.4). */
}
