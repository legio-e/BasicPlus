/*
 * aot_funcs.h — funciones AOT baked en firmware.
 * Ver aot_funcs.c para descripción.
 */
#ifndef BPVM_PICO_AOT_FUNCS_H
#define BPVM_PICO_AOT_FUNCS_H

struct bpvm;

#ifdef __cplusplus
extern "C" {
#endif

/* Registra las funciones AOT baked en el AOT registry global del
 * runtime. Tolerante a símbolos ausentes — si el módulo no está
 * cargado, lo loguea y sigue. Llamar tras link, antes de bpvm_run. */
void aot_funcs_register(struct bpvm* vm);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_AOT_FUNCS_H */
