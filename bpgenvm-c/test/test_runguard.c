/*
 * test_runguard.c — #339: prueba del guardián de fin de RUN.
 *
 * Lo que hay que demostrar no es "la VM no fuga" (eso lo dice el guardián cada
 * vez que corre): es que EL GUARDIÁN FUNCIONA. Un instrumento que siempre dice
 * "todo bien" no se distingue de uno averiado, así que aquí se le pone delante
 * una fuga de verdad y se comprueba que la ve, la mide y la señala.
 *
 * Cinco casos:
 *   1. init+destroy pelado                  -> "vuelve a su sitio"
 *   2. RUN completo de un .mod              -> "vuelve a su sitio"  (el control)
 *   3. un bloque de la VM sin liberar       -> FUGA, con bytes y fichero/línea
 *   4. un bloque de PLATAFORMA sin liberar  -> NO es fuga (contadores separados)
 *   5. reservado ANTES de la marca          -> NO es fuga (no es de este RUN)
 */
#include "bpvm.h"
#include "bpvm_alloc.h"
#include <stdio.h>
#include <string.h>

static char g_last[512];
static void cap(const char* linea) { snprintf(g_last, sizeof g_last, "%s", linea); }

static int g_fail = 0;
static void check(const char* que, int cond) {
    printf("  %-4s: %s\n", cond ? "ok" : "FAIL", que);
    if (!cond) g_fail = 1;
}

static uint8_t g_mem[512 * 1024];

int main(int argc, char** argv) {
    const char* mod = (argc > 1) ? argv[1] : "samples/MethodCall.mod";
    bpvm_alloc_set_report(cap);

    printf("--- 1. init+destroy pelado ---\n");
    {
        g_last[0] = 0;
        bpvm_t* vm = bpvm_init(g_mem, sizeof g_mem, 0);
        check("bpvm_init OK", vm != NULL);
        bpvm_destroy(vm);
        check("el guardian habla SIEMPRE (control)", g_last[0] != 0);
        check("dice que la memoria vuelve a su sitio", strstr(g_last, "vuelve a su sitio") != NULL);
    }

    printf("--- 2. RUN completo de %s ---\n", mod);
    {
        g_last[0] = 0;
        bpvm_t* vm = bpvm_init(g_mem, sizeof g_mem, 0);
        if (vm && bpvm_load_mod(vm, mod) == BPVM_OK) {
            bpvm_run(vm);
        } else {
            printf("  (aviso: no se pudo cargar %s; el caso 2 solo prueba init/destroy)\n", mod);
        }
        bpvm_destroy(vm);
        printf("       %s\n", g_last);
        check("un RUN de verdad no deja nada del programa",
              strstr(g_last, "vuelve a su sitio") != NULL);
    }

    printf("--- 3. fuga PROVOCADA (un bloque de la VM sin liberar) ---\n");
    {
        g_last[0] = 0;
        bpvm_t* vm = bpvm_init(g_mem, sizeof g_mem, 0);
        void* fuga = bpvm_malloc(1234);      /* a proposito: nadie lo libera */
        check("la reserva de la fuga sale bien", fuga != NULL);
        bpvm_destroy(vm);
        printf("       %s\n", g_last);
        check("la CAZA", strstr(g_last, "FUGA") != NULL);
        check("dice cuantos bytes (1234)", strstr(g_last, "1234") != NULL);
        check("senala el fichero donde se pidio", strstr(g_last, "test_runguard.c") != NULL);
        bpvm_free(fuga);                     /* limpieza para el caso siguiente */
    }

    printf("--- 4. bloque de PLATAFORMA vivo: NO es fuga del programa ---\n");
    {
        g_last[0] = 0;
        bpvm_t* vm = bpvm_init(g_mem, sizeof g_mem, 0);
        void* os = bpvm_malloc_os(4096);     /* mutex/cond/handle: ciclo de vida ajeno */
        bpvm_destroy(vm);
        printf("       %s\n", g_last);
        check("NO lo cuenta como fuga", strstr(g_last, "FUGA") == NULL);
        check("pero lo menciona como dato de plataforma",
              strstr(g_last, "plataforma") != NULL);
        bpvm_free(os);
    }

    printf("--- 5. reservado ANTES de la marca: no es de este RUN ---\n");
    {
        void* viejo = bpvm_malloc(777);      /* antes de bpvm_init */
        g_last[0] = 0;
        bpvm_t* vm = bpvm_init(g_mem, sizeof g_mem, 0);
        bpvm_destroy(vm);
        printf("       %s\n", g_last);
        check("un bloque anterior a la marca no se imputa al RUN",
              strstr(g_last, "FUGA") == NULL);
        bpvm_free(viejo);
    }

    printf(g_fail ? "FAIL test-runguard\n" : "PASS test-runguard\n");
    return g_fail;
}
