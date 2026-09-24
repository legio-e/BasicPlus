/*
 * bpvm_sd_spi_env.c — V7/H3 SD.3 (24-sep): la línea `sd` del ENV, variante SPI, al registro común de la SD.
 *
 * Aparte de `bpvm_sd_blk.c` a propósito: aquél es un dispositivo de bloque PURO (lo enlaza `test-blk` sin FS,
 * sin ENV y sin registro), y esto es el pegamento del arranque, que arrastra FAT. Lo compilan las placas cuyo
 * lector de SD va por SPI —la Metro y la C5—; el SDMMC del P4 tiene el suyo en `esp32p4/main/main.c`.
 */
#include "bpvm_sd_blk.h"
#include "bpvm_sd_placa.h"
#include "bpvm_log.h"

/* V7/H3 SD.3 (24-sep) — la línea `sd` del ENV (variante SPI) al registro común. Era el bloque del arranque de
 * la Metro (V5/H1), con los mismos textos en el log; ahora lo comparte con la C5. */
int bpvm_sd_blk_registrar_desde_env(const bpvm_env_t* env) {
    char linea[96], motivo[96];
    bpvm_sd_pines_t pines;
    if (!env || bpvm_env_get(env, "sd", linea, sizeof linea) < 0) {
        bpvm_sd_placa_sin_sd("esta placa no declara lector de SD (no hay entrada 'sd' en el env)");
        log_printf("sd: esta placa no declara lector de SD (no hay entrada 'sd' en el env)");
        return 0;
    }
    if (bpvm_sd_pines_parse(linea, &pines, motivo, sizeof motivo) != 0) {
        bpvm_sd_placa_sin_sd(motivo);            /* la línea está mal escrita: el parser dice qué falta */
        log_printf("sd: %s", motivo);
        return 0;
    }
    bpvm_sd_placa_registrar(bpvm_sd_blk(&pines), &bpvm_fs_fat_ops, "SPI");   /* los pines se copian */
    log_printf("sd: pines configurados (SPI, bus %d, cs %d)", pines.bus, pines.cs);
    return 1;
}
