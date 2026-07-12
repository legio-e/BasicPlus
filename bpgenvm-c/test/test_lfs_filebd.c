/*
 * test_lfs_filebd.c — B0.2 (H2 fase A): block-device RESPALDADO POR FICHERO.
 *
 * La pieza que convierte la VM-C del PC en el ORÁCULO del FS (§4 del plan):
 * el mismo littlefs que irá al micro, sobre una imagen .img en disco. Este
 * test prueba la persistencia REAL entre PROCESOS (no solo entre mounts):
 *
 *   test_lfs_filebd write <img>  → formatea la imagen, escribe /sys/boot.txt
 *                                  (texto con \n y \r\n: cazaría corrupción de
 *                                  modo texto en Windows) y /app/data.bin
 *                                  (10000 bytes deterministas, multi-bloque),
 *                                  desmonta y sale.
 *   test_lfs_filebd read  <img>  → proceso NUEVO: abre la imagen existente,
 *                                  monta SIN formatear y verifica ambos
 *                                  ficheros byte a byte (+ tamaños por stat).
 *
 * El target `make test-lfsfile` encadena write → read como dos invocaciones
 * separadas. Geometría 4K×32 = 128 KB (como la región FS de hoy).
 */
#include "lfs.h"
#include "bd/lfs_filebd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLOCK_SIZE   4096u
#define BLOCK_COUNT  32u
#define BIN_LEN      10000u   /* > 2 bloques → ejercita multi-bloque */

static lfs_t        lfs;
static lfs_filebd_t bd;

static const struct lfs_filebd_config bdcfg = {
    .read_size   = 16,
    .prog_size   = 16,
    .erase_size  = BLOCK_SIZE,
    .erase_count = BLOCK_COUNT,
};

static const struct lfs_config cfg = {
    .context = &bd,
    .read  = lfs_filebd_read,
    .prog  = lfs_filebd_prog,
    .erase = lfs_filebd_erase,
    .sync  = lfs_filebd_sync,

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

/* payload de texto con finales de línea mixtos (si el bd abriera el fichero
 * en modo texto en Windows, la traducción \n<->\r\n rompería el byte a byte). */
static const char* TXT = "boot v1\nlinea2\r\nfin\n";

static uint8_t bin_byte(uint32_t i) { return (uint8_t) ((i * 7u + 3u) & 0xFFu); }

static int do_write(const char* img) {
    lfs_file_t f;
    CHK(lfs_filebd_create(&cfg, img, &bdcfg), "filebd_create");
    CHK(lfs_format(&lfs, &cfg), "format");
    CHK(lfs_mount(&lfs, &cfg), "mount");

    CHK(lfs_mkdir(&lfs, "/sys"), "mkdir /sys");
    CHK(lfs_mkdir(&lfs, "/app"), "mkdir /app");

    CHK(lfs_file_open(&lfs, &f, "/sys/boot.txt",
                      LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), "open boot.txt");
    CHK(lfs_file_write(&lfs, &f, TXT, (lfs_size_t) strlen(TXT)), "write boot.txt");
    CHK(lfs_file_close(&lfs, &f), "close boot.txt");

    static uint8_t buf[BIN_LEN];
    for (uint32_t i = 0; i < BIN_LEN; i++) buf[i] = bin_byte(i);
    CHK(lfs_file_open(&lfs, &f, "/app/data.bin",
                      LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), "open data.bin");
    CHK(lfs_file_write(&lfs, &f, buf, BIN_LEN), "write data.bin");
    CHK(lfs_file_close(&lfs, &f), "close data.bin");

    CHK(lfs_unmount(&lfs), "unmount");
    lfs_filebd_destroy(&cfg);
    printf("filebd write OK: /sys/boot.txt (%d B) + /app/data.bin (%u B) → %s\n",
           (int) strlen(TXT), BIN_LEN, img);
    return 0;
}

static int do_read(const char* img) {
    lfs_file_t     f;
    struct lfs_info info;
    CHK(lfs_filebd_create(&cfg, img, &bdcfg), "filebd_create (imagen existente)");
    /* SIN format: si la imagen no persistió bien, este mount falla. */
    CHK(lfs_mount(&lfs, &cfg), "mount en frio (proceso nuevo)");

    CHK(lfs_stat(&lfs, "/sys/boot.txt", &info), "stat boot.txt");
    if (info.size != strlen(TXT)) {
        printf("FAIL: boot.txt size=%u esperado=%d\n", (unsigned) info.size,
               (int) strlen(TXT));
        return 1;
    }
    char txt[64] = {0};
    CHK(lfs_file_open(&lfs, &f, "/sys/boot.txt", LFS_O_RDONLY), "open boot.txt");
    CHK(lfs_file_read(&lfs, &f, txt, sizeof(txt) - 1), "read boot.txt");
    CHK(lfs_file_close(&lfs, &f), "close boot.txt");
    if (strcmp(txt, TXT) != 0) { printf("FAIL: boot.txt difiere\n"); return 1; }

    CHK(lfs_stat(&lfs, "/app/data.bin", &info), "stat data.bin");
    if (info.size != BIN_LEN) {
        printf("FAIL: data.bin size=%u esperado=%u\n", (unsigned) info.size, BIN_LEN);
        return 1;
    }
    static uint8_t buf[BIN_LEN];
    CHK(lfs_file_open(&lfs, &f, "/app/data.bin", LFS_O_RDONLY), "open data.bin");
    lfs_ssize_t n = lfs_file_read(&lfs, &f, buf, BIN_LEN);
    CHK(n, "read data.bin");
    CHK(lfs_file_close(&lfs, &f), "close data.bin");
    if ((uint32_t) n != BIN_LEN) { printf("FAIL: read corto %d\n", (int) n); return 1; }
    for (uint32_t i = 0; i < BIN_LEN; i++) {
        if (buf[i] != bin_byte(i)) {
            printf("FAIL: data.bin[%u]=0x%02X esperado=0x%02X\n",
                   (unsigned) i, buf[i], bin_byte(i));
            return 1;
        }
    }

    CHK(lfs_unmount(&lfs), "unmount");
    lfs_filebd_destroy(&cfg);
    printf("filebd read OK: %u+%u bytes byte-identicos (persistio entre procesos)\n",
           (unsigned) strlen(TXT), BIN_LEN);
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3 || (strcmp(argv[1], "write") != 0 && strcmp(argv[1], "read") != 0)) {
        printf("uso: test_lfs_filebd <write|read> <imagen.img>\n");
        return 2;
    }
    return (strcmp(argv[1], "write") == 0) ? do_write(argv[2]) : do_read(argv[2]);
}
