/*
 * stm32_flash.h — primitivas de flash interna del U5, COMPARTIDAS por la cintura
 * del FS (fs_lfs_stm32.c, block device) y el gestor de placa (board_mgr_stm32.c,
 * env A/B). Un solo sitio para escribir la flash → sin riesgo de que dos copias
 * diverjan y corrompan.
 *
 * El U5 programa por QUADWORD (128 bits = 16 B) y borra por PÁGINA (8 KB),
 * consciente de dual-bank. Estas funciones envuelven unlock/lock + ICACHE
 * (disable/invalidate) alrededor. Direcciones ABSOLUTAS (desde FLASH_BASE).
 */
#ifndef STM32_FLASH_H
#define STM32_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Programa `len` bytes (múltiplo de 16, alineado a 16) en `addr`. Rellena el
 * último quadword con 0xFF si `len` no es múltiplo exacto. 0 OK, -1 error. */
int stm32_flash_write(uint32_t addr, const uint8_t* data, uint32_t len);

/* Borra `npages` páginas de 8 KB desde `addr` (alineado a página). 0 OK, -1 error. */
int stm32_flash_erase(uint32_t addr, uint32_t npages);

#ifdef __cplusplus
}
#endif

#endif /* STM32_FLASH_H */
