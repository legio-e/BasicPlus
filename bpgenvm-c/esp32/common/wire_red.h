/*
 * wire_red.h — V8/H3 (6-nov): EL CABLE DE LA RED: el wire por TCP, y encontrar la placa por UDP. Lo que es un cable, en
 * bpvm_wire_cables.h; lo que dice el protocolo, en docs/BPVM_WIRE_PROTOCOL.md.
 */
#ifndef WIRE_RED_H
#define WIRE_RED_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIRE_RED_PUERTO 3333   /* el mismo para el TCP y para buscar por UDP */

/* Da de alta el cable. El puerto se abre solo al tener IP y se cierra al perderla. Una vez, al arrancar, en una placa con
 * WiFi. */
void wire_red_iniciar(void);

#ifdef __cplusplus
}
#endif

#endif /* WIRE_RED_H */
