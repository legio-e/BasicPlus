/*
 * flash_lock.c — implementación de la ventana exclusiva de flash.
 * Ver flash_lock.h para el porqué de los dos regímenes.
 */

#include "flash_lock.h"

#include "FreeRTOS.h"           /* configNUMBER_OF_CORES */
#include "hardware/sync.h"      /* save_and_disable_interrupts / restore */

#if ( configNUMBER_OF_CORES > 1 )
#include "pico/multicore.h"     /* multicore_lockout_* */
#endif

void bpvm_flash_lock_init_victim(void) {
#if ( configNUMBER_OF_CORES > 1 )
    /* Una sola vez por core víctima. Idempotente en el SDK (re-init
     * re-arma el handler), pero lo llamamos una vez al arrancar el
     * worker en core 1. */
    multicore_lockout_victim_init();
#endif
    /* Single-core: nada que hacer — no hay otro core que parquear. */
}

uint32_t bpvm_flash_lock_begin(void) {
#if ( configNUMBER_OF_CORES > 1 )
    /* Parquea el/los otro(s) core(s) en su handler RAM-resident ANTES
     * de tocar XIP. Bloqueante: vuelve cuando el otro core confirma que
     * está aparcado. */
    multicore_lockout_start_blocking();
#endif
    return save_and_disable_interrupts();
}

void bpvm_flash_lock_end(uint32_t token) {
    restore_interrupts(token);
#if ( configNUMBER_OF_CORES > 1 )
    multicore_lockout_end_blocking();
#endif
}
