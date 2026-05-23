/*
 * bpvm_spi.h — hooks de plataforma para los builtins Spi.*
 *
 * Mismo patrón que bpvm_i2c.h. Por defecto stubs con logging por
 * stdout. En el firmware Pico, main.c registra un backend cuyas
 * funciones llaman a spi_init / spi_set_format / spi_write_blocking
 * / spi_read_blocking / spi_write_read_blocking del SDK, además de
 * configurar los GPIOs SCK/MOSI/MISO con gpio_set_function.
 */
#ifndef BPVM_SPI_H
#define BPVM_SPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*init)(int bus, int sck, int mosi, int miso, int baudrate, int mode);
    /* Devuelve bytes intercambiados, o -1 en error. */
    int  (*write)(int bus, const uint8_t* data, size_t n);
    int  (*read)(int bus, uint8_t* data, size_t n);
    int  (*transfer)(int bus, const uint8_t* tx, uint8_t* rx, size_t n);
} bpvm_spi_backend_t;

void bpvm_spi_set_backend(const bpvm_spi_backend_t* backend);

/* Funciones efectivas. Stubs con logging si backend NULL. */
void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode);
int  bpvm_spi_write(int bus, const uint8_t* data, size_t n);
int  bpvm_spi_read(int bus, uint8_t* data, size_t n);
int  bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SPI_H */
