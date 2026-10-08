/*
 * test_destruirpoca.c — V7 #668 con POCA memoria de plataforma (8-oct; la ronda de V7 en la Pico 2): soltar un árbol de
 * owner grande no puede pedir memoria por nodo. En la Pico 2, OwnerHondo —una cadena de 4000 owners SIN destructores—
 * moría con «No space in heap»: para saber si el árbol tenía destructores, __liberaSiSinDestructores lo recorría con un
 * conjunto de vistos (32 KB para 4000, 48 mientras crecía) que su malloc no tenía, y lo «prudente» era ordenarlo, que
 * pedía más. Aquí, con bpvm_alloc_prueba_tope("destruir.c", 4096) —toda reserva de destruir.c de más de 4 KB falla,
 * como en una placa con la memoria troceada—:
 *
 *   Fase 1 — OwnerHondo.mod, sin un solo destructor: sin mirar nada (no hay nada vivo con destructor), hasta «fin».
 *   Fase 2 — OwnerHondoDtor.mod, las mismas formas con un vigía vivo CON destructor: hay que mirar los árboles, y la
 *            mirada va con la pila de su recorrido (un cursor por array), no con memoria por nodo. Hasta «fin», con los
 *            testigos muertos (la cascada llegó al fondo) y «~Vigia» al final.
 *   Control — las dos sin tope: lo mismo.
 *
 * Salida: una línea «ok …» por comprobación y [status=OK]; si algo falla, «FAIL …» y [status=FAIL].
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "bpvm_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_salida[16384];
static size_t g_n;
static int fallos;

static void salida(const char* s, size_t len, void* user) {
    (void) user;
    if (g_n + len < sizeof g_salida) { memcpy(g_salida + g_n, s, len); g_n += len; g_salida[g_n] = '\0'; }
}

static void comprueba(int cond, const char* que) {
    if (cond) printf("ok  %s\n", que);
    else { printf("FAIL %s\n", que); fallos++; }
}

static int tiene(const char* linea) { return strstr(g_salida, linea) != NULL; }

/* Corre `mod` con el tope de destruir.c (0: sin tope). Devuelve el estado del RUN; la salida, en g_salida. */
static int correr(const char* mod, size_t tope) {
    g_n = 0; g_salida[0] = '\0';
    size_t mem_size = 2u * 1024u * 1024u;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { printf("FAIL sin memoria\n"); return -1; }
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { printf("FAIL bpvm_init\n"); free(mem); return -1; }
    bpvm_set_output(vm, salida, NULL);
    bpvm_status_t s = bpvm_load_mod(vm, mod);
    if (s != BPVM_OK) { printf("FAIL load_mod %s: %s\n", mod, bpvm_status_str(s)); bpvm_destroy(vm); free(mem); return -1; }
    if (tope) bpvm_alloc_prueba_tope("destruir.c", tope);
    s = bpvm_run(vm);
    bpvm_alloc_prueba_tope(NULL, 0);
    bpvm_destroy(vm);
    free(mem);
    return (int) s;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* hondo = (argc > 1) ? argv[1] : "OwnerHondo.mod";
    const char* dtor  = (argc > 2) ? argv[2] : "OwnerHondoDtor.mod";
    const size_t TOPE = 4096;

    printf("--- fase 1: %s con destruir.c a %u B como mucho por reserva (sin destructores: no se mira nada) ---\n",
           hondo, (unsigned) TOPE);
    int rs = correr(hondo, TOPE);
    comprueba(rs == BPVM_OK, "fase 1: el programa acaba bien (antes de arreglarlo: «No space in heap»)");
    comprueba(tiene("fin\n"), "fase 1: hasta «fin»");
    comprueba(tiene("honda, muertos: 3 de 3") && tiene("ancha, muertos: 3 de 3"), "fase 1: la cascada llega al fondo");

    printf("--- fase 2: %s con el mismo tope (un vigía vivo con destructor: hay que mirar los árboles) ---\n", dtor);
    rs = correr(dtor, TOPE);
    comprueba(rs == BPVM_OK, "fase 2: el programa acaba bien");
    comprueba(tiene("honda, muertos: 2 de 2"), "fase 2: la cadena de 4000, mirada y soltada entera");
    comprueba(tiene("ancha, muertos: 2 de 2"), "fase 2: el array de 600 con sus hijos, mirado (con un cursor) y soltado");
    {
        const char* f = strstr(g_salida, "fin\n");
        comprueba(f != NULL && strstr(f, "~Vigia") != NULL, "fase 2: «fin» y, al acabar el main, «~Vigia»");
    }

    printf("--- control: las dos sin tope ---\n");
    rs = correr(hondo, 0);
    comprueba(rs == BPVM_OK && tiene("fin\n"), "control: OwnerHondo, hasta «fin»");
    rs = correr(dtor, 0);
    comprueba(rs == BPVM_OK && tiene("~Vigia"), "control: OwnerHondoDtor, hasta «~Vigia»");

    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
