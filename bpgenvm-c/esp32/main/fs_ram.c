/*
 * fs_ram.c — implementación RAM-only de fs.h para ESP32-S3 (H4.3).
 *
 * Misma API que el FS flash de la Pico (pico/fs.h), pero los ficheros
 * viven solo en RAM (malloc por fichero — el S3 tiene heap de sobra).
 * fs_save_to_flash() es no-op por ahora: la persistencia (SPIFFS o
 * partición) llega en H4.4. Esto basta para que el REPL del wire suba
 * .mod y los ejecute (PUT → RAM → RUN).
 */
#include "fs.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    char     name[FS_NAME_LEN];
    uint8_t* data;     /* malloc'd, NULL si slot libre */
    uint32_t size;
} fs_entry_t;

static fs_entry_t s_files[FS_MAX_FILES];
static uint32_t   s_used_bytes = 0;

fs_status_t fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        s_files[i].data = NULL;
        s_files[i].size = 0;
        s_files[i].name[0] = '\0';
    }
    s_used_bytes = 0;
    return FS_OK;
}

void fs_format_ram(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].data) { free(s_files[i].data); s_files[i].data = NULL; }
        s_files[i].size = 0;
        s_files[i].name[0] = '\0';
    }
    s_used_bytes = 0;
}

fs_status_t fs_save_to_flash(void) {
    /* H4.4: persistir a SPIFFS/partición. Por ahora RAM-only. */
    return FS_OK;
}

static int find_slot(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].data && strcmp(s_files[i].name, name) == 0) return i;
    }
    return -1;
}

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

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || !name[0]) return FS_ERR_INVALID;
    if (strlen(name) >= FS_NAME_LEN) return FS_ERR_NAME_TOO_LONG;

    int existing = find_slot(name);
    uint32_t old_size = (existing >= 0) ? s_files[existing].size : 0;
    /* Comprobación de espacio (cuenta el reemplazo: resta el viejo). */
    if (s_used_bytes - old_size + size > FS_DATA_SIZE) return FS_ERR_NO_SPACE;

    int slot = existing;
    if (slot < 0) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (!s_files[i].data) { slot = i; break; }
        }
        if (slot < 0) return FS_ERR_TABLE_FULL;
    }

    uint8_t* buf = (uint8_t*) malloc(size ? size : 1);
    if (!buf) return FS_ERR_NO_SPACE;
    if (data && size) memcpy(buf, data, size);

    if (existing >= 0) {                 /* sobreescritura: libera el viejo */
        free(s_files[slot].data);
        s_used_bytes -= old_size;
    }
    strncpy(s_files[slot].name, name, FS_NAME_LEN - 1);
    s_files[slot].name[FS_NAME_LEN - 1] = '\0';
    s_files[slot].data = buf;
    s_files[slot].size = size;
    s_used_bytes += size;
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
