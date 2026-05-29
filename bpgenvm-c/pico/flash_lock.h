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
 *     mientras dura la operación. Eso es `multicore_lockout`:
 *       * el core víctima (worker, core 1) se registra una vez con
 *         `bpvm_flash_lock_init_victim()`;
 *       * el core escritor (REPL/comm, core 0) llama
 *         `multicore_lockout_start_blocking()` → el víctima salta a su
 *         handler en RAM y gira con IRQs off; tras la operación,
 *         `multicore_lockout_end_blocking()` lo libera.
 *
 * ⚠️  RIESGO DE BRING-UP (#153): `multicore_lockout_victim_init()`
 *     instala un handler en la IRQ del FIFO inter-core (SIO_IRQ_PROC1).
 *     El port FreeRTOS-SMP del RP2350 TAMBIÉN usa ese FIFO para señalizar
 *     entre cores. Si chocan, el lockout o el scheduler se cuelga. Es el
 *     mismo tipo de conflicto que tumbó USB CDC al meter `pico_flash` en
 *     FP2. POR ESO el dual-core es opt-in y NO se da por validado hasta
 *     probarlo en placa (ver docs/SMP_ARCH.md §"Port a Pico" runbook).
 *     Plan B si choca: barrera a nivel FreeRTOS (suspender el worker +
 *     girar el idle de core 1 en RAM) en vez del lockout del SDK.
 */
#ifndef BPVM_FLASH_LOCK_H
#define BPVM_FLASH_LOCK_H

#include <stdint.h>

/* Registra el core llamante como víctima de lockout. Debe llamarse UNA
 * vez, desde el core que NO escribe flash (worker, core 1), tras
 * arrancar su task. No-op en single-core. */
void bpvm_flash_lock_init_victim(void);

/* Abre la ventana exclusiva. Devuelve el token de IRQs (pásalo a _end).
 * Single-core: == save_and_disable_interrupts().
 * Dual-core: parquea el otro core + save_and_disable_interrupts(). */
uint32_t bpvm_flash_lock_begin(void);

/* Cierra la ventana. `token` es el valor que devolvió _begin. */
void bpvm_flash_lock_end(uint32_t token);

#endif /* BPVM_FLASH_LOCK_H */
