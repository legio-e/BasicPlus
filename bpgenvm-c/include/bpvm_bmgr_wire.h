/*
 * bpvm_bmgr_wire.h — H9: núcleo ÚNICO del adaptador de wire de gestión de placa.
 *
 * Despacha UN comando (STATE, ENV_x, PART_x) contra el bpvm_bmgr y construye la línea
 * JSON de reply. Lo comparten el boardsim (TCP, host) y el firmware (USB) → las
 * replies son BYTE-IDÉNTICAS por construcción (misma filosofía dual-VM: una sola
 * fuente de verdad, y el firmware hereda replies ya host-verificadas por el smoke
 * del sim). El llamador solo pone: parsear su JSON al `req`, enviar `out`, y —si
 * hubo escritura— volcar el sector A/B indicado a su "flash" (fichero en host,
 * flash_range_* en el device). Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BPVM_BMGR_WIRE_H
#define BPVM_BMGR_WIRE_H

#include <stddef.h>
#include "bpvm_bmgr.h"
#include "bpvm_part.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Request ya EXTRAÍDO del JSON por el llamador (cada lado con su json_min). Campos
 * ausentes: has_key/has_value = 0; part_sizes[i] = -1. */
typedef struct {
    char type[32];                      /* "STATE", "ENV_SET", "PART_APPLY", … */
    long id;                            /* id de correlación del wire */
    char key[64];                       /* ENV_GET/SET/DEL */
    int  has_key;
    char value[192];                    /* ENV_SET (un valor de env: nombre/número/path) */
    int  has_value;
    long part_sizes[BPVM_PART_COUNT];   /* PART_APPLY (por partición, en bytes); -1 si falta */
} bpvm_bmgr_req_t;

/* Despacha el comando → escribe la línea JSON de reply en `out` (SIN '\n'; el
 * llamador enmarca y envía). Devuelve la longitud (>0), o -1 si no cabe en `out`.
 * En *wrote_slot deja el sector A/B modificado (0=A, 1=B) que el llamador debe
 * volcar a flash, o -1 si no hubo escritura. Un verbo desconocido produce un reply
 * ERROR UNSUPPORTED (longitud válida, wrote_slot=-1). */
int bpvm_bmgr_wire_dispatch(bpvm_bmgr_t* bm, const bpvm_bmgr_req_t* req,
                            char* out, size_t out_cap, int* wrote_slot);

/* Estado de arranque derivado del bmgr (para STATE y para el "escrutinio" del IDE):
 * virgen (sin env) → 0 (kernel); env pero sin particiones → 1; completo → 3. */
int bpvm_bmgr_wire_state(const bpvm_bmgr_t* bm);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BMGR_WIRE_H */
