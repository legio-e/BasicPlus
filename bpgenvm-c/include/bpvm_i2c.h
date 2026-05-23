/*
 * bpvm_i2c.h — hooks de plataforma para los builtins I2c.*
 *
 * Mismo patrón que bpvm_gpio.h. Por defecto stubs con logging por
 * stdout. En el firmware Pico, main.c registra un backend cuyas
 * funciones llaman a i2c_init / i2c_write_blocking / i2c_read_blocking
 * del SDK + gpio_set_function/gpio_pull_up para configurar pines.
 */
#ifndef BPVM_I2C_H
#define BPVM_I2C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*init)(int bus, int sda, int scl, int baudrate);
    /* Devuelve bytes escritos / leídos, o -1 en error. */
    int  (*write)(int bus, int addr, const uint8_t* data, size_t n);
    int  (*read)(int bus, int addr, uint8_t* data, size_t n);
} bpvm_i2c_backend_t;

void bpvm_i2c_set_backend(const bpvm_i2c_backend_t* backend);

/* Funciones efectivas. Si backend NULL → stub con logging. */
void bpvm_i2c_init(int bus, int sda, int scl, int baudrate);
int  bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n);
int  bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_I2C_H */
