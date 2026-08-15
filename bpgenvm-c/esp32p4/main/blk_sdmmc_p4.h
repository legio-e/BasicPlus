/*
 * blk_sdmmc_p4.h — lo que la cintura SDMMC del P4 comparte con el arranque.
 *
 * De momento, una sola cosa: EL RELOJ POR DEFECTO. Vive aquí porque lo usan dos
 * ficheros y tienen que decir lo mismo — `blk_sdmmc_p4.c` lo APLICA y `main.c`
 * lo IMPRIME en la línea de configuración del arranque.
 *
 * Que estuviera sólo en el .c costó lo suyo (15-ago): el log anunciaba «0 kHz»
 * —lo que pide el env, no lo que se usa— y con eso se dio por buena una prueba
 * del bus (2 MB de patrón, ida y vuelta, 0 diferencias) sin poder anotar a qué
 * velocidad se había hecho. Un chivato de configuración que declara un valor
 * imposible es peor que no tener chivato.
 */
#ifndef BLK_SDMMC_P4_H
#define BLK_SDMMC_P4_H

/* Reloj por defecto si el ENV no dice otra cosa. CONSERVADOR a propósito: los
 * pull-ups de esta placa son de 51 K, que es flojo para ir rápido, y un bus
 * marginal no falla al montar — falla a ratos y con la placa caliente. Subir
 * esto es una decisión que se toma MIDIENDO (el patrón conocido de ida y
 * vuelta: `samples/BusTest.bp`), no por optimismo. */
#define SDIO_KHZ_POR_DEFECTO  20000

#endif /* BLK_SDMMC_P4_H */
