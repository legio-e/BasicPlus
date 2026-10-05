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
 *     mientras dura la operación.
 *
 * V7/H12, etapa 3.1 (5-oct; docs/V7_IDEAS.md §H12 «La etapa 3.1, el diseño»):
 *   - APARCAR AL OTRO NÚCLEO COMO TAREA, NO POR INTERRUPCIÓN. Primero se hizo con `multicore_lockout` (el otro núcleo,
 *     aparcado en una interrupción de la FIFO), y la puerta de la 3.1 lo cazó: un interbloqueo intermitente. El otro
 *     núcleo puede estar a mitad de un `vTaskSuspendAll` (su tarea ociosa liberando una tarea borrada: el cerrojo de
 *     tareas del kernel, cogido, con las interrupciones abiertas); se aparca en la interrupción CON ESE CERROJO; y el
 *     que escribe, al soltar el mutex del SDK (la interoperación con FreeRTOS: `xEventGroupSetBits` →
 *     `vTaskSuspendAll`), lo necesita. Lo dice el propio SDK (pico_flash/flash.c): «we cannot mix multicore_lockout and
 *     FreeRTOS». Así que, como hace el SDK con FreeRTOS SMP, el otro núcleo se aparca con una TAREA —sólo corre cuando
 *     su núcleo no tiene cerrojos del kernel—: un aparcador estático por núcleo, de la prioridad más alta, que espera
 *     un aviso, apaga sus interrupciones y espera en la RAM a que lo suelten. Con TOPE (500 ms): si no se aparca, la
 *     escritura FALLA y el log lo dice; nunca un cuelgue mudo.
 *   - UN MUTEX NUESTRO para la ventana entera: una petición cada vez (el estado del aparcador es uno).
 *   - ANTES DEL PLANIFICADOR sólo vive el núcleo 0 (el 1 lo arranca vTaskStartScheduler): no hay a quién aparcar.
 *     El mutex y los aparcadores se crean en main() (`pico_flash_lock_prepara`), antes del planificador.
 */
#ifndef BPVM_FLASH_LOCK_H
#define BPVM_FLASH_LOCK_H

#include <stdint.h>

/* Ya no hace nada (V7/H12 3.1: no hay víctimas por interrupción; los aparcadores las sustituyen). Se queda la llamada
 * de platform_freertos.c, la de un hilo anclado. */
void pico_flash_lock_init_victim(void);

/* El mutex de la ventana y un aparcador anclado a cada núcleo (dos núcleos), creados en main() antes del
 * planificador. Sin ellos, _begin devuelve -1. No-op con un núcleo. */
void pico_flash_lock_prepara(void);

/* Cuántos aparcadores hay (con un núcleo, 1: no hace falta ninguno). */
int  pico_flash_lock_victimas(void);

/* Abre la ventana exclusiva y deja en *token el estado de las IRQs (pásalo a _end). 0 = abierta; -1 = no se puede
 * escribir (con dos núcleos: sin aparcadores, o el otro núcleo no se aparca en 500 ms —el log lo dice—): no se ha
 * tocado nada y no hay que llamar a _end. Single-core: save_and_disable_interrupts(). Dual-core: el mutex, aparcar al
 * otro núcleo (su aparcador) y apagar las IRQs. */
int  pico_flash_lock_begin(uint32_t* token);

/* Cierra la ventana. `token` es el valor que dejó _begin. */
void pico_flash_lock_end(uint32_t token);

#endif /* BPVM_FLASH_LOCK_H */
