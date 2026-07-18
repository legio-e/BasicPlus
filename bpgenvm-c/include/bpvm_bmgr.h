/*
 * bpvm_bmgr.h — H9: "board manager", el núcleo PORTABLE del protocolo de gestión de
 * placa (ver/editar particiones y variables de entorno). Es lo que corre en el KERNEL
 * (estado 0) detrás de los comandos de wire STATE / ENV_* / PART_*; el firmware es un
 * adaptador fino JSON↔esta API, y el IDE (FrmBoard) es el cliente.
 *
 * Compone las dos piezas ya host-probadas —bpvm_env (bloque A/B) y bpvm_part (conjunto
 * fijo, offsets derivados)— en las operaciones que expone el protocolo, y concentra la
 * decisión de "placa virgen" (faltan tamaños → proponer defaults) en un sitio probado.
 *
 * Igual que bpvm_env: SIN heap y con "buffers los pone el llamador". El bmgr trabaja
 * sobre DOS sectores A/B en RAM (`a`, `b`) + un `scratch` (>= sector). En el host esos
 * buffers son RAM; en el device, la cintura de flash por-micro los llena leyendo los dos
 * sectores del env, llama al bmgr, y cuando una operación de escritura devuelve un
 * `wrote_slot`, vuelca ese sector a flash. Así el ÚNICO trozo no portable (la flash) queda
 * en el borde. Ver docs/H9_KERNEL_CAPAS.md §Protocolo de gestión de placa.
 */
#ifndef BPVM_BMGR_H
#define BPVM_BMGR_H

#include <stdint.h>
#include <stddef.h>
#include "bpvm_env.h"
#include "bpvm_part.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Contexto del board manager. Todos los buffers los pone el llamador (kernel/host). */
typedef struct {
    uint8_t* a;             /* sector A del env (RAM; en device = copia leída de flash) */
    uint8_t* b;             /* sector B del env */
    uint8_t* scratch;       /* buffer intermedio para editar el payload (>= sector) */
    size_t   sector;        /* tamaño de sector (cada uno de a/b/scratch mide esto) */
    uint32_t part_base;     /* 1er offset de particiones (constante por familia) */
    uint32_t usable_flash;  /* flash usable YA con el clamp #292 aplicado */
} bpvm_bmgr_t;

/* --- Lectura: entorno --- */

/* Vista de solo-lectura del env actual (la copia A/B con seq mayor que pasa CRC).
 * Devuelve 1 si hay env válido (rellena `out`), 0 si la placa es virgen (sin env). */
int bpvm_bmgr_env(const bpvm_bmgr_t* bm, bpvm_env_t* out);

/* Nº de variables de entorno (0 si virgen). Para dimensionar la tabla del IDE. */
int bpvm_bmgr_env_count(const bpvm_bmgr_t* bm);

/* Par idx (0-based) → key/val NUL-terminados. 1 si existe, 0 si fuera de rango.
 * Es ENV_LS: la tabla nombre|valor de la pestaña "Variables del entorno". */
int bpvm_bmgr_env_pair_at(const bpvm_bmgr_t* bm, int idx,
                          char* key, size_t kcap, char* val, size_t vcap);

/* Valor de una clave (ENV_GET). Longitud (>=0) o -1 si no está. */
int bpvm_bmgr_env_get(const bpvm_bmgr_t* bm, const char* key, char* val, size_t vcap);

/* --- Escritura: entorno (ENV_SET / ENV_DEL) --- */

/* Fija (o crea) `key=value`; si `value` es NULL, BORRA la clave. Re-serializa a la copia
 * A/B rancia con seq+1 y deja en *wrote_slot el sector a volcar (0=A,1=B). El llamador
 * (cintura de flash) escribe ese sector. Devuelve 0 OK, -1 si no cabe / args malos. */
int bpvm_bmgr_env_set(bpvm_bmgr_t* bm, const char* key, const char* value, int* wrote_slot);

/* --- Particiones --- */

/* Layout actual (PART_LS): offsets DERIVADOS + tamaños del env. Rellena `out` y, si
 * `bad_idx` no es NULL, la partición culpable. Devuelve BPVM_PART_OK, o
 * BPVM_PART_ERR_MISSING si la placa es virgen a nivel de particiones (→ el IDE ofrece
 * defaults con bpvm_bmgr_part_defaults), o el error de validación. */
bpvm_part_err_t bpvm_bmgr_part_layout(const bpvm_bmgr_t* bm, uint32_t sector,
                                      bpvm_part_layout_t* out, int* bad_idx);

/* Tamaños por defecto (PART_DEFAULTS): reparto inicial para una placa virgen. El IDE los
 * muestra en el asistente para que el usuario los ajuste. */
void bpvm_bmgr_part_defaults(const bpvm_bmgr_t* bm, uint32_t sector,
                             uint32_t sizes_out[BPVM_PART_COUNT]);

/* Aplica tamaños de particiones (PART_APPLY): VALIDA (offsets derivados, no-cero,
 * alineado, cabe en la flash usable) y, solo si es válido, escribe los `part.<n>.size`
 * en el env (A/B, seq+1) — deja *wrote_slot para el volcado. NO reparticiona en vivo: el
 * device se reinicia y sube con el layout nuevo. Devuelve BPVM_PART_OK, o el error de
 * validación (y `bad_idx`) SIN tocar el env. `wrote_slot` solo es válido si devuelve OK. */
bpvm_part_err_t bpvm_bmgr_part_apply(bpvm_bmgr_t* bm, uint32_t sector,
                                     const uint32_t sizes[BPVM_PART_COUNT],
                                     int* bad_idx, int* wrote_slot);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BMGR_H */
