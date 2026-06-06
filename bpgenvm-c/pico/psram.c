/*
 * psram.c — H7.2.a/b: detección + habilitación de PSRAM (APS6404 sobre QMI CS1).
 *
 * Secuencia QMI/APS6404 PORTADA de SparkFun sparkfun-pico/sfe_psram.c
 * (BSD-3-Clause, (c) 2025 SparkFun Electronics). Es código de timing crítico ya
 * verificado en HW RP2350B; sólo se adaptan nombres, se añaden TIMEOUTS en todas
 * las esperas (para no colgar el boot jamás) y las constantes van en #define
 * (cero lectura de .rodata/flash con el XIP suspendido).
 *
 * CRÍTICO: el direct-mode del QMI suspende el XIP, así que todo lo que toca el
 * QMI corre desde RAM (__no_inline_not_in_flash_func) con IRQs off y SIEMPRE
 * restaura el XIP antes de volver.
 *
 *   psram_detect_init() — H7.2.a: sólo sondea (reset + read-ID) y reporta tamaño.
 *   psram_enable_xip()  — H7.2.b: pasa a QPI + mapea la ventana M1 escribible en
 *                          0x11000000 (para USAR la PSRAM como memoria).
 *   psram_rw_selftest() — H7.2.b: valida el camino de datos antes de confiarle el
 *                          heap (write/read de patrones repartidos).
 */
#include "psram.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "pico/platform.h"

#include <stdint.h>

/* --- Comandos APS6404 (defines → inmediatos, jamás lectura de flash) --- */
#define PSRAM_CMD_QUAD_END    0xF5u
#define PSRAM_CMD_QUAD_ENABLE 0x35u
#define PSRAM_CMD_READ_ID     0x9Fu
#define PSRAM_CMD_RSTEN       0x66u
#define PSRAM_CMD_RST         0x99u
#define PSRAM_CMD_QUAD_READ   0xEBu
#define PSRAM_CMD_QUAD_WRITE  0x38u
#define PSRAM_CMD_NOOP        0xFFu
#define PSRAM_KGD             0x5Du   /* Known-Good-Die de APS6404 */

/* --- Timing (fs = femtosegundos) --- */
#define SFE_SEC_TO_FS              1000000000000000ll
#define SFE_PSRAM_MAX_SELECT_FS64  125000000u
#define SFE_PSRAM_MIN_DESELECT_FS  50000000u
#define SFE_PSRAM_MAX_SCK_HZ       109000000u

/* Tope de espera por bucle. A 150 MHz ~1.3 ms; sobra para cualquier
 * transferencia QMI real. Si se excede → abortamos (sin colgar). */
#define QMI_SPIN_MAX 200000u

/* ====================== H7.2.a: detección (sondeo) ====================== */

static size_t __no_inline_not_in_flash_func(psram_probe_size)(void)
{
    size_t   psram_size = 0;
    uint8_t  kgd = 0, eid = 0;
    uint32_t spin;
    uint32_t intr_stash = save_and_disable_interrupts();

    qmi_hw->direct_csr = 30u << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
    spin = 0; while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) if (++spin > QMI_SPIN_MAX) goto bail;

    /* Salir de QPI por si quedó en quad (0xF5). */
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) |
                        PSRAM_CMD_QUAD_END;
    spin = 0; while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) if (++spin > QMI_SPIN_MAX) goto bail;
    (void) qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;

    /* Read-ID (0x9F): KGD en el byte 5, EID (densidad) en el 6. */
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    for (uint32_t i = 0; i < 7u; i++) {
        qmi_hw->direct_tx = (i == 0u) ? PSRAM_CMD_READ_ID : PSRAM_CMD_NOOP;
        spin = 0; while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS)) if (++spin > QMI_SPIN_MAX) goto bail;
        spin = 0; while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)       if (++spin > QMI_SPIN_MAX) goto bail;
        uint8_t rx = (uint8_t) qmi_hw->direct_rx;
        if      (i == 5u) kgd = rx;
        else if (i == 6u) eid = rx;
    }

    if (kgd == PSRAM_KGD) {
        psram_size = 1024u * 1024u;
        uint8_t size_id = eid >> 5;
        if      (eid == 0x26u || size_id == 2u) psram_size *= 8u;  /* 8 MB (APS6404) */
        else if (size_id == 0u)                 psram_size *= 2u;
        else if (size_id == 1u)                 psram_size *= 4u;
    }

bail:
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
    restore_interrupts(intr_stash);
    return psram_size;
}

size_t psram_detect_init(int cs_pin)
{
    if (cs_pin < 0) return 0;
    gpio_function_t prev_fn = gpio_get_function((uint) cs_pin);
    gpio_set_function((uint) cs_pin, GPIO_FUNC_XIP_CS1);
    size_t sz = psram_probe_size();
    if (sz == 0) gpio_set_function((uint) cs_pin, prev_fn);  /* restaurar pin */
    return sz;
}

/* ============== H7.2.b: habilitar QPI + mapear ventana M1 =============== */

static void __no_inline_not_in_flash_func(psram_set_timing)(void)
{
    uint32_t sysHz = (uint32_t) clock_get_hz(clk_sys);   /* XIP aún activo aquí */
    uint8_t  clockDivider = (sysHz + SFE_PSRAM_MAX_SCK_HZ - 1) / SFE_PSRAM_MAX_SCK_HZ;

    uint32_t intr_stash = save_and_disable_interrupts();
    uint32_t fsPerCycle  = SFE_SEC_TO_FS / sysHz;
    uint8_t  maxSelect   = SFE_PSRAM_MAX_SELECT_FS64 / fsPerCycle;
    uint8_t  minDeselect = (SFE_PSRAM_MIN_DESELECT_FS + fsPerCycle - 1) / fsPerCycle;

    qmi_hw->m[1].timing = QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
                          3u << QMI_M1_TIMING_SELECT_HOLD_LSB |
                          1u << QMI_M1_TIMING_COOLDOWN_LSB |
                          1u << QMI_M1_TIMING_RXDELAY_LSB |
                          maxSelect << QMI_M1_TIMING_MAX_SELECT_LSB |
                          minDeselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
                          clockDivider << QMI_M1_TIMING_CLKDIV_LSB;
    restore_interrupts(intr_stash);
}

int __no_inline_not_in_flash_func(psram_enable_xip)(void)
{
    uint32_t spin;
    uint32_t intr_stash = save_and_disable_interrupts();

    qmi_hw->direct_csr = 30u << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
    spin = 0; while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
                if (++spin > QMI_SPIN_MAX) { qmi_hw->direct_csr = 0; restore_interrupts(intr_stash); return 0; }

    /* Reset (0x66, 0x99) + entrar en QPI (0x35). */
    for (uint8_t i = 0; i < 3u; i++) {
        qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
        qmi_hw->direct_tx = (i == 0u) ? PSRAM_CMD_RSTEN
                          : (i == 1u) ? PSRAM_CMD_RST
                                      : PSRAM_CMD_QUAD_ENABLE;
        spin = 0; while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
                    if (++spin > QMI_SPIN_MAX) {
                        qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
                        restore_interrupts(intr_stash); return 0;
                    }
        qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
        for (size_t j = 0; j < 20u; j++) asm volatile("nop");
        (void) qmi_hw->direct_rx;
    }

    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
    restore_interrupts(intr_stash);

    psram_set_timing();

    /* Ventana M1: read 0xEB / write 0x38 en QPI + XIP escribible → 0x11000000. */
    intr_stash = save_and_disable_interrupts();
    qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q   << QMI_M1_RFMT_ADDR_WIDTH_LSB |
                         QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_RFMT_DUMMY_LEN_VALUE_24   << QMI_M1_RFMT_DUMMY_LEN_LSB |
                         QMI_M1_RFMT_DATA_WIDTH_VALUE_Q   << QMI_M1_RFMT_DATA_WIDTH_LSB |
                         QMI_M1_RFMT_PREFIX_LEN_VALUE_8   << QMI_M1_RFMT_PREFIX_LEN_LSB |
                         QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ << QMI_M1_RCMD_PREFIX_LSB | 0u << QMI_M1_RCMD_SUFFIX_LSB;
    qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q   << QMI_M1_WFMT_ADDR_WIDTH_LSB |
                         QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
                         QMI_M1_WFMT_DATA_WIDTH_VALUE_Q   << QMI_M1_WFMT_DATA_WIDTH_LSB |
                         QMI_M1_WFMT_PREFIX_LEN_VALUE_8   << QMI_M1_WFMT_PREFIX_LEN_LSB |
                         QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE << QMI_M1_WCMD_PREFIX_LSB | 0u << QMI_M1_WCMD_SUFFIX_LSB;
    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
    restore_interrupts(intr_stash);
    return 1;
}

int psram_rw_selftest(size_t size)
{
    if (size < 16u) return 0;
    volatile uint32_t* base = (volatile uint32_t*) (uintptr_t) PSRAM_XIP_BASE;
    size_t words = size / 4u;
    /* 4 offsets MUY separados (≥1 MB) → distintas líneas de caché, así la
     * lectura no la sirve siempre la caché y se ejercita la PSRAM de verdad. */
    size_t offs[4] = { 0u, words / 4u, words / 2u, words - 1u };
    uint32_t pats[4] = { 0xA5A5A5A5u, 0x5A5A5A5Au, 0xDEADBEEFu, 0x12345678u };
    for (int i = 0; i < 4; i++) base[offs[i]] = pats[i];
    for (int i = 0; i < 4; i++) if (base[offs[i]] != pats[i]) return 0;
    return 1;
}
