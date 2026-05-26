/*
 * aot_funcs.c — agregador del registro AOT.
 *
 * H3 #157 baseline: el contenido AOT real está en aot_<Mod>.c
 * AUTOGENERADO por AotCEmitter desde el .bp con `native function`.
 * Este fichero solo orquesta el registro al arrancar un RUN.
 *
 * (Fase #158 .mdn dinámico — pausada tras crash en first try.
 *  Volveremos a ella con instrumentación más fina.)
 */

#include "aot_funcs.h"
#include "bpvm.h"

extern void aot_Bench_register(struct bpvm* vm);

void aot_funcs_register(struct bpvm* vm) {
    aot_Bench_register(vm);
}
