/*
 * bpvm_raton.h — V8/H2, paso 3 (9-nov): EL RATÓN DEL SO. Lo común de la HAL BP, para la clase Mice.Mouse (V8_IDEAS §E1,
 * «El paso 3»). Como el teclado, en dos niveles (Eduardo, 8-nov): uno del SO, con su posición y sus botones, que junta
 * todos los ratones conectados, y los del programa, uno o varios.
 *
 * EL RATÓN VA CON UNA PANTALLA: su posición son los puntos de esa pantalla, como los de la GUI ((0,0) arriba a la
 * izquierda; con la pantalla girada, tal como se ve). Los botones, con los bits del informe de un ratón HID
 * (BPVM_RATON_*: los valores del enum Mice.Button).
 *
 * QUIÉN LO MUEVE (bpvm_raton_pon, desde cualquier hilo: lleva su cerrojo): en el PC, la ventana de la GUI, si hay un
 * ratón conectado (el simulado), con el punto ya en la pantalla; en las placas, el Bluetooth (el paso 5: un ratón manda
 * desplazamientos, que se sumarán y se limitarán a la pantalla); y las pruebas, con __msInject.
 *
 * QUIÉN LO LEE, de dos maneras:
 *   - preguntando (getX, getY, isPressed): tal como está ahora.
 *   - con eventos: cada Mice.Mouse se apunta (bpvm_raton_apunta) sin que eso lo mantenga vivo, como un teclado. Apretar
 *     un botón y la rueda dejan un AVISO, con el punto donde pasó (BPVM_RATON_AVISOS: quien se queda atrás pierde los
 *     más viejos); soltar no avisa, de momento, como el teclado. MOVERSE NO DEJA AVISO: cada ratón del programa recuerda
 *     la última posición que se le avisó, y si la del SO es otra, le toca un onMove (sólo la última: un ratón Bluetooth
 *     manda unos 100 informes por segundo). Entre turno y turno, después de las teclas, el planificador llama a
 *     bpvm_raton_reparte, que le encola a cada uno, en el hilo que lo creó, su __despacha(tipo, dato, x, y): con un aviso
 *     pendiente, si no está en su punto, primero un BPVM_RATON_MUEVE hasta allí (el programa ve llegar el ratón adonde
 *     se aprieta), y después el aviso; sin avisos, un BPVM_RATON_MUEVE a la posición del SO, si ha cambiado. UNO en la
 *     cola por ratón como mucho, como los teclados.
 *
 * LA MEMORIA: nada fijo más que un puntero y el cerrojo (12 B en la Pico). El estado (528 B) se reserva la primera vez
 * que se usa, con bpvm_malloc_os, como el teclado.
 *
 * El espejo, en la VM-Java: Raton.java (y el reparto, VirtualMachine.repartirRaton). Las dos VMs tienen que repartir lo
 * mismo y en el mismo orden.
 */
#ifndef BPVM_RATON_H
#define BPVM_RATON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/* Los botones: los bits del informe de un ratón HID. */
#define BPVM_RATON_IZQ       0x01u
#define BPVM_RATON_DER       0x02u
#define BPVM_RATON_CENTRAL   0x04u
#define BPVM_RATON_ATRAS     0x08u
#define BPVM_RATON_ADELANTE  0x10u
#define BPVM_RATON_BOTONES   0x1Fu

/* El tipo de cada aviso: el primer argumento de __despacha. */
#define BPVM_RATON_PULSA  1   /* un botón que se aprieta (dato: su bit) */
#define BPVM_RATON_MUEVE  2   /* moverse (dato: 0) */
#define BPVM_RATON_RUEDA  3   /* la rueda (dato: las muescas, +1 hacia arriba) */

#define BPVM_RATON_AVISOS   16   /* los de los botones y la rueda que faltan por repartir */
#define BPVM_RATON_OBJETOS  8    /* los Mice.Mouse apuntados a la vez */

/* Lo que manda un ratón, con el punto en vez del desplazamiento (el PC y las pruebas), desde cualquier hilo: la posición;
 * un aviso por cada botón que se aprieta ahora, del bit bajo al alto; y la rueda, en muescas (0, nada). */
void     bpvm_raton_pon(int32_t x, int32_t y, uint32_t botones, int32_t rueda);

/* Preguntando (Mouse.getX, getY, isPressed): tal como está. */
int32_t  bpvm_raton_x(void);
int32_t  bpvm_raton_y(void);
uint32_t bpvm_raton_botones(void);

/* Los ratones del programa. `ref`, la referencia entera (con la generación); `slot`, la ranura de su __despacha. */
int      bpvm_raton_apunta(struct bpvm* vm, uint64_t ref, int tid, int slot);   /* 1, o 0 si no caben más */
void     bpvm_raton_borra(uint64_t ref);
void     bpvm_raton_reparte(struct bpvm* vm);   /* el planificador, entre turno y turno, después de las teclas */
void     bpvm_raton_fin_de_run(void);           /* al acabar un programa: fuera los apuntados y los avisos */

/* EL PC: el ratón de la ventana de la GUI, sólo con un ratón conectado (en el PC, el simulado del Bluetooth). Lo llama la
 * ventana con el punto ya en la pantalla, los botones en bits BPVM_RATON_* y la rueda en muescas. La VM-Java hace lo
 * mismo con el ratón de Swing (RatonSwing). */
void     bpvm_raton_pc(int32_t x, int32_t y, uint32_t botones, int32_t rueda);

#ifdef __cplusplus
}
#endif
#endif
