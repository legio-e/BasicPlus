/*
 * test_lfs_smoke.c — B0.1 (H2 fase A): primer latido de littlefs vendorizado.
 *
 * Sobre el block-device de RAM de littlefs (lfs_rambd): formatea, monta, crea
 * el dir real /sys, escribe y cierra un fichero, DESMONTA + REMONTA (prueba que
 * las estructuras en el bd son coherentes en frío) y relee comparando byte a
 * byte. Es el motor que irá debajo de la fachada bpvm_fs.h — aún NO toca la VM;
 * solo valida que littlefs compila y opera limpio en host (el futuro oráculo).
 *
 * Verde = "littlefs smoke OK ...". Cualquier lfs_* < 0 aborta con el código.
 */
#include "lfs.h"
#include "bd/lfs_rambd.h"
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE   4096u
#define BLOCK_COUNT  32u          /* 128 KB, como la región FS de hoy */

static lfs_t       lfs;
static lfs_rambd_t bd;
static uint8_t     bd_buffer[BLOCK_SIZE * BLOCK_COUNT];

static const struct lfs_rambd_config bdcfg = {
    .read_size   = 16,
    .prog_size   = 16,
    .erase_size  = BLOCK_SIZE,
    .erase_count = BLOCK_COUNT,
    .buffer      = bd_buffer,     /* estático → sin malloc para el bd */
};

static const struct lfs_config cfg = {
    .context = &bd,
    .read  = lfs_rambd_read,
    .prog  = lfs_rambd_prog,
    .erase = lfs_rambd_erase,
    .sync  = lfs_rambd_sync,

    .read_size      = 16,
    .prog_size      = 16,
    .block_size     = BLOCK_SIZE,
    .block_count    = BLOCK_COUNT,
    .cache_size     = 16,
    .lookahead_size = 16,
    .block_cycles   = 500,
};

#define CHK(expr, msg) do {                                            \
    int _e = (int) (expr);                                             \
    if (_e < 0) { printf("FAIL: %s (lfs err=%d)\n", (msg), _e); return 1; } \
} while (0)

int main(void) {
    const char* payload = "hola littlefs — H2 fase A";
    char        readback[64] = {0};
    lfs_file_t  file;

    CHK(lfs_rambd_create(&cfg, &bdcfg), "rambd_create");

    CHK(lfs_format(&lfs, &cfg), "format");
    CHK(lfs_mount(&lfs, &cfg),  "mount");

    CHK(lfs_mkdir(&lfs, "/sys"), "mkdir /sys");

    CHK(lfs_file_open(&lfs, &file, "/sys/hello.txt",
                      LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), "open (w)");
    CHK(lfs_file_write(&lfs, &file, payload, (lfs_size_t) strlen(payload)), "write");
    CHK(lfs_file_close(&lfs, &file), "close (w)");

    /* Frío: desmontar + remontar antes de releer. */
    CHK(lfs_unmount(&lfs), "unmount");
    CHK(lfs_mount(&lfs, &cfg), "remount");

    CHK(lfs_file_open(&lfs, &file, "/sys/hello.txt", LFS_O_RDONLY), "open (r)");
    lfs_ssize_t n = lfs_file_read(&lfs, &file, readback, sizeof(readback) - 1);
    CHK(n, "read");
    CHK(lfs_file_close(&lfs, &file), "close (r)");
    CHK(lfs_unmount(&lfs), "unmount 2");
    lfs_rambd_destroy(&cfg);

    if ((size_t) n == strlen(payload) && strcmp(readback, payload) == 0) {
        printf("littlefs smoke OK: /sys/hello.txt = \"%s\" (%d bytes, tras remontar)\n",
               readback, (int) n);
        return 0;
    }
    printf("FAIL: releido \"%s\" (%d bytes) != escrito \"%s\"\n",
           readback, (int) n, payload);
    return 1;
}
