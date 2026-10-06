/*
 * test_nat2runs.c — V7/H12, LAS NATIVES CON DOS RUNS (samples/NatDosRuns.bp; docs/V7_IDEAS.md §H12 «Las natives con
 * dos runs»). El MISMO .mod con el AOT y con dos runs, con el GC colectando en CADA reserva; la receta
 * (`make test-nat2runs`) exige la salida de la pasada interpretada (--interp): «girar: 1», ninguna etiqueta ni suma mala
 * y «fin».
 *
 *   --interp              sin el AOT (el bytecode): la referencia.
 *   --sin-ejecutor        ninguna native consigue ejecutor (como sin memoria para uno): las de bucle corren en la pila
 *                         del run, y `girar` espera al mundo parado del otro run en su línea de bucle.
 *   --sabotaje-aparcada   una native que espera al mundo dentro de sí no apunta su pila de C: el intermedio de
 *                         `etiqueta` se recicla (etiquetas malas, o un «objeto eliminado»).
 *   --sabotaje-deposito   el depósito de ejecutores sin cerrojo, con la ventana entre ver uno libre y reservarlo: dos
 *                         natives con el mismo ejecutor.
 *   --sabotaje-anidada    (con --sin-ejecutor) la línea del bucle no espera al mundo: colgado (la receta lo corta).
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_NatDosRuns_register(struct bpvm* vm);

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* mod_path = (argc > 1) ? argv[1] : "NatDosRuns.mod";
    int interp = 0;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--interp")) interp = 1;
        else if (!strcmp(argv[i], "--sin-ejecutor")) bpvm_la_cola.ensayo_sin_ejecutor = 1;
        else if (!strcmp(argv[i], "--sabotaje-aparcada")) bpvm_la_cola.sabotaje_aparcada = 1;
        else if (!strcmp(argv[i], "--sabotaje-deposito")) bpvm_la_cola.sabotaje_deposito = 1;
        else if (!strcmp(argv[i], "--sabotaje-anidada")) bpvm_la_cola.sabotaje_anidada = 1;
    }
    /* La ventana entre ver libre un ejecutor y reservarlo, siempre: con el cerrojo no hace daño (va dentro). Y cada
     * colecta, 1 ms más larga con el mundo parado (espera activa): las reservas del otro run —también las de dentro de
     * una native— la encuentran casi siempre (sin esto, el sabotaje de N3 salía rojo o no según la carga de la
     * máquina; con 300 µs, a veces sólo 3 etiquetas malas). */
    bpvm_la_cola.ensayo_deposito_ms = 2;
    bpvm_la_cola.ensayo_colecta_us = 1000;

    size_t mem_size = 1024 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }
    bpvm_status_t s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }
    if (!interp) aot_NatDosRuns_register(vm);    /* las native → sus thunks de C */
    vm->gc_bump_threshold = 1;                   /* GC en CADA reserva, en los dos runs */

    s = bpvm_run_smp(vm, 2);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK) fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));
    bpvm_destroy(vm); free(mem);
    return (int) s;
}
