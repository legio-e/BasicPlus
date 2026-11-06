/*
 * bpvm_wire_cables.h — V8/H3 (6-nov): DOS CABLES A LA VEZ. El de siempre (la UART) y el de la red, con las mismas funciones
 * `wire_v1_*` por encima: el REPL no se entera de por dónde habla.
 *
 * Eduardo (19-oct): «el uso más inmediato: el IDE conectado a la placa por WiFi». Y tiene que poder engancharse a un
 * programa que ya corre, o a un autorun, y pararlo: por eso los dos cables escuchan a la vez, también durante un RUN.
 *
 * HABLA UNO A LA VEZ: el último que ha mandado un mensaje entero. Las respuestas y la salida del programa van a ése. Sólo se
 * cambia entre mensajes, y lo que no empieza por `{` no es un mensaje: ni cuenta ni hace esperar a nadie.
 *
 * Al cambiar, o al irse el cliente de la red, lo que se le mandaba al de antes se corta (bpvm_repl_cliente_ido): una
 * descarga a medias no sigue saliendo por otro cable. Cuando el de la red se va, habla el local.
 *
 * UN CABLE sólo mueve bytes. Lo demás —el mensaje, los plazos de una línea o un bulk a medias, el cerrojo— es de aquí: en
 * bpvm_wire_v1.h, «TRANSPORTE». Sin cable de la red, esto es exactamente el cable local.
 *
 * Va con su guarda (BPVM_WIRE_CABLES): lo compilan las imágenes que lo usan, y la STM32, que compila todo src/, no.
 */
#ifndef BPVM_WIRE_CABLES_H
#define BPVM_WIRE_CABLES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lo que devuelve `sondeo`. */
#define BPVM_WIRE_SIGUE       0
#define BPVM_WIRE_CLIENTE_IDO 1    /* el cliente de este cable se ha ido, o le ha sustituido otro */

typedef struct bpvm_wire_cable {
    const char* nombre;   /* para el log */
    /* Lee hasta `n` bytes y vuelve con lo que haya, esperando `ms` como mucho (0: sin esperar). Los bytes leídos; 0 si no
     * ha llegado nada; -1 si al otro lado ya no hay nadie. */
    int  (*leer)(uint8_t* buf, size_t n, int ms);
    /* Escribe `n` bytes. Si no puede, los pierde: no se queda colgado. Siempre con el cerrojo de aquí cogido. */
    void (*escribir)(const uint8_t* datos, size_t n);
    /* Opcional: se ha acabado un mensaje, que salga ya. El cable de la red junta lo que le llega en un paquete. */
    void (*fin)(void);
    /* Opcional: lo que el cable tenga que mirar mientras tanto (aceptar un cliente, por ejemplo). Se llama desde quien
     * lee, a menudo; BPVM_WIRE_SIGUE o BPVM_WIRE_CLIENTE_IDO. */
    int  (*sondeo)(void);
    /* Opcional: 1 si ahora puede llegar algo por él. Sin ella, siempre. */
    int  (*abierto)(void);
} bpvm_wire_cable_t;

/* Un byte sin esperar, de cualquiera de los dos: el de un mensaje que empieza, o -1. */
int wire_v1_try_getchar(void);

/* Los cables de la imagen: el local, al abrir el cable; el de la red, si la placa tiene. */
void bpvm_wire_cables_local(const bpvm_wire_cable_t* c);
void bpvm_wire_cables_red(const bpvm_wire_cable_t* c);

/* El cerrojo de la escritura, para quien cierre un cable: nadie estará escribiendo en él. */
void bpvm_wire_cables_bloquear(void);
void bpvm_wire_cables_soltar(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_WIRE_CABLES_H */
