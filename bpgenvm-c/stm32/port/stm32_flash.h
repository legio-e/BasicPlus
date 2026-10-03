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

/* V7/H10, el board_mgr común (D4): el CANDADO de la flash —el FS desde la VM, el gestor desde el REPL y el log pueden
 * escribir a la vez—. Se crea en el arranque, antes de que corra nadie más; hasta entonces (y en un fallo grave, que
 * vuelca el log desde su manejador sin poder esperar) se escribe sin él. */
void stm32_flash_init(void);

/* Programa `len` bytes (múltiplo de 16, alineado a 16) en `addr`. Rellena el
 * último quadword con 0xFF si `len` no es múltiplo exacto. 0 OK, -1 error.
 * #637: -1 sin tocar nada si el tramo no cae entero en la zona escribible —del log
 * al final de la flash— o si el `.ld` y flash_layout_stm32.h no casan. */
int stm32_flash_write(uint32_t addr, const uint8_t* data, uint32_t len);

/* Borra `npages` páginas de 8 KB desde `addr` (alineado a página). Un trozo que
 * cruce del banco 1 al 2 se parte y cada tramo va a su banco (V7/H6 G5.b0). Fuera
 * de la flash, -1 sin borrar ese tramo. 0 OK, -1 error. #637: la misma guarda que
 * la escritura, para el tramo entero. */
int stm32_flash_erase(uint32_t addr, uint32_t npages);

#ifdef __cplusplus
}
#endif

#endif /* STM32_FLASH_H */
