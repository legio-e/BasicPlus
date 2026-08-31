/*
 * aot_funcs_stub.c — H4 AOT: definicion FUERTE no-op de esp_aot_register para
 * los targets SIN AOT baked-in (ESP32-S3/Xtensa).
 *
 * repl_esp32.c (compartido) SOLO declara esp_aot_register y lo llama antes de
 * bpvm_run. Cada build enlaza EXACTAMENTE UNA definicion fuerte:
 *   - ESP32-P4 (RISC-V): esp32p4/main/aot_funcs_p4.c  → registra el thunk nativo.
 *   - ESP32-S3 (Xtensa): este stub                    → no hace nada.
 * Como repl.o deja la referencia sin resolver, el linker TIRA de este objeto del
 * archive (es el unico proveedor del simbolo) → siempre se enlaza el correcto.
 *
 * El S3 se queda sin AOT baked-in a proposito por AHORA (el foco de H4 es RISC-V);
 * cuando se quiera, activarlo es trivial: compilar aqui el mismo aot_Bench.c con
 * el toolchain Xtensa (C universal) — no hay nada arch-especifico que replicar.
 */

#include "bpvm.h"

void esp_aot_register(struct bpvm* vm) {
    (void) vm;   /* sin AOT baked-in en este target */
}
