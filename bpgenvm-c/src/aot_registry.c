/*
 * aot_registry.c — implementación del registry AOT.
 *
 * Tabla estática pequeña con scan lineal. La cantidad esperada de
 * entries es <50 incluso con AOT completo (stdlib + drivers + apps).
 * Si hace falta más, sustituir por hash o tabla ordenada con bsearch.
 */

#include "aot_registry.h"
#include "bpvm_internal.h"

#include <string.h>

#define BPVM_AOT_MAX  32

typedef struct {
    uint32_t           addr;
    bpvm_aot_thunk_t   thunk;
} aot_entry_t;

static aot_entry_t g_aot[BPVM_AOT_MAX];
static int         g_aot_count = 0;

int bpvm_aot_register(uint32_t addr, bpvm_aot_thunk_t thunk) {
    if (g_aot_count >= BPVM_AOT_MAX) return -1;
    /* Si la dirección ya está registrada, sobrescribir — útil para
     * recargar entre tests. */
    for (int i = 0; i < g_aot_count; i++) {
        if (g_aot[i].addr == addr) {
            g_aot[i].thunk = thunk;
            return 0;
        }
    }
    g_aot[g_aot_count].addr  = addr;
    g_aot[g_aot_count].thunk = thunk;
    g_aot_count++;
    return 0;
}

bpvm_aot_thunk_t bpvm_aot_lookup(uint32_t addr) {
    /* Hot path — debe ser MUY barato. Scan lineal con early-out. */
    for (int i = 0; i < g_aot_count; i++) {
        if (g_aot[i].addr == addr) return g_aot[i].thunk;
    }
    return NULL;
}

int bpvm_aot_register_by_name(struct bpvm* vm,
                                const char* qualified,
                                bpvm_aot_thunk_t thunk) {
    if (!vm || !qualified || !thunk) return -2;
    for (int i = 0; i < vm->symbol_count; i++) {
        if (strcmp(vm->symbols[i].name, qualified) == 0) {
            return bpvm_aot_register(vm->symbols[i].abs_addr, thunk);
        }
    }
    return -2;   /* símbolo no encontrado */
}

void bpvm_aot_clear(void) {
    g_aot_count = 0;
}

int bpvm_aot_count(void) {
    return g_aot_count;
}
