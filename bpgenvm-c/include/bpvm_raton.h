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
 * ratón conectado (el simulado), con el punto ya en la pantalla; en las placas, el Bluetooth (el paso 5, con
 * bpvm_raton_mueve: un ratón manda lo que se ha movido, que se suma sin salirse de la pantalla); y las pruebas, con
 * __msInject.
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
 * LA MEMORIA: nada fijo más que un puntero y el cerrojo (12 B en la Pico). El estado (552 B, con los lectores del paso
 * 4 y la pantalla del paso 5) se reserva la primera vez que se usa, con bpvm_malloc_os, como el teclado.
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

/* V8/H2, paso 5 (11-nov) — LO QUE MANDA UN RATÓN DE VERDAD (el Bluetooth, bpvm_hid.c): lo que se ha movido, no dónde
 * está. Se suma al punto de ahora sin salirse de la pantalla, un punto por unidad y sin aceleración; los botones y la
 * rueda, como bpvm_raton_pon. La pantalla, la que diga bpvm_raton_pantalla o, si nadie la ha dicho, la del driver de la
 * pantalla ('PANT', su tamaño a 0°), que se le pregunta cuando alguien empieza a usar el ratón (un Mice.Mouse que se
 * apunta, un lector de C), desde su hilo y no desde la radio; la primera vez que se sabe, el ratón empieza en su centro.
 * Sin pantalla (la S3), el ratón no hace nada (Eduardo, 8-nov). */
void     bpvm_raton_mueve(int32_t dx, int32_t dy, uint32_t botones, int32_t rueda);
/* El tamaño de la pantalla tal como se ve, de quien lo sabe (la GUI, que sabe si está girada; y las pruebas); 0, 0,
 * ninguna. La primera vez, el ratón va a su centro; después, el punto de ahora se queda dentro. */
void     bpvm_raton_pantalla(int32_t w, int32_t h);

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

/* V8/H2, paso 4 (10-nov) — LOS LECTORES DE C: quien lee el ratón desde C sin un Mice.Mouse de BP (LVGL, el puntero de
 * la GUI). Cada lectura da lo que ha pasado desde la anterior: dónde está; si el izquierdo está apretado, o se apretó
 * entre medias (un clic corto, entre dos lecturas, cuenta una vez); y las muescas de la rueda. Al acabar un programa
 * siguen abiertos y empiezan de cero, como los del teclado. */
#define BPVM_RATON_LECTORES 2
int      bpvm_raton_lector_abre(void);                                                  /* su número, o -1 */
int      bpvm_raton_lector_lee(int id, int32_t* x, int32_t* y, int* izq, int32_t* rueda);   /* 1, o 0 si no vale */

#ifdef __cplusplus
}
#endif
#endif
