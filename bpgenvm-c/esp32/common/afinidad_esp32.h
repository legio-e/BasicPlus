/* afinidad_esp32.h — V7/H12: el núcleo de las tareas de la VM en las ESP32 de dos (la S3 y la P4, encendidos los dos
 * por Eduardo el 6-oct: «En la Pico y Metro, 2 nucleos y 2 runs. En la P4 y la S3 igual»). Sin afinidad, como en la
 * Pico (validada así en H12): el planificador del IDF las pone donde quepan. Con `vm.cores=1` en el ENV —la vuelta
 * atrás sin grabar, como en la Pico—, todas en el núcleo 0. El FreeRTOS del IDF no cambia la afinidad de una tarea ya
 * creada, así que cada una se crea ya con la suya: `run` y `run2`, `io`, los ejecutores de las natives, los hilos y,
 * en la P4, la del cable y la del log por red (la clave la lee antes el arranque común: bpvm_boot2_repartir). La
 * tarea `main` de la S3 —la del REPL— la fija el IDF al 0 siempre (CONFIG_ESP_MAIN_TASK_AFFINITY_CPU0). En las de un
 * núcleo da igual: sólo hay uno. */
#ifndef AFINIDAD_ESP32_H
#define AFINIDAD_ESP32_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bpvm.h"   /* bpvm_cores */

static inline BaseType_t esp32_afinidad(void) {
    return (bpvm_cores() == 1) ? (BaseType_t) 0 : (BaseType_t) tskNO_AFFINITY;
}

#endif /* AFINIDAD_ESP32_H */
