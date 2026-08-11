/*
 * bpvm_blk_sdmmc.h — V5/H6 paso 2: la tarjeta por SDIO/SDMMC como dispositivo de bloque.
 *
 * El gemelo de `bpvm_sd_blk.h`, para el otro camino. Arriba, el MISMO contrato
 * (`bpvm_blk.h`) que ya usa FatFs; abajo, un mundo completamente distinto: aquí
 * el protocolo de la tarjeta lo habla el SDK del fabricante (`sdmmc_card_init`),
 * no nosotros. El porqué de que la frontera esté en el bloque y no en «el driver
 * de SD» está en la cabecera de `bpvm_blk.h`.
 *
 * ─── QUÉ ES PORTABLE Y QUÉ NO ──────────────────────────────────────────────
 *
 * Este fichero y `bpvm_blk_sdmmc_cfg.c` (la lectura del ENV) son C99 puro y se
 * prueban en el PC — `make test-sdio`. Lo que NO es portable es la
 * IMPLEMENTACIÓN del backend, que vive en la cintura de cada familia
 * (`esp32p4/main/blk_sdmmc_p4.c`) porque llama al SDK.
 *
 * Es la misma división que ya tiene el SPI, y por el mismo motivo: la parte que
 * decodifica texto es donde se esconden los errores mudos, y esa se prueba sin
 * placa.
 */
#ifndef BPVM_BLK_SDMMC_H
#define BPVM_BLK_SDMMC_H

#include "bpvm_blk.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * La configuración del zócalo. Sale del ENV, como todo lo de placa.
 *
 * ⚠️ `pwr` NO es un adorno de esta placa concreta: en la Metro el raíl de la
 * tarjeta es fijo y aquí lo conmuta un MOSFET. Eso da algo que la Metro no
 * tiene — poder CORTAR la alimentación, que es la única forma fiable de
 * resetear una tarjeta colgada a media transacción.
 */
typedef struct {
    int slot;          /* ranura del periférico (el P4 tiene dos)            */
    int clk, cmd;
    int d0;            /* obligatorios: clk, cmd, d0                        */
    int d1, d2, d3;    /* <0 en cualquiera => bus de 1 bit                  */
    int ancho;         /* 1 o 4. Lo DEDUCE el parseo, no se escribe a mano  */
    int pwr;           /* GPIO que enciende el raíl; <0 = raíl fijo         */
    int pwr_activo_alto; /* 0 = enciende poniendo el pin a 0 (lo normal)    */
    int khz;           /* reloj de trabajo; 0 = el que ponga el driver      */
} bpvm_sdio_pines_t;

/*
 * Lee la entrada `sd` del ENV en su variante SDIO. Mismo criterio que la de SPI
 * (una sola entrada, etiquetas dentro, el orden da igual):
 *
 *     sd=clk:43,cmd:44,d0:39,d1:40,d2:41,d3:42,pwr:45
 *
 * Opcionales: `slot` (0), `khz` (0), `pwr` (ninguno), `pwralto` (0 = activo
 * bajo), `d1`/`d2`/`d3` (sin ellas, 1 bit).
 *
 * ⚠️ Las claves desconocidas se IGNORAN, igual que en la de SPI — y eso tiene
 * aquí un premio inesperado: una MISMA línea de ENV puede llevar las etiquetas
 * de las dos placas (`sck/mosi/miso/cs` y `clk/cmd/d0…`), y cada firmware coge
 * las suyas. El precio es el de siempre: una errata en una clave opcional se
 * traga en silencio; una en una obligatoria sale como "falta cmd".
 *
 * Devuelve 0, o -1 y escribe el motivo.
 */
int bpvm_sdio_pines_parse(const char* valor, bpvm_sdio_pines_t* p,
                          char* motivo, unsigned motivo_cap);

/*
 * Apunta el dispositivo a ESTA configuración y devuelve el backend.
 *
 * Lo IMPLEMENTA la cintura de la familia, no este módulo. Con `pines` a NULL
 * devuelve el backend sin tocar lo configurado.
 */
const bpvm_blk_backend_t* bpvm_blk_sdmmc(const bpvm_sdio_pines_t* pines);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_BLK_SDMMC_H */
