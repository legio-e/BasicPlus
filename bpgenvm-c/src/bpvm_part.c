/*
 * bpvm_part.c — H9: tabla de particiones (estado 1). Conjunto FIJO y ORDENADO;
 * el usuario solo edita TAMAÑOS; los OFFSETS se DERIVAN (contiguos, en orden) → sin
 * solapes por construcción y una sola fuente de verdad (el env guarda solo tamaños).
 * Solo lógica, host-testable, sin heap. Ver bpvm_part.h.
 */
#include "bpvm_part.h"

#include <string.h>
#include <stdio.h>   /* snprintf (solo en sizes_to_payload, ruta no-caliente) */

/* Nombre fijo por kind (la clave del env es part.<name>.size). El ORDEN de este
 * array ES el orden del layout en flash. */
static const char* const PART_NAMES[BPVM_PART_COUNT] = {
    "fs",     /* BPVM_PART_FS */
    "packs"   /* BPVM_PART_PACKS */
};

const char* bpvm_part_name(bpvm_part_kind_t k) {
    if ((int) k < 0 || (int) k >= (int) BPVM_PART_COUNT) return "?";
    return PART_NAMES[k];
}

uint32_t bpvm_part_usable_flash(uint32_t flash_bytes, uint32_t image_max) {
    if (image_max > 0 && flash_bytes > image_max) return image_max;   /* clamp #292 */
    return flash_bytes;
}

static uint32_t align_down(uint32_t v, uint32_t sector) {
    if (sector == 0) return v;
    return v - (v % sector);
}

void bpvm_part_defaults(uint32_t base, uint32_t usable_flash, uint32_t sector,
                        uint32_t sizes_out[BPVM_PART_COUNT]) {
    uint32_t avail = (usable_flash > base) ? (usable_flash - base) : 0;
    avail = align_down(avail, sector);
    /* Reparto por defecto simple (el usuario lo ajusta): FS la mitad (alineada),
     * PACKS el resto. La última partición se lleva lo que quede → suma exacta. */
    uint32_t fs = align_down(avail / 2u, sector);
    uint32_t packs = (avail > fs) ? (avail - fs) : 0;
    sizes_out[BPVM_PART_FS]    = fs;
    sizes_out[BPVM_PART_PACKS] = packs;
}

bpvm_part_err_t bpvm_part_layout_from_sizes(const uint32_t sizes[BPVM_PART_COUNT],
                                            uint32_t base, uint32_t usable_flash,
                                            uint32_t sector, bpvm_part_layout_t* out,
                                            int* bad_idx) {
    if (bad_idx) *bad_idx = -1;
    if (!sizes || !out) return BPVM_PART_ERR_ZERO;
    out->base = base;
    out->usable_flash = usable_flash;
    out->complete = 1;

    /* La zona de DATOS [base, usable) alineada al sector es UNA sola región que se
     * reparten FS y Packs (Eduardo 19-jul: "FS + Packs es uno y el límite entre los
     * dos lo cambiamos cuando queramos"). El único MANDO es el tamaño de las knobs
     * (todas menos la última = hoy solo FS); la ÚLTIMA (packs) se lleva EL RESTO. */
    uint32_t avail = (usable_flash > base) ? (usable_flash - base) : 0;
    avail = align_down(avail, sector);

    uint32_t off  = base;
    uint32_t used = 0;
    for (int i = 0; i < BPVM_PART_COUNT - 1; i++) {   /* KNOBS */
        uint32_t sz = sizes[i];
        out->parts[i].kind   = (bpvm_part_kind_t) i;
        out->parts[i].offset = off;      /* offset DERIVADO: contiguo, en orden */
        out->parts[i].size   = sz;
        if (sz == 0) { if (bad_idx) *bad_idx = i; return BPVM_PART_ERR_ZERO; }
        if (sector != 0 && (sz % sector) != 0) {
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_UNALIGNED;
        }
        if (sz > avail - used) {   /* used<=avail aquí, así que avail-used válido */
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_OVERFLOW;
        }
        used += sz;
        off  += sz;
    }
    /* La ÚLTIMA (packs) = EL RESTO. Derivada: no la fija el usuario, no puede
     * contradecir a las knobs ni desbordar (used<=avail garantizado). Alineada
     * (avail y used lo son). Puede ser 0 si el usuario dio todo a las knobs. */
    {
        int last = BPVM_PART_COUNT - 1;
        out->parts[last].kind   = (bpvm_part_kind_t) last;
        out->parts[last].offset = off;
        out->parts[last].size   = avail - used;
    }
    return BPVM_PART_OK;
}

bpvm_part_err_t bpvm_part_layout(const bpvm_env_t* env, uint32_t base,
                                 uint32_t usable_flash, uint32_t sector,
                                 bpvm_part_layout_t* out, int* bad_idx) {
    if (bad_idx) *bad_idx = -1;
    if (!out) return BPVM_PART_ERR_ZERO;
    uint32_t sizes[BPVM_PART_COUNT];
    /* Solo las KNOBS viven en el env (part.<knob>.size). La última (packs) se
     * DERIVA → ni se lee ni se guarda. Falta una knob → virgen (MISSING). */
    for (int i = 0; i < BPVM_PART_COUNT - 1; i++) {
        char key[8 + 16];
        /* key = "part.<name>.size" */
        snprintf(key, sizeof key, "part.%s.size", PART_NAMES[i]);
        long sz = bpvm_env_get_long(env, key, -1);
        if (sz < 0) {                       /* falta → virgen a nivel de particiones */
            out->base = base; out->usable_flash = usable_flash; out->complete = 0;
            if (bad_idx) *bad_idx = i;
            return BPVM_PART_ERR_MISSING;
        }
        sizes[i] = (uint32_t) sz;
    }
    sizes[BPVM_PART_COUNT - 1] = 0;   /* derivada; from_sizes la recalcula (resto) */
    return bpvm_part_layout_from_sizes(sizes, base, usable_flash, sector, out, bad_idx);
}

const bpvm_part_t* bpvm_part_get(const bpvm_part_layout_t* lay, bpvm_part_kind_t k) {
    if (!lay || (int) k < 0 || (int) k >= (int) BPVM_PART_COUNT) return NULL;
    return &lay->parts[k];
}

const char* bpvm_part_err_str(bpvm_part_err_t e) {
    switch (e) {
    case BPVM_PART_OK:            return "ok";
    case BPVM_PART_ERR_MISSING:   return "falta algun tamano (placa virgen: proponer defaults)";
    case BPVM_PART_ERR_ZERO:      return "un tamano es 0";
    case BPVM_PART_ERR_UNALIGNED: return "un tamano no esta alineado al sector de borrado";
    case BPVM_PART_ERR_OVERFLOW:  return "los tamanos no caben en la flash usable";
    default:                      return "error desconocido";
    }
}

int bpvm_part_sizes_to_payload(const uint32_t sizes[BPVM_PART_COUNT],
                               char* buf, size_t cap) {
    if (!sizes || !buf || cap == 0) return -1;
    size_t off = 0;
    /* Solo las KNOBS (part.<knob>.size); la última (packs) se deriva → no se guarda. */
    for (int i = 0; i < BPVM_PART_COUNT - 1; i++) {
        int w = snprintf(buf + off, cap - off, "part.%s.size=%lu\n",
                         PART_NAMES[i], (unsigned long) sizes[i]);
        if (w < 0 || (size_t) w >= cap - off) return -1;
        off += (size_t) w;
    }
    return (int) off;
}
