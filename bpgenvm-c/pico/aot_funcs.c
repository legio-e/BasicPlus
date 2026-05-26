/*
 * aot_funcs.c — agregador de funciones AOT baked en firmware.
 *
 * H3 #157 fase 1: el contenido AOT real está en aot_<Mod>.c
 * AUTOGENERADOS por AotCEmitter desde el .bp con `native function`.
 * Este fichero solo orquesta el registro al arrancar un RUN.
 *
 * Cuando se generen más módulos AOT, añadir extern y llamada aquí.
 */

#include "aot_funcs.h"
#include "bpvm.h"

/* Forwards a los register de cada aot_<Mod>.c generado. */
extern void aot_Bench_register(struct bpvm* vm);

/* Registra TODAS las funciones AOT disponibles. Tolerante a símbolos
 * ausentes — si el .mod correspondiente no está cargado, el register
 * by name del módulo concreto devuelve -2 silenciosamente. */
void aot_funcs_register(struct bpvm* vm) {
    aot_Bench_register(vm);
    /* Añadir aquí futuros aot_<Mod>_register(vm); */
}
