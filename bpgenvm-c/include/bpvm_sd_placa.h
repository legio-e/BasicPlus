/*
 * bpvm_sd_placa.h — V7/H3 (24-sep): la SD de la placa, en el COMÚN.
 *
 * Hasta aquí cada familia con ranura hacía lo mismo por su cuenta —la línea `sd` del ENV, su dispositivo de
 * bloque, `bpvm_fs_fat_montar(dev, "/sd")`— y los dos verbos del diagnóstico (`SD_INFO`, `SD_MOUNT`) sólo
 * existían en la Metro, escritos para su SPI. Ahora la familia REGISTRA su dispositivo al arrancar —o el
 * porqué no lo hay— y lo demás es de aquí: el montaje de arranque con sus dos reglas, la tarjeta que entra y
 * sale en caliente, y los dos verbos, con los MISMOS campos que lee el explorador del IDE. Una placa sin
 * ranura no registra nada y contesta con el motivo, no con «type no implementado» (el veredicto de U3.0b).
 * Diseño: `docs/V7_IDEAS.md` §H3.7.
 *
 * ⚠️ El común NO enlaza FAT: las operaciones del sistema de ficheros de la tarjeta llegan por punteros al
 * registrar (`bpvm_fs_fat_ops`, definidas en `fs_fat.c`). Así sólo arrastra FatFs la imagen de una placa
 * que tiene SD — se diseña para el dispositivo pequeño.
 */
#ifndef BPVM_SD_PLACA_H
#define BPVM_SD_PLACA_H

#include <stdint.h>
#include "bpvm_blk.h"
#include "bpvm_fs_fat.h"   /* sólo el tipo del resumen: enlazar FAT es cosa de quien registra */

#ifdef __cplusplus
extern "C" {
#endif

/* Lo que el registro necesita del sistema de ficheros de la tarjeta. */
typedef struct {
    int      (*montar)(const bpvm_blk_backend_t* blk, const char* prefijo, char* motivo, unsigned cap);
    int      (*vigilar)(const bpvm_blk_backend_t* blk, const char* prefijo, char* motivo, unsigned cap);
    uint32_t (*lba_particion)(void);
    int      (*resumen)(bpvm_fs_fat_resumen_t* r);
} bpvm_sd_placa_fs_t;

/* Las de FAT, listas para registrar. Viven en `fs_fat.c`: sólo las ve quien enlaza FAT. */
extern const bpvm_sd_placa_fs_t bpvm_fs_fat_ops;

/* La SD de esta placa: su dispositivo de bloque (SPI, SDMMC, un fichero en el micro simulado), las
 * operaciones de su FS y el nombre del medio para el log ("SPI", "SDMMC"…). Una vez, al arrancar, con el
 * ENV ya leído y ANTES de `bpvm_sd_placa_montar_arranque`. */
void bpvm_sd_placa_registrar(const bpvm_blk_backend_t* dev, const bpvm_sd_placa_fs_t* fs,
                             const char* medio);

/* O el porqué no la hay («sin línea `sd` en el ENV», la línea mal escrita…): lo que contestan los dos
 * verbos. Una familia que no llama a ninguna de las dos (una placa sin ranura) contesta el motivo por
 * defecto: que esta placa no tiene SD. */
void bpvm_sd_placa_sin_sd(const char* motivo);

/* 1 si hay una SD registrada. */
int bpvm_sd_placa_hay(void);

/* El montaje de arranque, con las reglas de siempre: el PIN DE DETECCIÓN manda —con el zócalo vacío no se
 * toca el bus— (Eduardo, 8-ago), y un fallo se anota y se sigue: una tarjeta ilegible es un accesorio que
 * no va, no una placa rota. */
void bpvm_sd_placa_montar_arranque(void);

/* La tarjeta entra y sale en caliente: la familia lo llama desde su hueco ocioso; mira el pin cada medio
 * segundo y monta o desmonta si ha cambiado. Sin SD registrada no hace nada. */
void bpvm_sd_placa_vigilar(void);

/* Los dos verbos del wire; los llama `bpvm_repl_dispatch`. */
void bpvm_sd_placa_sd_info(long id);
void bpvm_sd_placa_sd_mount(long id);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SD_PLACA_H */
