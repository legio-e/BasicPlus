/*
 * bpvm_part.h — H9: tabla de particiones sobre el bloque de env. Es la fachada
 * "kind → región" del estado 1, y la versión PORTABLE y host-testable que
 * subsumirá el descriptor binario Pico-específico `bp_ptable_t` (fs_lfs_pico.c,
 * B2.b). Las particiones viven como entradas del propio env (decisión 18-jul):
 *
 *     partitions=fs,packs
 *     part.fs.offset=1048576
 *     part.fs.size=131072
 *     part.packs.offset=1179648
 *     part.packs.size=262144
 *
 * Aquí SOLO va la lógica (parse + kind→región + validación del layout); la
 * cintura de flash por-micro NO. Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BPVM_PART_H
#define BPVM_PART_H

#include <stdint.h>
#include <stddef.h>
#include "bpvm_env.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_PART_MAX       8u    /* máximo de particiones en la tabla */
#define BPVM_PART_NAME_MAX  16u   /* longitud de nombre (con NUL) */

/* kind de una partición (la fachada resuelve por kind). El nombre no reconocido
 * cae en APP (genérico) — abierto, como el env. */
typedef enum {
    BPVM_PART_NONE = 0,
    BPVM_PART_FS,       /* volumen littlefs */
    BPVM_PART_PACKS,    /* zona de packs (XIP) */
    BPVM_PART_APP       /* genérico / futuro */
} bpvm_part_kind_t;

typedef struct {
    char             name[BPVM_PART_NAME_MAX];
    bpvm_part_kind_t kind;
    uint32_t         offset;   /* offset absoluto en flash */
    uint32_t         size;     /* bytes */
} bpvm_part_t;

typedef struct {
    bpvm_part_t parts[BPVM_PART_MAX];
    int         count;
} bpvm_part_table_t;

/* Errores de validación (los usa el asistente de FrmBoard y el STATE). */
typedef enum {
    BPVM_PART_OK = 0,
    BPVM_PART_ERR_EMPTY,          /* size 0 */
    BPVM_PART_ERR_UNALIGNED,      /* offset o size no múltiplo del sector de borrado */
    BPVM_PART_ERR_BELOW_RESERVED, /* pisa la zona reservada (imagen + env) */
    BPVM_PART_ERR_OUT_OF_FLASH,   /* se sale de la flash USABLE */
    BPVM_PART_ERR_OVERLAP         /* dos particiones se solapan */
} bpvm_part_err_t;

/* Parsea las particiones del env a `out`. Devuelve el nº de particiones (>=0), o
 * -1 si args inválidos. Sin `partitions=` → 0 (placa sin tabla = estado 1 virgen).
 * Una entrada sin offset/size se ignora (tolerante). */
int bpvm_part_parse(const bpvm_env_t* env, bpvm_part_table_t* out);

/* Fachada kind→región (1er match) y por nombre. NULL si no está. */
const bpvm_part_t* bpvm_part_find(const bpvm_part_table_t* t, bpvm_part_kind_t kind);
const bpvm_part_t* bpvm_part_find_name(const bpvm_part_table_t* t, const char* name);

/* Valida el layout contra los límites físicos: cada partición alineada al sector,
 * dentro de [reserved_end, usable_flash), sin solaparse. Devuelve BPVM_PART_OK o el
 * PRIMER error; si `bad_idx` no es NULL, escribe el índice de la partición culpable
 * (-1 para OK). `usable_flash` = bpvm_part_usable_flash (clamp #292). */
bpvm_part_err_t bpvm_part_validate(const bpvm_part_table_t* t, uint32_t usable_flash,
                                   uint32_t reserved_end, uint32_t sector, int* bad_idx);

/* Flash USABLE = min(flash real por JEDEC, lo que la IMAGEN puede escribir). El
 * clamp de #292: aunque la Metro tenga 16 MB, una imagen que declara 4 MB no puede
 * escribir más allá → usar el mínimo. image_max=0 → sin clamp (devuelve flash_bytes). */
uint32_t bpvm_part_usable_flash(uint32_t flash_bytes, uint32_t image_max);

/* Mensaje legible de un error (para el asistente / logs). */
const char* bpvm_part_err_str(bpvm_part_err_t e);

/* Serializa la tabla a un fragmento de payload de env ("partitions=...\n
 * part.X.offset=...\npart.X.size=...\n") en `buf` (cap `cap`). Lo usa el asistente
 * al construir el env. Devuelve los bytes escritos (sin NUL), o -1 si no cabe. */
int bpvm_part_to_payload(const bpvm_part_table_t* t, char* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PART_H */
