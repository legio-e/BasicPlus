/*
 * flash_layout_stm32.h — H9: el mapa de flash del STM32 (U5) en UN sitio (lado C).
 *
 * Layout por capas (docs/H9_KERNEL_CAPAS.md; decidido con Eduardo). La AUTORIDAD
 * de los límites es el linker (.ld, regiones FLASH / BP_ENV / BP_DATA); este header
 * define lo que el código C necesita: los offsets DENTRO de la flash (desde
 * FLASH_BASE = 0x08000000) del env A/B y la base de particiones.
 *
 *   0x08000000  FLASH   firmware        (el enlazador SOLO usa esta región)
 *   ..BP_ENV_A  env A (1 página 8K)     ┐ SIN secciones → el flasheo no los graba
 *   ..BP_ENV_B  env B (1 página 8K)     ┘ → sobreviven al reflasheo (salvo mass-erase)
 *   ..BP_PART_BASE  PARTICIONES H9 (bpvm_part: FS + Packs, offsets derivados desde
 *                   BP_PART_BASE hasta BP_USABLE_FLASH; el env reparte FS|Packs).
 *
 * Board-aware por el símbolo BPVM_BOARD_DK2 (igual que board.h). Red anti-divergencia
 * en board_mgr_stm32.c: comprueba que el env cae entre __bp_flash_end y __bp_part_start
 * (símbolos del .ld) → si el .ld y este header divergen, las escrituras de env se
 * RECHAZAN en vez de borrar código en silencio.
 */
#ifndef FLASH_LAYOUT_STM32_H
#define FLASH_LAYOUT_STM32_H

#include <stdint.h>

/* Offsets del env y base de particiones, DESDE FLASH_BASE. La página de borrado
 * del U5 es de 8 KB → cada copia del env ocupa una página; el tombstone/escritura
 * es por página entera (bpvm_bmgr). */
#define BP_ENV_SECTOR     0x2000u        /* 8 KB (página de borrado U5) */

#if defined(BPVM_BOARD_DK2)
/* STM32U5G9J-DK2: 4 MB flash. Firmware 1.5 MB (holgura para LVGL). */
#define BP_ENV_A_OFFSET   0x00180000u
#define BP_ENV_B_OFFSET   0x00182000u
#define BP_PART_BASE      0x00184000u
#define BP_USABLE_FLASH   0x00400000u    /* 4 MB (fin de flash) */
#else
/* Nucleo-U575ZI-Q (default): 2 MB flash. Firmware 1 MB. */
#define BP_ENV_A_OFFSET   0x00100000u
#define BP_ENV_B_OFFSET   0x00102000u
#define BP_PART_BASE      0x00104000u
#define BP_USABLE_FLASH   0x00200000u    /* 2 MB (fin de flash) */
#endif

/* Log persistente (BP_LOG): 1 página (8 KB) JUSTO ANTES del env, en el hueco que
 * cede el firmware al encoger la región FLASH del .ld. SIN secciones → sobrevive
 * al reflasheo, como el env. Espejo del sector de log del Pico. Board-aware por
 * construcción (se deriva de BP_ENV_A_OFFSET). */
#define BP_LOG_SIZE       0x2000u                        /* 8 KB (1 página U5) */
#define BP_LOG_OFFSET     (BP_ENV_A_OFFSET - BP_LOG_SIZE)   /* desde FLASH_BASE */

/* Límites reales exportados por el .ld (direcciones absolutas de flash). */
#ifdef __cplusplus
extern "C" {
#endif
extern char __bp_flash_end[];    /* fin de la región FLASH del firmware (= inicio BP_LOG) */
extern char __bp_log_start[];    /* inicio del sector BP_LOG (justo antes del env) */
extern char __bp_part_start[];   /* inicio de la región BP_DATA (particiones) */
#ifdef __cplusplus
}
#endif

#endif /* FLASH_LAYOUT_STM32_H */
