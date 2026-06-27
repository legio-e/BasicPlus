/*
 * test/main.c — CLI para la VM C. Equivalente del bpgenvm de Java a
 * efectos de smoke-testing.
 *
 * Uso:
 *   bpgenvm-c <fichero.mod>            ejecuta el módulo
 *   bpgenvm-c --trace <fichero.mod>    activa trace per-instrucción
 *   bpgenvm-c --mem=N <fichero.mod>    memorySize en bytes (default 512 KiB)
 *   bpgenvm-c --debug-trace[=N] <m>    #139: instala un debug hook que
 *                                       cuenta cambios de línea (modo
 *                                       sintético: pc_to_line=NULL, así
 *                                       cada opcode es una "línea"). Sin
 *                                       N imprime sólo el total; con N
 *                                       imprime los primeros N hits.
 */

#include "bpvm.h"
#include "bpvm_fs.h"
#include "bpvm_net.h"   /* H11 — registro del backend TCP del host */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t parse_size(const char* s) {
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (v <= 0) return 0;
    return (size_t) v;
}

/* #139 — Smoke hook: incrementa counter, opcionalmente imprime las
 * primeras N invocaciones. Como el modo es sintético (pc_to_line=NULL)
 * cada `line` recibido es el valor de pc, así que vemos la secuencia
 * de PCs ejecutados — equivalente a --trace pero pasando por el
 * camino del hook. */
typedef struct {
    long total_hits;
    long max_print;
    long printed;
} debug_trace_state_t;

static void debug_trace_hook(bpvm_t* vm, bpvm_thread_t* tc,
                              uint32_t pc, int line, const char* source,
                              void* user) {
    (void) vm; (void) source;
    debug_trace_state_t* st = (debug_trace_state_t*) user;
    st->total_hits++;
    if (st->printed < st->max_print) {
        fprintf(stderr, "[dbg] hit tid=%d pc=%u line=%d\n",
                bpvm_thread_id(tc), pc, line);
        st->printed++;
    }
}

int main(int argc, char** argv) {
    /* stdout unbuffered para que un crash no se trague output. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    const char* path  = NULL;
    int   trace       = 0;
    int   debug_trace = 0;
    long  debug_print = 0;
    int   smp_workers = 0;        /* 0 = single-worker legacy */
    size_t mem_size   = 512 * 1024;
    const char* basedir = NULL;   /* H19-F1: raíz de proyecto (paths relativos) */

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (strcmp(a, "--trace") == 0)         { trace = 1; }
        else if (strcmp(a, "--debug-trace") == 0) {
            debug_trace = 1;
        }
        else if (strncmp(a, "--debug-trace=", 14) == 0) {
            debug_trace = 1;
            debug_print = strtol(a + 14, NULL, 10);
            if (debug_print < 0) debug_print = 0;
        }
        else if (strncmp(a, "--mem=", 6) == 0) {
            size_t n = parse_size(a + 6);
            if (n > 0) mem_size = n;
        }
        else if (strncmp(a, "--smp=", 6) == 0) {
            smp_workers = (int) strtol(a + 6, NULL, 10);
            if (smp_workers < 1) smp_workers = 1;
        }
        else if (strncmp(a, "--basedir=", 10) == 0) {
            basedir = a + 10;        /* H19-F1: raíz de proyecto (paths relativos) */
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
    bpvm_fs_register_host();   /* file I/O sobre libc (host) */
    if (basedir) bpvm_fs_set_basedir(basedir);   /* H19-F1: readFile/load relativos resuelven bajo la raíz */
    bpvm_net_register_host();  /* H11 — sockets TCP del SO (host) */

    debug_trace_state_t dbg_state = { 0, debug_print, 0 };
    if (debug_trace) {
        /* pc_to_line=NULL → modo sintético "todo opcode es una línea". */
        bpvm_set_debug_hook(vm, debug_trace_hook, NULL, &dbg_state);
    }

    bpvm_status_t s = bpvm_load_mod(vm, path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem);
        return (int) s;
    }

    if (smp_workers > 0) {
        printf("=== INICIANDO EJECUCION DE LA VM-C (SMP, workers=%d) ===\n",
               smp_workers);
        s = bpvm_run_smp(vm, smp_workers);
    } else {
        printf("=== INICIANDO EJECUCION DE LA VM-C ===\n");
        s = bpvm_run(vm);
    }
    {
        const char* le = bpvm_link_error(vm);   /* paso 4 — detalle de lib/símbolo no resuelto */
        if (le[0]) printf("=== ERROR DE LINK: %s ===\n", le);
    }
    printf("=== FIN DE LA EJECUCION (status=%s) ===\n", bpvm_status_str(s));

    if (debug_trace) {
        fprintf(stderr, "[dbg] total hook hits: %ld\n", dbg_state.total_hits);
    }

    bpvm_destroy(vm);
    free(mem);
    return (int) s;
}
