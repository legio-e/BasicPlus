/*
 * bpvm_nativa.h — el EJECUTOR de una native (V7/H5 P2b.3; antes, P2b.1, la pila prestada).
 *
 * Idea de Eduardo (26-sep): una native que tiene que ceder el hilo no se rechaza: se suspende y el intérprete
 * sigue con los demás hilos BP. Y cómo (27-sep): *«pensar en la ejecución de código nativo como un Thread BP,
 * con su propio stack. Y de vez en cuando devuelve el hilo de ejecución a otros Thread BP. Ese es un mecanismo que
 * ya existe.»* La native corre en un hilo propio de la plataforma —su EJECUTOR, con su propia pila: una tarea de
 * FreeRTOS, un pthread en el PC— y el intérprete y él se pasan el TESTIGO (dos semáforos binarios): sólo corre
 * uno de los dos, así que la VM sigue sin concurrencia. Suspender una native es pasarle el testigo al intérprete
 * y esperarlo; reanudarla, devolvérselo. El cambio de contexto lo hace el sistema, con sus vigilancias de pila en
 * regla (V7_IDEAS §H5.P2b.3: cambiar de pila a mano dentro de una tarea chocaba con ellas).
 *
 * Los ejecutores viven en un DEPÓSITO de la VM: se crean al entrar en una native y se reutilizan. Un hilo BP
 * tiene uno sólo mientras está DENTRO de una native —corriendo o suspendida—: `tc->nativa`. Una native que llama
 * a BP que llama a otra native (native → BP → native) corre la interior en el MISMO ejecutor, como una llamada de
 * C más.
 *
 * Sin memoria para un ejecutor, la native corre directa en el worker (no puede suspenderse; un `kill` la para
 * lanzando), y se dice una vez.
 */
#ifndef BPVM_NATIVA_H
#define BPVM_NATIVA_H

#include "bpvm_internal.h"
#include "aot_registry.h"
#include "bpvm_platform.h"

#include <setjmp.h>

/* La pila de un ejecutor. Dentro corren la native, sus helpers, el intérprete anidado del puente nativo→BP y los
 * builtins que llamen —el GC no: va al worker—. En el PC, holgada (256 KB, como la pila prestada de P2b.1); en las
 * placas, lo que diga la marca de agua (P2b.3.d; Eduardo: «en freertos el tamaño del stack de los Threads puede
 * ser más pequeño»): de entrada 6 KB, y cada familia puede fijar la suya con -D. */
#ifndef BPVM_NATIVA_PILA_BYTES
#  if defined(__arm__) || defined(__riscv) || defined(__XTENSA__)
#    define BPVM_NATIVA_PILA_BYTES (6u * 1024u)
#  else
#    define BPVM_NATIVA_PILA_BYTES (256u * 1024u)
#  endif
#endif

typedef enum {
    BPVM_NATIVA_LIBRE = 0,      /* en el depósito, sin hilo BP */
    BPVM_NATIVA_CORRIENDO,      /* tiene el testigo: corre la native */
    BPVM_NATIVA_SUSPENDIDA,     /* le devolvió el testigo al intérprete a mitad: su hilo BP está dentro, parado */
    BPVM_NATIVA_TERMINADA,      /* acaba de volver: se lo dice al intérprete */
    BPVM_NATIVA_PIDE_GC         /* le pasó el testigo al intérprete para que corra el GC en su pila */
} bpvm_nativa_estado_t;

typedef struct bpvm_nativa {
    struct bpvm_nativa* sig;       /* el depósito de la VM */
    bpvm_t*          vm;
    bpvm_thread_t*   tc;           /* el hilo BP que está dentro (NULL en el depósito) */
    bpvm_aot_thunk_t thunk;
    /* Los registros sp/bp del intérprete, en un sitio ESTABLE: el thunk los lee y los muta. */
    uint32_t         sp, bp;
    int              resultado;    /* lo que devolvió bpvm_aot_call_guarded: 0 / 1 / 2 */
    /* Dónde sigue el intérprete de fuera (tras su CALL) cuando una native que se suspendió termine: ya no está en
     * ninguna variable local, porque aquel quantum acabó. */
    uint32_t         pc_vuelta, cs_vuelta;
    /* La vista de la TLS: el límite de fallos (#186) y el contexto de la llamada. En el PC son TLS de verdad y cada
     * hilo tiene la suya; en las placas son globales y la de la native y la del worker se turnan en cada paso del
     * testigo. */
    bpvm_aot_fault_t    f_nativa, f_worker;
    bpvm_aot_callctx_t  cc_nativa, cc_worker;
    /* P2b.1.d — cuándo empezó a correr (al entrar o al reanudarse), en ms: la rebanada se mide desde aquí. */
    int64_t          t_corriendo;
    volatile int     estado;       /* bpvm_nativa_estado_t */
    volatile int     terminar;     /* la orden de terminar (bpvm_destroy) */
    /* Dónde se paró su pila de C al pasar el testigo (con los registros volcados por setjmp): el GC la escanea
     * desde aquí hasta `cc_nativa.cstack_hi`, como la de una native suspendida. */
    uintptr_t        sp_parado;
    int32_t          pila_libre;   /* la marca de agua al terminar (-1: la plataforma no la sabe) */
    bpvm_platform_thread_handle_t hilo;
    bpvm_platform_sem_handle_t    sem_nativa;   /* lo espera el ejecutor */
    bpvm_platform_sem_handle_t    sem_worker;   /* lo espera el intérprete */
    jmp_buf          fondo;        /* adónde salta el ejecutor si lo terminan parado a mitad de una native */
} bpvm_nativa_t;

/* Lo que devuelven bpvm_nativa_llamar y bpvm_nativa_reanudar cuando la native se ha SUSPENDIDO: el hilo se queda
 * dentro de ella (su estado, en tc) y el intérprete acaba su quantum sin tocarlo. */
#define BPVM_NATIVA_SUSPENDIDA_R 3

/* interp.c — el límite de fallos de #186 alrededor de un thunk. */
int bpvm_aot_call_guarded(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp);

/* Llama al thunk de una native. La más externa de un hilo, en un ejecutor; una anidada (native → BP → native),
 * directamente, porque ya está en él. Devuelve lo mismo que bpvm_aot_call_guarded: 0 = volvió normal, 1 = lanzó y
 * lo atrapó un try/catch de BP, 2 = lanzó y nadie lo atrapó (el hilo está muerto); o BPVM_NATIVA_SUSPENDIDA_R. */
int bpvm_nativa_llamar(bpvm_t* vm, bpvm_thread_t* tc, bpvm_aot_thunk_t aot, uint32_t* sp, uint32_t* bp,
                       uint32_t pc_vuelta, uint32_t cs_vuelta);

/* ¿El hilo está suspendido DENTRO de una native? Entonces no se interpreta: se reanuda. */
static inline int bpvm_nativa_suspendida(const bpvm_thread_t* tc) {
    return tc->nativa != NULL && tc->nativa->estado == BPVM_NATIVA_SUSPENDIDA;
}

/* La reanuda y vuelve cuando termina (0 / 1 / 2, como bpvm_nativa_llamar) o se suspende otra vez
 * (BPVM_NATIVA_SUSPENDIDA_R). Si termina, deja en pc, cs, sp y bp dónde sigue el intérprete —tras el CALL de la
 * native, con los sp/bp que dejó— y el ejecutor vuelve al depósito. */
int bpvm_nativa_reanudar(bpvm_t* vm, bpvm_thread_t* tc, uint32_t* pc, uint32_t* cs, uint32_t* sp, uint32_t* bp);

/* La llaman, DENTRO de una native, quienes tienen que ceder el hilo: el puente nativo→BP cuando la función BP se
 * bloquea o agota su quantum, la línea del bucle y un builtin que bloquea. El estado del hilo (tc->status) ya lo
 * ha puesto quien cede. Le pasa el testigo al intérprete y VUELVE cuando se lo devuelven: 1. Si el hilo no corre
 * en un ejecutor (sin memoria para él), no puede: 0. */
int bpvm_nativa_ceder(bpvm_t* vm, bpvm_thread_t* tc);

/* P2b.1.d — cuánto corre una native antes de ceder el hilo, si nadie lo pide antes. El intérprete mide su quantum
 * en operaciones (1024); una native no tiene operaciones que contar, así que se mide en tiempo. */
#ifndef BPVM_NATIVA_REBANADA_MS
#  define BPVM_NATIVA_REBANADA_MS 5
#endif

/* P2b.1.d — el helper `punto_control` de la tabla: lo llama la línea del bucle cada 1024 vueltas. Cede el hilo si
 * hay un `kill` o si la native ya ha corrido su rebanada; si no puede ceder (sin ejecutor), un `kill` LANZA por el
 * límite de fallos de #186 y la native se acaba. */
void bpvm_nativa_punto_control(struct bpvm* vm);

/* P2b.3 — el GC, pedido DENTRO de un ejecutor (heap.c lo sabe por el contexto de la llamada): le pasa el testigo
 * al intérprete con el encargo, y el marcado —recursivo— corre en la pila del worker. Vuelve con el GC hecho. */
void bpvm_nativa_gc_en_worker(bpvm_t* vm);

/* heap.c — el GC en sí (marcar, cerrar los File, barrer), sin el cerrojo del SMP: lo corre el worker por encargo
 * de un ejecutor, que ya lo tiene tomado. */
void bpvm_gc_cuerpo(bpvm_t* vm);

/* bpvm_destroy — termina los ejecutores del depósito (los parados a mitad de una native saltan a su fondo,
 * abandonando esos marcos) y devuelve su memoria. Dice la marca de agua de la pila de cada uno. */
void bpvm_nativa_liberar_todas(bpvm_t* vm);

#endif /* BPVM_NATIVA_H */
