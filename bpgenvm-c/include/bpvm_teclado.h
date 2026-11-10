/*
 * bpvm_teclado.h — V8/H2, paso 2 (9-nov): EL TECLADO DEL SO. Lo común de la HAL BP, para la clase Keyboards.Keyboard
 * (V8_IDEAS §E1, «El paso 2»). Eduardo: «Un teclado a nivel de SO. Un teclado a nivel de programa ( un objeto ). El
 * teclado a nivel de SO es único, con su buffer. El de programa puede ser mas de uno, y se relaciona con el del SO».
 *
 * UNA TECLA es un int32 y su carácter. El int32 (Eduardo: «la parte baja el código de la tecla y la parte alta los bits
 * de estado»): abajo, el código del estándar HID (el que manda un teclado Bluetooth o USB; SDL usa los mismos); arriba,
 * el byte de modificadores del estándar HID, tal cual lo manda el teclado, y los bloqueos (BPVM_TECLA_*). El carácter,
 * como código Unicode: en el PC, el que da el sistema; en las placas, el de las tablas de BasicPlus; 0 en una tecla
 * especial (Enter, las flechas...). Un aviso por pulsación: una tecla que se mantiene no repite.
 *
 * QUIÉN LAS PONE (bpvm_teclado_pon, desde cualquier hilo: lleva su cerrojo): en el PC, la ventana de la GUI, si hay un
 * teclado conectado (el simulado); en las placas, el Bluetooth (el paso 5); y las pruebas, con __kbInject.
 *
 * QUIÉN LAS LEE, de dos maneras (Eduardo: los eventos, para todos; preguntar lee el búfer del SO):
 *   - preguntando (hasKey, readKey): el búfer del SO, BPVM_TECLADO_BUFER teclas, y el primero que lee se la lleva.
 *     Lleno, la que llega se pierde, como en un BIOS. El carácter de la que se lee se queda para ese hilo (getChar).
 *   - con eventos: cada Keyboards.Keyboard se apunta (bpvm_teclado_apunta) sin que eso lo mantenga vivo: su referencia
 *     entera, con la generación, dice si sigue vivo. Entre turno y turno, el planificador llama a bpvm_teclado_reparte,
 *     que le avisa cada tecla nueva con un evento de la cola de siempre, en el hilo que lo creó: su método __despacha
 *     (k, cp), que hace el `raise onKey`. UNO en la cola por teclado como mucho: la cola es de todos (16), y un hilo que
 *     espera no la debe llenar. Las que esperan, en una lista con las últimas BPVM_TECLADO_AVISOS: quien se queda atrás
 *     pierde las más viejas.
 *
 * LA MEMORIA: nada fijo más que un puntero y el cerrojo. El estado (~0,7 KB) se reserva la primera vez que se usa, con
 * bpvm_malloc_os, como el Bluetooth: una placa sin teclado no lo gasta (la C3 y la C6 van justas).
 */
#ifndef BPVM_TECLADO_H
#define BPVM_TECLADO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bpvm;

/* Los bits de estado, la parte alta del int32: el byte de modificadores del estándar HID y los bloqueos. */
#define BPVM_TECLA_CODIGO      0x0000FFFFu
#define BPVM_TECLA_CTRL_IZQ    (1u << 16)
#define BPVM_TECLA_SHIFT_IZQ   (1u << 17)
#define BPVM_TECLA_ALT         (1u << 18)   /* el Alt izquierdo */
#define BPVM_TECLA_WIN_IZQ     (1u << 19)
#define BPVM_TECLA_CTRL_DER    (1u << 20)
#define BPVM_TECLA_SHIFT_DER   (1u << 21)
#define BPVM_TECLA_ALTGR       (1u << 22)   /* el Alt derecho */
#define BPVM_TECLA_WIN_DER     (1u << 23)
#define BPVM_TECLA_BLOQ_MAYUS  (1u << 24)
#define BPVM_TECLA_BLOQ_NUM    (1u << 25)
#define BPVM_TECLA_BLOQ_DESPL  (1u << 26)
#define BPVM_TECLA_FN          (1u << 27)

#define BPVM_TECLADO_BUFER    32   /* lo que lee readKey */
#define BPVM_TECLADO_AVISOS   16   /* las últimas, para los eventos */
#define BPVM_TECLADO_OBJETOS  8    /* los Keyboards.Keyboard apuntados a la vez */
#define BPVM_TECLADO_HILOS    8    /* el carácter de la última leída, por hilo */

/* Pone una tecla, desde cualquier hilo. */
void    bpvm_teclado_pon(int32_t k, int32_t cp);

/* Preguntando (Keyboard.hasKey, readKey, getChar). */
int     bpvm_teclado_hay(void);
int32_t bpvm_teclado_lee(int tid);         /* la saca; 0 si no hay. Su carácter se queda para el hilo `tid` */
int32_t bpvm_teclado_caracter(int tid);    /* el carácter de la última que leyó ese hilo; 0 si ninguna */

/* Los teclados del programa. `ref`, la referencia entera (con la generación); `slot`, la ranura de su __despacha. */
int     bpvm_teclado_apunta(struct bpvm* vm, uint64_t ref, int tid, int slot);   /* 1, o 0 si no caben más */
void    bpvm_teclado_borra(uint64_t ref);
void    bpvm_teclado_reparte(struct bpvm* vm);   /* el planificador, entre turno y turno, donde reparte los eventos */
void    bpvm_teclado_fin_de_run(void);           /* al acabar un programa: fuera los apuntados y lo que quede */

/* EL PC: las teclas de la ventana de la GUI, sólo con un teclado conectado (en el PC, el simulado del Bluetooth). Lo
 * llama la ventana con el código HID y los modificadores en bits BPVM_TECLA_* (sin repeticiones ni los modificadores
 * solos); el carácter llega después, aparte, si la tecla lo da. Una tecla que puede dar carácter espera a su texto; si
 * no llega, sale sin él en la siguiente pulsación o al soltarla. Con Ctrl, Alt o Windows (no AltGr), sale en seguida.
 * La VM-Java hace lo mismo con las teclas de Swing (Teclado.java). */
void    bpvm_teclado_pc_pulsada(int32_t codigo, uint32_t estado);
void    bpvm_teclado_pc_texto(int32_t cp, uint32_t estado);
void    bpvm_teclado_pc_soltada(int32_t codigo);

/* V8/H2, paso 4 (10-nov) — LA GUI (V8_IDEAS §E1, «El paso 4»), sin un Keyboards.Keyboard de BP en medio:
 *   - LOS LECTORES DE C: quien lee las teclas desde C (LVGL, el grupo de la GUI), cada uno con su número de la siguiente
 *     que le toca, como un teclado apuntado. Al acabar un programa siguen abiertos (en el micro simulado, LVGL vive de un
 *     RUN al siguiente) y empiezan de cero, con las teclas del programa nuevo.
 *   - LA PANTALLA de scr.onKey: la última Gui.Screen creada o cargada (__guiKeyTarget), con su referencia entera (no la
 *     mantiene viva); __guiRunOnce le entrega cada tecla en el hilo de la GUI, como un clic. Al acabar, fuera. */
#define BPVM_TECLADO_LECTORES 2
int     bpvm_teclado_lector_abre(void);                                 /* su número, o -1 si no caben */
int     bpvm_teclado_lector_lee(int id, int32_t* k, int32_t* cp);       /* 1 y la siguiente, o 0 si no hay */
void    bpvm_teclado_pantalla(uint64_t ref);                            /* la que recibe las teclas desde ahora */
int     bpvm_teclado_pantalla_lee(uint64_t* ref, int32_t* k, int32_t* cp);   /* 1, su pantalla y la siguiente; o 0 */

#ifdef __cplusplus
}
#endif
#endif
