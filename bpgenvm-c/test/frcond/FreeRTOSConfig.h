/*
 * FreeRTOSConfig.h — SÓLO para test/test_frcond.c (make test-frcond): el kernel de FreeRTOS de verdad en el PC, con su
 * port de Windows (portable/MSVC-MingW), para probar la variable de condición de la VM (src/platform_freertos_cond.inc)
 * con la semántica de FreeRTOS y no con la de pthreads, que no pierde avisos. Lo mínimo: un tick de 1 ms como en la
 * Pico y la STM32, prioridades, mutex, notificaciones con el índice propio de la condición, el heap 4.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configTICK_RATE_HZ                      ((TickType_t) 1000)
#define configMINIMAL_STACK_SIZE                ((unsigned short) 256)
#define configTOTAL_HEAP_SIZE                   ((size_t) (1024 * 1024))
#define configMAX_TASK_NAME_LEN                 (16)
#define configUSE_TRACE_FACILITY                0
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_APPLICATION_TASK_TAG          0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   2   /* el 1, el de la condición (platform_freertos_cond.inc) */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_TIMERS                        0
#define configUSE_CO_ROUTINES                   0
#define configMAX_PRIORITIES                    (7)
#define configENABLE_BACKWARD_COMPATIBILITY     1

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetSchedulerState          1

extern void vAssertCalled(unsigned long ulLine, const char* const pcFileName);
#define configASSERT(x) if ((x) == 0) vAssertCalled(__LINE__, __FILE__)

#endif /* FREERTOS_CONFIG_H */
