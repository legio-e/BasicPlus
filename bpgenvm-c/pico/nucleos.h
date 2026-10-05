/*
 * nucleos.h — V7/H12, etapa 3.1 (5-oct): los dos núcleos de la RP2350, lo que tienen que ver de ellos el resto de la
 * Pico (definido en pico/main.c; docs/V7_IDEAS.md §H12 «La etapa 3.1, el diseño»).
 */
#ifndef BPVM_PICO_NUCLEOS_H
#define BPVM_PICO_NUCLEOS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Las marcas del arranque, una por núcleo, en RAM que sobrevive al reinicio: el arranque siguiente cuenta en el log
 * hasta qué paso llegó cada núcleo (sin sonda SWD, es lo que dice dónde se para un arranque que se cuelga). */
void pico_marca_arranque(uint32_t paso);

/* El arranque, completo (el autorun o el REPL, tras su primer volcado del log): su marca y, con dos núcleos, el perro
 * guardián del arranque, apagado. Se puede llamar dos veces. */
void pico_arranque_completo(void);

/* `vm.cores=1` en el ENV, con la imagen de dos núcleos: la tarea `t`, anclada al núcleo 0 (la vuelta atrás sin grabar
 * que decidió Eduardo). Cada tarea nuestra pasa por aquí al crearse. Con un núcleo, o sin `vm.cores=1`, nada. */
void pico_tarea_nucleos(TaskHandle_t t);

/* El paso 4: lo llama cada camino fatal (el assert, la pila, la memoria de FreeRTOS, el panic del SDK, el HardFault)
 * ANTES de escribir nada. Con dos núcleos (Eduardo: «fatal error, el log y reset»): el perro guardián armado —la
 * placa se reinicia en 250 ms pase lo que pase—, la marca del fallo en la RAM que sobrevive, y el log sin la flash; si
 * el otro núcleo ya estaba en el suyo, éste se para aquí y no vuelve. Con uno, nada (como hoy). */
void pico_fatal(void);

/* El arranque anterior acabó en un fallo fatal (la marca de pico_fatal): sin autoarranque. */
int pico_arranque_tras_fatal(void);

#endif /* BPVM_PICO_NUCLEOS_H */
