/*
 * bench.c — micro-bench hardcoded para H3 (AOT).
 *
 * Objetivo: medir el techo de rendimiento de una función equivalente
 * a la `fib(28)` que se ejecuta en BP. Si la versión nativa C es muy
 * superior, AOT vale la pena; si están parejas, AOT no aporta y
 * aparcamos H3.
 *
 * Se llama UNA vez al boot, justo tras fs_init. Log persistente
 * recoge el resultado — el usuario lo lee con LOG desde el IDE.
 *
 * Para usarlo:
 *   1. Flashear este firmware.
 *   2. Connect en el IDE → LOG → ver "BENCH native ...".
 *   3. Subir Bench.mod, Run → ver "BP fib(28)=... in N ms".
 *   4. Comparar los dos tiempos.
 */

#include "bench.h"

#include "bpvm_pico.h"   /* bpvm_pico_uptime_ms */
#include "log.h"

#include <stdint.h>

/* Implementación naive recursiva — la misma forma que la fib BP de
 * Bench.bp para que la comparación sea apples-to-apples (no metemos
 * optimizaciones que el AOT no podría replicar). */
static int32_t fib_native(int32_t n) {
    if (n < 2) return n;
    return fib_native(n - 1) + fib_native(n - 2);
}

void bench_run_native(void) {
    const int32_t N = 28;
    uint32_t t0 = (uint32_t) bpvm_pico_uptime_ms();
    int32_t r = fib_native(N);
    uint32_t dt = (uint32_t) bpvm_pico_uptime_ms() - t0;
    log_printf("BENCH native fib(%ld)=%ld in %u ms", (long) N, (long) r, (unsigned) dt);
    log_flush();
}
