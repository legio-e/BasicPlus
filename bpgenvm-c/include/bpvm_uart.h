/*
 * bpvm_uart.h — hooks de plataforma para los builtins Uart.*
 *
 * Mismo patrón que bpvm_i2c.h / bpvm_spi.h. Por defecto stubs con
 * logging por stdout (útil en host para desarrollo sin HW). En el
 * firmware Pico, main.c registra un backend cuyas funciones llaman
 * a uart_init / uart_set_format / uart_write_blocking /
 * uart_read_blocking del SDK, además de configurar los GPIOs TX/RX
 * con gpio_set_function(GPIO_FUNC_UART).
 *
 * Notas del API:
 *  - `read` recibe un timeoutMs:
 *      <= 0 → bloquea hasta tener `n` bytes
 *      > 0  → devuelve cuando lleva `n` bytes O cuando expira el
 *             timeout. El retorno es bytes realmente leídos (0..n).
 *  - `available` devuelve cuántos bytes hay en el buffer RX listos
 *    para leer sin bloquear (-1 si el backend no lo soporta).
 */
#ifndef BPVM_UART_H
#define BPVM_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*init)(int bus, int tx, int rx, int baudrate,
                 int data_bits, int stop_bits, int parity);
    /* Devuelve bytes escritos / leídos, o -1 en error. */
    int  (*write)(int bus, const uint8_t* data, size_t n);
    int  (*read)(int bus, uint8_t* data, size_t n, int timeout_ms);
    int  (*available)(int bus);
} bpvm_uart_backend_t;

void bpvm_uart_set_backend(const bpvm_uart_backend_t* backend);

/* ─── V6/#473 — el buffer de RX que la fachada garantiza ─────────────────────
 *
 * «Las UARTs a veces tienen un pequeno buffer que siempre se queda corto, asi que
 * hay que anadir un buffer externo que lo amplie. Eso es necesario yo diria que
 * SIEMPRE» (Eduardo, 10-sep). Asi que el contrato promete un buffer de
 * BPVM_UART_RX_CAP bytes, y `available` cuenta LO QUE HAY EN EL.
 *
 * Como se llena es de la cintura: quien tenga driver con buffer propio (el ESP32)
 * no hace nada; quien lea el registro a pelo (Pico, STM32) enciende su IRQ de RX y
 * empuja cada byte. En V7, DMA donde la familia lo tenga — con DMA los bytes
 * entran sin CPU y sin despertar al micro por cada uno (ver #490).
 *
 * DOS llamadas y ni un verbo nuevo en BP. */

/** La familia se apunta: a partir de aqui `available`/`read` salen del anillo. */
void bpvm_uart_rx_ring_enable(int bus);

/** Desde la ISR de RX de la familia, un byte por llamada. Sin cerrojos: un
 *  productor y un consumidor. Si el anillo esta lleno CUENTA el descarte y lo dice
 *  en el siguiente `read` — un byte perdido en silencio cuesta una tarde. */
void bpvm_uart_rx_push(int bus, uint8_t b);

/* Funciones efectivas. Si backend NULL → stub con logging. */
void bpvm_uart_init(int bus, int tx, int rx, int baudrate,
                    int data_bits, int stop_bits, int parity);
int  bpvm_uart_write(int bus, const uint8_t* data, size_t n);
int  bpvm_uart_read(int bus, uint8_t* data, size_t n, int timeout_ms);
int  bpvm_uart_available(int bus);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_UART_H */
