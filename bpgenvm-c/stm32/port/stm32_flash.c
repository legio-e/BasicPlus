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

/* Borra `npages` páginas seguidas desde `addr`, PARTIENDO POR BANCO.
 *
 * V7/H6 G5.b0 (26-sep) — hasta hoy se sacaba el banco de la PRIMERA página y se
 * pedían N seguidas en ese banco. Si el trozo cruzaba del banco 1 al 2 (en la DK2
 * la frontera es 0x08200000, y la zona de datos empieza en el banco 1), funcionaba
 * DE CHIRIPA: `FLASH_PageErase` del HAL escribe `Page << PNB_Pos` sin recortarlo,
 * y la página 256 desborda el campo PNB (8 bits) justo sobre el bit BKER, que es el
 * que elige el banco. Ahora cada tramo va a su banco con su número de página, por
 * contrato y no por la posición de un bit. */
static int erase_en_banco(uint32_t bank, uint32_t page, uint32_t npages) {
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

int stm32_flash_erase(uint32_t addr, uint32_t npages) {
    /* Páginas de un banco: en doble banco, las de la mitad de la flash; en banco
     * único (lo que `addr_to_bank_page` trata como banco 1), las de toda. */
    const uint32_t por_banco = (FLASH->OPTR & FLASH_OPTR_DUALBANK)
                             ? (uint32_t) FLASH_PAGE_NB
                             : (uint32_t) (FLASH_SIZE / FLASH_PAGE_SIZE);
    while (npages > 0) {
        uint32_t bank, page;
        addr_to_bank_page(addr, &bank, &page);
        if (page >= por_banco) return -1;          /* fuera de la flash: no se borra nada */
        uint32_t n = por_banco - page;             /* lo que queda en ESTE banco */
        if (n > npages) n = npages;
        if (erase_en_banco(bank, page, n) != 0) return -1;
        addr   += n * FLASH_PAGE_SIZE;
        npages -= n;
    }
    return 0;
}
