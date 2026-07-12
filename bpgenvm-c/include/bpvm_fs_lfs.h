/*
 * bpvm_fs_lfs.h — backend littlefs de la fachada bpvm_fs (H2 fase A · B1.1).
 *
 * Separado de bpvm_fs.h a propósito: este header arrastra lfs.h; la fachada
 * se mantiene lfs-free (el firmware que no use littlefs no lo toca).
 *
 * Capas:
 *   fs_lfs.c      (portable)  ops de la fachada sobre UN lfs_t montado — no
 *                             sabe de block-devices; host y micros lo comparten.
 *   fs_lfs_host.c (host-only) block-device filebd sobre una imagen .img +
 *                             bpvm_fs_register_lfs_filebd() (ver bpvm_fs.h).
 *   <micro>       (B2/B3)     su cintura read/prog/erase/sync → mismo attach.
 */
#ifndef BPVM_FS_LFS_H
#define BPVM_FS_LFS_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Monta littlefs sobre el cfg dado (el block-device lo aporta el llamante:
 * filebd en host, la cintura de flash en el micro) y registra el backend en
 * la fachada bpvm_fs. Intenta MONTAR primero; si falla y format_if_needed,
 * formatea y remonta (primer arranque / bump de formato). `cfg` debe
 * SOBREVIVIR al attach (littlefs guarda el puntero). 0 / -1. */
int  bpvm_fs_lfs_attach(const struct lfs_config* cfg, int format_if_needed);

/* Desmonta y desregistra el backend (tests / shutdown limpio). */
void bpvm_fs_lfs_detach(void);

/* B2 — stats del volumen (INFO del IDE / logs de boot). 0 / -1. */
int  bpvm_fs_lfs_stats(uint32_t* total_bytes, uint32_t* used_bytes);

/* B2 - reformateo en caliente (FORMAT del wire). 0 / -1. */
int  bpvm_fs_lfs_format(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_FS_LFS_H */
