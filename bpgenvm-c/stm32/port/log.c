/*
 * log.c (STM32) — CINTURA del núcleo portable bpvm_log (src/bpvm_log.c). Solo lo
 * NO portable del STM32/U5: timestamp (HAL_GetTick) + leer/escribir el sector
 * BP_LOG (stm32_flash, compartido con FS/env). La lógica (buffer, formato,
 * snapshot, dump) la pone el núcleo → mismo comportamiento que Pico/ESP32.
 *
 * El buffer RAM (s_region) ES la imagen de flash del log → sin flash_buf aparte
 * (la versión previa gastaba s_buf 8K + flash_buf 8K; ésta solo 8K).
 */
#include "log.h"

#include "bpvm_log.h"
#include "flash_layout_stm32.h"   /* BP_LOG_OFFSET / BP_LOG_SIZE + __bp_log_start */
#include "stm32_flash.h"          /* stm32_flash_write / erase (compartidas) */
#include "main.h"                 /* HAL_GetTick + FLASH_BASE (CMSIS) */

#include <stdint.h>
#include <string.h>

/* La región del log EN RAM (= imagen de flash: header + data). aligned(8). */
/* #439 — la región va a `.noinit`, que el startup NO borra (sólo limpia de
 * _sbss a _ebss): el log sobrevive al reset y la autopsia ve las líneas de
 * ANTES del cuelgue. Gemelo del `__uninitialized_ram` de la Pico y del
 * `__NOINIT_ATTR` del ESP32; el núcleo común hace el resto.
 * La sección la declara el linker script del proyecto — y OJO, ese .ld lo
 * genera CubeIDE (ver el comentario que hay allí). */
static uint8_t s_region[BP_LOG_SIZE]
    __attribute__((aligned(8), section(".noinit")));

static uint32_t now_ms(void) { return HAL_GetTick(); }

/* Red anti-divergencia (misma filosofía que board_mgr): si el .ld y
 * flash_layout_stm32.h no cuadran, NO tocamos flash (podríamos leer/escribir una
 * dirección equivocada = corromper) → devolvemos error y el log queda solo-RAM. */
static int layout_ok(void) {
    return (uintptr_t) __bp_log_start == (uintptr_t) (FLASH_BASE + BP_LOG_OFFSET);
}

static int flash_read(uint8_t* dst, uint32_t len) {
    if (!layout_ok()) return -1;
    memcpy(dst, (const void*) (uintptr_t) (FLASH_BASE + BP_LOG_OFFSET), len);
    return 0;
}

static int flash_write(const uint8_t* src, uint32_t len) {
    if (!layout_ok()) return -1;
    uint32_t addr = FLASH_BASE + BP_LOG_OFFSET;
    if (stm32_flash_erase(addr, 1u) != 0) return -1;      /* 1 página de 8 KB */
    return stm32_flash_write(addr, src, len);
}

void log_init(void) {
    bpvm_log_cintura_t c;
    c.now_ms      = now_ms;
    c.flash_read  = flash_read;
    c.flash_write = flash_write;
    c.region_buf  = s_region;
    c.region_size = BP_LOG_SIZE;
    bpvm_log_init(&c);
}
