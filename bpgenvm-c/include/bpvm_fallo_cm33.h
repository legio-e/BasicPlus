/*
 * bpvm_fallo_cm33.h — QUE UN HARDFAULT HABLE, en las dos familias Cortex-M33 (la Pico 2 / Metro y las STM32).
 *
 * Era `#447` de la Pico (28-ago: seis programas colgados «en seco», indistinguibles de un bucle infinito de BP),
 * copiado sólo allí: la STM32 seguía con el bucle mudo de CubeMX. V7/H5 P2b.3 (27-sep) lo sube al común porque los
 * ejecutores de natives son tareas con pila pequeña y medida: en ARM, desbordar la pila de una tarea salta el límite
 * PSPLIM (STKOF) y escala a HardFault, y eso tiene que decir QUÉ tarea y POR QUÉ. Lo que el informe no sabía decir y
 * ahora dice: STKOF (desbordamiento por el límite de pila), STKERR/UNSTKERR (apilar/desapilar el marco de la
 * excepción), el nombre de la tarea en curso y la pila con su límite.
 */
#ifndef BPVM_FALLO_CM33_H
#define BPVM_FALLO_CM33_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* El marco que apila el hardware al entrar en la excepción. */
struct bpvm_fault_frame { uint32_t r0, r1, r2, r3, r12, lr, pc, psr; };

/* Escribe el informe en el registro persistente (log_printf + log_flush) y se queda PARADO, habiendo dicho por qué.
 * `f` es el marco apilado (NULL si no se sabe: entonces no hay PC ni LR); `exc_return`, el EXC_RETURN con que se
 * entró (su bit 2 dice qué pila: PSP, la de una tarea, o MSP); `cual`, el nombre de la excepción. No vuelve. */
void bpvm_cm33_fallo_informe(const struct bpvm_fault_frame* f, uint32_t exc_return, const char* cual)
    __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* BPVM_FALLO_CM33_H */
