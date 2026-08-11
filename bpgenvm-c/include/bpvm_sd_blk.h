/*
 * bpvm_sd_blk.h — V5/H6: la tarjeta SD **por SPI**, vista como dispositivo de bloque.
 *
 * El puente entre las dos capas: arriba el contrato genérico (`bpvm_blk.h`) que
 * es lo único que conoce FatFs, y abajo el protocolo SPI de H1 (`bpvm_sd.h`).
 *
 * Este fichero existe para que `fs_fat.c` **deje de saber qué es un pin**. Antes
 * guardaba los pines y el `bpvm_sd_info_t` en sus propios estáticos y los
 * paseaba en su firma pública; ahora ese estado vive donde le corresponde, que
 * es junto al driver que lo entiende.
 *
 * Sigue siendo código COMÚN, no de una familia: el protocolo SPI de la SD es el
 * mismo en cualquier micro (ver la cabecera de `bpvm_blk.h`).
 */
#ifndef BPVM_SD_BLK_H
#define BPVM_SD_BLK_H

#include "bpvm_blk.h"
#include "bpvm_sd.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Apunta el dispositivo de bloque a ESTOS pines y devuelve el backend.
 *
 * Configurar y devolver en la misma llamada, y no en dos, porque un backend
 * entregado antes de tener pines es un objeto a medias que alguien acabará
 * usando. Así no existe ese estado.
 *
 * Los pines se COPIAN: el llamante puede tenerlos en la pila.
 * Con `pines` a NULL devuelve el backend sin tocar la configuración anterior,
 * que es lo que necesita quien sólo quiere volver a montar lo de siempre.
 */
const bpvm_blk_backend_t* bpvm_sd_blk(const bpvm_sd_pines_t* pines);

/*
 * Lo que la tarjeta dijo de sí misma en el último arranque que salió bien.
 * Para el diagnóstico (SD_INFO) — el contrato de bloque no lo lleva a propósito:
 * un CID/CSD son de una SD, y un dispositivo de bloque puede no ser una SD.
 * Devuelve NULL si todavía no ha arrancado ninguna.
 */
const bpvm_sd_info_t* bpvm_sd_blk_info(void);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_SD_BLK_H */
