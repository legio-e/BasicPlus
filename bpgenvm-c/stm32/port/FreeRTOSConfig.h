/*
 * FreeRTOSConfig.h — V6/A1.5: el kernel en el STM32U5 (Cortex-M33, sin TrustZone).
 *
 * ─── POR QUÉ HAY UN RTOS AQUÍ, SI ESTO ERA BARE-METAL ────────────────────────
 *
 * Decisión de Eduardo (4-sep-2026, ficha A1): *«FreeRTOS en todas las familias»*,
 * para que el reparto en DOS hilos —`vm` sólo opcodes, `io` todo lo demás— sea el
 * mismo en el PC, en el ESP32, en la Pico y aquí. El STM32 era la única familia
 * sin RTOS, y sin hilos el módulo común `src/bpvm_io.c` no puede arrancar: su
 * `bpvm_io_start` recibe -1 de la plataforma y todo se queda como estaba.
 *
 * Lo que se gana está medido en esta misma placa (5-sep, antes de tocar nada):
 * 2 000 líneas de salida costaban **71 357 ms** en la Nucleo y 71 377 en la
 * Discovery, porque cada `print` de tres argumentos viajaba en cuatro mensajes
 * OUTPUT y la UART va a 115 200 baud: 820 018 bytes por el cable, que a esa
 * velocidad son exactamente esos 71 s. Con `io` armando líneas, los mismos
 * 2 000 renglones son ~239 KB. Aquí la salida ES el cuello, más que en ninguna
 * otra placa.
 *
 * ─── DE DÓNDE SALE ESTE KERNEL ───────────────────────────────────────────────
 *
 * De `C:/lenguajes/pm/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel` (V11.3.0), que es
 * EXACTAMENTE el árbol que ya compila la Pico — comprobado en su caché de build,
 * no en un comentario: `pico/build/CMakeCache.txt` dice
 * `FREERTOS_KERNEL_PATH:PATH=C:/lenguajes/pm/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel`.
 * Ojo: en el disco hay OTRA copia (`C:/lenguajes/pm/FreeRTOS-Kernel`, V11.0.1+)
 * y no son la misma; usar cada familia la suya sería tener dos kernels sin que
 * nada lo dijera. Puerto: `portable/GCC/ARM_CM33_NTZ/non_secure`.
 *
 * ⚠️ Y no sale de ST. El paquete STM32Cube_FW_U5_V1.8.0 NO trae kernel FreeRTOS:
 * trae ThreadX, y lo que ahí se llama «FreeRTOS» es una capa de emulación de su
 * API sobre ThreadX. El plugin `mcu.freertos` de CubeIDE es depuración
 * thread-aware (66 ficheros, iconos), no fuentes. Con ThreadX, el STM32 tendría
 * un planificador distinto al de las otras cuatro familias, que es justo lo que
 * A1 viene a evitar.
 *
 * ─── LO QUE ESTE FICHERO DECIDE, Y EL PORQUÉ DE LO QUE NO ES OBVIO ───────────
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* `SystemCoreClock` lo fija SystemClock_Config() antes de arrancar el
 * planificador, y el puerto lo lee al montar el SysTick — por eso vale la
 * variable y no una constante: si mañana cambia el reloj de una placa, el tick
 * sigue siendo de 1 ms sin tocar nada aquí. */
#include <stdint.h>
extern uint32_t SystemCoreClock;

#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t) 1000)

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1   /* A1: `vm` e `io` van a la MISMA prioridad y se turnan por tiempo */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configIDLE_SHOULD_YIELD                 1
#define configMAX_PRIORITIES                    8
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_TIMERS                        0   /* nadie usa timers de software; se ahorra su tarea y su cola */

/* La pila mínima, en PALABRAS (no bytes): 128 × 4 = 512 B. Es la de la tarea
 * idle, que aquí no hace nada. */
#define configMINIMAL_STACK_SIZE                ((uint16_t) 128)

/* ─── EL HEAP DEL KERNEL: heap_4, y separado del de la VM a propósito ────────
 *
 * De aquí salen las pilas de las tareas y los objetos del kernel; NO sale nada
 * de BasicPlus. El bloque de la VM es un array estático (`s_vm_mem`, tamaño
 * BOARD_VM_BYTES en stm32/port/board.h) y su tabla de símbolos y de handles
 * salen del malloc de newlib. Tenerlos en bolsas distintas cuesta unos KB y a
 * cambio hace que un fallo diga QUIÉN se quedó sin memoria — que en este
 * proyecto ya costó dos días de diagnóstico cuando la tabla de handles y el
 * heap eran bolsas distintas y nadie lo decía (#430).
 *
 * 24 KB: pila de `vm` (16 KB, lo que hoy reserva el enlazador como MSP porque
 * el intérprete corre sobre ella) + `io` (4 KB) + idle (0,5 KB) + los objetos.
 * En la Nucleo quedan ~174 KB libres para newlib y en la Discovery ~447 KB,
 * así que esto no aprieta en ninguna de las dos. */
#define configTOTAL_HEAP_SIZE                   ((size_t) (24 * 1024))
/* A 0 a proposito: nada de este firmware usa xTaskCreateStatic, y ponerlo a 1
 * obliga a dar vApplicationGetIdleTaskMemory. Todo sale del heap del kernel,
 * que esta dimensionado justo para eso. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* ─── QUE LOS FALLOS HAGAN RUIDO (norma del proyecto) ───────────────────────
 * Un desbordamiento de pila o un malloc fallido en un micro se manifiesta, si
 * nadie mira, como un cuelgue mudo tres pasos más allá. Estas tres cosas los
 * convierten en un mensaje. Cuestan un poco de CPU en cada cambio de contexto y
 * se dejan puestas: el firmware se publica sabiendo por qué se murió. */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configRECORD_STACK_HIGH_ADDRESS         1

/* ─── Cortex-M33 sin TrustZone ─────────────────────────────────────────────── */
#define configENABLE_TRUSTZONE                  0
#define configRUN_FREERTOS_SECURE_ONLY          1
#define configENABLE_MPU                        0
#define configENABLE_FPU                        1

/* Bits de prioridad del NVIC en el U5 (los mismos que declara el CMSIS del
 * dispositivo) y el umbral de FreeRTOS. Una ISR con prioridad numéricamente
 * MENOR que este umbral no puede llamar a ninguna API del kernel — ni a las
 * `...FromISR`.
 *
 * 📌 Aquí hay un dato de esta placa que conviene tener delante: la ISR del wire
 * está a 5 en la Nucleo (BOARD_WIRE_IRQ_PRIO, stm32/port/board.h) y a 0 en la
 * Discovery (la que le puso CubeMX). Con el umbral en 5, la de la Nucleo podría
 * llamar al kernel y la de la Discovery NO. Da igual mientras la ISR siga
 * haciendo lo que hace hoy —volcar bytes a un anillo que el otro lado sondea—,
 * que es justo por lo que el diseño de `io` no necesita despertar a nadie desde
 * la interrupción. Si algún día se quisiera, hay que bajar la de la Discovery
 * primero. */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Sólo lo que se usa de verdad. `uxTaskGetStackHighWaterMark` no es un lujo: es
 * cómo se mide la pila que le hace falta a `vm` en vez de elegirla a ojo. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/* El aserto del kernel: lo implementa stm32/port/platform_stm32.c, que lo manda
 * al log persistente (post-mortem en flash) y para. Sin esto, un configASSERT
 * fallido es un bucle infinito anónimo. */
void bpvm_stm32_assert_rtos(const char* fichero, unsigned linea);
#define configASSERT(x) \
    if ((x) == 0) bpvm_stm32_assert_rtos(__FILE__, (unsigned) __LINE__)

#endif /* FREERTOS_CONFIG_H */
