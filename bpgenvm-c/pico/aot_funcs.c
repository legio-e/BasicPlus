/*
 * aot_funcs.c — funciones AOT C-native baked en firmware (H3 #160).
 *
 * Experimento manual para validar el patrón ② de la frontera BP→AOT
 * (ver docs/opcodes.html §"ABI de frontera AOT ↔ BP"). Cada función
 * AOT tiene dos partes:
 *
 *   1) aot_<Mod>_<func>: la implementación C-nativa. Args en C-ABI
 *      normales, return en C return. Locales = C locals.
 *
 *   2) thunk_<Mod>_<func>: el adapter que el intérprete invoca cuando
 *      ve CALL apuntando a la función BP equivalente. Lee args del top
 *      del stack BP, llama la AOT, pushea el resultado. NO crea frame.
 *
 * Para registrar, el firmware llama `aot_funcs_register(vm)` justo
 * antes de bpvm_run, una vez que la global symbol table está poblada
 * (tras link de todos los módulos). bpvm_aot_register_by_name busca
 * "Bench.fib" → si está, registra el thunk al abs_addr resultante.
 *
 * Si Bench.mod NO está cargada (e.g., user corre Hello.mod en su
 * lugar), la registración falla con -2 y nada pasa. Es seguro
 * llamarla siempre.
 */

#include "aot_funcs.h"
#include "aot_registry.h"
#include "bpvm.h"
#include "bpvm_internal.h"
#include "log.h"

#include <stdint.h>

/* ===================== Bench::fib =====================
 * Equivalente C-nativo de:
 *
 *   function fib(n: integer): integer
 *     if n < 2 then return n endif
 *     return fib(n - 1) + fib(n - 2)
 *   end fib
 *
 * Recursión naive — mismo shape que bench.c::fib_native para
 * comparación apples-to-apples. */
static int32_t aot_Bench_fib(int32_t n) {
    if (n < 2) return n;
    return aot_Bench_fib(n - 1) + aot_Bench_fib(n - 2);
}

/* Thunk BP→AOT. Layout del stack BP justo antes del CALL:
 *
 *   sp →  (libre)
 *   sp-4: arg n  ← último arg pusheado, top del stack
 *   ...
 *
 * El intérprete consume el operando relAddr del CALL y nos invoca
 * SIN haber metido saved pc/bp/cs en el stack todavía. Por eso aquí
 * pop = leer mem[sp-4] y restar 4 a sp. Luego pushea result.
 *
 * bp_p no se toca — el thunk no crea frame, sólo es 'fly-by'. */
static void thunk_Bench_fib(bpvm_t* vm,
                             uint32_t* sp_p,
                             uint32_t* bp_p) {
    (void) bp_p;
    uint8_t* mem = vm->memory;
    uint32_t sp  = *sp_p;
    /* pop arg n */
    int32_t n = bpvm_read_i32_be(mem + sp - 4);
    sp -= 4;
    /* call C-native */
    int32_t r = aot_Bench_fib(n);
    /* push result */
    bpvm_write_i32_be(mem + sp, r);
    sp += 4;
    *sp_p = sp;
}

/* ===================== Registro =====================
 * Llamado tras link del módulo, antes de bpvm_run. */
void aot_funcs_register(bpvm_t* vm) {
    int rc = bpvm_aot_register_by_name(vm, "Bench.fib", thunk_Bench_fib);
    if (rc == 0) {
        log_printf("AOT registered: Bench.fib (count=%d)", bpvm_aot_count());
    } else if (rc == -2) {
        /* Símbolo no encontrado — Bench.mod no está cargada. Normal
         * si el usuario está corriendo otra cosa. */
        log_printf("AOT skip: Bench.fib not in symbol table");
    } else {
        log_printf("AOT register failed: rc=%d", rc);
    }
    log_flush();
}
