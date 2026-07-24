/*
 * aot_funcs_p4.c — H4 AOT RISC-V (Hito 1): registro AOT baked-in SOLO del P4.
 *
 * Espejo de pico/aot_funcs.c, con dos diferencias:
 *   (a) SOLO se compila en el build del ESP32-P4 (RISC-V). El S3 (Xtensa) NO lo
 *       incluye → allí `esp_aot_register` es el weak no-op de repl_esp32.c y no
 *       hay AOT (un blob RISC-V no corre en Xtensa igual que uno ARM no cruza).
 *   (b) Solo stage-1 (baked estático linkado en el firmware). El `.mdn` dinámico
 *       (cargar código RISC-V desde FS/pack, con relocación + GC-safe) = Hito 2.
 *
 * Provee la implementación FUERTE del hook `esp_aot_register()` (weak en el repl
 * compartido). El repl la llama tras link, antes de bpvm_run, precedida de
 * bpvm_aot_clear() para que cada RUN parta de un registry limpio (los módulos se
 * recargan en direcciones frescas → los thunks viejos serían stale).
 */

#include "bpvm.h"
#include "aot_registry.h"
#include "esp_log.h"

/* La función AOT nativa de este firmware (aot_Bench.c, compilado por el toolchain
 * RISC-V del IDF). Se registra por nombre ("Bench.fib") → tolerante a ausencia. */
extern void aot_Bench_register(struct bpvm* vm);

void esp_aot_register(struct bpvm* vm) {
    aot_Bench_register(vm);
    /* Huella verificable por consola: cuántos thunks quedaron. 0 = Bench.mod no
     * cargado (no-op); >=1 = el nativo RISC-V está armado y el OP_CALL lo
     * secuestrará. */
    ESP_LOGI("AOT", "RISC-V baked-in: %d thunk(s) en el registry", bpvm_aot_count());
}
