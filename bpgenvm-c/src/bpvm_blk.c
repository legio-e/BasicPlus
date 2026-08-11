/*
 * bpvm_blk.c — V5/H6: la parte del dispositivo de bloque que no depende del medio.
 *
 * De momento sólo la lectura de la tabla de particiones. Estaba metida dentro
 * de `bpvm_fs_fat_montar`, donde no se podía probar sin placa Y sin tarjeta; y
 * es justo el sitio con casos raros. Aquí es pura, y `make test-blk` la ejerce
 * con sectores construidos a mano.
 *
 * La lógica NO cambia respecto a la que había en fs_fat.c — esto es un traslado,
 * y el criterio del paso 1 de H6 es que la Metro se comporte exactamente igual.
 */
#include "bpvm_blk.h"

uint32_t bpvm_blk_lba0_de_mbr(const uint8_t sec0[BPVM_BLK_TAM]) {
    if (!sec0) return 0;

    /* Sin la firma 0x55AA no hay ni MBR ni sector de arranque que valga. */
    if (sec0[510] != 0x55 || sec0[511] != 0xAA) return 0;

    /* ¿Es un MBR o es YA el sector de arranque del sistema de ficheros?
     *
     * Los dos llevan la misma firma al final, así que hay que mirar dentro. El
     * truco: en un sector de arranque de FAT los bytes 3..10 son el nombre del
     * formateador ("MSDOS5.0", "mkfs.fat"...), o sea ASCII imprimible; en un
     * MBR esa zona es código máquina del gestor de arranque, que casi nunca lo
     * es. No es una prueba formal —no la hay, los dos sectores son legales—
     * pero es la heurística que usa todo el mundo y falla del lado seguro: si
     * se confundiera, leería el LBA de una tabla que no existe y el montaje
     * fallaría en el sitio, no más tarde. */
    for (int i = 0; i < 8; i++) {
        uint8_t c = sec0[3 + i];
        if (c < 32 || c >= 127) goto es_mbr;
    }
    return 0;                       /* ASCII: es el propio sector del FS */

es_mbr:
    /* Las cuatro entradas de la tabla, 16 B cada una desde el 446. Se coge la
     * PRIMERA con tipo distinto de cero; las tarjetas traen una sola partición
     * y las que traen más no las hemos visto todavía. Si algún día hacen falta,
     * el sitio de elegir es éste y no está enterrado en el montaje. */
    for (int i = 0; i < 4; i++) {
        const uint8_t* e = sec0 + 446 + i * 16;
        if (e[4] == 0) continue;               /* tipo 0 = entrada vacía */
        return (uint32_t) e[8]
             | ((uint32_t) e[9]  << 8)
             | ((uint32_t) e[10] << 16)
             | ((uint32_t) e[11] << 24);
    }
    return 0;                       /* MBR con la tabla vacía: FS en el 0 */
}
