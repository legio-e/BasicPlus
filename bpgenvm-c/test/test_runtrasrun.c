/*
 * test_runtrasrun.c — V7 #659 (7-oct): lo de un RUN empieza de cero en el siguiente.
 *
 * En una placa, o en el micro simulado (el «Run on Device» del IDE sin placa), un proceso corre un RUN tras otro, y
 * había cosas que valían por PROCESO: la rampa del ADC del stub (el segundo RUN leía otros valores; la de la VM-Java
 * empieza con cada VM), el aviso de «esta placa no registra backend de ADC» (sólo en el primero) y los avisos de un
 * `throw` en el diagnóstico (los cinco primeros de TODA la sesión: después, ningún RUN decía nada). Ahora valen por RUN.
 *
 * Corre samples/RunTrasRun.mod dos veces en el mismo proceso, con el backend de ADC del host: la misma salida las dos
 * veces y los cinco avisos de `throw` en las dos. Y otras dos sin backend: el aviso, en las dos.
 *
 * Salida: una línea por comprobación y, si todo va, «OK: ...» (la señal de check-testc). exit 0 si pasan.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_io.h"
#include "bpvm_fs.h"
#include "bpvm_adc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char s_salida[4096];
static size_t s_n = 0;
static int s_avisos = 0, s_avisos_builtin = 0;

static void recoge(const char* s, size_t len, void* user) {
    (void) user;
    if (s_n + len >= sizeof s_salida) len = sizeof s_salida - 1 - s_n;
    memcpy(s_salida + s_n, s, len);
    s_n += len;
    s_salida[s_n] = 0;
}

static void diag(const char* linea) {   /* cuenta los avisos de un throw (el primero de cada uno: el texto) */
    if (strstr(linea, "[bpvm] throw (interp): \"") != NULL) s_avisos++;
    if (strstr(linea, "[bpvm] throw: \"") != NULL) s_avisos_builtin++;
}

/* Un RUN del .mod en una VM nueva; deja su salida en `out` y devuelve cuántos avisos de throw dio el intérprete (y en
 * `*builtin`, los de un builtin); -1 si no corre. */
static int corre(const char* mod, char* out, size_t cap, int* builtin) {
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) return -1;
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm || bpvm_load_mod(vm, mod) != BPVM_OK) {
        fprintf(stderr, "no carga %s\n", mod);
        if (vm) bpvm_destroy(vm);
        free(mem);
        return -1;
    }
    s_n = 0; s_salida[0] = 0; s_avisos = 0; s_avisos_builtin = 0;
    bpvm_io_ops_t ops;
    memset(&ops, 0, sizeof ops);
    ops.line = recoge;
    if (bpvm_io_start(vm, &ops, 0) != 0) { bpvm_destroy(vm); free(mem); return -1; }
    bpvm_status_t s = bpvm_run(vm);
    bpvm_io_stop(vm);
    bpvm_destroy(vm);   /* el fin de RUN: aquí se pone a cero lo de este RUN */
    free(mem);
    snprintf(out, cap, "%s", s_salida);
    if (builtin) *builtin = s_avisos_builtin;
    return (s == BPVM_OK) ? s_avisos : -1;
}

static int g_fail = 0;
static void check(const char* que, int cond) {
    printf("[runtrasrun] %-4s %s\n", cond ? "ok" : "FAIL", que);
    if (!cond) g_fail = 1;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod = (argc > 1) ? argv[1] : "RunTrasRun.mod";
    static char a[4096], b[4096];
    bpvm_diag_set_sink(diag);

    /* 1. Con el backend del host (el PC, el micro simulado): dos RUN, lo mismo. */
    bpvm_adc_set_backend(bpvm_adc_backend_host());
    int bu1 = 0, bu2 = 0;
    int av1 = corre(mod, a, sizeof a, &bu1);
    int av2 = corre(mod, b, sizeof b, &bu2);
    printf("[runtrasrun] primer RUN : %s", a);
    printf("[runtrasrun] segundo RUN: %s", b);
    check("los dos RUN corren hasta el final", av1 >= 0 && av2 >= 0 && strstr(a, "cazadas: 12") && strstr(b, "cazadas: 12"));
    check("la misma salida los dos (la rampa del ADC empieza otra vez)", strcmp(a, b) == 0);
    printf("[runtrasrun] avisos de throw: el interprete %d y %d; un builtin %d y %d\n", av1, av2, bu1, bu2);
    check("cinco avisos de throw del interprete en cada RUN (no cinco en toda la sesion)", av1 == 5 && av2 == 5);
    check("y cinco de un builtin en cada RUN", bu1 == 5 && bu2 == 5);

    /* 2. Sin backend (una placa sin ADC): el aviso, en los dos. */
    bpvm_adc_set_backend(NULL);
    corre(mod, a, sizeof a, NULL);
    corre(mod, b, sizeof b, NULL);
    check("sin backend, el aviso en el primer RUN", strstr(a, "NO REGISTRA BACKEND DE ADC") != NULL);
    check("sin backend, el aviso tambien en el segundo", strstr(b, "NO REGISTRA BACKEND DE ADC") != NULL);

    bpvm_diag_set_sink(NULL);
    if (!g_fail)
        printf("OK: un RUN tras otro empieza de cero: la rampa del ADC del stub, el aviso de sin backend y los "
               "avisos de throw (#659)\n");
    return g_fail;
}
