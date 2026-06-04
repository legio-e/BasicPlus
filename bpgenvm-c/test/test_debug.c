/*
 * test/test_debug.c — H6.b.1: test del núcleo portable del debugger en la
 * VM-C. NO usa wire ni hardware: inyecta un pause_cb directamente y verifica
 * las tres primitivas portables:
 *   (A) STEP: pausa instrucción a instrucción; STOP aborta (status STOPPED).
 *   (B) Breakpoint por pc: pausa exactamente cuando pc == el pc registrado.
 *   (C) list / clear de breakpoints.
 *
 * Regla de oro (H6): el core sólo trabaja en pc/sp/bp crudos; el host (IDE)
 * resuelve los símbolos con el `.dbg`. Aquí comprobamos justo esa capa cruda.
 *
 * Uso: build/test_debug <fichero.mod>
 */

#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMSZ   (512 * 1024)
#define MAXP    64

typedef struct {
    int      n;                 /* nº de pausas vistas */
    uint32_t pcs[MAXP];
    uint32_t sps[MAXP];
    uint32_t bps[MAXP];
    int      step_limit;        /* >0: STEP hasta n>=limit, luego STOP */
} pstate_t;

static bpvm_dbg_action_t pause_cb(bpvm_t* vm, bpvm_thread_t* tc,
                                  uint32_t pc, void* user) {
    (void) vm;
    pstate_t* st = (pstate_t*) user;
    if (st->n < MAXP) {
        st->pcs[st->n] = pc;
        st->sps[st->n] = bpvm_thread_sp(tc);
        st->bps[st->n] = bpvm_thread_bp(tc);
    }
    st->n++;
    if (st->step_limit > 0) {
        if (st->n >= st->step_limit) return BPVM_DBG_STOP;
        return BPVM_DBG_STEP;
    }
    return BPVM_DBG_CONTINUE;
}

static bpvm_t* load_vm(uint8_t* mem, const char* path) {
    bpvm_t* vm = bpvm_init(mem, MEMSZ, 0);
    if (!vm) return NULL;
    if (bpvm_load_mod(vm, path) != BPVM_OK) { bpvm_destroy(vm); return NULL; }
    return vm;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2) { fprintf(stderr, "uso: test_debug <fichero.mod>\n"); return 2; }
    const char* path = argv[1];
    int fails = 0;

    /* ---- (A) STEP + STOP ---- */
    uint8_t* memA = (uint8_t*) calloc(1, MEMSZ);
    bpvm_t*  vmA  = load_vm(memA, path);
    if (!vmA) { fprintf(stderr, "load A fallo: %s\n", path); free(memA); return 1; }
    pstate_t A; memset(&A, 0, sizeof A); A.step_limit = 5;
    bpvm_set_pause_cb(vmA, pause_cb, &A);
    bpvm_debug_request_pause(vmA);   /* fuerza la 1ª pausa en el 1er opcode; luego STEP */
    bpvm_status_t sA = bpvm_run(vmA);
    if (A.n != 5) { printf("FAIL(A): esperaba 5 pausas, vi %d\n", A.n); fails++; }
    if (sA != BPVM_DBG_STOPPED) {
        printf("FAIL(A): esperaba STOPPED, vi %s\n", bpvm_status_str(sA)); fails++;
    }
    printf("(A) step pcs:");
    for (int i = 0; i < A.n && i < 5; i++) printf(" %u", A.pcs[i]);
    printf("  -> %s\n", bpvm_status_str(sA));
    uint32_t target = (A.n >= 4) ? A.pcs[3] : 0;   /* 4ª instrucción ejecutada */
    bpvm_destroy(vmA); free(memA);

    /* ---- (B) Breakpoint por pc + (C) list/clear ---- */
    uint8_t* memB = (uint8_t*) calloc(1, MEMSZ);
    bpvm_t*  vmB  = load_vm(memB, path);
    if (!vmB) { fprintf(stderr, "load B fallo\n"); free(memB); return 1; }
    int id = bpvm_debug_add_breakpoint(vmB, target);
    if (id <= 0) { printf("FAIL(B): add_breakpoint(%u) = %d\n", target, id); fails++; }
    /* (C) idempotencia + list */
    if (bpvm_debug_add_breakpoint(vmB, target) != id) {
        printf("FAIL(C): add_breakpoint no es idempotente por pc\n"); fails++;
    }
    if (bpvm_debug_list_breakpoints(vmB, NULL, NULL, 8) != 1) {
        printf("FAIL(C): list != 1 tras add idempotente\n"); fails++;
    }
    pstate_t B; memset(&B, 0, sizeof B); B.step_limit = 0;   /* CONTINUE */
    bpvm_set_pause_cb(vmB, pause_cb, &B);
    bpvm_status_t sB = bpvm_run(vmB);
    if (B.n < 1) { printf("FAIL(B): el breakpoint nunca disparo\n"); fails++; }
    else if (B.pcs[0] != target) {
        printf("FAIL(B): primer pause pc=%u != target %u\n", B.pcs[0], target); fails++;
    }
    printf("(B) breakpoint pc=%u disparo %d vez/veces, sp0=%u bp0=%u -> %s\n",
           target, B.n, (B.n>0?B.sps[0]:0), (B.n>0?B.bps[0]:0), bpvm_status_str(sB));
    /* (C) clear */
    if (!bpvm_debug_clear_breakpoint(vmB, id)) { printf("FAIL(C): clear no hallo id %d\n", id); fails++; }
    if (bpvm_debug_list_breakpoints(vmB, NULL, NULL, 8) != 0) {
        printf("FAIL(C): quedan breakpoints tras clear\n"); fails++;
    }
    bpvm_destroy(vmB); free(memB);

    if (fails == 0) printf("OK: H6.b.1 debug core (step + breakpoint-por-pc + stop + list/clear)\n");
    else            printf("FALLOS: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
