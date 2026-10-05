/*
 * flash_lock.h — ventana exclusiva de acceso a flash (XIP-safe).
 *
 * Centraliza el patrón "voy a borrar/programar flash, que nadie ejecute
 * desde XIP mientras tanto". Hay DOS regímenes:
 *
 *   - Single-core (BPVM_PICO_NUM_CORES == 1, el default): basta con
 *     `save_and_disable_interrupts()`. Mientras el core que escribe
 *     tiene los IRQs apagados, NADIE más corre — no hay otro core. Es
 *     EXACTAMENTE lo que hacían fs.c y log.c antes de #153; el wrapper
 *     compila al mismo código.
 *
 *   - Dual-core (BPVM_PICO_NUM_CORES == 2, opt-in #153): apagar IRQs en
 *     el core que escribe NO basta — el OTRO core sigue ejecutando
 *     instrucciones desde XIP flash, y durante un `flash_range_erase`
 *     el flash no es accesible para fetch ⇒ hard fault en el otro core.
 *     Hay que PARQUEAR el otro core en una rutina residente en RAM
 *     mientras dura la operación. Eso es `multicore_lockout`.
 *
 * V7/H12, etapa 3.1 (5-oct; docs/V7_IDEAS.md §H12 «La etapa 3.1, el diseño»):
 *   - LAS DOS VÍCTIMAS: cualquier núcleo puede escribir (las tareas no van ancladas), así que cada núcleo se registra a
 *     sí mismo (`pico_flash_lock_init_victim`, desde una tarea anclada a él: lo hace el arranque, vm_task); antes sólo
 *     se registraba un hilo anclado a un núcleo ≠ 0, y no hay ninguno.
 *   - UN MUTEX NUESTRO para la ventana entera: `multicore_lockout_start_blocking` suelta el cerrojo del SDK en cuanto
 *     aparca al otro núcleo, y antes de apagar las interrupciones otra tarea de ESTE núcleo podía abrir su propia
 *     ventana y, al cerrarla, soltar al otro con la primera escribiendo todavía.
 *   - ANTES DEL PLANIFICADOR sólo vive el núcleo 0 (el 1 lo arranca vTaskStartScheduler): no hay a quién aparcar. Y
 *     con el planificador en marcha, no se escribe hasta que los dos núcleos son víctimas (se espera un rato; si no
 *     llegan, la escritura FALLA y quien la pidió lo sabe: nunca a ciegas).
 *   (El puerto de FreeRTOS de la RP2350 avisa entre núcleos con los doorbells; la FIFO, que usa el lockout, sólo la
 *   usa al lanzar el núcleo 1, y el SDK apaga ahí su interrupción.)
 */
#ifndef BPVM_FLASH_LOCK_H
#define BPVM_FLASH_LOCK_H

#include <stdint.h>

/* Registra el NÚCLEO que la llama como víctima del lockout (cada núcleo, desde una tarea anclada a él). Idempotente.
 * No-op con un núcleo. */
void pico_flash_lock_init_victim(void);

/* El mutex de la ventana (dos núcleos), creado en main() antes del planificador. Sin él, _begin devuelve -1. No-op con
 * un núcleo. */
void pico_flash_lock_prepara(void);

/* Cuántos núcleos están registrados como víctimas (con un núcleo, 1: no hace falta ninguno). */
int  pico_flash_lock_victimas(void);

/* Abre la ventana exclusiva y deja en *token el estado de las IRQs (pásalo a _end). 0 = abierta; -1 = no se puede
 * escribir (con dos núcleos, uno no se ha registrado como víctima): no se ha tocado nada y no hay que llamar a _end.
 * Single-core: save_and_disable_interrupts(). Dual-core: el mutex, aparcar al otro núcleo y apagar las IRQs. */
int  pico_flash_lock_begin(uint32_t* token);

/* Cierra la ventana. `token` es el valor que dejó _begin. */
void pico_flash_lock_end(uint32_t token);

#endif /* BPVM_FLASH_LOCK_H */
