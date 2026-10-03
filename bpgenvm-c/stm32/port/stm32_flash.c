/*
 * stm32_flash.c — primitivas de flash interna del U5 (ver stm32_flash.h).
 * Reunidas aquí desde stm32_fs.c para que fs_lfs_stm32 y board_mgr_stm32 escriban
 * la flash por el MISMO camino (un solo sitio, sin divergencia).
 */
#include "stm32_flash.h"

#include "main.h"     /* HAL FLASH/ICACHE + CMSIS (FLASH_BASE, FLASH_PAGE_SIZE, FLASH->OPTR, __get_IPSR) */
#include "flash_layout_stm32.h"   /* #637: el log, el ENV y las particiones, y los símbolos del .ld */
#include "bpvm_flash_zona.h"      /* #637: la regla, común y probada en el PC (make test-flash) */
#include "bpvm_platform.h"        /* D4: el mutex */
#include "log.h"
#include <string.h>

/* ── V7/H10, el board_mgr común (3-oct) ──────────────────────────────────────────────────────────────────────────
 *
 * #637 — LA GUARDA, DENTRO DE LA CINTURA. Hasta hoy la primitiva borraba y programaba cualquier página que existiera,
 * y la comprobación del `.ld` contra flash_layout_stm32.h sólo estaba en el `handle` del gestor: el ENV que escribe el
 * ARRANQUE (la `imagen.fecha` de #626), el log y el FS no la pasaban. Si el `.ld` y la cabecera dejaban de casar, ese
 * arranque borraba la página que cayera —firmware incluido; se recupera por SWD—. Ahora todo lo que escribe pasa por
 * aquí con la misma regla: sólo de BP_LOG al final de la flash (el log, el ENV A/B y las particiones), y nada si el
 * layout no casa. Como la de la Pico (pico_flash.c). */
static int layout_casa(void) {
    static int s_casa = -1;   /* -1 sin mirar; 1 casan; 0 no */
    if (s_casa < 0) {
        s_casa = bpvm_flash_layout_casa((uintptr_t) __bp_log_start, (uintptr_t) __bp_part_start,
                                        (uintptr_t) __bp_flash_end,
                                        (uintptr_t) (FLASH_BASE + BP_LOG_OFFSET), (uintptr_t) (FLASH_BASE + BP_PART_BASE));
        if (!s_casa)
            log_printf("flash: el .ld y flash_layout_stm32.h NO casan (log %p/%p, particiones %p/%p, fin del firmware %p):"
                       " no se escribe nada", (void*) __bp_log_start, (void*) (FLASH_BASE + BP_LOG_OFFSET),
                       (void*) __bp_part_start, (void*) (FLASH_BASE + BP_PART_BASE), (void*) __bp_flash_end);
    }
    return s_casa;
}

static int zona_ok(uint32_t addr, uint32_t len) {
    const bpvm_flash_zona_t zona[] = { { FLASH_BASE + BP_LOG_OFFSET, FLASH_BASE + BP_USABLE_FLASH } };
    if (!layout_casa()) return 0;
    if (bpvm_flash_zona_ok(addr, len, zona, 1)) return 1;
    log_printf("flash: 0x%08lX (+%lu B) cae fuera de la zona que se puede escribir (0x%08lX..0x%08lX): no se escribe",
               (unsigned long) addr, (unsigned long) len, (unsigned long) zona[0].ini, (unsigned long) zona[0].fin);
    return 0;
}

/* D4 — EL CANDADO. El FS desde la VM, el gestor desde el REPL y el log pueden escribir a la vez, y el HAL no se deja
 * compartir (unlock/program/lock y la ICACHE). Se crea en el arranque (stm32_flash_init), antes de que corra nadie
 * más; hasta entonces el mutex no existe y lock/unlock no hacen nada. En un fallo grave el log se vuelca DESDE el
 * manejador (IPSR != 0): ahí no se puede esperar, y se escribe sin él. */
static bpvm_platform_mutex_handle_t s_mtx = NULL;

void stm32_flash_init(void) {
    if (s_mtx == NULL) (void) bpvm_platform_mutex_init(&s_mtx);
}

static void candado(void)    { if (__get_IPSR() == 0) bpvm_platform_mutex_lock(&s_mtx); }
static void descandado(void) { if (__get_IPSR() == 0) bpvm_platform_mutex_unlock(&s_mtx); }

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
    if (!zona_ok(addr, len)) return -1;   /* #637 */
    candado();                            /* D4 */
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
    descandado();
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
    if (npages > (uint32_t) (FLASH_SIZE / FLASH_PAGE_SIZE)
        || !zona_ok(addr, npages * (uint32_t) FLASH_PAGE_SIZE)) return -1;   /* #637: el tramo entero */
    candado();                                                               /* D4 */
    int r = 0;
    /* Páginas de un banco: en doble banco, las de la mitad de la flash; en banco
     * único (lo que `addr_to_bank_page` trata como banco 1), las de toda. */
    const uint32_t por_banco = (FLASH->OPTR & FLASH_OPTR_DUALBANK)
                             ? (uint32_t) FLASH_PAGE_NB
                             : (uint32_t) (FLASH_SIZE / FLASH_PAGE_SIZE);
    while (npages > 0) {
        uint32_t bank, page;
        addr_to_bank_page(addr, &bank, &page);
        if (page >= por_banco) { r = -1; break; }  /* fuera de la flash: no se borra nada */
        uint32_t n = por_banco - page;             /* lo que queda en ESTE banco */
        if (n > npages) n = npages;
        if (erase_en_banco(bank, page, n) != 0) { r = -1; break; }
        addr   += n * FLASH_PAGE_SIZE;
        npages -= n;
    }
    descandado();
    return r;
}
