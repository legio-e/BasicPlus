/*
 * bpvm_ctx.h — cambiar de pila: la pieza en ensamblador de la suspensión de natives (V7/H5 P2b.1.a).
 *
 * Idea de Eduardo (26-sep): «¿Y si el código nativo funcionase como una interrupción? Sin un stack
 * propio, usa el stack que le ceda el intérprete.» Una native corre en una pila que le presta el
 * intérprete. Suspenderla es guardar EN ESA PILA los registros que la ABI obliga a conservar y cambiar el
 * puntero de pila, como al entrar en una interrupción; reanudarla es lo contrario, y la native no se
 * entera. Un contexto suspendido es, por tanto, sólo su puntero de pila.
 *
 * Desde C no se puede mover el puntero de pila: de ahí unas pocas líneas de ensamblador por ABI
 * (bpvm_ctx.c). Todo lo demás —la pila prestada, el trampolín, el GC, el planificador— es C común.
 *
 * Hoy sólo x86-64 con la ABI de Windows (el PC, MinGW). ARM y RISC-V llegan con P2b.3; mientras,
 * BPVM_CTX_DISPONIBLE vale 0 en ellas y las natives corren como siempre, en la pila del worker.
 */
#ifndef BPVM_CTX_H
#define BPVM_CTX_H

#include <stddef.h>

#if defined(__x86_64__) && defined(_WIN32)
#  define BPVM_CTX_DISPONIBLE 1
#else
#  define BPVM_CTX_DISPONIBLE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Un contexto suspendido: su puntero de pila (sus registros están guardados en ella). */
typedef void* bpvm_ctx_t;

/* Lo mínimo que una pila prestada tiene que tener para el marco inicial y algo de margen. */
#define BPVM_CTX_PILA_MIN 1024u

/* Prepara en [pila, pila + tam) un contexto NUEVO: el primer salto a él llama a fn(dato), con el `dato`
 * de ese salto. fn NO vuelve: termina saltando a otro contexto (si volviera, la VM lo dice y se para).
 * Devuelve el contexto, o NULL si la pila no llega a BPVM_CTX_PILA_MIN o no hay implementación. */
bpvm_ctx_t bpvm_ctx_crear(void* pila, size_t tam, void (*fn)(void* dato));

/* Guarda el contexto en curso en *guardar y salta a `destino`, pasándole `dato`. Devuelve el `dato` con
 * que otro contexto vuelva a saltar aquí. */
void* bpvm_ctx_saltar(bpvm_ctx_t* guardar, bpvm_ctx_t destino, void* dato);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_CTX_H */
