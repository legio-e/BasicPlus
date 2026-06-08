/*
 * fs.c — implementación del filesystem RAM + flash.
 */

#include "fs.h"
#include "bpvm_fs.h"   /* backend de file I/O para los builtins (#247) */

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "flash_lock.h"     /* #153 — ventana XIP-safe (dual-core safe) */

/* ============================================================== */
/* Layout interno                                                  */
/* ============================================================== */

#define FS_MAGIC        0x42504656u    /* 'BPFV' big-endian, but stored as u32 le */
#define FS_VERSION      4u             /* v3 → v4: ficheros alineados a 4 bytes en
                                          s_data. Bump para que fs_init invalide
                                          persistencias v3 cuyos offsets no respetaban
                                          esa alineación — al recargar se ven en
                                          posiciones erradas y .mdn no ejecuta.
                                          v2 → v3: pre-install stdlib en /lib/, Hello
                                          en /app/. */

/* Cada entry es 48 bytes (40 + 4 + 4). 64 entries -> 3072 bytes.
 * Header = 4 magic + 4 ver + 4 count + 64*48 = 3084 bytes. Reservamos
 * 4 KB para que entre holgado y quede alineado a un sector de flash. */
typedef struct {
    char     name[FS_NAME_LEN];
    uint32_t offset;     /* offset dentro del buffer de datos */
    uint32_t size;       /* bytes (0 = slot libre, pese a estar la entry) */
} fs_entry_t;

#define FS_HEADER_BYTES  4096u                              /* 1 sector */
#define FS_REGION_SIZE   (FS_HEADER_BYTES + FS_DATA_SIZE)   /* 132 KB con FS_DATA_SIZE=128K */

/* Región en flash: últimos sectores del chip.
 * Pico 2: PICO_FLASH_SIZE_BYTES = 4 MB. Reservamos los últimos
 * FS_REGION_ALIGNED bytes — con header=4K + data=128K son
 * (132 KB + 4 KB - 1) / 4 KB = 33 sectores = 132 KB exactos. */
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif
#define FS_REGION_ALIGNED    (((FS_REGION_SIZE + FLASH_SECTOR_SIZE - 1) \
                                 / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE)
#define FS_FLASH_OFFSET      (PICO_FLASH_SIZE_BYTES - FS_REGION_ALIGNED)

/* En memoria. s_data alineado a 8 para que con offsets también
 * alineados (ver fs_put / compact) cualquier fichero quede 4/8-aligned.
 * Necesario para que .mdn cargado desde FS pueda ejecutarse: el código
 * Thumb-2 requiere PC bit 1 = 0, lo que exige que el code section esté
 * al menos 2-aligned (4 da margen).
 *
 * Trade-off: los ficheros se separan con padding hasta el siguiente
 * múltiplo de 4. Para un FS con ~12 ficheros eso son <50 bytes de
 * desperdicio total. Aceptable. */
static fs_entry_t s_entries[FS_MAX_FILES];
static uint8_t    s_data[FS_DATA_SIZE] __attribute__((aligned(8)));
static uint32_t   s_data_used;          /* cuántos bytes de s_data hay usados */
static int        s_initialized = 0;

#define FS_FILE_ALIGN  4u
static inline uint32_t fs_align_up(uint32_t v) {
    return (v + (FS_FILE_ALIGN - 1u)) & ~(FS_FILE_ALIGN - 1u);
}

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

/* Compacta el buffer de datos eliminando huecos. Re-asigna offsets.
 *
 * IN-PLACE, sin buffer temporal. Procesa los ficheros vivos en orden de
 * offset ASCENDENTE y mueve cada uno a su posición compactada, que es
 * SIEMPRE <= su offset actual (sólo quitamos huecos previos). Como
 * dst <= src, memmove() es overlap-safe. Único coste de stack: un array
 * de índices de FS_MAX_FILES ints (~cientos de bytes).
 *
 * Historia (#111): la versión original usaba `uint8_t tmp[FS_DATA_SIZE]`
 * LOCAL (128 KB en el stack de vm_task de 16 KB) → stack overflow al
 * primer PUT-overwrite → corrupción + USB CDC muerta. El fix intermedio
 * lo hizo `static` (128 KB en BSS), pero eso devoraba RAM: con los tres
 * buffers de 128 KB (s_data + s_vm_buffer + este tmp) el heap C quedaba
 * en ~28 KB y CUALQUIER malloc durante un RUN hacía panic("Out of
 * memory"). Esta versión in-place no necesita buffer y recupera ~128 KB
 * de heap, sin reintroducir el stack overflow de #111. */
static void compact(void) {
    int idx[FS_MAX_FILES];
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (s_entries[i].size != 0) idx[n++] = i;
    /* insertion sort de idx por offset ascendente (n es pequeño). */
    for (int a = 1; a < n; a++) {
        int key = idx[a];
        uint32_t ko = s_entries[key].offset;
        int b = a - 1;
        while (b >= 0 && s_entries[idx[b]].offset > ko) { idx[b + 1] = idx[b]; b--; }
        idx[b + 1] = key;
    }
    uint32_t cursor = 0;
    for (int k = 0; k < n; k++) {
        int e = idx[k];
        uint32_t c = fs_align_up(cursor);
        if (c != s_entries[e].offset) {
            /* c (dst) <= offset (src) garantizado ⇒ memmove overlap-safe. */
            memmove(s_data + c, s_data + s_entries[e].offset, s_entries[e].size);
        }
        s_entries[e].offset = c;
        cursor = c + s_entries[e].size;
    }
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
    /* Serializa el header en un buffer pequeño (un sector flash = 4 KB)
     * y luego programa los datos DIRECTAMENTE desde s_data sin
     * intermediario. Antes había un `save_buf[FS_REGION_ALIGNED]`
     * estático de 132 KB que no cabía en SRAM con FS_DATA_SIZE=128K.
     *
     * flash_range_program requiere que el size sea múltiplo de
     * FLASH_PAGE_SIZE (256 bytes), así que redondeamos s_data_used
     * hacia arriba al programar la zona de datos. Los bytes extra
     * son padding 0xFF (la región está erased antes del program).
     *
     * IRQs OFF: single-core con FreeRTOS preempted, deshabilitamos
     * IRQs para que el scheduler no nos eche a otra task mientras
     * estamos accediendo a XIP en modo write — lo cual cuelga el
     * chip. */
    static uint8_t header_buf[FS_HEADER_BYTES];
    memset(header_buf, 0xFF, sizeof(header_buf));

    uint32_t count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_entries[i].size != 0) count++;
    }

    uint32_t magic   = FS_MAGIC;
    uint32_t version = FS_VERSION;
    memcpy(header_buf + 0, &magic,   4);
    memcpy(header_buf + 4, &version, 4);
    memcpy(header_buf + 8, &count,   4);
    memcpy(header_buf + 12, s_entries, sizeof(s_entries));

    /* Datos: programamos solo lo que está usado, redondeado a página. */
    uint32_t data_to_write = s_data_used;
    if (data_to_write > 0) {
        data_to_write = (data_to_write + FLASH_PAGE_SIZE - 1)
                        & ~(uint32_t)(FLASH_PAGE_SIZE - 1);
    }

    /* #153 — bajo dual-core esto ADEMÁS parquea el otro core (que si no
     * estaría ejecutando desde XIP y haría hard fault durante el erase).
     * Single-core: idéntico a save_and_disable_interrupts() de antes. */
    uint32_t saved = bpvm_flash_lock_begin();
    flash_range_erase(FS_FLASH_OFFSET, FS_REGION_ALIGNED);
    flash_range_program(FS_FLASH_OFFSET, header_buf, sizeof(header_buf));
    if (data_to_write > 0) {
        flash_range_program(FS_FLASH_OFFSET + FS_HEADER_BYTES,
                            s_data, data_to_write);
    }
    bpvm_flash_lock_end(saved);

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

    /* Alinear el offset destino a FS_FILE_ALIGN para que cada
     * fichero quede 4-aligned (necesario para .mdn ejecutable; los
     * .mod no lo necesitan pero la política es uniforme). */
    uint32_t off = fs_align_up(s_data_used);
    if (off + size > FS_DATA_SIZE) {
        compact();    /* por si la compactación libera espacio */
        off = fs_align_up(s_data_used);
        if (off + size > FS_DATA_SIZE) return FS_ERR_NO_SPACE;
    }

    /* Copia. */
    memset(s_entries[slot].name, 0, FS_NAME_LEN);
    memcpy(s_entries[slot].name, name, namelen);
    s_entries[slot].offset = off;
    s_entries[slot].size   = size;
    if (size > 0) memcpy(s_data + off, data, size);
    s_data_used = off + size;

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

/* ============================================================
 * Backend de file I/O para BP (#247): conecta readFile/writeFile/appendFile/
 * fileExists (builtins VM-C) a este FS. Paths literales (p.ej. "/app/x.txt").
 * Las escrituras persisten a flash (fs_save_to_flash). append: copia el fichero
 * a un scratch, concatena y reescribe (cap = FS_BP_SCRATCH). Persistir en cada
 * escritura bloquea ~50 ms; para un log con muchos appends conviene buffer/flush.
 * ============================================================ */

#define FS_BP_SCRATCH  (8u * 1024u)
static uint8_t s_bpfs_scratch[FS_BP_SCRATCH];

static int pico_bpfs_stat(const char* path, uint32_t* size) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != FS_OK) return -1;
    if (size) *size = sz;
    return 0;
}

static long pico_bpfs_read(const char* path, uint8_t* dst, uint32_t cap) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != FS_OK) return -1;
    uint32_t n = (sz < cap) ? sz : cap;
    if (n > 0) memcpy(dst, d, n);
    return (long) n;
}

static int pico_bpfs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
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

static const bpvm_fs_backend_t s_pico_fs_backend = {
    .stat  = pico_bpfs_stat,
    .read  = pico_bpfs_read,
    .write = pico_bpfs_write,
};

void fs_register_bpvm(void) {
    bpvm_fs_set_backend(&s_pico_fs_backend);
}
