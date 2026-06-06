/*
 * psram.c — H7.2.a: detección + init de PSRAM (APS6404 sobre QMI CS1) en RP2350.
 *
 * PORTADO de SparkFun sparkfun-pico/sfe_psram.c (licencia BSD-3-Clause,
 * Copyright (c) 2025 SparkFun Electronics). La secuencia de registros QMI y los
 * comandos APS6404 se conservan VERBATIM porque es código de timing crítico ya
 * verificado en hardware RP2350B; sólo se adaptan nombres, la guarda cs_pin<0 y
 * el logging. Ref: https://github.com/sparkfun/sparkfun-pico
 *
 * CRÍTICO: todo esto corre desde RAM (__no_inline_not_in_flash_func) con IRQs
 * deshabilitadas — el QMI direct-mode suspende el XIP, así que durante la
 * ventana no se puede leer flash (ni código ni datos).
 */
#include "psram.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "pico/platform.h"

/* --- Comandos APS6404 --- */
static const uint8_t PSRAM_CMD_QUAD_END    = 0xF5;
static const uint8_t PSRAM_CMD_QUAD_ENABLE = 0x35;
static const uint8_t PSRAM_CMD_READ_ID     = 0x9F;
static const uint8_t PSRAM_CMD_RSTEN       = 0x66;
static const uint8_t PSRAM_CMD_RST         = 0x99;
static const uint8_t PSRAM_CMD_QUAD_READ   = 0xEB;
static const uint8_t PSRAM_CMD_QUAD_WRITE  = 0x38;
static const uint8_t PSRAM_CMD_NOOP        = 0xFF;
static const uint8_t PSRAM_KGD             = 0x5D;  /* Known-Good-Die de APS6404 */

/* --- Timing (fs = femtosegundos) --- */
#define SFE_SEC_TO_FS              1000000000000000ll
static const uint32_t SFE_PSRAM_MAX_SELECT_FS64 = 125000000;
static const uint32_t SFE_PSRAM_MIN_DESELECT_FS = 50000000;
static const uint32_t SFE_PSRAM_MAX_SCK_HZ      = 109000000;

/* Sondeo: reset + read-ID. Devuelve el tamaño en bytes (0 si no responde). */
static size_t __no_inline_not_in_flash_func(psram_size_probe)(void)
{
    size_t psram_size = 0;
    uint32_t intr_stash = save_and_disable_interrupts();

    /* Modo directo del QMI, CLKDIV=30. */
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {}

    /* Salir de QPI por si la PSRAM quedó en quad (0xF5). */
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB |
                        PSRAM_CMD_QUAD_END;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {}
    (void) qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

    /* Read-ID (0x9F): 8 bytes; KGD en el byte 5, EID (densidad) en el 6. */
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    uint8_t kgd = 0;
    uint8_t eid = 0;
    for (size_t i = 0; i < 7; i++) {
        qmi_hw->direct_tx = (i == 0 ? PSRAM_CMD_READ_ID : PSRAM_CMD_NOOP);
        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0) {}
        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {}
        if (i == 5)      kgd = (uint8_t) qmi_hw->direct_rx;
        else if (i == 6) eid = (uint8_t) qmi_hw->direct_rx;
        else             (void) qmi_hw->direct_rx;
    }

    /* Cerrar direct-mode (restaura el XIP). */
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

    if (kgd == PSRAM_KGD) {
        psram_size = 1024 * 1024;                /* base 1 MB */
        uint8_t size_id = eid >> 5;
        if (eid == 0x26 || size_id == 2)      psram_size *= 8;   /* 8 MB (APS6404) */
        else if (size_id == 0)                psram_size *= 2;   /* 2 MB */
        else if (size_id == 1)                psram_size *= 4;   /* 4 MB */
    }

    restore_interrupts(intr_stash);
    return psram_size;
}

/* Configura el timing de la ventana M1 según clk_sys. */
static void __no_inline_not_in_flash_func(psram_set_timing)(void)
{
    uint32_t sysHz = (uint32_t) clock_get_hz(clk_sys);
    volatile uint8_t clockDivider = (sysHz + SFE_PSRAM_MAX_SCK_HZ - 1) / SFE_PSRAM_MAX_SCK_HZ;

    uint32_t intr_stash = save_and_disable_interrupts();
    uint32_t fsPerCycle = SFE_SEC_TO_FS / sysHz;
    volatile uint8_t maxSelect   = SFE_PSRAM_MAX_SELECT_FS64 / fsPerCycle;
    volatile uint8_t minDeselect = (SFE_PSRAM_MIN_DESELECT_FS + fsPerCycle - 1) / fsPerCycle;

    qmi_hw->m[1].timing = QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
                          3 << QMI_M1_TIMING_SELECT_HOLD_LSB |
                          1 << QMI_M1_TIMING_COOLDOWN_LSB |
                          1 << QMI_M1_TIMING_RXDELAY_LSB |
                          maxSelect << QMI_M1_TIMING_MAX_SELECT_LSB |
                          minDeselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
                          clockDivider << QMI_M1_TIMING_CLKDIV_LSB;

    restore_interrupts(intr_stash);
}

size_t psram_detect_init(int cs_pin)
{
    if (cs_pin < 0) return 0;

    /* Enrutar el pin del CS al QMI (segundo chip select). Guardamos la función
     * previa: si no hay PSRAM ahí, la restauramos para no dejar mal configurado
     * un pin que en otra placa sea un periférico real (p.ej. GP0=UART en Pico). */
    gpio_function_t prev_fn = gpio_get_function((uint) cs_pin);
    gpio_set_function((uint) cs_pin, GPIO_FUNC_XIP_CS1);

    size_t psram_size = psram_size_probe();
    if (psram_size == 0) {
        gpio_set_function((uint) cs_pin, prev_fn);   /* restaurar */
        return 0;                                    /* no hay PSRAM en ese CS */
    }

    /* Reset + entrar en QPI (0x66, 0x99, 0x35). */
    uint32_t intr_stash = save_and_disable_interrupts();
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {}

    for (uint8_t i = 0; i < 3; i++) {
        qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
        if (i == 0)      qmi_hw->direct_tx = PSRAM_CMD_RSTEN;
        else if (i == 1) qmi_hw->direct_tx = PSRAM_CMD_RST;
        else             qmi_hw->direct_tx = PSRAM_CMD_QUAD_ENABLE;
        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {}
        qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
        for (size_t j = 0; j < 20; j++) asm volatile("nop");
        (void) qmi_hw->direct_rx;
    }

    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
    restore_interrupts(intr_stash);

    psram_set_timing();

    /* Configurar la ventana M1 para read/write QPI (0xEB / 0x38) + XIP escribible. */
    intr_stash = save_and_disable_interrupts();
    qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q   << QMI_M1_RFMT_ADDR_WIDTH_LSB |
                         QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_RFMT_DUMMY_LEN_VALUE_24   << QMI_M1_RFMT_DUMMY_LEN_LSB |
                         QMI_M1_RFMT_DATA_WIDTH_VALUE_Q   << QMI_M1_RFMT_DATA_WIDTH_LSB |
                         QMI_M1_RFMT_PREFIX_LEN_VALUE_8   << QMI_M1_RFMT_PREFIX_LEN_LSB |
                         QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ << QMI_M1_RCMD_PREFIX_LSB |
                        0 << QMI_M1_RCMD_SUFFIX_LSB;

    qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q   << QMI_M1_WFMT_ADDR_WIDTH_LSB |
                         QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
                         QMI_M1_WFMT_DATA_WIDTH_VALUE_Q   << QMI_M1_WFMT_DATA_WIDTH_LSB |
                         QMI_M1_WFMT_PREFIX_LEN_VALUE_8   << QMI_M1_WFMT_PREFIX_LEN_LSB |
                         QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE << QMI_M1_WCMD_PREFIX_LSB |
                        0 << QMI_M1_WCMD_SUFFIX_LSB;

    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
    restore_interrupts(intr_stash);

    return psram_size;
}
