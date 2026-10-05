/*
 * flash_lock.c — implementación de la ventana exclusiva de flash.
 * Ver flash_lock.h para el porqué de los dos regímenes (y de lo de la etapa 3.1 de H12).
 */

#include "flash_lock.h"

#include "FreeRTOS.h"           /* configNUMBER_OF_CORES */
#include "hardware/sync.h"      /* save_and_disable_interrupts / restore */

#if ( configNUMBER_OF_CORES > 1 )
#include "task.h"
#include "semphr.h"
#include "pico/time.h"          /* make_timeout_time_ms, time_reached */
#include "pico/platform.h"      /* get_core_num, __not_in_flash_func */
#include "nucleos.h"            /* V7/H12, 3.1: las marcas (0x5X, el candado) */
#include "bpvm_log.h"           /* el diagnóstico de un núcleo que no se aparca */

/* V7/H12, etapa 3.1 — APARCAR AL OTRO NÚCLEO COMO TAREA, NO POR INTERRUPCIÓN (flash_lock.h). Cada núcleo tiene su
 * aparcador: una tarea ESTÁTICA anclada a él, de la prioridad más alta, que espera un aviso. Quien va a escribir la
 * flash avisa al del otro núcleo, que —como tarea, sólo corre cuando su núcleo no tiene cerrojos del kernel— apaga sus
 * interrupciones, dice «aparcado» y espera en la RAM a que lo suelten. Es lo que hace el SDK para FreeRTOS SMP
 * (pico_flash/flash.c: «we cannot mix multicore_lockout and FreeRTOS»), con la tarea permanente en vez de una por
 * escritura. Una petición cada vez (bajo s_ventana): un solo estado. */
enum { NADA = 0, PIDE = 1, APARCADO = 2, SOLTAR = 3, SUELTO = 4 };
static volatile uint8_t s_estado = NADA;

static StaticSemaphore_t s_ventana_mem;
static SemaphoreHandle_t s_ventana = NULL;   /* el mutex de la ventana entera (flash_lock.h) */
static int s_aparcado = 0;                   /* bajo s_ventana: esta ventana aparcó al otro núcleo */

#define APARCADOR_PILA  configMINIMAL_STACK_SIZE
static TaskHandle_t s_aparcador[2];
static StaticTask_t s_aparcador_tcb[2];
static StackType_t  s_aparcador_pila[2][APARCADOR_PILA];

/* En la RAM: mientras está aparcado, el otro núcleo puede estar borrando la flash (sin XIP). Lo que se llama antes
 * de apagar las interrupciones (el aviso de FreeRTOS) sí puede estar en la flash. */
static void __not_in_flash_func(aparcador)(void* arg) {
    (void) arg;
    if (get_core_num() == 1u) pico_marca_arranque(0x11);   /* el núcleo 1, vivo (y su aparcador, listo) */
    for (;;) {
        (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t save = save_and_disable_interrupts();
        uint8_t esperado = PIDE;
        if (__atomic_compare_exchange_n(&s_estado, &esperado, (uint8_t) APARCADO, 0,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            __sev();
            while (__atomic_load_n(&s_estado, __ATOMIC_ACQUIRE) == APARCADO) __wfe();
            restore_interrupts(save);
            __atomic_store_n(&s_estado, (uint8_t) SUELTO, __ATOMIC_RELEASE);
            __sev();
        } else {
            restore_interrupts(save);   /* una petición que ya se rindió (su tope): nada que hacer */
        }
    }
}
#endif

void pico_flash_lock_init_victim(void) {
    /* V7/H12, etapa 3.1: ya no hay víctimas por interrupción (el aparcador de cada núcleo las sustituye). Se queda la
     * llamada (platform_freertos.c la hace para un hilo anclado) y no hace nada. */
}

void pico_flash_lock_prepara(void) {
#if ( configNUMBER_OF_CORES > 1 )
    /* En main(), antes del planificador: con un solo núcleo vivo y ninguna tarea, crear no compite con nadie. */
    if (s_ventana == NULL) s_ventana = xSemaphoreCreateMutexStatic(&s_ventana_mem);
    for (unsigned c = 0; c < 2u; c++)
        if (s_aparcador[c] == NULL)
            s_aparcador[c] = xTaskCreateStaticAffinitySet(aparcador, c ? "aparca1" : "aparca0", APARCADOR_PILA, NULL,
                                                          configMAX_PRIORITIES - 1, s_aparcador_pila[c],
                                                          &s_aparcador_tcb[c], (UBaseType_t) 1u << c);
#endif
}

int pico_flash_lock_victimas(void) {
#if ( configNUMBER_OF_CORES > 1 )
    return (s_aparcador[0] != NULL) + (s_aparcador[1] != NULL);   /* los aparcadores creados */
#else
    return 1;
#endif
}

int pico_flash_lock_begin(uint32_t* token) {
#if ( configNUMBER_OF_CORES > 1 )
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        if (s_ventana == NULL || s_aparcador[0] == NULL || s_aparcador[1] == NULL)
            return -1;   /* sin pico_flash_lock_prepara (main): no se escribe a ciegas */
        pico_marca_arranque(0x51);   /* a por el mutex */
        xSemaphoreTake(s_ventana, portMAX_DELAY);
        unsigned otro = get_core_num() ^ 1u;
        pico_marca_arranque(0x52);   /* el mutex; a aparcar al otro núcleo */
        __atomic_store_n(&s_estado, (uint8_t) PIDE, __ATOMIC_RELEASE);
        xTaskNotifyGive(s_aparcador[otro]);
        absolute_time_t hasta = make_timeout_time_ms(500);
        while (__atomic_load_n(&s_estado, __ATOMIC_ACQUIRE) != APARCADO && !time_reached(hasta)) __wfe();
        if (__atomic_load_n(&s_estado, __ATOMIC_ACQUIRE) != APARCADO) {
            uint8_t esperado = PIDE;
            if (__atomic_compare_exchange_n(&s_estado, &esperado, (uint8_t) NADA, 0,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
                /* El otro núcleo no se ha aparcado en 500 ms (tiene su planificador parado o una tarea de la misma
                 * prioridad que no suelta): esperar para siempre sería colgar la placa sin decir nada. La escritura
                 * FALLA (-1), y el log dice qué corre en el otro núcleo y las marcas. */
                TaskHandle_t t = xTaskGetCurrentTaskHandleForCore((BaseType_t) otro);
                pico_marca_arranque(0x5E);
                log_printf("flash: el nucleo %u NO se aparca en 500 ms (corre '%s'; marcas 0=%02lx 1=%02lx): "
                           "la escritura FALLA", otro, t ? pcTaskGetName(t) : "?",
                           (unsigned long) pico_marca_de(0), (unsigned long) pico_marca_de(1));
                xSemaphoreGive(s_ventana);
                return -1;
            }
            /* se aparcó justo al vencer el tope: sigue */
        }
        pico_marca_arranque(0x53);   /* el otro, aparcado */
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
        pico_marca_arranque(0x58);   /* a soltar al otro núcleo */
        __atomic_store_n(&s_estado, (uint8_t) SOLTAR, __ATOMIC_RELEASE);
        __sev();
        absolute_time_t hasta = make_timeout_time_ms(500);
        while (__atomic_load_n(&s_estado, __ATOMIC_ACQUIRE) != SUELTO && !time_reached(hasta)) __wfe();
        if (__atomic_load_n(&s_estado, __ATOMIC_ACQUIRE) != SUELTO)
            log_printf("flash: el aparcador no dice SUELTO en 500 ms (sigue: el estado se reinicia)");
        __atomic_store_n(&s_estado, (uint8_t) NADA, __ATOMIC_RELEASE);
        xSemaphoreGive(s_ventana);
        pico_marca_arranque(0x5A);   /* la ventana, cerrada */
    }
#endif
}
