/*
 * test_oqentero.c — V7/H7 #600 (28-sep): lo que se empuja DE UNA VEZ a la cola de salida sale SEGUIDO.
 *
 * La línea de un `print` (lo que el hilo llevaba guardado, lo último y el salto) y un bloque de varias líneas
 * impreso con un solo `print` van a la cola como varios trozos (bpvm_oq_pushv). El push de antes copiaba lo que
 * cupiera, soltaba el cerrojo esperando sitio y seguía: con varios hilos empujando y la cola casi llena, otro
 * productor entraba en medio. Ahora, si el total cabe en la cola, se espera sitio para todo y se copia de una vez.
 *
 * Cuatro productores empujan registros de tres trozos —«Pn:seq:», una carga de largo variable y el salto— a una
 * cola PEQUEÑA (256 B) que un consumidor vacía a tragos de 64 B, así que casi siempre hay que esperar sitio.
 * Verde: cada línea es un registro entero y bien formado, y de cada productor están todos y en orden. (Un registro
 * MAYOR que la cola no puede salir entero —va por partes— y la prueba no los usa.)
 *
 * Salida: «OK: …» (la señal de check-testc) y exit 0; si no, las primeras líneas rotas y exit 1.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "comm_queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRODUCTORES 4
#define REGISTROS   3000
#define CAP         256

static bpvm_output_queue_t g_q;

static int largo_de(int id, int s) { return 5 + (s * 37 + id * 11) % 150; }   /* 5..154: cabe entero en 256 */
static char letra(int id, int k)   { return (char) ('a' + (id + k) % 26); }

static void* productor(void* raw) {
    int id = *(int*) raw;
    char cab[32];
    char carga[160];
    for (int s = 0; s < REGISTROS; s++) {
        int n = snprintf(cab, sizeof cab, "P%d:%d:", id, s);
        int largo = largo_de(id, s);
        for (int k = 0; k < largo; k++) carga[k] = letra(id, k);
        bpvm_trozo_t t[3];
        t[0].p = cab;   t[0].n = (size_t) n;
        t[1].p = carga; t[1].n = (size_t) largo;
        t[2].p = "\n";  t[2].n = 1;
        bpvm_oq_pushv(&g_q, t, 3);
    }
    return NULL;
}

static char*  g_todo;
static size_t g_len, g_cap;

static void* consumidor(void* raw) {
    (void) raw;
    char buf[64];
    for (;;) {
        size_t n = bpvm_oq_pop(&g_q, buf, sizeof buf);
        if (n == 0) break;
        if (g_len + n > g_cap) {
            g_cap = (g_cap + n) * 2;
            g_todo = (char*) realloc(g_todo, g_cap);
            if (!g_todo) { fprintf(stderr, "OOM\n"); exit(2); }
        }
        memcpy(g_todo + g_len, buf, n);
        g_len += n;
    }
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (bpvm_oq_init(&g_q, CAP) != 0) { fprintf(stderr, "bpvm_oq_init\n"); return 2; }
    pthread_t pc, pp[PRODUCTORES];
    int ids[PRODUCTORES];
    pthread_create(&pc, NULL, consumidor, NULL);
    for (int i = 0; i < PRODUCTORES; i++) { ids[i] = i; pthread_create(&pp[i], NULL, productor, &ids[i]); }
    for (int i = 0; i < PRODUCTORES; i++) pthread_join(pp[i], NULL);
    bpvm_oq_close(&g_q);
    pthread_join(pc, NULL);
    bpvm_oq_destroy(&g_q);

    int siguiente[PRODUCTORES] = {0};
    long lineas = 0, rotas = 0;
    size_t i0 = 0;
    for (size_t i = 0; i < g_len; i++) {
        if (g_todo[i] != '\n') continue;
        size_t L = i - i0;
        int ok = 0, id = -1, s = -1, n = 0;
        char linea[300];
        if (L < sizeof linea) {
            memcpy(linea, g_todo + i0, L);
            linea[L] = '\0';
            if (sscanf(linea, "P%d:%d:%n", &id, &s, &n) == 2 && n > 0 && id >= 0 && id < PRODUCTORES
                    && s == siguiente[id]) {
                int largo = largo_de(id, s);
                ok = ((int) L - n == largo);
                for (int k = 0; ok && k < largo; k++) ok = (linea[n + k] == letra(id, k));
            }
        }
        if (ok) siguiente[id]++;
        else if (rotas++ < 4) printf("  rota: %.*s\n", (int) (L < 80 ? L : 80), g_todo + i0);
        lineas++;
        i0 = i + 1;
    }
    int completos = 1;
    for (int p = 0; p < PRODUCTORES; p++) if (siguiente[p] != REGISTROS) completos = 0;
    printf("[oqentero] %ld lineas, %ld rotas; de cada productor: %d %d %d %d de %d\n", lineas, rotas,
           siguiente[0], siguiente[1], siguiente[2], siguiente[3], REGISTROS);
    free(g_todo);
    if (rotas == 0 && completos && i0 == g_len) {
        printf("OK: lo que se empuja de una vez a la cola de salida sale seguido, con %d productores y la cola casi "
               "llena\n", PRODUCTORES);
        return 0;
    }
    return 1;
}
