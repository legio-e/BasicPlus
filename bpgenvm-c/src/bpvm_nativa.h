/*
 * bpvm_nativa.h — la pila que el intérprete le PRESTA a una native (V7/H5 P2b.1).
 *
 * Idea de Eduardo (26-sep): «¿Y si el código nativo funcionase como una interrupción? Sin un stack propio,
 * usa el stack que le ceda el intérprete.» La primera vez que un hilo entra en una native, el intérprete le
 * reserva un bloque: una cabecera (esta estructura) y detrás la pila de C sobre la que corre. La native se
 * llama desde un trampolín en el fondo de esa pila (bpvm_ctx), así que su estado de C no vive en la pila del
 * worker: vive en la del hilo, y por eso podrá SUSPENDERSE (P2b.1.c) mientras el worker atiende a otros
 * hilos.
 *
 * Una native que llama a BP que llama a otra native (native → BP → native) corre la interior en la MISMA
 * pila prestada, como una llamada de C más: sólo la más externa cambia de pila.
 *
 * Sin cambio de contexto para la arquitectura (BPVM_CTX_DISPONIBLE == 0: hoy, las placas) todo esto es una
 * llamada directa, como siempre.
 */
#ifndef BPVM_NATIVA_H
#define BPVM_NATIVA_H

#include "bpvm_internal.h"
#include "aot_registry.h"
#include "bpvm_ctx.h"

/* Lo que se presta, en el PC. Holgado a propósito: dentro corren la native, sus helpers, el intérprete
 * anidado del puente nativo→BP, los builtins y —si una reserva lo dispara— el GC, cuyo marcado es
 * recursivo. Las placas tendrán su propio número con P2b.3 (y el GC, en la pila del intérprete). */
#ifndef BPVM_NATIVA_PILA_BYTES
#  define BPVM_NATIVA_PILA_BYTES (256u * 1024u)
#endif

typedef enum {
    BPVM_NATIVA_LIBRE = 0,      /* el hilo no está dentro de ninguna native */
    BPVM_NATIVA_CORRIENDO,      /* está dentro, y corriendo (en la pila prestada) */
    BPVM_NATIVA_SUSPENDIDA,     /* está dentro, suspendida (P2b.1.c) */
    BPVM_NATIVA_TERMINADA       /* acaba de volver: el trampolín se lo dice al intérprete */
} bpvm_nativa_estado_t;

typedef struct bpvm_nativa {
    bpvm_t*          vm;
    bpvm_thread_t*   tc;
    bpvm_aot_thunk_t thunk;
    /* Los registros sp/bp del intérprete, en un sitio ESTABLE: el thunk los lee y los muta. Hasta ahora
     * apuntaban a variables locales del intérprete, que ya no existirán cuando una native suspendida se
     * reanude. */
    uint32_t         sp, bp;
    int              resultado;    /* lo que devolvió bpvm_aot_call_guarded: 0 / 1 / 2 */
    /* P2b.1.c — dónde sigue el intérprete de fuera (tras su CALL) cuando una native que se suspendió
     * termine: ya no está en ninguna variable local, porque aquel quantum acabó. */
    uint32_t         pc_vuelta, cs_vuelta;
    /* P2b.1.c — la vista de la TLS (el límite de fallos y el contexto de la llamada son UNO por worker):
     * la de la native, guardada al suspenderse, y la del worker, guardada al entrar en ella. Con varias
     * natives vivas por worker (una corriendo, otras suspendidas), cada cambio de contexto pone la suya. */
    bpvm_aot_fault_t    f_nativa, f_worker;
    bpvm_aot_callctx_t  cc_nativa, cc_worker;
    volatile int     estado;       /* bpvm_nativa_estado_t */
    bpvm_ctx_t       ctx_nativa;   /* el contexto de la native (el que se reanuda) */
    bpvm_ctx_t       ctx_worker;   /* adónde se vuelve: el worker que la entró */
    uint32_t         tam;          /* bytes de la pila prestada */
    uint8_t*         pila;         /* [pila, pila + tam): va detrás de esta cabecera, en el mismo bloque */
} bpvm_nativa_t;

/* P2b.1.c — lo que devuelven bpvm_nativa_llamar y bpvm_nativa_reanudar cuando la native se ha SUSPENDIDO:
 * el hilo se queda dentro de ella (su estado, en tc) y el intérprete acaba su quantum sin tocarlo. */
#define BPVM_NATIVA_SUSPENDIDA_R 3

/* interp.c — el límite de fallos de #186 alrededor de un thunk (era `static aot_call_guarded`). */
int bpvm_aot_call_guarded(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp);

/* Llama al thunk de una native. La más externa de un hilo, en su pila prestada; una anidada (native → BP →
 * native), directamente, porque ya está en ella. Devuelve lo mismo que bpvm_aot_call_guarded: 0 = volvió
 * normal, 1 = lanzó y lo atrapó un try/catch de BP, 2 = lanzó y nadie lo atrapó (el hilo está muerto). */
int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp,
                       uint32_t pc_vuelta, uint32_t cs_vuelta);

/* P2b.1.c — ¿el hilo está suspendido DENTRO de una native? Entonces no se interpreta: se reanuda. */
static inline int bpvm_nativa_suspendida(const bpvm_thread_t* tc) {
    return tc->nativa != NULL && tc->nativa->estado == BPVM_NATIVA_SUSPENDIDA;
}

/* P2b.1.c — la reanuda y vuelve cuando termina (0 / 1 / 2, como bpvm_nativa_llamar) o se suspende otra vez
 * (BPVM_NATIVA_SUSPENDIDA_R). Si termina, el intérprete sigue en pc_vuelta con los sp/bp de la cabecera. */
int bpvm_nativa_reanudar(bpvm_t* vm, bpvm_thread_t* tc);

/* P2b.1.c — la llaman, DENTRO de una native, quienes tienen que ceder el hilo: el puente nativo→BP cuando la
 * función BP se bloquea o agota su quantum (y, con P2b.1.d y P2b.2, la línea del bucle y los builtins). El
 * estado del hilo (tc->status) ya lo ha puesto quien cede. Suspende la native —todo lo que hay en su pila
 * prestada: la native, el intérprete anidado, otras natives anidadas— y VUELVE cuando se reanuda: 1. Si el
 * hilo no corre en una pila prestada (sin cambio de contexto, o sin memoria), no puede: 0. */
int bpvm_nativa_ceder(bpvm_t* vm, bpvm_thread_t* tc);

/* La devuelve a la memoria del programa (bpvm_destroy). */
void bpvm_nativa_liberar(bpvm_thread_t* tc);

#endif /* BPVM_NATIVA_H */
