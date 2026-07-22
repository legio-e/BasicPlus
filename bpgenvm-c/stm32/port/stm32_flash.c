/*
 * stm32_flash.c — primitivas de flash interna del U5 (ver stm32_flash.h).
 * Reunidas aquí desde stm32_fs.c para que fs_lfs_stm32 y board_mgr_stm32 escriban
 * la flash por el MISMO camino (un solo sitio, sin divergencia).
 */
#include "stm32_flash.h"

#include "main.h"     /* HAL FLASH/ICACHE + CMSIS (FLASH_BASE, FLASH_PAGE_SIZE, FLASH->OPTR) */
#include <string.h>

/* Traduce una dirección de flash a (banco, página) según el modo (dual/single). */
static void addr_to_bank_page(uint32_t addr, uint32_t* bank, uint32_t* page) {
    uint32_t off = addr - FLASH_BASE;
    if ((FLASH->OPTR & FLASH_OPTR_DUALBANK) && off >= FLASH_BANK_SIZE) {
        *bank = FLASH_BANK_2;
        *page = (off - FLASH_BANK_SIZE) / FLASH_PAGE_SIZE;
    } else {
        *bank = FLASH_BANK_1;
        *page = off / FLASH_PAGE_SIZE;
    }
}

int stm32_flash_write(uint32_t addr, const uint8_t* data, uint32_t len) {
    HAL_ICACHE_Disable();
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef st = HAL_OK;
    for (uint32_t o = 0; o < len && st == HAL_OK; o += 16u) {
        uint32_t qw[4];
        memset(qw, 0xFF, sizeof qw);
        uint32_t n = len - o; if (n > 16u) n = 16u;
        memcpy(qw, data + o, n);
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr + o, (uint32_t) (uintptr_t) qw);
    }
    HAL_FLASH_Lock();
    HAL_ICACHE_Invalidate();
    HAL_ICACHE_Enable();
    return (st == HAL_OK) ? 0 : -1;
}

int stm32_flash_erase(uint32_t addr, uint32_t npages) {
    uint32_t bank, page;
    addr_to_bank_page(addr, &bank, &page);
    FLASH_EraseInitTypeDef er = {0};
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.Banks     = bank;
    er.Page      = page;
    er.NbPages   = npages;
    HAL_ICACHE_Disable();
    HAL_FLASH_Unlock();
    uint32_t pe = 0;
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&er, &pe);
    HAL_FLASH_Lock();
    HAL_ICACHE_Invalidate();
    HAL_ICACHE_Enable();
    return (st == HAL_OK) ? 0 : -1;
}
