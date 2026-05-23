/*
 * FreeRTOSConfig.h — configuración del kernel para el firmware bpgenvm-c
 * en Pico 2 (RP2350, Cortex-M33).
 *
 * Basado en los requisitos del port RP2350_ARM_NTZ del FreeRTOS-Kernel:
 * sin TrustZone, sin MPU, secure-only, FPU activa. Mantenemos
 * configNUMBER_OF_CORES=1 para el bring-up — luego se puede subir a 2.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --- Modelo de ejecución -------------------------------------- */
#define configNUMBER_OF_CORES                   1
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configCPU_CLOCK_HZ                      150000000  /* RP2350 default */
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ((unsigned short)256)
#define configMAX_TASK_NAME_LEN                 16
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_16_BIT_TICKS                  0

/* --- Sync primitives ------------------------------------------ */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
/* configUSE_TIMERS=1 lo exige el port RP2350_ARM_NTZ (doorbell IRQ
 * handler usa xEventGroupSetBitsFromISR → necesita pendable function
 * calls que viven en el timer daemon). */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                4
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_EVENT_GROUPS                  1

/* --- Heap del kernel (heap_4) --------------------------------- */
/* El "memory[]" de la VM NO sale de aquí; es un array estático en main.c.
 * Este heap solo se usa para tasks/mutexes de FreeRTOS. */
#define configTOTAL_HEAP_SIZE                   ((size_t)(32 * 1024))
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* --- Checks --------------------------------------------------- */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

/* --- API set incluida ----------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTimerPendFunctionCall          1

/* --- Requisitos del port RP2350_ARM_NTZ (ver README) ---------- */
#define configENABLE_MPU                        0
#define configENABLE_TRUSTZONE                  0
#define configRUN_FREERTOS_SECURE_ONLY          1
#define configENABLE_FPU                        1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    16

/* --- Pico SDK + FreeRTOS interop ------------------------------ */
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

/* --- Assert custom -------------------------------------------- */
extern void vAssertCalled(const char *pcFileName, unsigned long ulLine);
#define configASSERT(x) if ((x) == 0) vAssertCalled(__FILE__, __LINE__)

#endif /* FREERTOS_CONFIG_H */
