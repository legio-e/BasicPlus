/*
 * bpvm_nativa.c — la pila prestada de una native (V7/H5 P2b.1). Ver bpvm_nativa.h.
 */
#include "bpvm_nativa.h"
#include "bpvm_alloc.h"
#include "bpvm_platform.h"   /* P2b.1.d: bpvm_platform_now_ms, la rebanada */
#include "bpvm_aot_helpers.h"

#include <stdint.h>
#include <string.h>

#if BPVM_CTX_DISPONIBLE

/* En el fondo de la pila prestada. Llama al thunk con su límite de fallos de #186 (que así vive también en
 * esta pila, donde el longjmp de un throw lo encuentra) y le dice al intérprete que ha terminado. No vuelve:
 * una native terminada no se reanuda — la siguiente llamada crea un contexto nuevo en la misma pila. */
static void trampolin(void* dato) {
    bpvm_nativa_t* n = (bpvm_nativa_t*) dato;
    n->resultado = bpvm_aot_call_guarded(n->vm, n->tc, n->thunk, &n->sp, &n->bp);
    n->estado = BPVM_NATIVA_TERMINADA;
    bpvm_ctx_saltar(&n->ctx_nativa, n->ctx_worker, n);
}

static bpvm_nativa_t* nativa_de(bpvm_thread_t* tc) {
    bpvm_nativa_t* n = tc->nativa;
    if (n == NULL) {
        size_t total = sizeof(bpvm_nativa_t) + 16u + BPVM_NATIVA_PILA_BYTES;
        n = (bpvm_nativa_t*) bpvm_malloc(total);
        if (n == NULL) return NULL;
        memset(n, 0, sizeof *n);
        n->pila = (uint8_t*) (((uintptr_t) (n + 1) + 15u) & ~(uintptr_t) 15u);
        n->tam = BPVM_NATIVA_PILA_BYTES;
        tc->nativa = n;
    }
    return n;
}

/* Salta a la native —nueva o suspendida— y vuelve cuando termina o se suspende. Alrededor, la vista de la
 * TLS del worker: se guarda al entrar y se repone al volver (la native pone la suya al reanudarse). */
static int entrar(bpvm_nativa_t* n) {
    n->f_worker = *bpvm_aot_fault_slot();
    n->cc_worker = *bpvm_aot_callctx();
    n->t_corriendo = bpvm_platform_now_ms();   /* P2b.1.d: su rebanada empieza ahora */
    bpvm_ctx_saltar(&n->ctx_worker, n->ctx_nativa, n);
    *bpvm_aot_fault_slot() = n->f_worker;
    *bpvm_aot_callctx() = n->cc_worker;
    if (n->estado == BPVM_NATIVA_SUSPENDIDA) return BPVM_NATIVA_SUSPENDIDA_R;
    n->estado = BPVM_NATIVA_LIBRE;   /* TERMINADA: la pila queda libre para la siguiente */
    return n->resultado;
}

int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp,
                       uint32_t pc_vuelta, uint32_t cs_vuelta) {
    bpvm_nativa_t* n = tc->nativa;
    int r;
    /* Anidada: el hilo ya corre en su pila prestada (native → BP → native). Una llamada de C más. */
    if (n != NULL && n->estado != BPVM_NATIVA_LIBRE)
        return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
    n = nativa_de(tc);
    if (n == NULL) {
        /* Sin memoria para prestarle una pila: corre como hasta ahora, en la del worker. Se dice una vez. */
        static int dicho = 0;
        if (!dicho) {
            dicho = 1;
            bpvm_diag_urgente("[bpvm] native sin pila prestada (no caben %u B): corre en la del worker",
                              (unsigned) BPVM_NATIVA_PILA_BYTES);
        }
        return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
    }
    n->vm = vm;
    n->tc = tc;
    n->thunk = aot;
    n->sp = *sp;
    n->bp = *bp;
    n->pc_vuelta = pc_vuelta;
    n->cs_vuelta = cs_vuelta;
    n->estado = BPVM_NATIVA_CORRIENDO;
    n->ctx_nativa = bpvm_ctx_crear(n->pila, n->tam, trampolin);
    r = entrar(n);
    if (r != BPVM_NATIVA_SUSPENDIDA_R) {   /* terminó sin suspenderse: el intérprete sigue aquí mismo */
        *sp = n->sp;
        *bp = n->bp;
    }
    return r;
}

int bpvm_nativa_reanudar(bpvm_t* vm, bpvm_thread_t* tc) {
    bpvm_nativa_t* n = tc->nativa;
    (void) vm;
    n->estado = BPVM_NATIVA_CORRIENDO;
    return entrar(n);
}

int bpvm_nativa_ceder(bpvm_t* vm, bpvm_thread_t* tc) {
    bpvm_nativa_t* n = tc->nativa;
    (void) vm;
    if (n == NULL || n->estado != BPVM_NATIVA_CORRIENDO) return 0;
    n->f_nativa = *bpvm_aot_fault_slot();
    n->cc_nativa = *bpvm_aot_callctx();
    n->estado = BPVM_NATIVA_SUSPENDIDA;
    bpvm_ctx_saltar(&n->ctx_nativa, n->ctx_worker, n);
    /* Reanudada —quizá por otro worker, con su propia TLS—: la vista de la native, otra vez. */
    *bpvm_aot_fault_slot() = n->f_nativa;
    *bpvm_aot_callctx() = n->cc_nativa;
    return 1;
}

#else  /* sin cambio de contexto para esta arquitectura todavía: la llamada de siempre */

int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp,
                       uint32_t pc_vuelta, uint32_t cs_vuelta) {
    (void) pc_vuelta; (void) cs_vuelta;
    return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
}

int bpvm_nativa_reanudar(bpvm_t* vm, bpvm_thread_t* tc) {
    (void) vm; (void) tc;
    return 0;   /* nunca se suspende nada sin cambio de contexto */
}

int bpvm_nativa_ceder(bpvm_t* vm, bpvm_thread_t* tc) {
    (void) vm; (void) tc;
    return 0;
}

#endif

void bpvm_nativa_punto_control(struct bpvm* vm) {
    bpvm_aot_callctx_t* cc = bpvm_aot_callctx();
    bpvm_thread_t* tc = cc->tc;
    bpvm_nativa_t* n = tc ? tc->nativa : NULL;
    if (tc == NULL) return;
    if (n != NULL && n->estado == BPVM_NATIVA_CORRIENDO) {
        /* En su pila prestada: cede si hay un `kill` (el planificador lo verá y parará el programa) o si ya
         * ha corrido su rebanada (los demás hilos, y el cable: el planificador atiende al IDE entre
         * quantums). Al reanudarse, sigue en su bucle como si nada. */
        if (vm->kill_requested || bpvm_platform_now_ms() - n->t_corriendo >= BPVM_NATIVA_REBANADA_MS) {
            tc->status = BPVM_THREAD_RUNNABLE;
            bpvm_nativa_ceder(vm, tc);
        }
        return;
    }
    /* Sin pila prestada no puede suspenderse; pero un `kill` sí la para: lanza por el límite de fallos de
     * #186 (si un try/catch de BP lo atrapara, el planificador para el programa en el siguiente quantum). */
    if (vm->kill_requested)
        vm->aot_helpers->throw_runtime(vm, "el programa se ha detenido (kill) dentro de una native");
}

void bpvm_nativa_liberar(bpvm_thread_t* tc) {
    if (tc && tc->nativa) {
        bpvm_free(tc->nativa);
        tc->nativa = NULL;
    }
}
