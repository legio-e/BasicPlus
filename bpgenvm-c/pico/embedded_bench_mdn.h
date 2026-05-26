/*
 * embedded_bench_mdn.h — declaración del blob .mdn embebido.
 *
 * El .c lo genera `xxd -i Bench.mdn`, que produce símbolos sin
 * `extern`. Aquí declaramos los `extern` para que aot_funcs.c
 * pueda referenciarlos sin emitir definiciones duplicadas.
 */
#ifndef BPVM_PICO_EMBEDDED_BENCH_MDN_H
#define BPVM_PICO_EMBEDDED_BENCH_MDN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char Bench_mdn[];
extern unsigned int  Bench_mdn_len;

#ifdef __cplusplus
}
#endif

#endif
