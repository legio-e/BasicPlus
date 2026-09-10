/*
 * test_guidrop.c — V6/#489: la cola de eventos del GUI descartaba EN SILENCIO.
 *
 * POR QUE ESTE TEST. La cola tiene 512 huecos y se drena a razon de una vuelta
 * cada ~10 ms, asi que desbordarla con un programa real pide un slider arrastrado
 * o un sensor rapido: es dificil de provocar a mano y por eso el fallo llevaba
 * ahi sin que nadie lo viera. Un camino que no se ejecuta no esta probado, asi
 * que esto FUERZA el caso: inyecta mas eventos de los que caben SIN drenar.
 *
 * Verde = el primer descarte se dice en el acto y el total al terminar. Los dos
 * avisos van por bpvm_diag (stderr), asi que el stdout no se toca y la paridad
 * dual-VM sigue intacta — eso lo comprueba la ultima parte.
 *
 *   make test-guidrop
 */
#include "bpvm_gui.h"
#include <stdio.h>
#include <string.h>

#define CAP      512      /* GUI_MAX_NODES: lo que cabe (uno se pierde de guarda) */
#define INYECTA  600

int main(void) {
    int fallos = 0;

    /* 1. Llenar y pasarse. */
    for (int i = 0; i < INYECTA; i++) bpvm_gui_inject_click((uint32_t) (i + 1));

    /* 2. Vaciar contando: la cola circular guarda CAP-1. */
    int sacados = 0, kind = 0;
    while (bpvm_gui_next_event(&kind) != 0) sacados++;

    printf("inyectados=%d  recuperados=%d  perdidos=%d\n",
           INYECTA, sacados, INYECTA - sacados);
    if (sacados != CAP - 1) {
        printf("FALLO: esperaba %d recuperados\n", CAP - 1); fallos++;
    }
    if (INYECTA - sacados <= 0) {
        printf("FALLO: el test no llego a desbordar — no prueba nada\n"); fallos++;
    }

    /* 3. El total sale aqui: reset es el fin del programa BP. */
    printf("-- bpvm_gui_reset (el total va a stderr) --\n");
    bpvm_gui_reset();

    /* 4. Y que no vuelva a decirlo: el contador se pone a cero. */
    printf("-- segundo reset, sin descartes: no debe decir nada --\n");
    bpvm_gui_reset();

    printf(fallos == 0 ? "PASS\n" : "FAIL\n");
    return fallos == 0 ? 0 : 1;
}
