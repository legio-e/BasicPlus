/*
 * fs_lfs_stm32.c — H9 · Paso 1 (#297): littlefs en el STM32 (U5). REEMPLAZA al
 * FS viejo (arena RAM + snapshot) de stm32_fs.c. Dos piezas:
 *
 * 1) CINTURA block-device de littlefs sobre la flash interna del U5:
 *      - read  : lectura MAPEADA (memcpy desde FLASH_BASE + offset).
 *      - prog  : HAL_FLASH_Program en QUADWORD (16 B) — el U5 solo programa así.
 *      - erase : HAL_FLASHEx_Erase por página (8 KB), consciente de dual-bank.
 *    Con ICACHE disable/invalidate alrededor (no servir instrucciones rancias).
 *    prog/erase DELEGAN en stm32_flash.c (primitivas COMPARTIDAS con board_mgr →
 *    un solo sitio escribe la flash del U5, sin copias que diverjan).
 *
 * 2) IMPLEMENTACIÓN del API legado stm32_fs.h SOBRE la fachada bpvm_fs+littlefs
 *    (mismo shape que fs_lfs_pico.c): fs_get a un scratch (contrato "válido hasta
 *    el siguiente fs op"), fs_put crea dirs padre, fs_count/fs_entry por recorrido
 *    BFS con paths PLANOS (como el FS viejo), fs_save no-op (littlefs committea en
 *    cada close), fs_load/boot monta. stm32_fs_register_bpvm no-op (el attach ya
 *    registró el backend).
 *
 * H9: la región del FS ya NO es fija — la pasa el boot a fs_init_at(offset,size).
 * En el Paso 1 fs_load la fija a la de siempre (BOARD_FS_FLASH_ADDR); en el Paso 2
 * la dará el env (bpvm_part). Nota: multi-.mdn con el scratch único es limitación
 * LATENTE compartida con la Pico (0-1 .mdn/RUN = caso real OK).
 */
#include "stm32_fs.h"
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"
#include "board.h"        /* BOARD_FS_FLASH_ADDR / _REGION_SIZE / _ARENA_SIZE (por placa) */

#include "main.h"         /* CMSIS: FLASH_BASE + FLASH_PAGE_SIZE (flash mapeada) */
#include "stm32_flash.h"  /* stm32_flash_write / erase — primitivas COMPARTIDAS con board_mgr */
#include <string.h>
#include <stdio.h>

/* ── región del FS (la fija fs_init_at) ───────────────────────────────── */
static uint32_t s_fs_offset = 0;   /* offset del volumen DESDE FLASH_BASE */
static uint32_t s_fs_size   = 0;   /* bytes (múltiplo de página) */

#define FS_BLOCK_SIZE   FLASH_PAGE_SIZE   /* 8 KB en U5 (página de borrado) */
#define FS_CACHE        256u              /* == BPVM_FS_LFS_CACHE; múltiplo de 16 (quadword U5) */

/* ── cintura block-device de littlefs (prog/erase → stm32_flash compartido) ── */

static int stm32_bd_read(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    const uint8_t* src = (const uint8_t*) (uintptr_t)
        (FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE + off);
    memcpy(buffer, src, size);
    return 0;
}

static int stm32_bd_prog(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    /* `size` viene de littlefs = múltiplo de FS_CACHE (256) → siempre 16-aligned. */
    uint32_t dst = FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE + off;
    return (stm32_flash_write(dst, (const uint8_t*) buffer, size) == 0) ? 0 : LFS_ERR_IO;
}

static int stm32_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    uint32_t addr = FLASH_BASE + s_fs_offset + (uint32_t) block * FS_BLOCK_SIZE;
    return (stm32_flash_erase(addr, 1u) == 0) ? 0 : LFS_ERR_IO;   /* 1 página de 8 KB */
}

static int stm32_bd_sync(const struct lfs_config* c) { (void) c; return 0; }

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en fs_lfs.c). */
static uint8_t s_read_buf[FS_CACHE];
static uint8_t s_prog_buf[FS_CACHE];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));
static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* scratch de fs_get: contrato "válido hasta el siguiente fs op" → un buffer.
 * Tamaño = arena vieja per-placa → MISMO presupuesto de SRAM que el FS anterior
 * (neutral). Alineado a 8 para el .mdn zero-copy (Thumb-2 ejecutado in-place). */
static uint8_t s_get_scratch[BOARD_FS_ARENA_SIZE] __attribute__((aligned(8)));

/* ── listado (compartido por fs_count/fs_entry y clear_lib) ────────────── */
#define LIST_MAX_ENTRIES  64
#define LIST_MAX_DIRS     16
#define LIST_NAME_MAX     64

typedef struct {
    char     names[LIST_MAX_ENTRIES][LIST_NAME_MAX];
    uint8_t  isdir[LIST_MAX_ENTRIES];
    uint32_t sizes[LIST_MAX_ENTRIES];
    int      n;
} dirlist_t;

/* El cb de la fachada corre BAJO el lock → SOLO acumula un directorio; el
 * descenso a subdirs y las mutaciones pasan FUERA del callback. */
static void dirlist_cb(const char* name, int is_dir, uint32_t size, void* user) {
    dirlist_t* d = (dirlist_t*) user;
    if (d->n >= LIST_MAX_ENTRIES) return;
    snprintf(d->names[d->n], LIST_NAME_MAX, "%s", name);
    d->isdir[d->n] = (uint8_t) is_dir;
    d->sizes[d->n] = size;
    d->n++;
}

/* snapshot BFS de paths PLANOS (raíz → "Hello.mod"; subdir → "/lib/Core.mod"),
 * como el FS viejo. Lo rellena fs_count(); fs_entry() lo lee (el wire llama
 * fs_count y luego fs_entry en secuencia). */
static char     s_snap_names[LIST_MAX_ENTRIES][LIST_NAME_MAX];
static uint32_t s_snap_sizes[LIST_MAX_ENTRIES];
static int      s_snap_n = 0;

/* Los paths planos están acotados a LIST_NAME_MAX por contrato del FS (nombres
 * ≤ 64 incl. el path); snprintf trunca a salvo → silenciamos el aviso conservador
 * -Wformat-truncation (el build STM32 va con -Wall). El -Wrestrict lo evitamos
 * copiando pending[head] a un local antes de escribir en pending[tail]. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
static void rebuild_snapshot(void) {
    static char      pending[LIST_MAX_DIRS][LIST_NAME_MAX];
    static dirlist_t dl;
    int head = 0, tail = 0;
    s_snap_n = 0;
    snprintf(pending[tail++], LIST_NAME_MAX, "/");

    while (head < tail) {
        char dir[LIST_NAME_MAX];
        snprintf(dir, sizeof(dir), "%s", pending[head++]);   /* copia local: sin aliasing con pending */
        int is_root = (dir[1] == '\0');
        dl.n = 0;
        if (bpvm_fs_list(dir, dirlist_cb, &dl) != 0) continue;
        for (int i = 0; i < dl.n; i++) {
            if (dl.isdir[i]) {
                if (tail < LIST_MAX_DIRS)
                    snprintf(pending[tail++], LIST_NAME_MAX, "%s%s%s",
                             dir, is_root ? "" : "/", dl.names[i]);
                continue;                        /* los dirs no se emiten (legado plano) */
            }
            if (s_snap_n >= LIST_MAX_ENTRIES) return;
            if (is_root) snprintf(s_snap_names[s_snap_n], LIST_NAME_MAX, "%s", dl.names[i]);
            else         snprintf(s_snap_names[s_snap_n], LIST_NAME_MAX, "%s/%s", dir, dl.names[i]);
            s_snap_sizes[s_snap_n] = dl.sizes[i];
            s_snap_n++;
        }
    }
}
#pragma GCC diagnostic pop

/* Vacía /lib tras montar: el FS viejo NO persistía /lib (fs_load lo saltaba) →
 * stm32_mods_install lo re-embebe FRESCO cada boot (evita stdlib rancia tras
 * actualizar el firmware). littlefs SÍ persiste → lo limpiamos a mano para
 * conservar esa semántica (mismo desgaste que el snapshot viejo). */
static void clear_lib(void) {
    static dirlist_t dl;
    dl.n = 0;
    if (bpvm_fs_list("/lib", dirlist_cb, &dl) != 0) return;   /* no existe aún → nada */
    for (int i = 0; i < dl.n; i++) {
        if (dl.isdir[i]) continue;                            /* /lib es plano */
        char path[LIST_NAME_MAX + 8];
        snprintf(path, sizeof(path), "/lib/%s", dl.names[i]);
        bpvm_fs_remove(path);
    }
}

/* ── crear dirs padre (el FS viejo era PLANO; littlefs necesita /lib antes de /lib/x) ── */
static void ensure_parent_dirs(const char* name) {
    const char* last = strrchr(name, '/');
    if (!last || last == name) return;          /* raíz o sin dir */
    char dir[128];
    size_t n = (size_t) (last - name);
    if (n >= sizeof(dir)) return;
    memcpy(dir, name, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);                          /* recursivo, ok-si-existe */
}

/* ── mount / boot (H9: región parametrizada) ──────────────────────────── */

int fs_init_at(uint32_t fs_offset, uint32_t fs_size) {
    s_fs_offset = fs_offset;
    s_fs_size   = fs_size;

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = stm32_bd_read;
    s_cfg.prog  = stm32_bd_prog;
    s_cfg.erase = stm32_bd_erase;
    s_cfg.sync  = stm32_bd_sync;
    s_cfg.read_size      = FS_CACHE;
    s_cfg.prog_size      = FS_CACHE;
    s_cfg.block_size     = FS_BLOCK_SIZE;          /* 8 KB (página U5) */
    s_cfg.block_count    = fs_size / FS_BLOCK_SIZE;
    s_cfg.cache_size     = FS_CACHE;
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) return -1;   /* formatea si no monta */
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    clear_lib();                                          /* stdlib siempre fresca del firmware */
    return 0;
}

int fs_load(void) {
    /* Paso 1: región FIJA de siempre (por placa). En el Paso 2 la fija el env. */
    return fs_init_at(BOARD_FS_FLASH_ADDR - FLASH_BASE, BOARD_FS_REGION_SIZE);
}

/* ── API legado stm32_fs.h sobre la fachada ───────────────────────────── */

int fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (!name || name[0] == '\0') return -1;
    ensure_parent_dirs(name);
    return (bpvm_fs_write(name, data, size, 0) == 0) ? 0 : -1;
}

int fs_get(const char* name, const uint8_t** data, uint32_t* size) {
    uint32_t sz = 0;
    if (bpvm_fs_stat(name, &sz) != 0) return -1;
    if (sz > sizeof(s_get_scratch)) return -1;              /* no cabe en el scratch */
    long n = bpvm_fs_read(name, s_get_scratch, sizeof(s_get_scratch));
    if (n < 0 || (uint32_t) n != sz) return -1;
    if (data) *data = s_get_scratch;
    if (size) *size = sz;
    return 0;
}

int fs_del(const char* name) {
    if (!bpvm_fs_exists(name)) return -1;
    return (bpvm_fs_remove(name) == 0) ? 0 : -1;
}

int fs_count(void) {
    rebuild_snapshot();
    return s_snap_n;
}

int fs_entry(int i, const char** name, uint32_t* size) {
    if (i < 0 || i >= s_snap_n) return -1;
    if (name) *name = s_snap_names[i];
    if (size) *size = s_snap_sizes[i];
    return 0;
}

uint32_t fs_total_bytes(void) {
    uint32_t t = 0;
    bpvm_fs_lfs_stats(&t, NULL);
    return t;
}

uint32_t fs_used_bytes(void) {
    uint32_t u = 0;
    bpvm_fs_lfs_stats(NULL, &u);
    return u;
}

void fs_format(void) {
    if (bpvm_fs_lfs_format() == 0) {
        bpvm_fs_mkdir("/sys");
        bpvm_fs_mkdir("/lib");
        bpvm_fs_mkdir("/app");
    }
}

void fs_save(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada) → el SAVE del
     * wire queda como no-op de compatibilidad. */
}

void stm32_fs_register_bpvm(void) {
    /* no-op: bpvm_fs_lfs_attach (en fs_init_at) ya registró el backend littlefs
     * en la fachada → readFile/writeFile/... van DIRECTOS a littlefs. */
}
