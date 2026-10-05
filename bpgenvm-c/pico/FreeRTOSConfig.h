/*
 * FreeRTOSConfig.h — configuración del kernel para el firmware bpgenvm-c
 * en Pico 2 (RP2350, Cortex-M33).
 *
 * Basado en los requisitos del port RP2350_ARM_NTZ del FreeRTOS-Kernel:
 * sin TrustZone, sin MPU, secure-only, FPU activa.
 *
 * #153 — Número de cores parametrizado por BPVM_PICO_NUM_CORES (lo
 * inyecta CMake). Default 1 (single-core, el firmware que se envía).
 * =2 activa SMP dual-core: el port RP2350_ARM_NTZ es SMP-capaz y
 * arranca el segundo core en vTaskStartScheduler. Ver docs/SMP_ARCH.md
 * §"Port a Pico" para el runbook de bring-up dual-core y los riesgos
 * (lockout FIFO vs scheduler SMP, flash XIP-safe, USB en core 0).
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --- Modelo de ejecución -------------------------------------- */
#ifndef BPVM_PICO_NUM_CORES
#define BPVM_PICO_NUM_CORES                     1
#endif
#define configNUMBER_OF_CORES                   BPVM_PICO_NUM_CORES

#if ( configNUMBER_OF_CORES > 1 )
/* --- Flags SMP (solo dual-core) ------------------------------- */
/* Permite que corran simultáneamente tasks de DISTINTA prioridad en
 * cores distintos (sin esto, SMP solo paraleliza tasks de igual prio).
 * Nuestro comm (IDLE+1) y worker (IDLE+1) son iguales hoy, pero lo
 * dejamos en 1 por si el comm sube de prioridad. */
#define configRUN_MULTIPLE_PRIORITIES           1
/* Habilita vTaskCoreAffinitySet / xTaskCreateAffinitySet — base del
 * pinning comm→core0 / worker→core1 (bpvm_platform_thread_create_pinned). */
#define configUSE_CORE_AFFINITY                 1
/* V7/H12, etapa 3.1: las tareas ociosas DUERMEN (__wfi) en vez de girar ejecutando desde la flash, que compite por
 * la caché del XIP con el otro núcleo (pico/main.c, los ganchos). Medido en la Pico 2 con un run: fib(26) de 2 992
 * a 2 895 ms con los dos núcleos, y de 3 027 a 2 924 con vm.cores=1. */
#define configUSE_PASSIVE_IDLE_HOOK             1
#endif

#define configUSE_PREEMPTION                    1
#if ( configNUMBER_OF_CORES > 1 )
#define configUSE_IDLE_HOOK                     1   /* V7/H12 3.1: ver arriba */
#else
#define configUSE_IDLE_HOOK                     0
#endif
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

/* --- V7/H12, etapas 0 y 1 (4-oct): LA TAREA `run` (pico/repl_v1.c) ----------
 * El interruptor de la imagen: 1, el programa en la tarea `run` (permanente, su
 * pila estática en el .bss) y `vm_task` hace de `io`; 0, el RUN de antes (el
 * programa en `vm_task` y un `io` de 4 KB, de este heap, en cada RUN). */
#ifndef BPVM_PICO_RUN_TAREA
#define BPVM_PICO_RUN_TAREA                     1
#endif

/* --- Heap del kernel (heap_4) --------------------------------- */
/* El "memory[]" de la VM NO sale de aquí; es un array estático en main.c.
 * Este heap solo se usa para tasks/mutexes de FreeRTOS. V7/H12: con la tarea
 * `run`, el `io` de cada RUN (4 KB) ya no nace, y lo que sobra va a compensar
 * la pila de `run` (Eduardo, 4-oct: «si necesitamos más stack y nos ahorramos
 * heap, podemos reducir un poco el heap para compensar»). Medido con 28 KB en la
 * Pico 2 y la Metro (la tanda, y ~120 minutos de resistencia): el peor momento,
 * 10,4–10,7 KB libres. Con 24, ~6,4 KB de margen —lo que vive aquí es la pila de
 * `vm_task` (16 KB) y los objetos del núcleo—; `rtosHeapMinFreeBytes` en el INFO
 * lo dice. La pila de `run` (16 KB, en el .bss) cuesta así 8 KB netos. */
#if BPVM_PICO_RUN_TAREA
#define configTOTAL_HEAP_SIZE                   ((size_t)(24 * 1024))
#else
#define configTOTAL_HEAP_SIZE                   ((size_t)(32 * 1024))
#endif
#define configSUPPORT_STATIC_ALLOCATION         1
/* V7/H12 2b (5-oct): dos índices de notificación por tarea; el 1, el de la condición de la VM
 * (src/platform_freertos_cond.inc), que no se cruza con el 0 de los stream buffers y los drivers. */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   2
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
#if ( configNUMBER_OF_CORES > 1 )
/* V7/H12, etapa 3.1, paso 5 — cerrar el ejecutor de una native con dos núcleos: esperar a que esté SUSPENDIDO (y fuera
 * de los dos núcleos) antes de borrarlo y devolver su memoria (pico/platform_freertos.c, bpvm_platform_thread_join). */
#define INCLUDE_eTaskGetState                   1
#endif
/* #354 — marca de agua de las pilas. GRATIS aquí: lo que hace posible la medida
 * es que las pilas se rellenen con un patrón conocido al crearlas, y eso YA se
 * está pagando por `configCHECK_FOR_STACK_OVERFLOW 2` (arriba). Este define solo
 * compila la función que lee ese patrón.
 *
 * De momento es SOLO DIAGNÓSTICO: sirve para saber si a las pilas les sobra
 * sitio. No se recorta nada hasta ver los números con carga real en placa, y en
 * la duda se deja como está — desbordar una pila en un micro no da un error
 * limpio, da corrupción muda. */
#define INCLUDE_uxTaskGetStackHighWaterMark      1

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
