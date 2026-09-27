/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32u5xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32u5xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bpvm_fallo_cm33.h"   /* V7/H5 P2b.3: que el HardFault hable (comun con la Pico) */
extern void stm32_wire_rx_drain();
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim17;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  /* V7/H5 P2b.3 — QUE HABLE (bpvm_fallo_cm33.h): hasta hoy aqui habia el bucle mudo de CubeMX, y un desborde de la
   * pila de una tarea —el limite PSPLIM, STKOF— o cualquier otro fallo dejaba la placa colgada sin nada en el log.
   * El EXC_RETURN sigue en LR al entrar (el prologo de esta funcion lo apila, pero no lo cambia), y su bit 2 dice
   * donde dejo el marco el hardware: en la pila de la TAREA (PSP), que el prologo no toca, o en la del manejador
   * (MSP), que el prologo ya movio — entonces el marco no se da y el informe lo dice. No vuelve. */
  {
    uint32_t exc_return;
    struct bpvm_fault_frame* marco = 0;
    __asm volatile ("mov %0, lr" : "=r" (exc_return));
    if (exc_return & 4u) __asm volatile ("mrs %0, psp" : "=r" (marco));
    bpvm_cm33_fallo_informe(marco, exc_return, "HardFault");
  }
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */

/**
  * @brief This function handles System tick timer.
  */

/******************************************************************************/
/* STM32U5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32u5xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM17 global interrupt.
  */
void TIM17_IRQHandler(void)
{
  /* USER CODE BEGIN TIM17_IRQn 0 */

  /* USER CODE END TIM17_IRQn 0 */
  HAL_TIM_IRQHandler(&htim17);
  /* USER CODE BEGIN TIM17_IRQn 1 */

  /* USER CODE END TIM17_IRQn 1 */
}


/* V6/A1.5 - SVC_Handler, PendSV_Handler y SysTick_Handler LOS DEFINE FreeRTOS.
 * El puerto Cortex-M33 (portable/GCC/ARM_CM33_NTZ/non_secure: port.c y portasm.c)
 * los declara con ESOS nombres y no son weak ni renombrables por macro, asi que
 * dejarlos aqui es un simbolo duplicado en el enlace. Los tres estaban vacios en
 * esta placa (el HAL late en TIM17, ver Core/Src/stm32u5xx_hal_timebase_tim.c), asi
 * que no se pierde nada al quitarlos. */
/* USER CODE BEGIN 1 */
void USART1_IRQHandler(void)
{
  stm32_wire_rx_drain();
}
/* USER CODE END 1 */
