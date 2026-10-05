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
/* V7/H12, 2b — la memoria de la fachada (velocidad, modo y CS de cada bus, a «no se sabe»), con un solo hilo vivo:
 * la llama bpvm_cola_asegurar. */
void bpvm_spi_preparar(void);

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

/* V7/H6.c G4.a2 — EL TURNO (V7_IDEAS §H6.11.6b). Cada petición trae SU
 * configuración; la fachada sólo recuerda la velocidad y el modo ACTUALES del bus
 * y los cambia si los del turno son distintos.
 *   tomar   espera el turno (árbitro de bpvm_bus.h, con plazo y error con nombre),
 *           ajusta si hace falta y baja `cs` (-1 = el CS no lo toca la fachada);
 *   cs      sube o baja el CS del turno sin soltarlo (la TF: sus relojes con el
 *           CS arriba);
 *   xfer    transfiere dentro del turno: tx y rx, sólo tx o sólo rx;
 *   dejar   sube el CS y suelta el turno.
 * El código nativo hace varias transferencias en un turno; desde BP, cada llamada
 * es un turno entero. 0 / bytes, o -1 (el porqué en `err` o en el log). */
int  bpvm_spi_tomar(int bus, int cs, int hz, int modo, int quien, char* err, size_t cap);
int  bpvm_spi_cs(int bus, int nivel);
int  bpvm_spi_xfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n);
void bpvm_spi_dejar(int bus);
/* G4.a3: el turno A MANO (tomar … dejar) no se puede calcular antes: se MIDE, y si
 * pasa del tope del bus el log lo dice (bpvm_bus.h). Es el de la TF y los packs v7;
 * lo nuevo va por transacciones. */

/* ── V7/H6.c G4.a3 — LA TRANSACCIÓN (V7_IDEAS §H6.11.9) ──────────────────────
 *
 * Varias operaciones en UN turno, con el CS abajo de principio a fin. Se describe
 * antes y se ejecuta de una vez: dentro sólo hay transferencias —ni esperas ni código
 * de quien llama—, así que es corta por construcción. Y ANTES de empezar se calcula
 * lo que ocupará el bus (8 relojes por byte a `hz`): si pasa del tope del bus, -1 con
 * el porqué y no se transfiere nada. Eduardo: «tiene más sentido que se llamen desde
 * C» — los packs la tienen en la BIOS v8; BP va encima.
 *
 * Una operación: `tx` y `rx` a la vez es full-duplex; sólo `tx`, escribir; sólo `rx`,
 * leer (la cintura manda su relleno). Devuelve los bytes movidos, o -1 con `err`. */
typedef struct {
    const uint8_t* tx;
    uint8_t*       rx;
    uint32_t       n;
} bpvm_spi_op_t;
int  bpvm_spi_transaccion(int bus, int cs, int hz, int modo, int quien,
                          const bpvm_spi_op_t* ops, int nops, char* err, size_t cap);
/* El turno de una transacción YA calculada que quien llama ejecuta por tramos (los
 * builtins de BP, que convierten el array de BP a bytes de 256 en 256): como
 * bpvm_spi_tomar, pero no se mide —ya pasó el tope—. */
int  bpvm_spi_tomar_transaccion(int bus, int cs, int hz, int modo, int quien, char* err, size_t cap);
/* Engancharse a un bus YA establecido (el ENV o quien lo abrió), sin pines ni
 * velocidad: lo que hace un `Spi.Device`. No toca la velocidad de las ranuras de
 * siempre. 0, o -1 con `err`. */
int  bpvm_spi_enganchar(int bus, int quien, char* err, size_t cap);

/* Las ranuras de SIEMPRE, cada llamada un TURNO entero (G4.a2) con la velocidad
 * del `abrir` de quien llama —hasta G4.a3, que la pasa el objeto BP—. Stubs con
 * logging si backend NULL. `init` es el camino de SIEMPRE (abre y ajusta de una
 * vez): lo usa bpvm_spi_abrir con una cintura sin `abrir`, y el stub del PC imprime
 * con él la línea que es contrato de paridad. Las tres de BP son del programa;
 * `write_como` dice quién (la ranura v6 de la BIOS: un pack). */
void bpvm_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode);
int  bpvm_spi_write(int bus, const uint8_t* data, size_t n);
int  bpvm_spi_read(int bus, uint8_t* data, size_t n);
int  bpvm_spi_transfer(int bus, const uint8_t* tx, uint8_t* rx, size_t n);
int  bpvm_spi_write_como(int quien, int bus, const uint8_t* data, size_t n);
/* El turno de esas mismas ranuras, suelto: para la llamada de BP que la fachada ve en
 * varios tramos (#534) y tiene que ser UN turno entero (tomar_como, xfer ×N, dejar). */
int  bpvm_spi_tomar_como(int quien, int bus);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SPI_H */
