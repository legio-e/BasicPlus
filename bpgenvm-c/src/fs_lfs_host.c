/*
 * fs_lfs_host.c — constructor HOST del backend littlefs: block-device filebd
 * sobre una imagen .img en disco (H2 fase A · B1.1).
 *
 * HOST-ONLY (como fs_host.c): lfs_filebd usa open/pread/pwrite POSIX — no se
 * linka en firmware. El micro (B2/B3) montará el MISMO fs_lfs.c con su
 * cintura de flash y llamará a bpvm_fs_lfs_attach() igual que aquí.
 *
 * Éste es el modo ORÁCULO de la VM-C en PC (§4 del plan): mismo motor que el
 * micro, sobre una imagen inspeccionable en disco.
 */
#include "bpvm_fs.h"
#include "bpvm_fs_lfs.h"
#include "bd/lfs_filebd.h"
#include <string.h>

static lfs_filebd_t             s_bd;
static struct lfs_filebd_config s_bdcfg;
static struct lfs_config        s_cfg;    /* debe sobrevivir (littlefs guarda el puntero) */
static int                      s_open = 0;

int bpvm_fs_register_lfs_filebd(const char* img_path,
                                unsigned block_size, unsigned block_count,
                                int format_if_needed) {
    if (s_open) return -1;
    if (block_size == 0)  block_size  = 4096;   /* sector de flash típico */
    if (block_count == 0) block_count = 256;    /* 1 MB por defecto en host */

    memset(&s_bdcfg, 0, sizeof s_bdcfg);
    s_bdcfg.read_size   = 16;
    s_bdcfg.prog_size   = 16;
    s_bdcfg.erase_size  = block_size;
    s_bdcfg.erase_count = block_count;

    memset(&s_cfg, 0, sizeof s_cfg);
    s_cfg.context        = &s_bd;
    s_cfg.read           = lfs_filebd_read;
    s_cfg.prog           = lfs_filebd_prog;
    s_cfg.erase          = lfs_filebd_erase;
    s_cfg.sync           = lfs_filebd_sync;
    s_cfg.read_size      = 16;
    s_cfg.prog_size      = 16;
    s_cfg.block_size     = block_size;
    s_cfg.block_count    = block_count;
    s_cfg.cache_size     = 256;
    s_cfg.lookahead_size = 16;
    s_cfg.block_cycles   = 500;

    if (lfs_filebd_create(&s_cfg, img_path, &s_bdcfg) < 0) return -1;
    if (bpvm_fs_lfs_attach(&s_cfg, format_if_needed) < 0) {
        lfs_filebd_destroy(&s_cfg);
        return -1;
    }
    s_open = 1;
    return 0;
}

void bpvm_fs_lfs_filebd_close(void) {
    if (!s_open) return;
    bpvm_fs_lfs_detach();
    lfs_filebd_destroy(&s_cfg);
    s_open = 0;
}
