/*
 * flash_layout.h — H9: el mapa de flash del RP2350 en UN sitio (lado C).
 *
 * Layout de 3 zonas (docs/H9_KERNEL_CAPAS.md §Layout de flash en 3 ZONAS;
 * decidido con Eduardo 19-jul). La AUTORIDAD de los limites de zona es el
 * linker script bp_memmap.ld (regiones FLASH_BOOT/FLASH_MAIN); este header
 * define lo que el codigo C necesita: los offsets DENTRO del hueco (zona 2),
 * que el linker no conoce a proposito (ahi no hay secciones — por eso el UF2
 * no los graba y el env sobrevive al reflasheo).
 *
 *   0x000000-0x00FFFF  ZONA 1 arranque (64K FIJOS, region FLASH_BOOT)
 *   0x010000-0x010FFF  ZONA 2: env A (4K)          ┐ hueco del UF2:
 *   0x011000-0x011FFF          env B (4K)          │ NUNCA se graba
 *   0x012000-0x013FFF          reserva kernel (8K) ┘ (modo-seguro, futuro)
 *   0x014000-0x1FFFFF  ZONA 3 firmware (region FLASH_MAIN)
 *   0x200000-...       FS littlefs (fs_lfs_pico.c) + arriba: log 0x3FC000 +
 *                      bp_ptable_t 0x3FF000 (se quedan donde estan de momento)
 *
 * Red anti-divergencia: board_mgr_pico.c comprueba en runtime que estos
 * offsets caen dentro del hueco real usando __bp_zone1_end / __bp_zone3_start
 * (simbolos exportados por bp_memmap.ld). Si alguien mueve las regiones del
 * linker sin tocar esto (o al reves), las escrituras de env se RECHAZAN con
 * error claro en vez de borrar codigo en silencio.
 */
#ifndef BP_FLASH_LAYOUT_H
#define BP_FLASH_LAYOUT_H

/* Zona 2 — bloque de env A/B (offsets de flash, relativos a XIP_BASE). */
#define BP_ENV_A_OFFSET       0x00010000u
#define BP_ENV_B_OFFSET       0x00011000u
/* 0x012000-0x013FFF: reserva del kernel (flag modo-seguro, futuro). */

/* Limites de zona reales, exportados por bp_memmap.ld (direcciones XIP). */
#ifdef __cplusplus
extern "C" {
#endif
extern char __bp_zone1_end[];    /* fin de FLASH_BOOT  = XIP_BASE + 0x10000 */
extern char __bp_zone3_start[];  /* inicio FLASH_MAIN  = XIP_BASE + 0x14000 */
#ifdef __cplusplus
}
#endif

#endif /* BP_FLASH_LAYOUT_H */
