/*
 * bpvm_fs_fat.h — V5/H2: montar la tarjeta SD como sistema de ficheros.
 *
 * Une FatFs (motor vendorizado), bpvm_sd (H1) y la fachada `bpvm_fs`. Los
 * motivos de diseño están en fs_fat.c; aquí sólo el contrato.
 */
#ifndef BPVM_FS_FAT_H
#define BPVM_FS_FAT_H

#include <stdint.h>
#include "bpvm_sd.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Arranca la tarjeta, localiza la partición, monta FAT y registra el backend
 * bajo `prefijo` (por defecto "/sd" si se pasa NULL).
 *
 * Devuelve 0, o -1 y escribe en `motivo` un texto para el log. El motivo NO es
 * decorativo: distingue "la tarjeta no contesta" (peldaño de H1, cableado) de
 * "no hay FAT32 en la partición" (la tarjeta viene en exFAT), que mandan a
 * sitios opuestos.
 *
 * ⚠️ Llamarlo DESPUÉS de que el ENV haya dado los pines. No lo llama el
 * arranque por su cuenta: hablar con hardware que puede no estar se dispara
 * cuando el usuario quiere, no cuando la placa enciende.
 */
int bpvm_fs_fat_montar(const bpvm_sd_pines_t* pines, const char* prefijo,
                       char* motivo, unsigned motivo_cap);

/*
 * Suelta el volumen: a partir de aquí todas las operaciones sobre el prefijo
 * fallan limpio en vez de hablarle a una tarjeta que ya no está.
 *
 * NO es simetría por gusto. Sin esto, sacar la tarjeta con el FS montado deja a
 * FatFs creyendo que sigue ahí: la siguiente escritura iría a un bus mudo y
 * podría dejar la FAT a medias — que es como se corrompe una tarjeta de verdad,
 * y encima en la de otro.
 */
void bpvm_fs_fat_desmontar(void);

/*
 * Mira el pin de detección y monta o desmonta si ha cambiado. Barata: si no hay
 * cambio son dos escrituras a registro y una lectura.
 *
 * Devuelve 1 si HA HABIDO cambio (para que el llamante pueda contarlo), 0 si
 * todo sigue igual.
 */
int bpvm_fs_fat_vigilar(const bpvm_sd_pines_t* pines, const char* prefijo,
                        char* motivo, unsigned motivo_cap);

/* Para el diagnóstico: dónde empezó la partición y si hay algo montado. */
uint32_t bpvm_fs_fat_lba_particion(void);
int      bpvm_fs_fat_montado(void);

/*
 * Lo que la tarjeta dice de su sistema de ficheros UNA VEZ MONTADO.
 *
 * No es adorno: que `f_mount` devuelva OK sólo prueba que el sector de arranque
 * cuadra. Esto prueba que el motor ENTIENDE la tarjeta — sabe cuánto ocupa,
 * cuánto queda y sabe recorrer el directorio raíz. Y `primera` es la respuesta
 * conocida: un nombre que se compara con lo que enseña el PC. Si sale bien, la
 * cadena entera (SPI → bloques → FAT → nombres largos) está probada; si sale
 * texto raro, la cadena falla en la traducción, no en la lectura.
 */
typedef struct {
    char     etiqueta[16];   /* nombre del volumen; "" si no tiene            */
    uint32_t kb_total;
    uint32_t kb_libres;
    int      entradas_raiz;  /* cuántas entradas tiene la raíz                */
    char     primera[64];    /* nombre de la primera — la respuesta conocida  */
} bpvm_fs_fat_resumen_t;

/* Devuelve 0, o -1 si no hay nada montado o el recorrido falla. */
int bpvm_fs_fat_resumen(bpvm_fs_fat_resumen_t* r);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_FS_FAT_H */
