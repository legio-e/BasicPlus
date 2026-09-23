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
    /* V7/H6.c G4.a1 — el bus se ABRE una vez y se AJUSTA aparte. `init` reabría el
     * bus en cada llamada, y el segundo que lo abría se lo quitaba al primero (la
     * TF a la pantalla) en silencio. Una cintura que rellena estos tres ya no
     * reabre nada; la que no, sigue con `init` como hasta ahora. */
    int  (*abrir)  (int bus, int sck, int mosi, int miso);    /* 0 = abierto (si ya lo estaba, 0 y nada) */
    void (*ajustar)(int bus, int baudrate, int mode);         /* velocidad y modo de lo que se transfiera */
    void (*cerrar) (int bus);
} bpvm_spi_backend_t;

void bpvm_spi_set_backend(const bpvm_spi_backend_t* backend);

/* V7/H6.c G4.a1 — la puerta de los tres usuarios de un bus (el programa, el
 * firmware, los packs: BPVM_BUS_* de bpvm_bus.h). Aplica la regla de bpvm_bus.h
 * —la configuración se establece una vez (el ENV o el primero); otra es un error
 * con nombre; -1 en un pin = el que tenga el bus— y luego abre (sólo la primera
 * vez) y ajusta la cintura. Deja los pines efectivos en `pins` (si no es NULL).
 * 0, o -1 con `err`. */
int  bpvm_spi_abrir(int bus, int sck, int mosi, int miso, int baudrate, int mode,
                    int quien, int pins[3], char* err, size_t cap);
/* `quien` deja de usar el bus; si era el último (y no es del ENV) se cierra. */
int  bpvm_spi_soltar(int bus, int quien, char* err, size_t cap);
/* Cierra el bus en la cintura (lo usa el fin de RUN, vía bpvm_bus_fin_de_run). */
void bpvm_spi_cerrar(int bus);

/* Funciones efectivas. Stubs con logging si backend NULL. `init` es el camino de
 * SIEMPRE (abre y ajusta de una vez): lo usa bpvm_spi_abrir con una cintura sin
 * `abrir`, y el stub del PC imprime con él la línea que es contrato de paridad. */
void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode);
int  bpvm_spi_write(int bus, const uint8_t* data, size_t n);
int  bpvm_spi_read(int bus, uint8_t* data, size_t n);
int  bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SPI_H */
