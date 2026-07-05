/*
 * stm32_fs.c — filesystem en RAM (mini). Arena lineal + tabla de entradas.
 * Borrar compacta el arena (reclama espacio) → re-subir no crece sin fin.
 */
#include "stm32_fs.h"
#include "bpvm_fs.h"   /* backend de file I/O para los builtins (#247) */

#include "main.h"     /* HAL FLASH/ICACHE + CMSIS (FLASH_BASE, FLASH->OPTR) */
#include "board.h"    /* BOARD_FS_FLASH_ADDR (región FS reservada, por placa) */
#include <string.h>

#define FS_MAX_FILES   40   /* 13 stdlib core en /lib + módulos de app + holgura */
#define FS_NAME_MAX    64
/* Tamaño del arena (RAM) per-placa, definido en board.h (DK2 496 KB / Nucleo 96 KB).
 * Fallback conservador 96 KB si alguna placa no lo define. */
#ifndef BOARD_FS_ARENA_SIZE
#define BOARD_FS_ARENA_SIZE  (96u * 1024u)
#endif
#define FS_ARENA_SIZE  BOARD_FS_ARENA_SIZE

typedef struct {
    char     name[FS_NAME_MAX];
    uint32_t off;       /* offset dentro del arena */
    uint32_t size;
    int      used;
} fs_entry_t;

static uint8_t    s_arena[FS_ARENA_SIZE];
static uint32_t   s_arena_used = 0;
static fs_entry_t s_files[FS_MAX_FILES];

/* H9.5 — cada fichero ARRANCA 4-aligned en el arena: el loader .mdn zero-copy
 * lo exige (código Thumb-2 ejecutado in-place desde el buffer del FS; con data
 * desalineado devuelve MDN_ERR_TRUNCATED y el RUN cae a interpretado). El span
 * reservado se redondea a 4 y del/compact mueven por el span reservado, así la
 * alineación de los ficheros posteriores se preserva. fs_load reconstruye el
 * arena al boot vía fs_put, así que un FS persistido pre-alineación queda
 * realineado automáticamente al primer arranque. (El FS del Pico hace lo mismo
 * desde su v4.) */
#define FS_ALIGN4(x) (((x) + 3u) & ~3u)

static fs_entry_t* find(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].used && strcmp(s_files[i].name, name) == 0) {
            return &s_files[i];
        }
    }
    return NULL;
}

int fs_del(const char* name) {
    fs_entry_t* e = find(name);
    if (!e) return -1;
    uint32_t hole_off = e->off;
    uint32_t hole_len = FS_ALIGN4(e->size);   /* span reservado (alineado) */
    /* Guarda defensiva: una entrada legado (pre-alineación) pudo reservar solo
     * size — no comernos los primeros bytes del siguiente fichero. */
    if (hole_off + hole_len > s_arena_used) hole_len = s_arena_used - hole_off;
    /* Compacta: mueve todo lo que hay detrás del hueco hacia delante. */
    uint32_t tail = s_arena_used - (hole_off + hole_len);
    if (tail > 0) {
        memmove(&s_arena[hole_off], &s_arena[hole_off + hole_len], tail);
    }
    s_arena_used -= hole_len;
    /* Reajusta offsets de las entradas que estaban detrás del hueco. */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (s_files[i].used && s_files[i].off > hole_off) {
            s_files[i].off -= hole_len;
        }
    }
    e->used = 0;
    return 0;
}

int fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || name[0] == '\0' || strlen(name) >= FS_NAME_MAX) return -1;
    /* Sobreescritura: borra el viejo (compacta) y vuelve a añadir al final. */
    fs_del(name);
    /* H9.5 — inicio 4-aligned (tolera una cola legado desalineada) + span
     * reservado redondeado a 4. Ver nota en FS_ALIGN4. */
    uint32_t off = FS_ALIGN4(s_arena_used);
    uint32_t rsv = FS_ALIGN4(size);
    if (off + rsv > FS_ARENA_SIZE) return -1;   /* sin espacio */
    /* Busca slot libre. */
    fs_entry_t* slot = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].used) { slot = &s_files[i]; break; }
    }
    if (!slot) return -1;   /* demasiados ficheros */

    slot->off  = off;
    slot->size = size;
    strncpy(slot->name, name, FS_NAME_MAX - 1);
    slot->name[FS_NAME_MAX - 1] = '\0';
    slot->used = 1;
    if (size > 0 && data) memcpy(&s_arena[off], data, size);
    s_arena_used = off + rsv;
    return 0;
}

int fs_get(const char* name, const uint8_t** data, uint32_t* size) {
    fs_entry_t* e = find(name);
    if (!e) return -1;
    if (data) *data = &s_arena[e->off];
    if (size) *size = e->size;
    return 0;
}

int fs_count(void) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) if (s_files[i].used) n++;
    return n;
}

int fs_entry(int i, const char** name, uint32_t* size) {
    int seen = 0;
    for (int k = 0; k < FS_MAX_FILES; k++) {
        if (!s_files[k].used) continue;
        if (seen == i) {
            if (name) *name = s_files[k].name;
            if (size) *size = s_files[k].size;
            return 0;
        }
        seen++;
    }
    return -1;
}

uint32_t fs_total_bytes(void) { return FS_ARENA_SIZE; }
uint32_t fs_used_bytes(void)  { return s_arena_used; }

void fs_format(void) {
    s_arena_used = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) s_files[i].used = 0;
}

/* ============================================================
 * Persistencia en flash interna (H9.3).
 *
 * Región: los últimos BOARD_FS_REGION_SIZE bytes de la flash, en BOARD_FS_FLASH_ADDR
 * (per-placa, board.h: Nucleo 128 KB @0x081E0000 de 2 MB · DK2 512 KB @0x08380000 de
 * 4 MB). DISJUNTA del área del programa (el .ld le recorta esos KB a FLASH) ⇒ un fallo
 * de escritura NUNCA corrompe el firmware; el peor caso es perder el FS (recuperable
 * con re-subida o FORMAT).
 *
 * Layout en la región: [header 16 B][tabla N×72 B] en la 1ª página (8 KB);
 * arena cruda a partir de +8 KB. magic+version invalidan imágenes viejas.
 *
 * Escribir la misma flash desde la que se ejecuta es seguro en STM32: el
 * controlador para el bus durante cada erase/program y reanuda el fetch al
 * terminar. NO desactivamos IRQs (romperíamos los timeouts de la HAL, que usan
 * HAL_GetTick); sí la ICACHE, para no servir datos rancios tras escribir.
 * ============================================================ */

#define FS_FLASH_ADDR   BOARD_FS_FLASH_ADDR   /* región FS reservada (por placa, ver board.h) */
/* Tamaño de la región de persistencia per-placa (board.h). Debe ser ≥ 8 KB (cabecera) +
 * FS_ARENA_SIZE y múltiplo de la página de flash (8 KB en U5). Fallback conservador. */
#ifndef BOARD_FS_REGION_SIZE
#define BOARD_FS_REGION_SIZE  (128u * 1024u)
#endif
#define FS_REGION_SIZE  BOARD_FS_REGION_SIZE
#define FS_HDR_BYTES    0x2000u              /* 1 página (8 KB): header + tabla */
#define FS_MAGIC        0x42504653u          /* 'BPFS' */
#define FS_VERSION      1u

typedef struct { uint32_t magic, version, count, arena_used; } fs_flash_hdr_t;
typedef struct { char name[FS_NAME_MAX]; uint32_t off, size; } fs_flash_entry_t;

/* header+tabla en RAM antes de programar (≤ 1 página). */
static uint8_t s_flash_hdr[sizeof(fs_flash_hdr_t) + FS_MAX_FILES * sizeof(fs_flash_entry_t)]
    __attribute__((aligned(8)));

/* Traduce una dirección de flash a (banco, página) según el modo (dual/single). */
static void addr_to_bank_page(uint32_t addr, uint32_t* bank, uint32_t* page) {
    uint32_t off = addr - FLASH_BASE;
    if ((FLASH->OPTR & FLASH_OPTR_DUALBANK) && off >= FLASH_BANK_SIZE) {
        *bank = FLASH_BANK_2;
        *page = (off - FLASH_BANK_SIZE) / FLASH_PAGE_SIZE;
    } else {
        *bank = FLASH_BANK_1;
        *page = off / FLASH_PAGE_SIZE;
    }
}

/* Programa `len` bytes en `dst` por quad-words (16 B); rellena el último con 0xFF. */
static HAL_StatusTypeDef flash_program_block(uint32_t dst, const uint8_t* src, uint32_t len) {
    for (uint32_t o = 0; o < len; o += 16u) {
        uint32_t qw[4];
        memset(qw, 0xFF, sizeof(qw));
        uint32_t n = len - o; if (n > 16u) n = 16u;
        memcpy(qw, src + o, n);
        HAL_StatusTypeDef st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
                                                 dst + o, (uint32_t) (uintptr_t) qw);
        if (st != HAL_OK) return st;
    }
    return HAL_OK;
}

void fs_save(void) {
    /* 1) Serializa header + tabla en RAM. */
    fs_flash_hdr_t*   h   = (fs_flash_hdr_t*) (void*) s_flash_hdr;
    fs_flash_entry_t* tbl = (fs_flash_entry_t*) (void*) (s_flash_hdr + sizeof(fs_flash_hdr_t));
    memset(s_flash_hdr, 0, sizeof(s_flash_hdr));
    uint32_t n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].used) continue;
        strncpy(tbl[n].name, s_files[i].name, FS_NAME_MAX - 1);
        tbl[n].name[FS_NAME_MAX - 1] = '\0';
        tbl[n].off  = s_files[i].off;
        tbl[n].size = s_files[i].size;
        n++;
    }
    h->magic = FS_MAGIC; h->version = FS_VERSION;
    h->count = n; h->arena_used = s_arena_used;
    uint32_t hdr_len = (uint32_t) (sizeof(fs_flash_hdr_t) + n * sizeof(fs_flash_entry_t));

    /* 2) Erase de la región (toda en un banco: 16 páginas de 8 KB). */
    uint32_t bank, page;
    addr_to_bank_page(FS_FLASH_ADDR, &bank, &page);
    FLASH_EraseInitTypeDef er = {0};
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.Banks     = bank;
    er.Page      = page;
    er.NbPages   = FS_REGION_SIZE / FLASH_PAGE_SIZE;

    HAL_ICACHE_Disable();
    HAL_FLASH_Unlock();
    uint32_t pe = 0;
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&er, &pe);
    if (st == HAL_OK) {
        st = flash_program_block(FS_FLASH_ADDR, s_flash_hdr, hdr_len);
    }
    if (st == HAL_OK && s_arena_used > 0) {
        st = flash_program_block(FS_FLASH_ADDR + FS_HDR_BYTES, s_arena, s_arena_used);
    }
    HAL_FLASH_Lock();
    HAL_ICACHE_Invalidate();
    HAL_ICACHE_Enable();
    (void) st;   /* best-effort: si falló, el próximo fs_load ve magic inválido. */
}

int fs_load(void) {
    const uint8_t* base = (const uint8_t*) FS_FLASH_ADDR;     /* flash mapeada */
    const fs_flash_hdr_t* h = (const fs_flash_hdr_t*) (const void*) base;
    if (h->magic != FS_MAGIC || h->version != FS_VERSION) return -1;
    if (h->count > (uint32_t) FS_MAX_FILES || h->arena_used > FS_ARENA_SIZE) return -1;

    fs_format();   /* parte de cero y reconstruye */
    const fs_flash_entry_t* tbl = (const fs_flash_entry_t*) (const void*) (base + sizeof(fs_flash_hdr_t));
    const uint8_t* data = base + FS_HDR_BYTES;
    for (uint32_t i = 0; i < h->count; i++) {
        if ((uint64_t) tbl[i].off + tbl[i].size > h->arena_used) continue;  /* entrada corrupta */
        /* /lib lo re-instala el firmware embebido al boot → no lo restauramos
         * (evita usar una stdlib vieja persistida tras actualizar el firmware). */
        if (strncmp(tbl[i].name, "/lib/", 5) == 0) continue;
        fs_put(tbl[i].name, data + tbl[i].off, tbl[i].size);
    }
    return 0;
}

/* ============================================================
 * Backend de file I/O para BP (#247): conecta readFile/writeFile/appendFile/
 * fileExists (builtins de la VM-C) a este FS. Paths literales (p.ej.
 * "/app/data.txt"). Las escrituras persisten (fs_save) → sobreviven al reset
 * (salvo /lib, que re-provee el embebido al boot). append: copia el fichero a
 * un scratch, concatena y reescribe (cap = FS_BP_SCRATCH; ficheros mayores →
 * error). Nota: persistir en cada escritura es ~150 ms (erase+program); para
 * un log con muchos appends conviene una capa de buffer/flush por encima.
 * ============================================================ */

#define FS_BP_SCRATCH  (8u * 1024u)
static uint8_t s_bpfs_scratch[FS_BP_SCRATCH];

static int stm32_bpfs_stat(const char* path, uint32_t* size) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != 0) return -1;
    if (size) *size = sz;
    return 0;
}

static long stm32_bpfs_read(const char* path, uint8_t* dst, uint32_t cap) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(path, &d, &sz) != 0) return -1;
    uint32_t n = (sz < cap) ? sz : cap;
    if (n > 0) memcpy(dst, d, n);
    return (long) n;
}

static int stm32_bpfs_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    int rc;
    if (append) {
        const uint8_t* old; uint32_t oldsz;
        if (fs_get(path, &old, &oldsz) == 0) {
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
    if (rc != 0) return -1;
    fs_save();   /* persiste; /lib no se restaura al boot (lo re-embebe el firmware) */
    return 0;
}

/* #240 (logger) — borrado y rename (rotación del log). rename = get+put+del
 * sobre el scratch (el FS no tiene rename nativo); sobreescribe el destino,
 * como la VM-Java (REPLACE_EXISTING). */
static int stm32_bpfs_remove(const char* path) {
    if (fs_del(path) != 0) return -1;
    fs_save();
    return 0;
}

static int stm32_bpfs_rename(const char* from, const char* to) {
    const uint8_t* d; uint32_t sz;
    if (fs_get(from, &d, &sz) != 0) return -1;
    if (sz > FS_BP_SCRATCH) return -1;
    memcpy(s_bpfs_scratch, d, sz);            /* copia ANTES de fs_put */
    if (fs_put(to, s_bpfs_scratch, sz) != 0) return -1;
    fs_del(from);
    fs_save();
    return 0;
}

static const bpvm_fs_backend_t s_stm32_fs_backend = {
    .stat   = stm32_bpfs_stat,
    .read   = stm32_bpfs_read,
    .write  = stm32_bpfs_write,
    .remove = stm32_bpfs_remove,
    .rename = stm32_bpfs_rename,
};

void stm32_fs_register_bpvm(void) {
    bpvm_fs_set_backend(&s_stm32_fs_backend);
}
