/*
 * bpvm_boot.c — H9: máquina de estados del arranque escalonado + STATE.
 * Portable, sin heap; conduce las capas como callbacks falibles. Ver bpvm_boot.h.
 */
#include "bpvm_boot.h"

#include <string.h>
#include <stdio.h>   /* snprintf (solo en state_report) */

static void copy_reason(char* dst, const char* src) {
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n > BPVM_BOOT_REASON_MAX - 1) n = BPVM_BOOT_REASON_MAX - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void bpvm_boot_climb(const bpvm_boot_layers_t* layers, bpvm_boot_status_t* out) {
    if (!out) return;
    out->state    = BPVM_BOOT_KERNEL;
    out->degraded = 0;
    out->reason[0] = '\0';
    if (!layers) return;

    bpvm_boot_layer_fn steps[3] = {
        layers->to_partitions,   /* 0→1 */
        layers->to_fs,           /* 1→2 */
        layers->to_app           /* 2→3 */
    };

    for (int target = BPVM_BOOT_PARTITIONS; target <= BPVM_BOOT_APP; target++) {
        if (target > (int) layers->max_state) break;   /* modo seguro: no subir (no degradado) */
        bpvm_boot_layer_fn fn = steps[target - 1];
        if (!fn) break;                                  /* capa no provista: parar (no degradado) */
        bpvm_boot_step_t r = fn(layers->user);
        if (!r.ok) {                                     /* fallo: quedarse en el último bueno */
            out->degraded = 1;
            copy_reason(out->reason, r.reason);
            return;
        }
        out->state = (bpvm_boot_state_t) target;         /* capa arrancada: subir */
    }
    /* Alcanzó el tope sin fallo → degraded queda 0. */
}

void bpvm_boot_fault(bpvm_boot_status_t* st, bpvm_boot_state_t drop_to, const char* reason) {
    if (!st) return;
    if ((int) drop_to < (int) st->state) st->state = drop_to;
    st->degraded = 1;
    copy_reason(st->reason, reason);
}

const char* bpvm_boot_state_name(bpvm_boot_state_t s) {
    switch (s) {
    case BPVM_BOOT_KERNEL:     return "kernel";
    case BPVM_BOOT_PARTITIONS: return "particiones";
    case BPVM_BOOT_FS:         return "fs";
    case BPVM_BOOT_APP:        return "app";
    default:                   return "?";
    }
}

int bpvm_boot_state_report(const bpvm_boot_status_t* st, char* buf, size_t cap) {
    if (!st || !buf || cap == 0) return -1;
    int w = snprintf(buf, cap, "state=%d name=%s degraded=%d reason=%s",
                     (int) st->state, bpvm_boot_state_name(st->state),
                     st->degraded, st->reason);
    if (w < 0 || (size_t) w >= cap) return -1;
    return w;
}
