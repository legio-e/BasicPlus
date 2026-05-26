/*
 * aot_funcs.c — agregador del registro AOT (H3 #157 + #158).
 *
 * Política de carga:
 *  1. Stage 1: aot_<Mod>_register() del .o linkado-estático.
 *     Esto es la versión AOT compilada con el firmware como
 *     fallback siempre disponible.
 *
 *  2. Stage 2: si hay un .mdn embebido (Bench_mdn[]), lo cargamos
 *     con bpvm_load_mdn — registry queda OVERWRITTEN apuntando
 *     a la versión .mdn (idéntica al .o linkado en este caso,
 *     pero es el path que validamos para AOT dinámico).
 *
 * El log final muestra cuántos thunks AOT terminaron en el registry.
 *
 * Fase D (#158): añadir stage 3 que busque .mdn en FS (e.g.
 * /app/Bench.mdn) y lo cargue desde allí.
 */

#include "aot_funcs.h"
#include "bpvm.h"
#include "log.h"

#include "mdn_loader.h"
#include "embedded_bench_mdn.h"

extern void aot_Bench_register(struct bpvm* vm);

void aot_funcs_register(struct bpvm* vm) {
    /* Stage 1: AOT linkado-estático del firmware. */
    aot_Bench_register(vm);

    /* Stage 2: .mdn embebido (Bench_mdn[]). Zero-copy: el thunk se
     * registra apuntando dentro del array de .data. */
    int rc = bpvm_load_mdn(vm, Bench_mdn, (size_t) Bench_mdn_len);
    if (rc != MDN_OK) {
        log_printf("AOT: embedded Bench.mdn load failed rc=%d "
                   "(linked-in se conserva)", rc);
    }
    log_flush();
}
