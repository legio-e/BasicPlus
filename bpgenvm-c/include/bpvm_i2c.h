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

/* V7/H6.c G4.a3 — un MENSAJE de una transacción I2C (ver bpvm_i2c_transaccion). */
typedef struct {
    uint8_t* buf;    /* escribir: lo que se manda (no se toca); leer: dónde se deja */
    uint32_t n;      /* bytes */
    uint32_t lee;    /* 0 = escribir, 1 = leer */
} bpvm_i2c_msg_t;
/* Lo que cabe en UNA transacción, igual en todas las familias: lo que puede el más
 * pequeño (manda el piso). El ESP32 la hace en una sola lista de ÓRDENES del bus
 * —START, los datos, STOP— y tiene 8 registros para ellas (I2C_LL_CMD_REG_NUM), sin
 * forma de encadenar dos listas sin un STOP en medio: cada mensaje son 2 órdenes (su
 * START y sus datos), una lectura de más de un byte 3 (el último byte va con NACK), y el
 * STOP final 1. De ahí: hasta 3 mensajes, cada uno con al menos un byte, y 8 órdenes. */
#define BPVM_I2C_MENSAJES_MAX 3
#define BPVM_I2C_ORDENES_MAX  8

typedef struct {
    void (*init)(int bus, int sda, int scl, int baudrate);
    /* Devuelve bytes escritos / leídos, o -1 en error. */
    int  (*write)(int bus, int addr, const uint8_t* data, size_t n);
    int  (*read)(int bus, int addr, uint8_t* data, size_t n);
    /* V7/H6.c G4.a1 — abrir UNA vez y cerrar; ver bpvm_spi.h. En I2C la velocidad
     * es del bus (la aguantan todos), así que va con abrir y no hay «ajustar». */
    int  (*abrir) (int bus, int sda, int scl, int baudrate);  /* 0 = abierto (si ya lo estaba, 0 y nada) */
    void (*cerrar)(int bus);
    /* V7/H6.c G4.a3 — los mensajes de UNA transacción: START, cada mensaje, un
     * repeated start entre dos, STOP al final. 0 = bien; -1 = NAK o plazo (y el bus,
     * libre: con su STOP). Una cintura sin esto: un mensaje tras otro, cada uno con su
     * STOP, dentro del mismo turno. */
    int  (*transaccion)(int bus, int addr, const bpvm_i2c_msg_t* m, int nm);
} bpvm_i2c_backend_t;

void bpvm_i2c_set_backend(const bpvm_i2c_backend_t* backend);

/* V7/H6.c G4.a1 — la puerta de los tres usuarios; ver bpvm_spi_abrir. `baudrate`
 * <= 0 = la que tenga el bus. */
int  bpvm_i2c_abrir(int bus, int sda, int scl, int baudrate, int quien, char* err, size_t cap);
int  bpvm_i2c_soltar(int bus, int quien, char* err, size_t cap);
void bpvm_i2c_cerrar(int bus);

/* Funciones efectivas. Si backend NULL → stub con logging. V7/H6.c G4.a2: cada
 * llamada es un TURNO del árbitro (bpvm_bus.h); las de BP son del programa y las
 * `_como` dicen quién (las ranuras de la BIOS: un pack). */
void bpvm_i2c_init(int bus, int sda, int scl, int baudrate);
int  bpvm_i2c_write(int bus, int addr, const uint8_t* data, size_t n);
int  bpvm_i2c_read(int bus, int addr, uint8_t* data, size_t n);
int  bpvm_i2c_write_como(int quien, int bus, int addr, const uint8_t* data, size_t n);
int  bpvm_i2c_read_como(int quien, int bus, int addr, uint8_t* data, size_t n);
/* Escribir y leer en UNA transacción, con repeated start entre las dos (G4.a3; hasta
 * entonces, STOP): la de dos mensajes de abajo. Bytes leídos, o -1 (el porqué, al log). */
int  bpvm_i2c_escribir_leer(int quien, int bus, int addr, const uint8_t* tx, size_t ntx,
                            uint8_t* rx, size_t nrx);

/* ── V7/H6.c G4.a3 — LA TRANSACCIÓN I2C (V7_IDEAS §H6.11.9) ──────────────────
 *
 * Cada operación es un MENSAJE, con repeated start entre dos y STOP al final: la
 * semántica de Linux (`i2c_transfer`). Sirve para leer un registro (escribir su
 * número, leer) y para escribir varios de un golpe. En UN turno, y ANTES de empezar
 * se calcula lo que ocupará el bus —9 relojes por byte, la dirección de cada mensaje
 * incluida, a la velocidad del bus—: si pasa del tope, -1 con el porqué y nada. De 1 a
 * BPVM_I2C_MENSAJES_MAX mensajes. Devuelve los bytes movidos; -1 si no empezó (el
 * porqué en `err`: el tope, el bus, el turno) o -2 si el dispositivo no contestó (NAK
 * o plazo) — lo que un `scan` espera de una dirección vacía. */
int  bpvm_i2c_transaccion(int bus, int addr, int quien, const bpvm_i2c_msg_t* m, int nm,
                          char* err, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_I2C_H */
