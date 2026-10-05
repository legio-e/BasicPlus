/*
 * flash_lock.c — implementación de la ventana exclusiva de flash.
 * Ver flash_lock.h para el porqué de los dos regímenes (y de lo de la etapa 3.1 de H12).
 */

#include "flash_lock.h"

#include "FreeRTOS.h"           /* configNUMBER_OF_CORES */
#include "hardware/sync.h"      /* save_and_disable_interrupts / restore */

#if ( configNUMBER_OF_CORES > 1 )
#include "task.h"               /* xTaskGetSchedulerState, vTaskDelay */
#include "semphr.h"
#include "pico/multicore.h"     /* multicore_lockout_* */

static StaticSemaphore_t s_ventana_mem;
static SemaphoreHandle_t s_ventana = NULL;   /* el mutex de la ventana entera (flash_lock.h) */
static int s_aparcado = 0;                   /* bajo s_ventana: esta ventana aparcó al otro núcleo */
static uint32_t s_victimas = 0;              /* un bit por núcleo registrado (__atomic) */
#endif

void pico_flash_lock_init_victim(void) {
#if ( configNUMBER_OF_CORES > 1 )
    /* Idempotente en el SDK (mira su bandera por núcleo); el bit, también. */
    multicore_lockout_victim_init();
    __atomic_fetch_or(&s_victimas, 1u << get_core_num(), __ATOMIC_RELEASE);
#endif
    /* Single-core: nada que hacer — no hay otro core que parquear. */
}

void pico_flash_lock_prepara(void) {
#if ( configNUMBER_OF_CORES > 1 )
    /* En main(), antes del planificador: con un solo núcleo vivo y ninguna tarea, crearlo no compite con nadie. */
    if (s_ventana == NULL) s_ventana = xSemaphoreCreateMutexStatic(&s_ventana_mem);
#endif
}

int pico_flash_lock_victimas(void) {
#if ( configNUMBER_OF_CORES > 1 )
    uint32_t v = __atomic_load_n(&s_victimas, __ATOMIC_ACQUIRE);
    return (int) ((v & 1u) + ((v >> 1) & 1u));
#else
    return 1;
#endif
}

int pico_flash_lock_begin(uint32_t* token) {
#if ( configNUMBER_OF_CORES > 1 )
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        /* Con el planificador en marcha: los dos núcleos, víctimas (el arranque los registra al empezar vm_task; un
         * rato de margen por si alguien escribe a la vez), o no se escribe. */
        for (int i = 0; pico_flash_lock_victimas() < 2; i++) {
            if (i >= 500) return -1;   /* ~0,5 s: no ha llegado; quien pidió la escritura lo sabe */
            vTaskDelay(1);
        }
        if (s_ventana == NULL) return -1;   /* sin pico_flash_lock_prepara (main): no se escribe a ciegas */
        xSemaphoreTake(s_ventana, portMAX_DELAY);
        /* Parquea el otro core en su handler RAM-resident ANTES de tocar XIP. Bloqueante: vuelve cuando el otro core
         * confirma que está aparcado. */
        multicore_lockout_start_blocking();
        s_aparcado = 1;
    }
#endif
    *token = save_and_disable_interrupts();
    return 0;
}

void pico_flash_lock_end(uint32_t token) {
    restore_interrupts(token);
#if ( configNUMBER_OF_CORES > 1 )
    if (s_aparcado) {
        s_aparcado = 0;
        multicore_lockout_end_blocking();
        xSemaphoreGive(s_ventana);
    }
#endif
}
