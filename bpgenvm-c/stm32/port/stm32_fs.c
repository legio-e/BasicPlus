/*
 * stm32_fs.c — filesystem en RAM (mini). Arena lineal + tabla de entradas.
 * Borrar compacta el arena (reclama espacio) → re-subir no crece sin fin.
 */
#include "stm32_fs.h"

#include "main.h"     /* HAL FLASH/ICACHE + CMSIS (FLASH_BASE, FLASH->OPTR) */
#include <string.h>

#define FS_MAX_FILES   40   /* 13 stdlib core en /lib + módulos de app + holgura */
#define FS_NAME_MAX    64
#define FS_ARENA_SIZE  (96u * 1024u)

typedef struct {
    char     name[FS_NAME_MAX];
    uint32_t off;       /* offset dentro del arena */
    uint32_t size;
    int      used;
} fs_entry_t;

static uint8_t    s_arena[FS_ARENA_SIZE];
static uint32_t   s_arena_used = 0;
static fs_entry_t s_files[FS_MAX_FILES];

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
    uint32_t hole_len = e->size;
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
    if (s_arena_used + size > FS_ARENA_SIZE) return -1;   /* sin espacio */
    /* Busca slot libre. */
    fs_entry_t* slot = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!s_files[i].used) { slot = &s_files[i]; break; }
    }
    if (!slot) return -1;   /* demasiados ficheros */

    slot->off  = s_arena_used;
    slot->size = size;
    strncpy(slot->name, name, FS_NAME_MAX - 1);
    slot->name[FS_NAME_MAX - 1] = '\0';
    slot->used = 1;
    if (size > 0 && data) memcpy(&s_arena[slot->off], data, size);
    s_arena_used += size;
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
 * Región: últimos 128 KB de los 2 MB (0x081E0000..0x08200000), DISJUNTA del
 * área del programa (el .ld limita FLASH a 1920 KB). ⇒ un fallo de escritura
 * NUNCA corrompe el firmware; el peor caso es perder el FS (recuperable con
 * re-subida o FORMAT).
 *
 * Layout en la región: [header 16 B][tabla N×72 B] en la 1ª página (8 KB);
 * arena cruda a partir de +8 KB. magic+version invalidan imágenes viejas.
 *
 * Escribir la misma flash desde la que se ejecuta es seguro en STM32: el
 * controlador para el bus durante cada erase/program y reanuda el fetch al
 * terminar. NO desactivamos IRQs (romperíamos los timeouts de la HAL, que usan
 * HAL_GetTick); sí la ICACHE, para no servir datos rancios tras escribir.
 * ============================================================ */

#define FS_FLASH_ADDR   0x081E0000u          /* últimos 128 KB de 2 MB */
#define FS_REGION_SIZE  (128u * 1024u)
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
