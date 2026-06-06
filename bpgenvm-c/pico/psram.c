/*
 * psram.c — H7.2.a: DETECCIÓN de PSRAM (APS6404 sobre QMI CS1) en RP2350.
 *
 * Secuencia QMI/APS6404 basada en la referencia BSD-3 de SparkFun (sparkfun-pico
 * sfe_psram.c). Aquí SÓLO sondeamos (reset + read-ID) para reconocer presencia y
 * tamaño — NO reconfiguramos la ventana M1 ni pasamos a QPI (eso, que es para
 * USAR la PSRAM como memoria, va en H7.2.b). Así el sondeo no deja estado
 * persistente en el QMI: entra en direct-mode, lee el ID, sale, restaura el XIP.
 *
 * CRÍTICO: corre desde RAM (__no_inline_not_in_flash_func) con IRQs off — el
 * direct-mode suspende el XIP, no se puede tocar flash en esa ventana. TODAS las
 * esperas son ACOTADAS (timeout): si el QMI no responde, abortamos y arrancamos
 * igual (nunca colgamos el boot). Constantes en #define (nunca .rodata de flash).
 */
#include "psram.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/qmi.h"
#include "pico/platform.h"

#include <stdint.h>

/* Comandos APS6404 (defines → inmediatos, jamás lectura de flash) */
#define PSRAM_CMD_QUAD_END  0xF5u
#define PSRAM_CMD_READ_ID   0x9Fu
#define PSRAM_CMD_NOOP      0xFFu
#define PSRAM_KGD           0x5Du   /* Known-Good-Die de APS6404 */

/* Tope de espera por bucle. A 150 MHz ~0.7 ms; con margen sobra para cualquier
 * transferencia QMI real. Si se excede → no hay PSRAM (o el QMI no responde). */
#define QMI_SPIN_MAX 200000u

/* Sondeo: reset-quad + read-ID. Devuelve tamaño en bytes (0 si no responde). */
static size_t __no_inline_not_in_flash_func(psram_probe_size)(void)
{
    size_t   psram_size = 0;
    uint8_t  kgd = 0, eid = 0;
    uint32_t spin;
    uint32_t intr_stash = save_and_disable_interrupts();

    /* Direct-mode del QMI, CLKDIV=30. */
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
        psram_size = 1024u * 1024u;                 /* base 1 MB */
        uint8_t size_id = eid >> 5;
        if      (eid == 0x26u || size_id == 2u) psram_size *= 8u;  /* 8 MB (APS6404) */
        else if (size_id == 0u)                 psram_size *= 2u;  /* 2 MB */
        else if (size_id == 1u)                 psram_size *= 4u;  /* 4 MB */
    }

bail:
    /* Cerrar direct-mode SIEMPRE (restaura el XIP) y reactivar IRQs. */
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);
    restore_interrupts(intr_stash);
    return psram_size;
}

size_t psram_detect_init(int cs_pin)
{
    if (cs_pin < 0) return 0;

    /* Enrutar el pin del CS al QMI (segundo chip select). Guardamos la función
     * previa: si no hay PSRAM ahí, la restauramos para no dejar mal configurado
     * un pin que en otra placa sea un periférico real (p.ej. GP0=UART en Pico). */
    gpio_function_t prev_fn = gpio_get_function((uint) cs_pin);
    gpio_set_function((uint) cs_pin, GPIO_FUNC_XIP_CS1);

    size_t sz = psram_probe_size();
    if (sz == 0) {
        gpio_set_function((uint) cs_pin, prev_fn);   /* restaurar */
    }
    return sz;
}
