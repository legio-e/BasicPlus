/*
 * fs.c — implementación del filesystem RAM + flash.
 */

#include "fs.h"

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

/* ============================================================== */
/* Layout interno                                                  */
/* ============================================================== */

#define FS_MAGIC        0x42504656u    /* 'BPFV' big-endian, but stored as u32 le */
#define FS_VERSION      1u

/* Cada entry es 48 bytes (40 + 4 + 4). 16 entries -> 768 bytes.
 * Header = 4 magic + 4 ver + 4 count + 16*48 = 780 bytes. Reservamos 1 KB. */
typedef struct {
    char     name[FS_NAME_LEN];
    uint32_t offset;     /* offset dentro del buffer de datos */
    uint32_t size;       /* bytes (0 = slot libre, pese a estar la entry) */
} fs_entry_t;

#define FS_HEADER_BYTES  1024u
#define FS_REGION_SIZE   (FS_HEADER_BYTES + FS_DATA_SIZE)   /* 9 KB */

/* Región en flash: últimos sectores del chip.
 * Pico 2: PICO_FLASH_SIZE_BYTES = 4 MB. Reservamos los últimos
 * FS_REGION_ALIGNED bytes (3 sectores de 4 KB = 12 KB). */
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif
#define FS_REGION_ALIGNED    (((FS_REGION_SIZE + FLASH_SECTOR_SIZE - 1) \
                                 / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE)
#define FS_FLASH_OFFSET      (PICO_FLASH_SIZE_BYTES - FS_REGION_ALIGNED)

/* En memoria. */
static fs_entry_t s_entries[FS_MAX_FILES];
static uint8_t    s_data[FS_DATA_SIZE];
static uint32_t   s_data_used;          /* cuántos bytes de s_data hay usados */
static int        s_initialized = 0;

/* ============================================================== */
/* Helpers                                                         */
/* ============================================================== */

static int find_by_name(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size != 0 && strcmp(s_entries[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size == 0) return i;
    }
    return -1;
}

/* Compacta el buffer de datos eliminando huecos. Re-asigna offsets. */
static void compact(void) {
    uint32_t cursor = 0;
    uint8_t tmp[FS_DATA_SIZE];
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size == 0) continue;
        memcpy(tmp + cursor, s_data + s_entries[i].offset, s_entries[i].size);
        s_entries[i].offset = cursor;
        cursor += s_entries[i].size;
    }
    memcpy(s_data, tmp, cursor);
    s_data_used = cursor;
}

/* ============================================================== */
/* Persistencia                                                    */
/* ============================================================== */

/* Carga el estado del FS desde la región de flash. Devuelve 0 si
 * había datos válidos (magic correcto), -1 si la región está vacía
 * o corrupta. La flash mapeada por XIP es accesible como puntero
 * desde XIP_BASE + offset. */
static int load_from_flash(void) {
    const uint8_t* flash_base = (const uint8_t*)(XIP_BASE + FS_FLASH_OFFSET);
    uint32_t magic, version, count;
    memcpy(&magic,   flash_base + 0, 4);
    memcpy(&version, flash_base + 4, 4);
    memcpy(&count,   flash_base + 8, 4);
    if (magic != FS_MAGIC || version != FS_VERSION) return -1;
    if (count > FS_MAX_FILES) return -1;

    /* Limpia RAM. */
    memset(s_entries, 0, sizeof(s_entries));
    s_data_used = 0;

    /* Lee la tabla completa de entries (incluyendo libres). */
    const uint8_t* entries_ptr = flash_base + 12;
    memcpy(s_entries, entries_ptr, sizeof(s_entries));

    /* Calcula s_data_used como el max(offset+size) sobre los entries
     * válidos. Y valida ranges. */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size == 0) continue;
        if (s_entries[i].offset + s_entries[i].size > FS_DATA_SIZE) {
            /* corrupto: limpia todo y abandona. */
            memset(s_entries, 0, sizeof(s_entries));
            s_data_used = 0;
            return -1;
        }
        uint32_t end = s_entries[i].offset + s_entries[i].size;
        if (end > s_data_used) s_data_used = end;
    }

    /* Copia los datos. */
    const uint8_t* data_ptr = flash_base + FS_HEADER_BYTES;
    if (s_data_used > 0) {
        memcpy(s_data, data_ptr, s_data_used);
    }
    return 0;
}

fs_status_t fs_save_to_flash(void) {
    /* Serializa header + entries + data en un buffer en RAM, luego
     * erase + program directos al flash. Single-core con IRQs OFF
     * — patrón mínimo del SDK que no requiere pico_flash. */
    static uint8_t save_buf[FS_REGION_ALIGNED];
    memset(save_buf, 0xFF, sizeof(save_buf));   /* 0xFF = erased flash */

    uint32_t count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size != 0) count++;
    }

    /* Header. */
    uint32_t magic   = FS_MAGIC;
    uint32_t version = FS_VERSION;
    memcpy(save_buf + 0, &magic,   4);
    memcpy(save_buf + 4, &version, 4);
    memcpy(save_buf + 8, &count,   4);
    memcpy(save_buf + 12, s_entries, sizeof(s_entries));

    if (s_data_used > 0) {
        memcpy(save_buf + FS_HEADER_BYTES, s_data, s_data_used);
    }

    /* IRQs OFF durante erase+program. Single-core: con FreeRTOS
     * preempted, deshabilitamos IRQs para que el scheduler no nos
     * eche a otra task mientras estamos accediendo a XIP en modo
     * write (lo cual cuelga el chip). */
    uint32_t saved = save_and_disable_interrupts();
    flash_range_erase(FS_FLASH_OFFSET, FS_REGION_ALIGNED);
    flash_range_program(FS_FLASH_OFFSET, save_buf, sizeof(save_buf));
    restore_interrupts(saved);

    return FS_OK;
}

/* ============================================================== */
/* API pública                                                     */
/* ============================================================== */

fs_status_t fs_init(void) {
    memset(s_entries, 0, sizeof(s_entries));
    s_data_used = 0;
    s_initialized = 1;
    (void) load_from_flash();   /* si falla, FS queda vacío y OK. */
    return FS_OK;
}

void fs_format_ram(void) {
    memset(s_entries, 0, sizeof(s_entries));
    s_data_used = 0;
}

int fs_list(fs_list_cb_t cb, void* user) {
    if (!cb) return 0;
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size == 0) continue;
        count++;
        if (cb(s_entries[i].name, s_entries[i].size, user) != 0) break;
    }
    return count;
}

fs_status_t fs_get(const char* name, const uint8_t** data_out, uint32_t* size_out) {
    int idx = find_by_name(name);
    if (idx < 0) return FS_ERR_NOT_FOUND;
    if (data_out) *data_out = s_data + s_entries[idx].offset;
    if (size_out) *size_out = s_entries[idx].size;
    return FS_OK;
}

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    size_t namelen = strlen(name);
    if (namelen == 0 || namelen >= FS_NAME_LEN) return FS_ERR_NAME_TOO_LONG;
    if (size > FS_DATA_SIZE) return FS_ERR_TOO_BIG;

    /* Si ya existe, lo borramos primero. */
    int existing = find_by_name(name);
    if (existing >= 0) {
        s_entries[existing].size = 0;
        compact();
    }

    /* ¿Hay slot? */
    int slot = find_free_slot();
    if (slot < 0) return FS_ERR_TABLE_FULL;

    /* ¿Cabe? */
    if (s_data_used + size > FS_DATA_SIZE) {
        compact();    /* por si la compactación libera espacio */
        if (s_data_used + size > FS_DATA_SIZE) return FS_ERR_NO_SPACE;
    }

    /* Copia. */
    memset(s_entries[slot].name, 0, FS_NAME_LEN);
    memcpy(s_entries[slot].name, name, namelen);
    s_entries[slot].offset = s_data_used;
    s_entries[slot].size   = size;
    if (size > 0) memcpy(s_data + s_data_used, data, size);
    s_data_used += size;

    return FS_OK;
}

fs_status_t fs_delete(const char* name) {
    int idx = find_by_name(name);
    if (idx < 0) return FS_ERR_NOT_FOUND;
    s_entries[idx].size = 0;
    compact();
    return FS_OK;
}

uint32_t fs_total_bytes(void) { return FS_DATA_SIZE; }
uint32_t fs_used_bytes(void)  { return s_data_used; }
uint32_t fs_free_bytes(void)  { return FS_DATA_SIZE - s_data_used; }

int fs_file_count(void) {
    int c = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size != 0) c++;
    }
    return c;
}

const char* fs_status_str(fs_status_t s) {
    switch (s) {
        case FS_OK:                  return "OK";
        case FS_ERR_NOT_FOUND:       return "not found";
        case FS_ERR_EXISTS:          return "already exists";
        case FS_ERR_NO_SPACE:        return "no space";
        case FS_ERR_NAME_TOO_LONG:   return "name too long";
        case FS_ERR_TOO_BIG:         return "file too big";
        case FS_ERR_TABLE_FULL:      return "file table full";
        case FS_ERR_BAD_FLASH:       return "flash op failed";
        case FS_ERR_INVALID:         return "invalid argument";
        default:                     return "?";
    }
}
