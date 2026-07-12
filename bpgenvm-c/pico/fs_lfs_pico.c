/*
 * fs_lfs_pico.c — H2·B2: littlefs en el RP2350 (Metro 16M + Pico 2 4M,
 * MISMO .uf2). Reemplaza al stopgap pico/fs.c con TRES piezas:
 *
 * 1) DESCRIPTOR DE PARTICIONES en sitio FIJO fuera del FS (huevo-y-gallina:
 *    board.json vive DENTRO del FS, no puede decir dónde está el FS).
 *    Sector 0x3FF000 (último 4K de los primeros 4 MB — existe en ambas
 *    placas). Struct binario magic+versión+regiones+CRC. Si falta/corrupto,
 *    el firmware lo AUTO-INICIALIZA con el tamaño de flash real (JEDEC):
 *      Pico 2 (4M):  FS = [0x200000, 0x3FC000)  ≈ 2 MB   (antes 128 KB)
 *      Metro (16M):  FS = [0x400000, 0x1000000) = 12 MB  (antes 128 KB)
 *    (La herramienta de PC con el "automático" fino vendrá después; hasta
 *    entonces el descriptor auto-init ES la fuente de verdad.)
 *
 * 2) CINTURA block-device de littlefs (§2.3 del plan): read por XIP
 *    (memcpy desde XIP_BASE), prog/erase por flash_range_* BAJO
 *    bpvm_flash_lock (el guard XIP de #153: IRQs off en single-core;
 *    parquea el otro core en dual-core). El quirk se absorbe AQUÍ.
 *
 * 3) SHIM del API legado fs.h SOBRE la fachada bpvm_fs: los 5 llamadores
 *    (repl_v1, repl, main, board_desc, bench) no cambian NI UNA LÍNEA →
 *    mismo comportamiento externo, motor littlefs debajo. La migración de
 *    los llamadores a la fachada directa es B2 v2 (tras validar en placa).
 *    - fs_get: read a un scratch estático (contrato documentado del API:
 *      "puntero válido hasta el siguiente fs_put/fs_delete" — los
 *      llamadores ya lo respetan; el loader COPIA el .mod a vm->memory).
 *    - fs_put: crea los dirs padre (el FS viejo era plano; littlefs no).
 *    - fs_list: paseo BFS que emite paths PLANOS como el FS viejo
 *      ("Hello.mod" en la raíz, "/lib/Core.mod" en dirs) → el wire LS no
 *      cambia. Los dirs pendientes se acumulan FUERA del callback (el cb
 *      corre bajo el lock del FS: prohibido re-entrar en la fachada).
 *    - fs_save_to_flash: no-op (littlefs committea en cada close).
 *    - fs_format_ram: reformateo EN CALIENTE + recrear /sys /lib /app.
 *
 * Nota heredada: log.c conserva su sector propio (0x3FC000) — que con el
 * fs.c viejo SOLAPABA con la región del FS (bug latente: FS_REGION_BYTES=12K
 * en log.c vs 132K reales en fs.c). Con este layout el solape desaparece:
 * el FS nuevo termina JUSTO donde empieza el log.
 */
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"
#include "flash_lock.h"
#include "board_desc.h"
#include "crc32.h"

#include "pico/stdlib.h"
#include "hardware/flash.h"

#include <string.h>
#include <stdio.h>

/* ───────────────────────── 1) descriptor de particiones ─────────────── */

#define BP_PTABLE_OFFSET   0x003FF000u          /* último 4K del 1er 4MB */
#define BP_PTABLE_MAGIC    0x42505054u          /* 'BPPT' */
#define BP_PTABLE_VERSION  1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t fs_offset;      /* offset absoluto en flash del volumen littlefs */
    uint32_t fs_size;        /* bytes (múltiplo de sector) */
    uint32_t packs_offset;   /* reservado (0 = sin zona de packs aún) */
    uint32_t packs_size;
    uint32_t reserved0;
    uint32_t crc;            /* bpvm_crc32 de los 7 campos anteriores */
} bp_ptable_t;

static bp_ptable_t s_pt;

/* límites del FS legado/log que este layout NO debe pisar (ver cabecera) */
#define BP_LOG_SECTOR      0x003FC000u          /* log.c, intacto */

static uint32_t ptable_crc(const bp_ptable_t* p) {
    return bpvm_crc32((const uint8_t*) p, sizeof(*p) - sizeof(uint32_t));
}

static int ptable_sane(const bp_ptable_t* p, uint32_t flash_bytes) {
    if (p->magic != BP_PTABLE_MAGIC || p->version != BP_PTABLE_VERSION) return 0;
    if (p->crc != ptable_crc(p)) return 0;
    if (p->fs_offset % FLASH_SECTOR_SIZE || p->fs_size % FLASH_SECTOR_SIZE) return 0;
    if (p->fs_size < 64u * 1024u) return 0;               /* mínimo útil */
    if (p->fs_offset < 0x100000u) return 0;               /* nunca sobre el firmware */
    uint32_t top = (flash_bytes >= 4u * 1024u * 1024u) ? flash_bytes
                                                       : 4u * 1024u * 1024u;
    if (p->fs_offset + p->fs_size > top) return 0;
    /* si la región cae bajo los 4MB, no puede pisar el log ni el descriptor */
    if (p->fs_offset < 0x400000u &&
        p->fs_offset + p->fs_size > BP_LOG_SECTOR) return 0;
    return 1;
}

static void ptable_defaults(bp_ptable_t* p, uint32_t flash_bytes) {
    memset(p, 0, sizeof(*p));
    p->magic   = BP_PTABLE_MAGIC;
    p->version = BP_PTABLE_VERSION;
    if (flash_bytes > 4u * 1024u * 1024u) {
        /* Metro (16M) y familia: TODO lo que hay por encima de los 4MB */
        p->fs_offset = 0x400000u;
        p->fs_size   = flash_bytes - 0x400000u;
    } else {
        /* Pico 2 (4M): [2MB, log) — el firmware (~0.5MB) va muy holgado */
        p->fs_offset = 0x200000u;
        p->fs_size   = BP_LOG_SECTOR - 0x200000u;          /* 0x1FC000 ≈ 2 MB */
    }
    p->crc = ptable_crc(p);
}

static void ptable_write(const bp_ptable_t* p) {
    /* páginas de 256: el struct (32B) viaja en la primera, resto 0xFF */
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));
    memcpy(page, p, sizeof(*p));
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(BP_PTABLE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(BP_PTABLE_OFFSET, page, sizeof(page));
    bpvm_flash_lock_end(tok);
}

/* ───────────────────────── 2) cintura block-device ──────────────────── */

static int pico_bd_read(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    const uint8_t* src = (const uint8_t*)(XIP_BASE + s_pt.fs_offset
                                          + block * FLASH_SECTOR_SIZE + off);
    memcpy(buffer, src, size);
    return 0;
}

static int pico_bd_prog(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_program(s_pt.fs_offset + block * FLASH_SECTOR_SIZE + off,
                        buffer, size);
    bpvm_flash_lock_end(tok);
    return 0;
}

static int pico_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(s_pt.fs_offset + block * FLASH_SECTOR_SIZE,
                      FLASH_SECTOR_SIZE);
    bpvm_flash_lock_end(tok);
    return 0;
}

static int pico_bd_sync(const struct lfs_config* c) { (void) c; return 0; }

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en
 * fs_lfs.c). cache_size DEBE ser BPVM_FS_LFS_CACHE (attach lo valida). */
static uint8_t s_read_buf[256];
static uint8_t s_prog_buf[256];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));

static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* ───────────────────────── 3) shim del API legado fs.h ──────────────── */

/* fs_get: contrato "válido hasta el siguiente put/delete" → un scratch.
 * 128K = mismo presupuesto SRAM que el mirror del fs.c viejo (neutral);
 * la ganancia real de RAM llega en B2 v2 al retirar el shim. */
#define FS_GET_SCRATCH_BYTES  (128u * 1024u)
static uint8_t s_get_scratch[FS_GET_SCRATCH_BYTES] __attribute__((aligned(8)));

fs_status_t fs_init(void) {
    uint32_t flash_bytes = board_desc_probe_flash_bytes();
    if (flash_bytes == 0) flash_bytes = 4u * 1024u * 1024u;   /* JEDEC raro → 4M */

    /* descriptor: leer del sitio fijo; auto-init si falta o no es sano */
    memcpy(&s_pt, (const void*)(XIP_BASE + BP_PTABLE_OFFSET), sizeof(s_pt));
    if (!ptable_sane(&s_pt, flash_bytes)) {
        ptable_defaults(&s_pt, flash_bytes);
        ptable_write(&s_pt);
        printf("[fs] descriptor de particiones AUTO-INIT (flash=%uMB)\n",
               (unsigned)(flash_bytes >> 20));
    }
    printf("[fs] littlefs: off=0x%06X size=%uKB (flash=%uMB)\n",
           (unsigned) s_pt.fs_offset, (unsigned)(s_pt.fs_size >> 10),
           (unsigned)(flash_bytes >> 20));

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = pico_bd_read;
    s_cfg.prog  = pico_bd_prog;
    s_cfg.erase = pico_bd_erase;
    s_cfg.sync  = pico_bd_sync;
    s_cfg.read_size      = 256;
    s_cfg.prog_size      = 256;                 /* página de flash */
    s_cfg.block_size     = FLASH_SECTOR_SIZE;   /* 4096 */
    s_cfg.block_count    = s_pt.fs_size / FLASH_SECTOR_SIZE;
    s_cfg.cache_size     = 256;                 /* == BPVM_FS_LFS_CACHE */
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) {   /* formatea si no monta */
        printf("[fs] ERROR: littlefs no monta ni formatea\n");
        return FS_ERR_BAD_FLASH;
    }
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    return FS_OK;
}

void fs_format_ram(void) {
    /* legado: "vaciar". Con littlefs = reformateo EN CALIENTE del volumen. */
    if (bpvm_fs_lfs_format() == 0) {
        bpvm_fs_mkdir("/sys");
        bpvm_fs_mkdir("/lib");
        bpvm_fs_mkdir("/app");
    }
}

fs_status_t fs_save_to_flash(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada, B1) —
     * el SAVE del wire queda como no-op de compatibilidad. */
    return FS_OK;
}

fs_status_t fs_get(const char* name, const uint8_t** data_out, uint32_t* size_out) {
    uint32_t sz = 0;
    if (bpvm_fs_stat(name, &sz) != 0) return FS_ERR_NOT_FOUND;
    if (sz > FS_GET_SCRATCH_BYTES) return FS_ERR_TOO_BIG;
    long n = bpvm_fs_read(name, s_get_scratch, FS_GET_SCRATCH_BYTES);
    if (n < 0 || (uint32_t) n != sz) return FS_ERR_INVALID;
    if (data_out) *data_out = s_get_scratch;
    if (size_out) *size_out = sz;
    return FS_OK;
}

/* fs_put crea los dirs padre: el FS viejo era PLANO ("/lib/x" era un nombre
 * con barras); en littlefs /lib debe existir antes de escribir /lib/x. */
static void ensure_parent_dirs(const char* name) {
    const char* last = strrchr(name, '/');
    if (!last || last == name) return;          /* raíz o sin dir */
    char dir[128];
    size_t n = (size_t)(last - name);
    if (n >= sizeof(dir)) return;
    memcpy(dir, name, n);
    dir[n] = '\0';
    bpvm_fs_mkdir(dir);                          /* recursivo, ok-si-existe */
}

fs_status_t fs_put(const char* name, const uint8_t* data, uint32_t size) {
    if (strlen(name) >= 128) return FS_ERR_NAME_TOO_LONG;
    ensure_parent_dirs(name);
    if (bpvm_fs_write(name, data, size, 0) != 0) return FS_ERR_NO_SPACE;
    return FS_OK;
}

fs_status_t fs_delete(const char* name) {
    if (!bpvm_fs_exists(name)) return FS_ERR_NOT_FOUND;
    return (bpvm_fs_remove(name) == 0) ? FS_OK : FS_ERR_INVALID;
}

/* ── fs_list: BFS emitiendo paths PLANOS como el FS viejo ──────────────
 * El cb de la fachada corre BAJO el lock → SOLO acumula; el descenso a
 * subdirs y la emisión al cliente pasan FUERA del callback. */
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
    static dir_snapshot_t snap;                  /* estáticos: fuera del stack */
    int head = 0, tail = 0;
    snprintf(pending[tail++], LIST_NAME_MAX, "/");

    while (head < tail) {
        const char* dir = pending[head++];
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
                continue;                        /* los dirs no se emiten (legado) */
            }
            /* raíz → nombre pelado ("Hello.mod"); subdir → "/lib/Core.mod" */
            if (is_root) snprintf(full, sizeof(full), "%s", snap.names[i]);
            else         snprintf(full, sizeof(full), "%s/%s", dir, snap.names[i]);
            if (cb(full, snap.sizes[i], user) != 0) return 1;
        }
    }
    return 0;
}

/* stats para INFO del IDE / logs de boot */
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

uint32_t fs_free_bytes(void) {
    uint32_t t = 0, u = 0;
    bpvm_fs_lfs_stats(&t, &u);
    return (t > u) ? (t - u) : 0;
}

static int count_cb(const char* name, uint32_t size, void* user) {
    (void) name; (void) size;
    (*(int*) user)++;
    return 0;
}

int fs_file_count(void) {
    int n = 0;
    fs_list(count_cb, &n);
    return n;
}

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
     * fachada — los builtins readFile/writeFile/... van DIRECTOS a littlefs
     * (con el lock grueso de B1.4), sin pasar por este shim. */
}
