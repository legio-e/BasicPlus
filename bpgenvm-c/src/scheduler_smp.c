/*
 * scheduler_smp.c — los runs de bpvm_run_smp (el PC: `--smp=N`, las pruebas de C).
 *
 * Desde V7/H12, etapa 2a.2, el planificador es UNO, el de la cola (scheduler.c), y sus runs son los mismos en todas
 * partes. Aquí queda sólo cómo arranca bpvm_run_smp los suyos: el hilo que llama hace de run 0 y n-1 hilos más, que
 * acaban con el programa; al volver, todos unidos. Antes (H2) cada RUN creaba aquí su estado SMP, sus N workers y un
 * planificador propio que no cedía el núcleo ni veía el interbloqueo. La comm task del `--smp` sin `io` (comm_host.c)
 * la arranca y la para la cola, al publicar y al descolgar el programa.
 */

#include "bpvm_smp.h"
#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include "bpvm_alloc.h"

#include <stdint.h>

/* Un run temporal: corre lo publicado para los de bpvm_run y vuelve cuando acaba. `arg`: su número. */
static void run_temporal(void* arg) {
    bpvm_cola_correr(0, (int) (intptr_t) arg);
}

void bpvm_cola_correr_n(int n) {
    bpvm_platform_thread_handle_t* hilos = NULL;
    int creados = 0;
    if (n > 1) {
        hilos = (bpvm_platform_thread_handle_t*) bpvm_calloc((size_t) (n - 1), sizeof *hilos);
        /* Modelo ASIMÉTRICO (#153): los runs, al núcleo 1; el 0, para la E/S. Con un núcleo, o en el PC, el ancla no
         * hace nada. Uno que no nace no para el programa: lo corren los que haya (antes, #473 punto 7, se volvía con
         * un error). */
        for (int i = 1; hilos != NULL && i < n; i++) {
            if (bpvm_platform_thread_create_pinned(&hilos[creados], run_temporal, (void*) (intptr_t) i, 1) != 0) break;
            creados++;
        }
    }
    bpvm_cola_correr(0, 0);   /* el que llama, también: hasta que el programa acaba */
    for (int i = 0; i < creados; i++) bpvm_platform_thread_join(&hilos[i]);
    bpvm_free(hilos);
}
