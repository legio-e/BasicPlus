/*
 * test/main.c — CLI para la VM C. Equivalente del bpgenvm de Java a
 * efectos de smoke-testing.
 *
 * Uso:
 *   bpgenvm-c <fichero.mod>            ejecuta el módulo
 *   bpgenvm-c --trace <fichero.mod>    activa trace per-instrucción
 *   bpgenvm-c --mem=N <fichero.mod>    memorySize en bytes (default 512 KiB)
 */

#include "bpvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t parse_size(const char* s) {
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (v <= 0) return 0;
    return (size_t) v;
}

int main(int argc, char** argv) {
    /* stdout unbuffered para que un crash no se trague output. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    const char* path  = NULL;
    int   trace       = 0;
    size_t mem_size   = 512 * 1024;

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (strcmp(a, "--trace") == 0)         { trace = 1; }
        else if (strncmp(a, "--mem=", 6) == 0) {
            size_t n = parse_size(a + 6);
            if (n > 0) mem_size = n;
        }
        else if (a[0] == '-')                  {
            fprintf(stderr, "Argumento desconocido: %s\n", a);
            return 2;
        }
        else if (!path)                        { path = a; }
        else {
            fprintf(stderr, "Sólo se admite un .mod por invocación.\n");
            return 2;
        }
    }
    if (!path) {
        fprintf(stderr, "Uso: bpgenvm-c [--trace] [--mem=N] <fichero.mod>\n");
        return 1;
    }

    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) {
        fprintf(stderr, "No se pudo alocar %zu bytes\n", mem_size);
        return 1;
    }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) {
        fprintf(stderr, "bpvm_init falló (memSize=%zu)\n", mem_size);
        free(mem);
        return 1;
    }
    bpvm_set_tracing(vm, trace);

    bpvm_status_t s = bpvm_load_mod(vm, path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem);
        return (int) s;
    }

    printf("=== INICIANDO EJECUCION DE LA VM-C ===\n");
    s = bpvm_run(vm);
    printf("=== FIN DE LA EJECUCION (status=%s) ===\n", bpvm_status_str(s));

    bpvm_destroy(vm);
    free(mem);
    return (int) s;
}
