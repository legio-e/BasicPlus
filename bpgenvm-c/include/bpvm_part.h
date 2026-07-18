/*
 * bpvm_part.h — H9: tabla de particiones (estado 1). Modelo (Eduardo, 18-jul):
 * las particiones son un CONJUNTO FIJO y ORDENADO (las que son, en su orden); el
 * usuario NO las crea/borra — solo edita TAMAÑOS. Por eso los OFFSETS se DERIVAN
 * (contiguos desde el fin de la zona reservada) y NO se guardan: no pueden
 * contradecir al orden, y no hay solapes por construcción. El env guarda SOLO
 * los tamaños (`part.fs.size`, `part.packs.size`); una sola fuente de verdad.
 *
 * La primera vez (env sin tamaños): se proponen defaults (bpvm_part_defaults), el
 * usuario ajusta y confirma → se escriben `part.<n>.size` en el env (via
 * bpvm_env_payload_set). Es la versión portable/host-testable que subsumirá el
 * bp_ptable_t binario Pico (fs_lfs_pico.c, B2.b). Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BPVM_PART_H
#define BPVM_PART_H

#include <stdint.h>
#include <stddef.h>
#include "bpvm_env.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Conjunto FIJO y ORDENADO de particiones. El orden ES el layout en flash
 * (offsets contiguos desde la base, en este orden). Ampliar = añadir al final. */
typedef enum {
    BPVM_PART_FS = 0,     /* volumen littlefs (1º) */
    BPVM_PART_PACKS,      /* zona de packs XIP (2º) */
    BPVM_PART_COUNT       /* nº de particiones (fijo) */
} bpvm_part_kind_t;

typedef struct {
    bpvm_part_kind_t kind;
    uint32_t         offset;   /* DERIVADO: contiguo desde la base, en orden */
    uint32_t         size;     /* del env (o default la 1ª vez) */
} bpvm_part_t;

typedef struct {
    bpvm_part_t parts[BPVM_PART_COUNT];
    uint32_t    base;          /* fin de la zona reservada (imagen + env) = 1er offset */
    uint32_t    usable_flash;  /* clamp #292 */
    int         complete;      /* 1 si el env traía todos los tamaños (no virgen) */
} bpvm_part_layout_t;

typedef enum {
    BPVM_PART_OK = 0,
    BPVM_PART_ERR_MISSING,     /* falta algún tamaño en el env (1ª vez / virgen) */
    BPVM_PART_ERR_ZERO,        /* un tamaño es 0 */
    BPVM_PART_ERR_UNALIGNED,   /* un tamaño no es múltiplo del sector de borrado */
    BPVM_PART_ERR_OVERFLOW     /* la suma no cabe en [base, usable_flash) */
} bpvm_part_err_t;

/* Nombre de una partición (la clave en el env es `part.<name>.size`). */
const char* bpvm_part_name(bpvm_part_kind_t k);

/* Flash USABLE = min(flash real por JEDEC, lo que la imagen puede escribir) — clamp
 * de #292. image_max=0 → sin clamp. */
uint32_t bpvm_part_usable_flash(uint32_t flash_bytes, uint32_t image_max);

/* Propone TAMAÑOS por defecto (1ª vez): reparte el espacio disponible
 * [base, usable_flash) entre las particiones, alineado al sector. El usuario los
 * ajusta luego. Escribe sizes_out[BPVM_PART_COUNT]. */
void bpvm_part_defaults(uint32_t base, uint32_t usable_flash, uint32_t sector,
                        uint32_t sizes_out[BPVM_PART_COUNT]);

/* Construye el layout desde el env: lee `part.<name>.size` de cada partición EN
 * ORDEN, deriva los offsets contiguos desde `base`, y valida (no-cero, alineado al
 * sector, la suma cabe). Rellena `out` (offsets+sizes) y devuelve OK o el 1er
 * error; si `bad_idx` no es NULL, el índice de la partición culpable (-1 si OK/none).
 * Si falta algún tamaño → BPVM_PART_ERR_MISSING (out->complete=0): es una placa
 * virgen a nivel de particiones → el asistente propone defaults. */
bpvm_part_err_t bpvm_part_layout(const bpvm_env_t* env, uint32_t base,
                                 uint32_t usable_flash, uint32_t sector,
                                 bpvm_part_layout_t* out, int* bad_idx);

/* Igual que bpvm_part_layout pero desde un array de tamaños ya en mano (lo usa el
 * asistente para validar lo que el usuario tecleó antes de escribir el env). */
bpvm_part_err_t bpvm_part_layout_from_sizes(const uint32_t sizes[BPVM_PART_COUNT],
                                            uint32_t base, uint32_t usable_flash,
                                            uint32_t sector, bpvm_part_layout_t* out,
                                            int* bad_idx);

/* Fachada kind→región (offset+size derivados). NULL si el kind está fuera de rango. */
const bpvm_part_t* bpvm_part_get(const bpvm_part_layout_t* lay, bpvm_part_kind_t k);

/* Mensaje legible de un error (para el asistente / logs). */
const char* bpvm_part_err_str(bpvm_part_err_t e);

/* Serializa SOLO los tamaños a un fragmento de payload de env
 * ("part.fs.size=...\npart.packs.size=...\n") en `buf` (cap). Lo usa el asistente
 * al confirmar. Devuelve los bytes escritos, o -1 si no cabe. */
int bpvm_part_sizes_to_payload(const uint32_t sizes[BPVM_PART_COUNT],
                               char* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PART_H */
