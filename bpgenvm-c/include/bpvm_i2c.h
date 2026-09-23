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
    /* V7/H6.c G4.a1 — abrir UNA vez y cerrar; ver bpvm_spi.h. En I2C la velocidad
     * es del bus (la aguantan todos), así que va con abrir y no hay «ajustar». */
    int  (*abrir) (int bus, int sda, int scl, int baudrate);  /* 0 = abierto (si ya lo estaba, 0 y nada) */
    void (*cerrar)(int bus);
} bpvm_i2c_backend_t;

void bpvm_i2c_set_backend(const bpvm_i2c_backend_t* backend);

/* V7/H6.c G4.a1 — la puerta de los tres usuarios; ver bpvm_spi_abrir. `baudrate`
 * <= 0 = la que tenga el bus. */
int  bpvm_i2c_abrir(int bus, int sda, int scl, int baudrate, int quien, char* err, size_t cap);
int  bpvm_i2c_soltar(int bus, int quien, char* err, size_t cap);
void bpvm_i2c_cerrar(int bus);

/* Funciones efectivas. Si backend NULL → stub con logging. */
void bpvm_i2c_init(int bus, int sda, int scl, int baudrate);
int  bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n);
int  bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_I2C_H */
