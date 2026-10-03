/*
 * fs_lfs_pico.c — H2·B2 + H9: littlefs en el RP2350 (Metro 16M + Pico 2 4M,
 * MISMO .uf2). DOS piezas:
 *
 * 1) CINTURA block-device de littlefs (§2.3 del plan): read por XIP
 *    (memcpy desde XIP_BASE), prog/erase por la cintura de flash de la
 *    familia (pico_flash.c, V7/H7 U1.4), que lleva dentro el candado XIP de
 *    #153 (IRQs off en single-core; parquea el otro core en dual-core).
 *
 * 2) SHIM del API legado fs.h SOBRE la fachada bpvm_fs (retirarlo = #305).
 *    - fs_put: escribe creando las carpetas que falten (el instalador de /lib).
 *    - fs_save_to_flash: no-op (littlefs committea en cada close).
 *    - fs_format_ram: reformateo EN CALIENTE + recrear /sys /lib /app.
 *    - las cuentas (bytes, ficheros) para el INFO, el DF y el log.
 * #473 (lote D, 3-oct): fuera lo que no tenía llamadores (264) y las carpetas
 * padre las crea la fachada (268, `bpvm_fs_mkdir_padres`); y el FORMAT dice si
 * pudo (466: contestaba OK pasara lo que pasara).
 *
 * H9 (unificación, 19-jul — norma de Eduardo: "si no hay partición, nada
 * con el sistema de ficheros"): el DESCRIPTOR binario propio (bp_ptable_t
 * en 0x3FF000, B2.b) MURIÓ, y con él su auto-init. La región del FS ya no
 * se decide aquí: viene del ENV (bpvm_part: tamaños `part.*.size`, offsets
 * DERIVADOS desde BP_PART_BASE) y la pasa el boot a fs_init_at(). Sin
 * particiones definidas no se monta nada — el boot se queda en estado 1,
 * reporta por STATE, y el host conduce (FrmBoard: defaults → apply →
 * reinicio). El clamp #292 vive en bpvm_part_usable_flash (el llamador).
 * El log ya no acota el volumen: se mudó a la zona 2 (flash_layout.h) y
 * el espacio [BP_PART_BASE, usable) es TODO de las particiones.
 */
#include "fs.h"
#include "bpvm_fs.h"
#include "bpvm.h"          /* #338: la zona de rascar compartida */
#include "bpvm_fs_lfs.h"
#include "pico_flash.h"     /* V7/H7 U1.4: la cintura de flash (el candado va dentro) */
#include "log.h"            /* log_printf: al log PERSISTENTE (printf = solo consola USB) */

#include "pico/stdlib.h"
#include "hardware/flash.h"

#include <string.h>
#include <stdio.h>

/* ───────────────── región del FS (viene del env vía fs_init_at) ───────
 * H9: el volumen littlefs vive en la partición FS que definió el usuario
 * (env → bpvm_part → offset derivado). Nada se auto-decide aquí. */

static uint32_t s_fs_offset = 0;   /* offset absoluto en flash del volumen */
static uint32_t s_fs_size   = 0;   /* bytes (múltiplo de sector) */

/* ───────────────────────── 1) cintura block-device ──────────────────── */

static int pico_bd_read(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, void* buffer, lfs_size_t size) {
    (void) c;
    const uint8_t* src = (const uint8_t*)(XIP_BASE + s_fs_offset
                                          + block * FLASH_SECTOR_SIZE + off);
    memcpy(buffer, src, size);
    return 0;
}

static int pico_bd_prog(const struct lfs_config* c, lfs_block_t block,
                        lfs_off_t off, const void* buffer, lfs_size_t size) {
    (void) c;
    /* V7/H7 U1.4 — por la cintura (el candado va dentro). Un rechazo es un error de
     * E/S para littlefs, no un «hecho» que deja el bloque a medias. */
    return pico_flash_write(s_fs_offset + block * FLASH_SECTOR_SIZE + off,
                            (const uint8_t*) buffer, size) == 0 ? 0 : LFS_ERR_IO;
}

static int pico_bd_erase(const struct lfs_config* c, lfs_block_t block) {
    (void) c;
    return pico_flash_erase(s_fs_offset + block * FLASH_SECTOR_SIZE,
                            FLASH_SECTOR_SIZE) == 0 ? 0 : LFS_ERR_IO;
}

static int pico_bd_sync(const struct lfs_config* c) { (void) c; return 0; }

/* buffers ESTÁTICOS de littlefs (cero malloc; los de fichero viven en
 * fs_lfs.c). cache_size DEBE ser BPVM_FS_LFS_CACHE (attach lo valida). */
static uint8_t s_read_buf[256];
static uint8_t s_prog_buf[256];
static uint8_t s_look_buf[64] __attribute__((aligned(8)));

static struct lfs_config s_cfg;   /* debe sobrevivir (littlefs guarda el ptr) */

/* ───────────────────────── 3) shim del API legado fs.h ──────────────── */

/* H11 — AQUÍ VIVÍAN 128 KB. El shim `fs_get` devolvía un puntero "válido hasta
 * el siguiente put/delete", y sostener ese contrato costaba un scratch estático
 * del tamaño del fichero más grande imaginable — el mismo presupuesto que el
 * mirror del fs.c viejo, sólo que mudado de sitio. Retirado: quien necesitaba
 * los bytes era el loader de módulos, y ahora los lee POR TROZOS directamente a
 * su sitio final (bpvm_load_mod_stream); el overlay .mdn, que sí necesita el
 * blob residente porque ejecuta en sitio, se lo pide a la arena de la VM
 * (bpvm_arena_reserve) y pide lo que ocupa de verdad. */

fs_status_t fs_init_at(uint32_t fs_offset, uint32_t fs_size) {
    /* La región viene VALIDADA por bpvm_part (alineada, no-cero, cabe en la
     * flash usable con el clamp #292). Aquí solo se monta. */
    s_fs_offset = fs_offset;
    s_fs_size   = fs_size;
    log_printf("fs: littlefs off=0x%06X size=%uKB (region del env)",
               (unsigned) s_fs_offset, (unsigned)(s_fs_size >> 10));

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg.read  = pico_bd_read;
    s_cfg.prog  = pico_bd_prog;
    s_cfg.erase = pico_bd_erase;
    s_cfg.sync  = pico_bd_sync;
    s_cfg.read_size      = 256;
    s_cfg.prog_size      = 256;                 /* página de flash */
    s_cfg.block_size     = FLASH_SECTOR_SIZE;   /* 4096 */
    s_cfg.block_count    = s_fs_size / FLASH_SECTOR_SIZE;
    s_cfg.cache_size     = 256;                 /* == BPVM_FS_LFS_CACHE */
    s_cfg.lookahead_size = 64;
    s_cfg.block_cycles   = 500;
    s_cfg.read_buffer      = s_read_buf;
    s_cfg.prog_buffer      = s_prog_buf;
    s_cfg.lookahead_buffer = s_look_buf;

    if (bpvm_fs_lfs_attach(&s_cfg, 1) != 0) {   /* formatea si no monta */
        log_printf("fs: ERROR littlefs no monta ni formatea");
        return FS_ERR_BAD_FLASH;
    }
    bpvm_fs_mkdir("/sys");
    bpvm_fs_mkdir("/lib");
    bpvm_fs_mkdir("/app");
    return FS_OK;
}

int fs_format_ram(void) {
    /* legado: "vaciar". Con littlefs = reformateo EN CALIENTE del volumen.
     * #473 (466): 0 sólo si formateó Y están las tres carpetas; el motivo de un
     * fallo queda anotado en la fachada. */
    if (bpvm_fs_lfs_format() != 0) return -1;
    if (bpvm_fs_mkdir("/sys") != 0 || bpvm_fs_mkdir("/lib") != 0 || bpvm_fs_mkdir("/app") != 0) return -1;
    return 0;
}

fs_status_t fs_save_to_flash(void) {
    /* littlefs committea en cada close (durabilidad-por-llamada, B1) —
     * el SAVE del wire queda como no-op de compatibilidad. */
    return FS_OK;
}

/* #329 — que el fallo de escritura NO MIENTA (la 3ª y última familia; ESP32 y
 * STM32 ya lo tienen). La fachada bpvm_fs colapsa todo a 0/-1 por ser
 * backend-agnóstica, y aquí ese -1 se traducía a "FS lleno" pasara lo que
 * pasara. En el S3 (#328) ese mensaje falso mandó la depuración al sitio
 * equivocado una tarde entera: el volumen estaba al 1% y el error decía que
 * estaba lleno. Ahora se reporta el lfs_error REAL y queda en el log
 * persistente, que sobrevive al reset y se saca con LOG_DUMP. */
static fs_status_t map_lfs_err(const char* op, const char* name, uint32_t size) {
    int e = bpvm_fs_lfs_last_err();
    uint32_t tot = fs_total_bytes(), usa = fs_used_bytes();
    log_printf("fs: %s '%s' (%lu B) FALLO lfs=%d %s  [libre %lu/%lu B]",
               op, name, (unsigned long) size, e, bpvm_fs_lfs_err_str(e),
               (unsigned long) ((tot > usa) ? (tot - usa) : 0u), (unsigned long) tot);
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
    (void) bpvm_fs_mkdir_padres(name);     /* #473 (268): si no puede, que lo diga el write */
    if (bpvm_fs_write(name, data, size, 0) != 0) return map_lfs_err("put", name, size);
    return FS_OK;
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

/* #461 — EL RECORRIDO PLANO DE ESTA FAMILIA SE FUE AL COMUN.
 *
 * `fs_list` + `dir_snapshot_t` existian para el verbo LIST; desde que LIST vive
 * en `src/bpvm_repl.c` (`U3.23`) su unico llamador era `fs_file_count`, o sea que
 * guardaban 96 nombres de 64 caracteres para devolver UN numero. Ahora lo hace
 * `bpvm_fs_count_files()` en la fachada, sin guardar nombres y con la cola de
 * directorios en un pool. Tres copias -> una. */
int fs_file_count(void) { return bpvm_fs_count_files(); }   /* #461 */

void fs_register_bpvm(void) {
    /* no-op: bpvm_fs_lfs_attach (en fs_init) ya registró el backend en la
     * fachada — los builtins readFile/writeFile/... van DIRECTOS a littlefs
     * (con el lock grueso de B1.4), sin pasar por este shim. */
}
