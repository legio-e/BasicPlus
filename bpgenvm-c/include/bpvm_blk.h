/*
 * bpvm_blk.h — V5/H6: el DISPOSITIVO DE BLOQUE, y por qué la frontera está aquí.
 *
 * ─── EL HALLAZGO QUE OBLIGA A ESTE FICHERO ─────────────────────────────────
 *
 * Hasta H2 sólo había una forma de hablar con una tarjeta —SPI— y el driver
 * (`bpvm_sd.c`) resultó ser portable: el diálogo CMD0/CMD8/ACMD41/CMD17 es el
 * mismo en cualquier micro, y va sobre las fachadas `bpvm_spi_*`/`bpvm_gpio_*`.
 * Así que FatFs podía llamarlo directamente y no pasaba nada.
 *
 * El P4 rompe eso, y no por ser otra familia:
 *
 *     Metro (SPI)    FatFs -> disk_read -> bpvm_sd.c     <- el protocolo es NUESTRO
 *     P4    (SDMMC)  FatFs -> disk_read -> ?             <- el protocolo es del SDK
 *
 * En SDMMC los comandos los manda `sdmmc_card_init()` del ESP-IDF; ahí
 * `bpvm_sd.c` **no tiene sitio**. O sea que **la frontera no es «familia», es
 * QUIEN HABLA EL PROTOCOLO**. Y lo único común a los dos caminos está una capa
 * por debajo de FatFs: *dame N bloques, toma N bloques, cuantos hay, hay medio*.
 *
 * Eso es este fichero. Encima de aquí (FatFs, la fachada `bpvm_fs`, los verbos
 * del wire) se reutiliza ENTERO en las dos placas; debajo hay dos
 * implementaciones que no se parecen en nada.
 *
 * ⚠️ Ojo al repartir el proyecto por mitades (V6): `bpvm_sd.c` es codigo COMUN
 * aunque hoy solo lo use la Pico. Empujarlo a la mitad de hardware por estar
 * cerca de una placa obligaria a duplicarlo el dia que aparezca un STM32 con el
 * zocalo cableado a SPI. Es protocolo, no familia.
 *
 * ─── LA FORMA: LA MISMA QUE EL RESTO DE BACKENDS DE LA CASA ────────────────
 *
 * Struct de punteros a funcion sin `ctx`, igual que `bpvm_fs_backend_t` y
 * `bpvm_spi_backend_t`: el estado vive en el fichero que implementa. No es
 * pereza — hay UNA tarjeta y UN volumen (`fs_fat.c` ya lo dice), y un `ctx` que
 * nadie necesita es un puntero mas que puede venir mal.
 *
 * ─── EL RESULTADO NO ES UN BOOL ────────────────────────────────────────────
 *
 * Se conserva el criterio de H1: `motivo()` devuelve el PELDAÑO donde se paro,
 * no «fallo». La diferencia entre «no hay tarjeta», «hay tarjeta pero el bus
 * esta mudo» y «contesta pero no arranca» manda a sitios completamente
 * distintos — zocalo, cableado, alimentacion.
 */
#ifndef BPVM_BLK_H
#define BPVM_BLK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* El bloque son 512 B SIEMPRE. No es una simplificacion: es lo que fija el
 * direccionamiento por LBA de las tarjetas SD/SDHC/SDXC, y lo que FatFs tiene
 * configurado (FF_MIN_SS == FF_MAX_SS == 512). Si algun dia entra un medio con
 * otro tamaño, sera un cambio consciente en los dos sitios y no una sorpresa. */
#define BPVM_BLK_TAM 512u

typedef struct {
    /* Arranca el medio y deja `bloques()` contestando. 0 = bien, -1 = no.
     * Lo llama el montaje; volver a llamarlo con el medio ya listo tiene que
     * ser inofensivo. */
    int (*init)(void);

    /* Lee/escribe `n` bloques CONSECUTIVOS desde `lba`. 0 = bien, -1 = no.
     *
     * El `n` esta en el contrato a proposito aunque hoy la implementacion de
     * SPI haga un bucle de bloques sueltos: en SDMMC la lectura multiple
     * (CMD18/CMD25) es donde esta la ganancia, y si el contrato pidiera bloques
     * de uno en uno la habriamos tirado antes de empezar. */
    int (*leer)    (uint32_t lba, uint32_t n, uint8_t* dst);
    int (*escribir)(uint32_t lba, uint32_t n, const uint8_t* src);

    /* ¿Hay medio metido? 1 sí, 0 no.
     * Si la placa no tiene detector devuelve 1: no podemos saberlo, y decir
     * «no hay» seria mentir. */
    int (*hay_medio)(void);

    /* Capacidad del MEDIO en bloques (no de la particion). 0 si no se sabe. */
    uint32_t (*bloques)(void);

    /* Fuerza el volcado de lo que el medio tenga en el aire. NULL si no hace
     * falta — en SPI no lo hace: `bpvm_sd_escribir_bloque` ya espera a que la
     * tarjeta suelte la linea de ocupada antes de volver. */
    int (*sincronizar)(void);

    /* El PELDAÑO del ultimo fallo, en texto. Literal estatico, nunca se
     * libera. Nunca NULL: si no ha fallado nada, devuelve algo inocuo. */
    const char* (*motivo)(void);
} bpvm_blk_backend_t;

/*
 * ¿En que bloque empieza el sistema de ficheros, segun el sector 0?
 *
 * Una SD viene con MBR y el FS **no** empieza en el bloque 0 (en la tarjeta de
 * banco empieza en el 2048; en una de 128 GB sin reformatear, en el 32768).
 * Montar en el 0 es el error clasico.
 *
 * Vive aqui, y como FUNCION PURA, por el mismo motivo que
 * `bpvm_sd_csd_capacidad`: es una decodificacion con casos raros —una tarjeta
 * «superfloppy» sin particionar, un sector de arranque que no es MBR— donde
 * equivocarse **no revienta**, devuelve un numero. Y un numero equivocado aqui
 * se manifiesta como «la tarjeta no tiene FAT32», que manda a reformatear una
 * tarjeta que estaba bien. Siendo pura se prueba en el PC con sectores reales.
 *
 * Devuelve el LBA de la primera particion, o 0 si no hay MBR (que tambien es
 * la respuesta correcta: entonces el FS empieza en el bloque 0).
 */
uint32_t bpvm_blk_lba0_de_mbr(const uint8_t sec0[BPVM_BLK_TAM]);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_BLK_H */
