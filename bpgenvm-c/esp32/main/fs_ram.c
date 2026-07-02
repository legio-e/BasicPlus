/*
 * fs_ram.c — FS de fs.h para ESP32-S3 (H4.3 + persistencia H4.4).
 *
 * Mirror en RAM (malloc por fichero) + persistencia a una partición de
 * datos dedicada ("bpfs") vía esp_partition — mismo modelo que el FS
 * flash de la Pico. fs_init carga el mirror desde la partición al
 * arrancar; fs_put/fs_delete auto-guardan. Así los .mod subidos por el
 * wire sobreviven al reset.
 *
 * Formato en flash (little-endian nativo):
 *   [magic u32][version u32][count u32][reserved u32]   (16 B)
 *   count × { char name[FS_NAME_LEN]; u32 size }         (44 B c/u)
 *   datos concatenados en el mismo orden que las entries
 */
#include "fs.h"
#include "bpvm_fs.h"   /* backend de file I/O para los builtins (#247) */

#include "esp_partition.h"
#include "esp_log.h"
#include "esp_heap_caps.h"   /* P4 — datos de fichero a PSRAM (libera RAM interna para LVGL) */

#include <stdlib.h>
#include <string.h>

#define FS_MAGIC    0x42504656u   /* 'BPFV' */
#define FS_VERSION  1u
#define FS_HDR_SZ   16u
#define FS_ENTRY_SZ (FS_NAME_LEN + 4u)

static const char* TAG = "bpfs";

typedef struct {
    char     name[FS_NAME_LEN];
    uint8_t* data;     /* malloc'd, NULL si slot libre */
    uint32_t size;
} fs_entry_t;

static fs_entry_t s_files[FS_MAX_FILES];
static uint32_t   s_used_bytes = 0;
static const esp_partition_t* s_part = NULL;

/* Aloca los datos de un fichero. Prefiere PSRAM (se leen UNA vez en la carga; el
 * loader COPIA el .mod a la RAM interna de la VM antes de ejecutar → cero impacto
 * en rendimiento) y así LIBERA RAM interna para LVGL. Fallback a malloc interno
 * (S3 sin PSRAM, o PSRAM agotada). */
static void* fs_alloc(size_t n) {
    void* p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
    return p ? p : malloc(n);
}

static void clear_ram(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].data) { free(s_files[i].data); s_files[i].data = NULL; }
        s_files[i].size = 0;
        s_files[i].name[0] = '\0';
    }
    s_used_bytes = 0;
}

static int find_slot(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (s_files[i].data && strcmp(s_files[i].name, name) == 0) return i;
    return -1;
}

static int free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (!s_files[i].data) return i;
    return -1;
}

/* ---------- Persistencia ---------- */

fs_status_t fs_init(void) {
    clear_ram();
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      ESP_PARTITION_SUBTYPE_ANY, "bpfs");
    if (!s_part) {
        ESP_LOGW(TAG, "particion 'bpfs' no encontrada — FS solo en RAM");
        return FS_OK;
    }

    uint32_t hdr[4];
    if (esp_partition_read(s_part, 0, hdr, FS_HDR_SZ) != ESP_OK) return FS_OK;
    if (hdr[0] != FS_MAGIC) {
        ESP_LOGI(TAG, "particion vacia/sin formato — FS vacio");
        return FS_OK;
    }
    uint32_t count = hdr[2];
    if (count > FS_MAX_FILES) {
        ESP_LOGW(TAG, "count corrupto (%u) — ignoro", (unsigned) count);
        return FS_OK;
    }

    size_t entry_off = FS_HDR_SZ;
    size_t data_off  = FS_HDR_SZ + (size_t) count * FS_ENTRY_SZ;
    for (uint32_t k = 0; k < count; k++) {
        char name[FS_NAME_LEN];
        uint32_t size;
        esp_partition_read(s_part, entry_off, name, FS_NAME_LEN);
        esp_partition_read(s_part, entry_off + FS_NAME_LEN, &size, 4);
        entry_off += FS_ENTRY_SZ;
        name[FS_NAME_LEN - 1] = '\0';

        int slot = free_slot();
        if (slot < 0) break;
        uint8_t* buf = (uint8_t*) fs_alloc(size ? size : 1);
        if (!buf) break;
        if (size) esp_partition_read(s_part, data_off, buf, size);
        data_off += size;
        memcpy(s_files[slot].name, name, FS_NAME_LEN);
        s_files[slot].data = buf;
        s_files[slot].size = size;
        s_used_bytes += size;
    }
    ESP_LOGI(TAG, "cargados %d ficheros (%u bytes) desde flash",
             fs_file_count(), (unsigned) s_used_bytes);
    return FS_OK;
}

void fs_format_ram(void) { clear_ram(); }

fs_status_t fs_save_to_flash(void) {
    if (!s_part) return FS_OK;   /* sin partición: best-effort (RAM sigue OK) */

    uint32_t count = 0, data_total = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].data) { count++; data_total += s_files[i].size; }
    }
    uint32_t img_sz = FS_HDR_SZ + count * FS_ENTRY_SZ + data_total;
    img_sz = (img_sz + 3u) & ~3u;
    if (img_sz > s_part->size) return FS_ERR_NO_SPACE;

    uint8_t* img = (uint8_t*) calloc(1, img_sz ? img_sz : 1);
    if (!img) return FS_ERR_NO_SPACE;

    uint32_t* h = (uint32_t*) img;
    h[0] = FS_MAGIC; h[1] = FS_VERSION; h[2] = count; h[3] = 0;

    size_t eoff = FS_HDR_SZ;
    size_t doff = FS_HDR_SZ + (size_t) count * FS_ENTRY_SZ;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].data) continue;
        memcpy(img + eoff, s_files[i].name, FS_NAME_LEN);
        memcpy(img + eoff + FS_NAME_LEN, &s_files[i].size, 4);
        eoff += FS_ENTRY_SZ;
        if (s_files[i].size) memcpy(img + doff, s_files[i].data, s_files[i].size);
        doff += s_files[i].size;
    }

    fs_status_t rc = FS_OK;
    if (esp_partition_erase_range(s_part, 0, s_part->size) != ESP_OK ||
        esp_partition_write(s_part, 0, img, img_sz) != ESP_OK) {
        ESP_LOGE(TAG, "fallo persistiendo a flash");
        rc = FS_ERR_BAD_FLASH;
    }
    free(img);
    return rc;
}

/* ---------- API ---------- */

int fs_list(fs_list_cb_t cb, void* user) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].data) continue;
        n++;
        if (cb && cb(s_files[i].name, s_files[i].size, user) != 0) break;
    }
    return n;
}

fs_status_t fs_get(const char* name, const uint8_t** data_out, uint32_t* size_out) {
    int i = find_slot(name);
    if (i < 0) return FS_ERR_NOT_FOUND;
    if (data_out) *data_out = s_files[i].data;
    if (size_out) *size_out = s_files[i].size;
    return FS_OK;
}

/* Suspensión del auto-guardado para LOTES (p.ej. esp32_mods_install): cada
 * fs_put/fs_delete auto-persiste borrando+reescribiendo la partición ENTERA
 * (~3 s en la bpfs de 10 MB del P4) → los 14 puts del primer boot eran ~46 s.
 * En lote: suspender, hacer los puts y UN solo save al final. */
static int s_autosave_off = 0;

void fs_autosave_suspend(void) { s_autosave_off = 1; }

void fs_autosave_resume(int save_now) {
    s_autosave_off = 0;
    if (save_now) fs_save_to_flash();
}

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || !name[0]) return FS_ERR_INVALID;
    if (strlen(name) >= FS_NAME_LEN) return FS_ERR_NAME_TOO_LONG;

    int existing = find_slot(name);
    uint32_t old_size = (existing >= 0) ? s_files[existing].size : 0;
    if (s_used_bytes - old_size + size > FS_DATA_SIZE) return FS_ERR_NO_SPACE;

    int slot = existing;
    if (slot < 0) {
        slot = free_slot();
        if (slot < 0) return FS_ERR_TABLE_FULL;
    }

    uint8_t* buf = (uint8_t*) fs_alloc(size ? size : 1);
    if (!buf) return FS_ERR_NO_SPACE;
    if (data && size) memcpy(buf, data, size);

    if (existing >= 0) { free(s_files[slot].data); s_used_bytes -= old_size; }
    strncpy(s_files[slot].name, name, FS_NAME_LEN - 1);
    s_files[slot].name[FS_NAME_LEN - 1] = '\0';
    s_files[slot].data = buf;
    s_files[slot].size = size;
    s_used_bytes += size;

    if (!s_autosave_off) fs_save_to_flash();   /* auto-persistir (best-effort) */
    return FS_OK;
}

fs_status_t fs_delete(const char* name) {
    int i = find_slot(name);
    if (i < 0) return FS_ERR_NOT_FOUND;
    free(s_files[i].data);
    s_used_bytes -= s_files[i].size;
    s_files[i].data = NULL;
    s_files[i].size = 0;
    s_files[i].name[0] = '\0';
    if (!s_autosave_off) fs_save_to_flash();
    return FS_OK;
}

uint32_t fs_total_bytes(void) { return FS_DATA_SIZE; }
uint32_t fs_used_bytes(void)  { return s_used_bytes; }
uint32_t fs_free_bytes(void)  { return FS_DATA_SIZE - s_used_bytes; }

int fs_file_count(void) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) if (s_files[i].data) n++;
    return n;
}

const char* fs_status_str(fs_status_t s) {
    switch (s) {
        case FS_OK:                  return "ok";
        case FS_ERR_NOT_FOUND:       return "not found";
        case FS_ERR_EXISTS:          return "exists";
        case FS_ERR_NO_SPACE:        return "no space";
        case FS_ERR_NAME_TOO_LONG:   return "name too long";
        case FS_ERR_TOO_BIG:         return "too big";
        case FS_ERR_TABLE_FULL:      return "table full";
        case FS_ERR_BAD_FLASH:       return "flash error";
        case FS_ERR_INVALID:         return "invalid";
        default:                     return "unknown";
    }
}

/* ============================================================
 * Backend de file I/O para BP (#247): conecta readFile/writeFile/appendFile/
 * fileExists + readFileBytes/writeFileBytes (builtins VM-C) a este FS. Paths
 * literales (p.ej. "/app/x.txt"). Las escrituras persisten a flash
 * (fs_save_to_flash). append: copia el fichero a un scratch, concatena y reescribe
 * (cap = FS_BP_SCRATCH). Mismo patrón que pico/fs.c (API fs_get/fs_put idéntica).
 * ============================================================ */

#define FS_BP_SCRATCH  (8u * 1024u)
static uint8_t s_bpfs_scratch[FS_BP_SCRATCH];

static int esp32_bpfs_stat(const char* path, uint32_t* size) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != FS_OK) return -1;
    if (size) *size = sz;
    return 0;
}

static long esp32_bpfs_read(const char* path, uint8_t* dst, uint32_t cap) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != FS_OK) return -1;
    uint32_t n = (sz < cap) ? sz : cap;
    if (n > 0) memcpy(dst, d, n);
    return (long) n;
}

static int esp32_bpfs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    fs_status_t rc;
    if (append) {
        const uint8_t* old; uint32_t oldsz;
        if (fs_get(path, &old, &oldsz) == FS_OK) {
            if ((uint64_t) oldsz + len > FS_BP_SCRATCH) return -1;   /* no cabe */
            memcpy(s_bpfs_scratch, old, oldsz);                       /* copia ANTES de fs_put */
            if (len > 0) memcpy(s_bpfs_scratch + oldsz, data, len);
            rc = fs_put(path, s_bpfs_scratch, oldsz + len);
        } else {
            rc = fs_put(path, data, len);                             /* fichero nuevo */
        }
    } else {
        rc = fs_put(path, data, len);
    }
    if (rc != FS_OK) return -1;
    fs_save_to_flash();   /* persiste (sobrevive al reset) */
    return 0;
}

/* #240 (logger) — borrado y rename (rotación del log). rename = get+put+delete
 * sobre el scratch (el FS no tiene rename nativo); sobreescribe el destino,
 * como la VM-Java (REPLACE_EXISTING). fs_delete/fs_put ya auto-persisten. */
static int esp32_bpfs_remove(const char* path) {
    return (fs_delete(path) == FS_OK) ? 0 : -1;
}

static int esp32_bpfs_rename(const char* from, const char* to) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(from, &d, &sz) != FS_OK) return -1;
    if (sz > FS_BP_SCRATCH) return -1;
    memcpy(s_bpfs_scratch, d, sz);            /* copia ANTES de fs_put */
    if (fs_put(to, s_bpfs_scratch, sz) != FS_OK) return -1;
    fs_delete(from);
    return 0;
}

static const bpvm_fs_backend_t s_esp32_fs_backend = {
    .stat   = esp32_bpfs_stat,
    .read   = esp32_bpfs_read,
    .write  = esp32_bpfs_write,
    .remove = esp32_bpfs_remove,
    .rename = esp32_bpfs_rename,
};

void fs_register_bpvm(void) {
    bpvm_fs_set_backend(&s_esp32_fs_backend);
}
