/*
 * test_palabras.c — V7/H12 (5-oct), #660 y #661: EL CAMINO FINO. Las tres piezas que impiden que un programa que comparte
 * memoria sin Mutex rompa la VM (docs/V7_IDEAS.md §H12 «El camino fino»). La regla de Eduardo: lo que LEE un programa
 * así es cosa suya —las propiedades y las listas se protegen con los Mutex de BP—; la VM y lo privado de cada hilo, son
 * nuestros.
 *
 *  1. (a) UNA PALABRA, DE UNA VEZ: un hilo escribe sin parar, con bpvm_write_u32_be y bpvm_write_u16_be, dos valores que
 *     no comparten ningún byte, en cuatro palabras alineadas; otro las lee con bpvm_read_u32_be/u16_be durante un
 *     segundo. Ninguna lectura a medias. Sabotaje (--sabotaje-bytes): el escritor, byte a byte como antes (`volatile`:
 *     que el compilador no los junte, como no los junta la Pico a -Os): lecturas a medias.
 *  2. (b) EL NÚMERO, DE VUELTA EN EL GC: con varios runs, soltar un objeto sube su generación en el acto (la referencia
 *     rancia muere) pero deja su dirección y no recicla su número: otra reserva no lo hereda, y el GC lo devuelve.
 *     Sabotaje (--sabotaje-reciclar): reciclado en el acto, como antes: la siguiente reserva lo hereda.
 *  3. (c) LA REFERENCIA QUE NO PUEDE EXISTIR: sin TAG y con la palabra alta distinta de 0 (a medio escribir por otro
 *     run), o con TAG y un índice fuera de la tabla: muerta («objeto eliminado»), no la dirección 0. Null, una constante
 *     y un handle vivo: vivos. Sabotaje (--sabotaje-desgarro): la regla de antes: la de a medias, viva.
 *
 * Sale 0, con la línea «OK:», si todo pasa.
 */

#include "bpvm_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int fallos = 0;

static void ok(const char* que, int cond) {
    if (cond) { printf("ok   %s\n", que); }
    else      { printf("FALLO %s\n", que); fallos++; }
}

/* ---- 1. (a) una palabra, de una vez ---- */

#define A32 0x11223344u   /* ningún byte en común con B32 */
#define B32 0xAABBCCDDu
#define A16 0x1122u
#define B16 0xAABBu

static uint32_t g_buf32[4] __attribute__((aligned(16)));
static uint16_t g_buf16[4] __attribute__((aligned(16)));
static volatile int g_parar = 0;
static int g_bytes = 0;   /* el sabotaje */

static void bytes_u32(uint8_t* p, uint32_t v) {
    volatile uint8_t* q = p;
    q[0] = (uint8_t) (v >> 24); q[1] = (uint8_t) (v >> 16); q[2] = (uint8_t) (v >> 8); q[3] = (uint8_t) v;
}
static void bytes_u16(uint8_t* p, uint16_t v) {
    volatile uint8_t* q = p;
    q[0] = (uint8_t) (v >> 8); q[1] = (uint8_t) v;
}

static void* escritor(void* arg) {
    (void) arg;
    uint8_t* p32 = (uint8_t*) g_buf32;
    uint8_t* p16 = (uint8_t*) g_buf16;
    unsigned long n = 0;
    while (!g_parar) {
        uint32_t v = (n & 1u) ? B32 : A32;
        uint16_t w = (uint16_t) ((n & 1u) ? B16 : A16);
        for (int k = 0; k < 4; k++) {
            if (g_bytes) { bytes_u32(p32 + 4 * k, v); bytes_u16(p16 + 2 * k, w); }
            else         { bpvm_write_u32_be(p32 + 4 * k, v); bpvm_write_u16_be(p16 + 2 * k, w); }
        }
        n++;
    }
    return NULL;
}

static void palabras(void) {
    uint8_t* p32 = (uint8_t*) g_buf32;
    uint8_t* p16 = (uint8_t*) g_buf16;
    for (int k = 0; k < 4; k++) { bpvm_write_u32_be(p32 + 4 * k, A32); bpvm_write_u16_be(p16 + 2 * k, A16); }
    pthread_t h;
    if (pthread_create(&h, NULL, escritor, NULL) != 0) { ok("1. el hilo escritor", 0); return; }
    unsigned long lecturas = 0, mal32 = 0, mal16 = 0, vistos_b = 0;
    clock_t fin = clock() + CLOCKS_PER_SEC;
    while (clock() < fin) {
        for (int r = 0; r < 4096; r++) {
            int k = r & 3;
            uint32_t v = bpvm_read_u32_be(p32 + 4 * k);
            uint16_t w = bpvm_read_u16_be(p16 + 2 * k);
            if (v != A32 && v != B32) mal32++;
            if (w != A16 && w != B16) mal16++;
            if (v == B32) vistos_b++;
            lecturas++;
        }
    }
    g_parar = 1;
    pthread_join(h, NULL);
    printf("     (%lu lecturas; %lu con el valor nuevo; a medias: %lu de 32 bits, %lu de 16)\n", lecturas, vistos_b,
           mal32, mal16);
    ok("1. el escritor escribía de verdad mientras se leía", vistos_b > 0 && vistos_b < lecturas);
    ok("1. ninguna palabra de 32 bits leída a medias", mal32 == 0);
    ok("1. ninguna de 16 bits leída a medias", mal16 == 0);
}

/* ---- 2. (b) el número, de vuelta en el GC; 3. (c) la referencia que no puede existir ---- */

static uint32_t indice(bpref_t r) { return (uint32_t) r.v & ~BPVM_HANDLE_TAG; }

static void numeros_y_rotas(int sab_reciclar, int sab_desgarro) {
    static uint8_t mem[64 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    if (!vm) { ok("2. bpvm_init", 0); return; }
    if (!bpvm_arena_reserve(vm, 256, 4)) { ok("2. la arena", 0); return; }
    bpvm_set_gc_enabled(vm, 0);
    /* VARIOS RUNS: la cola es la del programa (como la cuelga bpvm_cola_colgar con n_runs > 1). */
    if (bpvm_cola_asegurar() != 0) { ok("2. la cola", 0); return; }
    vm->smp = &bpvm_la_cola;
    bpvm_sabotaje_reciclar = sab_reciclar;
    bpvm_sabotaje_desgarro = sab_desgarro;

    bpref_t a = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, 16, 16);
    ok("2. una reserva", a.v != 0u);
    uint32_t ia = indice(a);
    uint32_t dir_a = vm->handles.addr[ia];
    ok("2. soltarlo lo mata", bpvm_handle_kill(vm, a) == 1);
    ok("2. y su referencia rancia muere en el acto", bpvm_ref_dead(vm, a) == 1);
    ok("2. pero su dirección sigue en la tabla hasta el GC", vm->handles.addr[ia] == dir_a && dir_a != 0u);
    bpref_t b = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, 16, 16);
    ok("2. y otra reserva NO hereda su número", b.v != 0u && indice(b) != ia);
    bpvm_handles_aplicar_pendientes(vm);   /* lo que hace el GC, con el mundo parado, antes de marcar */
    ok("2. el GC le pone la dirección a 0", vm->handles.addr[ia] == 0u);
    bpref_t c = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I8, 16, 16);
    ok("2. y después sí se recicla", c.v != 0u && indice(c) == ia);

    bpref_t r;
    r.v = ((uint64_t) 7u << 32) | 0u;
    ok("3. sin TAG y la palabra alta 7, la baja 0 (a medio escribir): muerta", bpvm_ref_dead(vm, r) == 1);
    r.v = ((uint64_t) 7u << 32) | 0x100u;
    ok("3. sin TAG y la palabra alta 7, la baja una constante: muerta", bpvm_ref_dead(vm, r) == 1);
    r.v = 0u;
    ok("3. null: viva", bpvm_ref_dead(vm, r) == 0);
    r.v = 0x100u;
    ok("3. una constante (la palabra alta 0): viva", bpvm_ref_dead(vm, r) == 0);
    ok("3. un handle vivo: vivo", bpvm_ref_dead(vm, b) == 0);
    r.v = ((uint64_t) 3u << 32) | (uint64_t) (BPVM_HANDLE_TAG | 0u);
    ok("3. con TAG y el índice 0: muerta", bpvm_ref_dead(vm, r) == 1);
    r.v = ((uint64_t) 3u << 32) | (uint64_t) (BPVM_HANDLE_TAG | (vm->handles.next + 5u));
    ok("3. con TAG y un índice fuera de la tabla: muerta", bpvm_ref_dead(vm, r) == 1);

    bpvm_sabotaje_reciclar = 0;
    bpvm_sabotaje_desgarro = 0;
    vm->smp = NULL;
    bpvm_destroy(vm);
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int sab_bytes = 0, sab_reciclar = 0, sab_desgarro = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--sabotaje-bytes")) sab_bytes = 1;
        else if (!strcmp(argv[i], "--sabotaje-reciclar")) sab_reciclar = 1;
        else if (!strcmp(argv[i], "--sabotaje-desgarro")) sab_desgarro = 1;
    }
    g_bytes = sab_bytes;
    palabras();
    numeros_y_rotas(sab_reciclar, sab_desgarro);
    if (fallos == 0)
        printf("OK: el camino fino (#660, #661): las palabras alineadas, de una vez; el número de un handle soltado con "
               "varios runs, de vuelta en el GC; y la referencia que no puede existir, muerta\n");
    return fallos ? 1 : 0;
}
