/*
 * bpvm_nativa.c — la pila prestada de una native (V7/H5 P2b.1). Ver bpvm_nativa.h.
 */
#include "bpvm_nativa.h"
#include "bpvm_alloc.h"

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

int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp) {
    bpvm_nativa_t* n = tc->nativa;
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
    n->estado = BPVM_NATIVA_CORRIENDO;
    n->ctx_nativa = bpvm_ctx_crear(n->pila, n->tam, trampolin);
    bpvm_ctx_saltar(&n->ctx_worker, n->ctx_nativa, n);
    /* Aquí se vuelve cuando la native termina. */
    *sp = n->sp;
    *bp = n->bp;
    n->estado = BPVM_NATIVA_LIBRE;
    return n->resultado;
}

#else  /* sin cambio de contexto para esta arquitectura todavía: la llamada de siempre */

int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp) {
    return bpvm_aot_call_guarded(vm, tc, aot, sp, bp);
}

#endif

void bpvm_nativa_liberar(bpvm_thread_t* tc) {
    if (tc && tc->nativa) {
        bpvm_free(tc->nativa);
        tc->nativa = NULL;
    }
}
