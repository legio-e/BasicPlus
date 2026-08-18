/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32U575xI Device from STM32U5 series
**                      2048Kbytes FLASH
**                      784Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2026 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);	/* end of "RAM" Ram type memory */
_sstack = _estack - _Min_Stack_Size;

_Min_Heap_Size = 0x4000;	/* required amount of heap  */
_Min_Stack_Size = 0x1000;	/* required amount of stack */

/* Memories definition */
/* H9 — layout por capas (Eduardo): [firmware][log][env A/B][datos FS+Packs].
 *   FLASH   1016K  → firmware (el enlazador SOLO coloca secciones aquí).
 *   BP_LOG     8K  → log persistente de diagnóstico (1 página), SIN secciones.
 *   BP_ENV    16K  → env A/B (2 páginas de 8K), SIN secciones.
 *   BP_DATA 1008K  → zona de datos FS+Packs (el env reparte FS|Packs), SIN secciones.
 * BP_LOG/BP_ENV/BP_DATA no llevan secciones → el flasheo no los escribe → sobreviven
 * al reflasheo (salvo borrado masivo). Disjuntas del firmware → un fallo de escritura
 * del log/FS/env NUNCA corrompe el programa. El log sale del hueco que cede el firmware
 * al encoger FLASH 1024K→1016K (el firmware usa ~320K, sobra de sobra). */
MEMORY
{
  RAM	  (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
  SRAM4	  (xrw) : ORIGIN = 0x28000000, LENGTH = 16K
  FLASH	  (rx)  : ORIGIN = 0x08000000, LENGTH = 1016K
  BP_LOG  (r)   : ORIGIN = 0x080FE000, LENGTH = 8K
  BP_ENV  (r)   : ORIGIN = 0x08100000, LENGTH = 16K
  BP_DATA (r)   : ORIGIN = 0x08104000, LENGTH = 1008K
}

/* H9 — límites reales para la red anti-divergencia de board_mgr_stm32.c: el env
 * debe caer entre el fin del firmware y el inicio de los datos, o las escrituras
 * de env se RECHAZAN (nunca tocar código). Deben cuadrar con flash_layout_stm32.h. */
__bp_flash_end  = ORIGIN(FLASH) + LENGTH(FLASH);   /* 0x080FE000 (fin del código; el log ocupa la página siguiente) */
__bp_log_start  = ORIGIN(BP_LOG);                  /* 0x080FE000 (sector del log persistente) */
__bp_part_start = ORIGIN(BP_DATA);                 /* 0x08104000 */

/* Sections */
SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .isr_vector :
  {
    KEEP(*(.isr_vector)) /* Startup code */
  } >FLASH

  /* The program code and other data into "FLASH" Rom type memory */
  .text :
  {
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data into "FLASH" Rom type memory */
  .rodata :
  {
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH
  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into "RAM" Ram type memory */
  .data :
  {
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  /* Uninitialized data section into "RAM" Ram type memory */
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* #439 — SECCION QUE EL ARRANQUE NO BORRA.
   *
   * El log post-mortem vive aqui para que sobreviva al reset: el startup solo
   * limpia de _sbss a _ebss, asi que lo que caiga FUERA de ese rango llega
   * intacto. Gemelo de `__uninitialized_ram` (Pico) y `__NOINIT_ATTR` (ESP32).
   *
   * ATENCION: este .ld lo genera CubeIDE. Si algun dia se regenera el proyecto
   * hay que volver a poner esto — y se notara porque el log dejara de
   * sobrevivir, en silencio. Comprobacion: `arm-none-eabi-objdump -h` tiene que
   * enseñar `.noinit` con ALLOC y SIN LOAD, y el simbolo del log dentro.
   */
  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    *(.noinit)
    *(.noinit*)
    . = ALIGN(4);
  } >RAM

  /* User_heap_stack section, used to check that there is enough "RAM" Ram type memory left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
