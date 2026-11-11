/*
 * bpvm_hid.h — V8/H2, paso 5, la pieza 2 (11-nov): LO QUE MANDA UN TECLADO O UN RATÓN HID, al teclado y al ratón del SO
 * (V8_IDEAS §E1, «El paso 5»). Lo común: lo usa la radio de las ESP (bt_esp32.c) con lo que llega por Bluetooth, y un
 * día podría usarlo un USB, que habla el mismo HID. Probado en el PC (test_hid.c) con los descriptores y los informes de
 * verdad del teclado y del ratón de Eduardo (los registros del 14-oct).
 *
 * EL DESCRIPTOR (bpvm_hid_lee_mapa), al conectar, una vez: de cada informe de entrada, por su número, qué es y dónde
 * está cada cosa, como lo declara el aparato y no por su longitud (el teclado de Eduardo declara también un ratón, y su
 * ratón, un teclado). Un teclado: sus ocho modificadores (los códigos 0xE0 a 0xE7, un bit cada uno) y la lista de las
 * teclas apretadas (un código en cada sitio). Un ratón: sus botones (un bit cada uno), y X, Y y la rueda, relativos. Lo
 * demás (las teclas multimedia, las de encendido) es «otro», y de momento no se usa. Y, si lo tiene, el informe de
 * salida con las luces de los bloqueos.
 *
 * UN INFORME (bpvm_hid_informe), con su número (en el Bluetooth no va dentro: lo dice su referencia, 0x2908):
 *   - DE UN TECLADO: se compara con el anterior del mismo aparato, y cada tecla que aparece es una pulsación, a
 *     bpvm_teclado_pon: el código, los modificadores y los bloqueos, y su carácter, de la tabla del teclado americano,
 *     con las reglas del PC (bpvm_teclado.c): sólo las teclas que dan texto, y ninguno con Ctrl, Alt, AltGr o Windows.
 *     Soltar no avisa, y mantener no repite. Con más teclas de las que caben (ErrorRollOver), el informe no vale.
 *   - LOS BLOQUEOS los lleva la placa, como un SO: una pulsación de Bloq Mayús, Bloq Num o Bloq Despl los cambia (Bloq
 *     Mayús, sólo para las letras), y bpvm_hid_informe devuelve 1 para que la radio le mande a cada teclado sus luces
 *     (bpvm_hid_luces). Al arrancar, Bloq Num encendido, como suele estar en Windows (Eduardo, 11-nov: «Si»).
 *   - DE UN RATÓN: lo que se ha movido, sus botones y la rueda, a bpvm_raton_mueve, que lo suma sin salirse de la
 *     pantalla; un punto por unidad, sin aceleración.
 *
 * LO TOCA UNA SOLA TAREA, la que recibe los informes (en las ESP, la de NimBLE): los bloqueos no llevan cerrojo. Ni
 * memoria propia: el mapa y el estado de cada teclado los guarda quien lo usa (la radio, con cada aparato).
 */
#ifndef BPVM_HID_H
#define BPVM_HID_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_HID_INFORMES 8        /* los informes de entrada de un aparato que se apuntan */
#define BPVM_HID_TECLAS   16       /* las teclas apretadas a la vez que se miran de un informe */
#define BPVM_HID_SIN      0xFFFFu  /* un bit que el informe no tiene */

/* Lo que es un informe (bits: uno puede ser las dos cosas), y la clase de un aparato. */
#define BPVM_HID_OTRO     0u
#define BPVM_HID_TECLADO  1u
#define BPVM_HID_RATON    2u

/* Un campo: dónde empieza, en bits desde el principio del informe (sin su número), y cuánto mide. */
typedef struct {
    uint16_t bit;
    uint8_t  bits;        /* 0: el informe no lo tiene */
    uint8_t  con_signo;
} bpvm_hid_campo_t;

typedef struct {
    uint8_t          id;          /* su número (0: el aparato no numera sus informes) */
    uint8_t          que;         /* BPVM_HID_TECLADO y/o BPVM_HID_RATON, o BPVM_HID_OTRO */
    uint16_t         bits;        /* lo que mide entero */
    uint16_t         mod[8];      /* teclado: el bit de cada modificador (el 0xE0 + i), o BPVM_HID_SIN */
    bpvm_hid_campo_t teclas;      /* teclado: el primer sitio de la lista de las teclas apretadas */
    uint8_t          nteclas;     /* cuántos sitios tiene */
    uint16_t         boton[5];    /* ratón: el bit de cada botón (el 1 + i), o BPVM_HID_SIN */
    bpvm_hid_campo_t x, y, rueda; /* ratón: relativos */
} bpvm_hid_informe_t;

typedef struct {
    int                n;                          /* los informes de entrada apuntados */
    bpvm_hid_informe_t inf[BPVM_HID_INFORMES];
    uint8_t            clase;         /* la primera colección de aplicación que es un teclado o un ratón */
    uint8_t            mapa_de_bits;  /* declara un teclado de mapa de bits (NKRO), que no se sabe leer */
    uint8_t            luces;         /* declara las luces de un teclado, en un informe de salida */
    uint8_t            luces_id;      /* su número */
    uint16_t           luz[3];        /* el bit de Bloq Num, Bloq Mayús y Bloq Despl en él, o BPVM_HID_SIN */
    uint16_t           luces_bits;    /* lo que mide ese informe */
} bpvm_hid_mapa_t;

/* Lo que tenía apretado un teclado en su informe anterior. */
typedef struct {
    uint8_t n;
    uint8_t teclas[BPVM_HID_TECLAS];
} bpvm_hid_teclado_t;

/* El descriptor de informes (0x2A4B en el Bluetooth): devuelve cuántos informes de entrada apunta. */
int         bpvm_hid_lee_mapa(const uint8_t* d, int n, bpvm_hid_mapa_t* m);
/* La clase, como la conoce lo común del Bluetooth: "keyboard", "mouse" o "unknown". */
const char* bpvm_hid_clase(const bpvm_hid_mapa_t* m);
/* Para el log: «1 teclado, 2 otro, 3 ratón; luces en el 1». Devuelve lo escrito. */
int         bpvm_hid_describe(const bpvm_hid_mapa_t* m, char* s, size_t n);

/* Un informe que llega, con su número: al teclado o al ratón del SO. `t`, lo apretado del teclado de ese aparato.
 * Devuelve 1 si han cambiado los bloqueos: hay que mandar sus luces a los teclados. */
int         bpvm_hid_informe(const bpvm_hid_mapa_t* m, uint8_t id, const uint8_t* d, int n, bpvm_hid_teclado_t* t);
/* El informe de salida con las luces de los bloqueos de ahora, para el teclado de ese mapa: su largo, o 0 si no tiene. */
int         bpvm_hid_luces(const bpvm_hid_mapa_t* m, uint8_t* d, int max);

/* La tabla del teclado americano: el carácter de un código HID, sin Mayús o con ella (el teclado numérico, con Bloq Num).
 * 0 si no da carácter (Enter, las flechas, las teclas de función...). */
int32_t     bpvm_hid_caracter(uint16_t codigo, int mayus, int bloq_num);
/* Los bloqueos de ahora, en los bits del teclado del SO (BPVM_TECLA_BLOQ_*); y volver a los del arranque (las pruebas). */
uint32_t    bpvm_hid_bloqueos(void);
void        bpvm_hid_reinicia(void);

#ifdef __cplusplus
}
#endif
#endif
