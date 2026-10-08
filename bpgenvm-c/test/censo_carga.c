/*
 * censo_carga.c — V7 #651 (8-oct): LO QUE SE LLEVA CARGAR UN PROGRAMA, por quién lo pide. Cada .mod de la línea de
 * órdenes, en una VM nueva: se marca el guardián, se carga (con sus dependencias, del directorio de trabajo) y, ANTES
 * de correrlo, el censo de los bloques de la VM vivos (bpvm_alloc_censo): cuántos y de dónde. Es la medida de
 * MEMORIA.md §«Por qué un módulo cuesta de 3 a 6 KB aunque venga de un pack» y la de antes y después de tocar el
 * cargador.
 *
 *     (cd build/censo && ../censo_carga.exe Nada.mod Importa5.mod GuiWinJson.mod GuiArbolG2.mod)
 *
 * Con `--tope=N` delante, además, es una PRUEBA: cada programa, como mucho N bloques cargado, o FAIL. Lo que vigila
 * (test-censo, en la red): que el cargador no vuelva a pedir un bloque por nombre —con la GUI eran 818; con un bloque
 * por módulo, 54—: en la placa, cada bloque lleva la cabecera del guardián de #339.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "bpvm_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void linea(const char* s) { printf("  %s\n", s); }

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    long tope = -1;
    int fallos = 0;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--tope=", 7) == 0) { tope = atol(argv[i] + 7); continue; }
        size_t mem_size = 4u * 1024u * 1024u;
        uint8_t* mem = (uint8_t*) calloc(1, mem_size);
        if (!mem) { printf("sin memoria\n"); return 1; }
        uint64_t marca = bpvm_alloc_mark();
        bpvm_t* vm = bpvm_init(mem, mem_size, 0);
        if (!vm) { printf("%s: bpvm_init falla\n", argv[i]); free(mem); return 1; }
        bpvm_status_t s = bpvm_load_mod(vm, argv[i]);
        printf("== %s (carga: %s; %d módulo(s))\n", argv[i], bpvm_status_str(s), vm->module_count);
        uint32_t bloques = bpvm_alloc_censo(marca, 14, linea);
        if (tope >= 0) {
            if (s != BPVM_OK) { printf("FAIL %s no carga\n", argv[i]); fallos++; }
            else if ((long) bloques > tope) {
                printf("FAIL %s: %lu bloques cargado, más que el tope (%ld): ¿un bloque por nombre otra vez?\n",
                       argv[i], (unsigned long) bloques, tope);
                fallos++;
            } else {
                printf("ok  %s: %lu bloques cargado (tope %ld)\n", argv[i], (unsigned long) bloques, tope);
            }
        }
        bpvm_destroy(vm);
        free(mem);
    }
    if (tope >= 0) printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
