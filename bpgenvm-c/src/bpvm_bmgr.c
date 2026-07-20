/*
 * bpvm_bmgr.c — H9: núcleo portable del protocolo de gestión de placa. Compone
 * bpvm_env (bloque A/B) + bpvm_part (conjunto fijo, offsets derivados). SIN heap;
 * buffers del llamador. Ver bpvm_bmgr.h + docs/H9_KERNEL_CAPAS.md.
 */
#include "bpvm_bmgr.h"

#include <string.h>
#include <stdio.h>   /* snprintf (ruta no-caliente: solo al aplicar tamaños) */

int bpvm_bmgr_env(const bpvm_bmgr_t* bm, bpvm_env_t* out) {
    if (out) { out->payload = NULL; out->payload_len = 0; out->seq = 0; out->valid = 0; }
    if (!bm) return 0;
    bpvm_env_t e;
    int who = bpvm_env_pick(bm->a, bm->sector, bm->b, bm->sector, &e);
    if (who < 0 || !e.valid) return 0;   /* placa virgen: sin env */
    if (out) *out = e;
    return 1;
}

int bpvm_bmgr_env_count(const bpvm_bmgr_t* bm) {
    bpvm_env_t e;
    if (!bpvm_bmgr_env(bm, &e)) return 0;
    return bpvm_env_count(&e);
}

int bpvm_bmgr_env_pair_at(const bpvm_bmgr_t* bm, int idx,
                          char* key, size_t kcap, char* val, size_t vcap) {
    bpvm_env_t e;
    if (!bpvm_bmgr_env(bm, &e)) return 0;
    return bpvm_env_pair_at(&e, idx, key, kcap, val, vcap);
}

int bpvm_bmgr_env_get(const bpvm_bmgr_t* bm, const char* key, char* val, size_t vcap) {
    bpvm_env_t e;
    if (!bpvm_bmgr_env(bm, &e)) return -1;
    return bpvm_env_get(&e, key, val, vcap);
}

int bpvm_bmgr_env_set(bpvm_bmgr_t* bm, const char* key, const char* value, int* wrote_slot) {
    if (!bm) return -1;
    return bpvm_env_apply(bm->a, bm->b, bm->sector, bm->scratch, bm->sector,
                          key, value, wrote_slot);
}

bpvm_part_err_t bpvm_bmgr_part_layout(const bpvm_bmgr_t* bm, uint32_t sector,
                                      bpvm_part_layout_t* out, int* bad_idx) {
    if (!bm) return BPVM_PART_ERR_ZERO;
    bpvm_env_t e;
    bpvm_bmgr_env(bm, &e);   /* si virgen, e.valid=0 → part_layout devuelve MISSING */
    return bpvm_part_layout(&e, bm->part_base, bm->usable_flash, sector, out, bad_idx);
}

void bpvm_bmgr_part_defaults(const bpvm_bmgr_t* bm, uint32_t sector,
                             uint32_t sizes_out[BPVM_PART_COUNT]) {
    if (!bm) return;
    bpvm_part_defaults(bm->part_base, bm->usable_flash, sector, sizes_out);
}

/* ¿la clave pertenece al espacio gestionado de particiones? ("part.<x>.size").
 * Al aplicar tamaños re-escribimos ESE bloque entero, así que las líneas viejas se
 * descartan primero (evita dejar una partición fantasma si el set cambia). */
static int is_managed_part_key(const char* ls, const char* eq) {
    size_t n = (size_t)(eq - ls);
    if (n < 11) return 0;                               /* "part.X.size" = 11 mínimo */
    if (memcmp(ls, "part.", 5) != 0) return 0;
    return memcmp(eq - 5, ".size", 5) == 0;
}

bpvm_part_err_t bpvm_bmgr_part_apply(bpvm_bmgr_t* bm, uint32_t sector,
                                     const uint32_t sizes[BPVM_PART_COUNT],
                                     int* bad_idx, int* wrote_slot) {
    if (!bm || !sizes) return BPVM_PART_ERR_ZERO;

    /* 1) VALIDAR primero — si no es válido, NO se toca el env. */
    bpvm_part_layout_t lay;
    bpvm_part_err_t err = bpvm_part_layout_from_sizes(sizes, bm->part_base,
                                                      bm->usable_flash, sector, &lay, bad_idx);
    if (err != BPVM_PART_OK) return err;

    /* 2) Construir el nuevo payload en `scratch`: las líneas actuales MENOS las
     *    part.*.size, más el bloque fresco de tamaños. Leemos del env actual (en a/b)
     *    y escribimos en scratch (disjunto) → sin aliasing. */
    bpvm_env_t e;
    int have = bpvm_bmgr_env(bm, &e);
    char*  out = (char*) bm->scratch;
    size_t cap = bm->sector, o = 0;

    if (have) {
        const uint8_t* p   = e.payload;
        const uint8_t* end = p + e.payload_len;
        while (p < end) {
            const uint8_t* lsb = p;
            const uint8_t* leb = lsb;
            while (leb < end && *leb != '\n') leb++;
            const uint8_t* eqb = lsb;
            while (eqb < leb && *eqb != '=') eqb++;
            int drop = (eqb < leb) && is_managed_part_key((const char*) lsb, (const char*) eqb);
            if (!drop) {
                size_t linelen = (size_t)(leb - lsb) + (leb < end ? 1u : 0u);  /* con '\n' */
                if (o + linelen > cap) return BPVM_PART_ERR_OVERFLOW;
                memcpy(out + o, lsb, linelen);
                o += linelen;
            }
            p = (leb < end) ? leb + 1 : end;
        }
    }
    if (o > 0 && out[o - 1] != '\n') {                  /* asegurar separador */
        if (o + 1 > cap) return BPVM_PART_ERR_OVERFLOW;
        out[o++] = '\n';
    }
    /* Solo las KNOBS (part.<knob>.size); la última (packs) es el RESTO, se deriva
     * al leer → no se guarda (una sola fuente de verdad = el límite). */
    for (int i = 0; i < BPVM_PART_COUNT - 1; i++) {
        int w = snprintf(out + o, cap - o, "part.%s.size=%lu\n",
                         bpvm_part_name((bpvm_part_kind_t) i), (unsigned long) sizes[i]);
        if (w < 0 || (size_t) w >= cap - o) return BPVM_PART_ERR_OVERFLOW;
        o += (size_t) w;
    }

    /* 3) Escritura A/B: serializar el payload a la copia rancia con seq+1. La copia
     *    actual queda intacta hasta que la nueva pasa CRC (seguro ante corte). */
    uint32_t ns = bpvm_env_next_seq(bm->a, bm->sector, bm->b, bm->sector);
    int cur = bpvm_env_pick(bm->a, bm->sector, bm->b, bm->sector, &e);  /* 0=A,1=B,-1=none */
    int stale = (cur == 0) ? 1 : 0;                     /* virgen o B-actual → A(0)? */
    /* cur==-1 → stale=0(A); cur==0 → 1(B); cur==1 → 0(A). Consistente con env_apply. */
    uint8_t* dst = (stale == 0) ? bm->a : bm->b;
    if (bpvm_env_serialize(out, o, ns, dst, bm->sector) < 0) return BPVM_PART_ERR_OVERFLOW;
    if (wrote_slot) *wrote_slot = stale;
    if (bad_idx) *bad_idx = -1;
    return BPVM_PART_OK;
}
