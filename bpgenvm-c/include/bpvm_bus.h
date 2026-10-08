/* bpvm_bus.h — los buses SPI e I2C como RECURSO COMPARTIDO de la HAL BP (V7/H6.c G4.a1).
 *
 * ─── LA DECISIÓN (Eduardo, 23-sep; V7_IDEAS §H6.11) ─────────────────────────
 *
 * Un bus se COMPARTE —en la C5 la TF y la pantalla van en el mismo SPI2, y el
 * expansor y el táctil en el mismo I2C 0—; lo exclusivo es el DISPOSITIVO (su CS,
 * su dirección). La CONFIGURACIÓN del bus —sus pines, y en I2C también la
 * velocidad, que la tienen que aguantar todos— se establece UNA vez:
 *
 *   · la declara el ENV:       spi2=sck:6,mosi:7,miso:8      i2c0=sda:0,scl:1,khz:400
 *   · o, si el ENV no dice nada, el PRIMERO que abre el bus (un programa con
 *     `Spi.Bus(0, 2, 3, 4, …)`, la línea `sd=` de la Metro con sus pines…).
 *
 * Quien llegue después con OTRA configuración recibe un error con nombre; con la
 * misma, se engancha. Un -1 en un pin quiere decir «el que tenga el bus»: así un
 * dispositivo usa un bus declarado sin repetir sus pines, y el MISO a -1 de una
 * pantalla («no lo uso») casa con un bus que sí lo tiene. Y se puede SOLTAR: el que
 * lo abrió deja de usarlo, y cuando no queda nadie y no es del ENV, el bus se
 * cierra y sus pines vuelven a libres. El fin de RUN suelta lo del programa.
 *
 * ─── POR QUÉ AQUÍ ────────────────────────────────────────────────────────────
 *
 * Un bus tiene TRES usuarios: el programa BP (los builtins), el firmware (la TF por
 * SPI, `bpvm_sd.c`) y los packs (por la BIOS). Ésta es la única capa por la que
 * pasan los tres; en la BIOS se lo saltarían los otros dos. Y la cintura de cada
 * familia ADELGAZA: abre el bus una vez, lo ajusta y transfiere — ya no lo
 * reabre en cada `init`, que era lo que le quitaba el bus al anterior en silencio.
 *
 * ─── EL ÁRBITRO (G4.a2, más abajo) ──────────────────────────────────────────
 *
 * Compartir el bus no es mezclar sus TRANSACCIONES. Quien va a transferir pide un
 * TURNO, y mientras lo tiene nadie más transfiere por ese bus.
 */
#ifndef BPVM_BUS_H
#define BPVM_BUS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { BPVM_BUS_SPI = 0, BPVM_BUS_I2C = 1 };

/* Quién usa un bus (máscara): los tres usuarios de la cabecera. */
enum { BPVM_BUS_PROGRAMA = 1, BPVM_BUS_FIRMWARE = 2, BPVM_BUS_PACK = 4 };

/* Los ids posibles de un bus (el SPI 2, el I2C 0…): la tabla de capacidades dice
 * cuáles existen en cada micro; esto sólo es el tope del casillero. */
#define BPVM_BUS_ID_MAX  8

/* El ENV al arrancar: lee `spiN=` e `i2cN=` para cada bus que tenga el micro y los
 * DECLARA (reserva sus pines). `env_get` es el de la familia —el mismo contrato que
 * el de la BIOS: longitud, o <0 si la clave no está o no cabe—. Lo que esté mal se
 * dice en el log con su nombre y el bus se queda sin declarar. Devuelve cuántos
 * declaró. */
typedef int (*bpvm_bus_env_get_fn)(const char* clave, char* buf, size_t cap);
int  bpvm_bus_desde_env(bpvm_bus_env_get_fn env_get);

/* Declara un bus desde el VALOR de su línea (`sck:6,mosi:7,miso:8`). 0, o -1 con
 * `err`. (La usa bpvm_bus_desde_env; va aparte para poder probarla.) */
int  bpvm_bus_declarar(int tipo, int id, const char* valor, char* err, size_t cap);

/* V7 #669 (8-oct) — un bus que DECLARA LA IMAGEN: el del táctil de su pantalla (la P4, la DK2), al publicar su
 * driver, antes que el ENV. Como uno del ENV —sus pines, del bus («el pin 7 es del I2C 0»); se abre en la cintura con
 * su primer usuario y se cierra cuando no queda ninguno; un programa no lo suelta— y COMPARTIDO con lo demás que haya
 * en él (Eduardo: «El I2c del táctil tiene que ser compartido. Hay placas donde ese I2c se utiliza también para
 * controlar otros dispositivos»): un programa se engancha con sus mismos pines y su velocidad. `que` (una cadena
 * estática) dice de quién es, y sale en los errores. En I2C, p2 = -1. 0, o -1 con `err`. */
int  bpvm_bus_declarar_imagen(int tipo, int id, int p0, int p1, int p2, int hz, const char* que, char* err, size_t cap);

/* Abre un bus, o se engancha a él. SPI: p0..p2 = sck, mosi, miso; I2C: p0, p1 =
 * sda, scl (p2 se ignora). -1 = el que tenga. `hz`: en I2C la velocidad del bus
 * (<= 0 = la que tenga); en SPI se ignora (es de cada dispositivo). `quien`: un
 * BPVM_BUS_*. Deja en `pins` (si no es NULL) los tres pines EFECTIVOS y en `*hz_ef`
 * la velocidad efectiva (I2C). 0 = abierto (y `*recien` = 1 si hay que abrirlo en la
 * cintura); -1 con `err`. */
int  bpvm_bus_abrir(int tipo, int id, int p0, int p1, int p2, int hz, int quien,
                    int pins[3], int* hz_ef, int* recien, char* err, size_t cap);

/* Lo contrario de abrir sin llegar a usarlo: la cintura no pudo abrir el bus que
 * `bpvm_bus_abrir` dio por recién establecido. Deshace el establecimiento. */
void bpvm_bus_abrir_fallo(int tipo, int id, int quien);

/* `quien` deja de usar el bus. Si es el último y el bus no es del ENV, se cierra:
 * devuelve 1 y la cintura tiene que cerrarlo. 0 = sigue abierto (otro lo usa, o es
 * del ENV). -1 con `err`: un programa que suelta un bus de la placa, o uno que usa
 * otro — el porqué con nombre, no un no-op. */
int  bpvm_bus_soltar(int tipo, int id, int quien, char* err, size_t cap);

/* Fin de RUN: el programa suelta todo lo suyo. Llama a `cerrar(tipo, id)` por cada
 * bus que se queda sin nadie (la cintura lo cierra). */
void bpvm_bus_fin_de_run(void (*cerrar)(int tipo, int id));

/* ¿Está abierto? (1/0) Y su configuración en texto, para el log y los mensajes. */
int  bpvm_bus_abierto(int tipo, int id);
int  bpvm_bus_describe(int tipo, int id, char* out, size_t cap);

/* Para las pruebas de host y el simulador: todo a libre. */
void bpvm_bus_reset(void);

/* ── V7/H6.c G4.a2 — EL ÁRBITRO: el TURNO ────────────────────────────────────
 *
 * Lo que no se puede mezclar es la TRANSACCIÓN: del CS abajo al CS arriba en SPI,
 * de START a STOP en I2C. Quien va a transferir pide un turno y lo suelta al
 * acabar; mientras, nadie más transfiere por ese bus. Cada petición trae SU
 * configuración —Eduardo, 23-sep: «todas son operaciones petición-fin, no veo que
 * haya que guardar nada intermedio»—, así que aquí sólo vive de quién es el turno
 * en curso y desde cuándo: para esperar, y para decirlo. Se borra al soltar.
 *
 * UN mutex y UNA condvar para toda la tabla (bpvm_platform: FreeRTOS en las placas,
 * pthread en el PC): el mutex sólo guarda el «quién» y se suelta enseguida; la
 * transferencia va fuera. Se crean al arrancar (bpvm_bus_desde_env, un solo hilo),
 * o en el primer turno si nadie los creó (el PC, las pruebas).
 *
 * NADA ESPERA PARA SIEMPRE: pasado el plazo, -1 con el porqué —«SPI 2 ocupado (CS
 * 10, un pack) desde hace 1000 ms»—. También el hilo que pide un bus que ya tiene:
 * era un cuelgue, y es ese mismo error. `cs`: el CS (SPI) o la dirección (I2C) del
 * que pide, sólo para decirlo; -1 = ninguno. `plazo_ms` <= 0 = BPVM_BUS_PLAZO_MS. */
#define BPVM_BUS_PLAZO_MS 1000
int  bpvm_bus_tomar(int tipo, int id, int quien, int cs, int plazo_ms, char* err, size_t cap);
void bpvm_bus_dejar(int tipo, int id);
int  bpvm_bus_tomado(int tipo, int id);            /* 1 = hay un turno en curso */
void bpvm_bus_arbitro_init(void);

/* ── V7/H6.c G4.a3 — EL TOPE: un turno es CORTO (V7_IDEAS §H6.11.9) ──────────
 *
 * Eduardo, 24-sep: «Lo que hay que evitar es que si trabajo con un conversor con
 * bus SPI y hago una conversión que me lleva 200 ms, mientras esté trabajando, nadie
 * más pueda utilizar el bus.» El tope es del BUS: lo dice su línea del ENV (`ms:20`),
 * BPVM_BUS_TOPE_MS si no lo dice, y nunca más de BPVM_BUS_TOPE_MAX_MS.
 *
 *   · Una TRANSACCIÓN (bpvm_spi_transaccion, bpvm_i2c_transaccion) se calcula ANTES
 *     de empezar —los relojes que ocupará el bus a su velocidad— y si no cabe, no
 *     empieza: -1 con el porqué. La cuenta es entera, la misma en miVM.
 *   · El turno A MANO (tomar … dejar) no se puede calcular antes, y cortarlo a medias
 *     dejaría un dispositivo a mitad de algo: se MIDE, y si pasó del tope el log lo
 *     dice con el nombre de quién. */
#define BPVM_BUS_TOPE_MS      20
#define BPVM_BUS_TOPE_MAX_MS 100
int  bpvm_bus_tope_ms(int tipo, int id);
/* 1 = el bus tiene configuración (la del ENV o la del primero que lo abrió). */
int  bpvm_bus_establecido(int tipo, int id);
/* I2C: la velocidad del bus (la de su configuración); 0 si no está establecido. */
int  bpvm_bus_hz(int tipo, int id);
/* ¿Cabe en el tope una transacción de `relojes` ciclos de reloj a `hz`? 0, o -1 con
 * el porqué: «SPI 2: la transacción ocuparía el bus 32 ms a 1 MHz (máximo 20 ms)». */
int  bpvm_bus_cabe(int tipo, int id, unsigned long long relojes, int hz, char* err, size_t cap);
/* El turno en curso se MIDE al dejarlo. Lo pide quien lo tiene, justo tras tomarlo. */
void bpvm_bus_medir(int tipo, int id);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_BUS_H */
