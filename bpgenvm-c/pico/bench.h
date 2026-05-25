/*
 * bench.h — micro-bench hardcoded para H3 (AOT).
 * Ver bench.c para descripción y protocolo de uso.
 */
#ifndef BPVM_PICO_BENCH_H
#define BPVM_PICO_BENCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Ejecuta fib_native(28) y loguea el tiempo via log_printf + log_flush.
 * Llamar UNA vez en vm_task tras fs_init. */
void bench_run_native(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_PICO_BENCH_H */
