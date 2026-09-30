/*
 * pico_flash.c — la cintura de flash de la Pico. Ver pico_flash.h para el porqué.
 */
#include "pico_flash.h"

#include "flash_lock.h"      /* #153 — la ventana XIP-safe (single + dual-core) */
#include "flash_layout.h"    /* BP_PART_BASE y los límites de la zona 2 (bp_memmap.ld) */
#include "board_desc.h"      /* el tamaño de la flash, sondeado al arrancar */
#include "hardware/flash.h"  /* flash_range_*, FLASH_SECTOR_SIZE, FLASH_PAGE_SIZE, XIP_BASE */

#include <stddef.h>

/* ¿[off, off+len) cae ENTERO en un sitio que se puede escribir? La zona 2 la da el
 * enlazador (entre FLASH_BOOT y FLASH_MAIN: el ENV A/B y el log); las particiones, de
 * BP_PART_BASE al final de la flash. El tamaño lo sabe board_desc desde el arranque; antes
 * de eso (0) sólo se escribe el log, que va en la zona 2, así que no se mira. */
static int zona_valida(uint32_t off, uint32_t len) {
    uint32_t fin = off + len;
    if (fin < off) return 0;                                              /* da la vuelta */
    uint32_t z2_ini = (uint32_t) ((uintptr_t) __bp_zone1_end   - XIP_BASE);
    uint32_t z2_fin = (uint32_t) ((uintptr_t) __bp_zone3_start - XIP_BASE);
    if (off >= z2_ini && fin <= z2_fin) return 1;                         /* el ENV y el log */
    unsigned total = board_desc()->flash_bytes;
    if (off >= BP_PART_BASE && (total == 0 || fin <= total)) return 1;     /* las particiones */
    return 0;                                                             /* la imagen, o fuera */
}

int pico_flash_erase(uint32_t off, uint32_t len) {
    if (len == 0) return 0;
    if (off % FLASH_SECTOR_SIZE != 0 || len % FLASH_SECTOR_SIZE != 0) return -1;
    if (!zona_valida(off, len)) return -1;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_erase(off, len);
    bpvm_flash_lock_end(tok);
    return 0;
}

int pico_flash_write(uint32_t off, const uint8_t* src, uint32_t len) {
    if (len == 0) return 0;
    if (src == NULL) return -1;
    if (off % FLASH_PAGE_SIZE != 0 || len % FLASH_PAGE_SIZE != 0) return -1;
    if (!zona_valida(off, len)) return -1;
    uint32_t tok = bpvm_flash_lock_begin();
    flash_range_program(off, src, len);
    bpvm_flash_lock_end(tok);
    return 0;
}
