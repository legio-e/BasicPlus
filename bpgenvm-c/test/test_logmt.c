/*
 * test_logmt.c — #653 (4-oct): EL LOG CON DOS ESCRITORES.
 *
 * `append_raw` (src/bpvm_log.c) mira si la línea cabe y después copia y suma. Con dos tareas que escriben a la vez —en
 * las placas, `io` y `run` desde la etapa 1 de H12— una podía mirar que cabe, perder el turno, y copiar ya FUERA de la
 * región, encima de lo que hubiera detrás. El arreglo es la sección crítica de la cintura (entrar/salir).
 *
 * Aquí: una región pequeña (512 B, para que se llene y tire líneas sin parar: el camino del memmove) con una GUARDA
 * detrás, y dos hilos que escriben líneas de largo variable. Con la sección (un mutex en el PC; en las placas, las
 * interrupciones enmascaradas), la guarda queda intacta y lo usado no pasa de la región. Con --sin-seccion (el sabotaje:
 * la cintura sin entrar/salir, el log de antes), la guarda se pisa o lo usado se pasa —rojo—.
 *
 * Uso: test_logmt [--sin-seccion]. Sale 0 y escribe la línea «OK:» si todo está bien.
 */
#include "bpvm_log.h"
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REGION   512u
#define GUARDA   4096u
#define VUELTAS  50000

static uint8_t g_mem[REGION + GUARDA];
static pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;

static uint32_t entrar(void) { pthread_mutex_lock(&g_m); return 0u; }
static void     salir(uint32_t e) { (void) e; pthread_mutex_unlock(&g_m); }
/* La ventana ensanchada (sin ella dura unos nanosegundos y el sabotaje no se ve): ceder el turno entre «cabe» y
 * copiar. Con la sección, el otro hilo espera fuera; sin ella, entra en medio. */
static void entre(void) { sched_yield(); }

static void* escritor(void* arg) {
    int id = (int) (intptr_t) arg;
    for (int i = 0; i < VUELTAS; i++) {
        int largo = (i * 7 + id * 13) % 90;   /* de 0 a 89 caracteres, más el prefijo */
        log_printf("h%d %d %.*s", id, i, largo,
                   "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst");
    }
    return NULL;
}

int main(int argc, char** argv) {
    int sin_seccion = (argc > 1 && !strcmp(argv[1], "--sin-seccion"));
    memset(g_mem, 0xA5, sizeof g_mem);
    bpvm_log_cintura_t c;
    memset(&c, 0, sizeof c);
    c.region_buf  = g_mem;
    c.region_size = REGION;
    c.ensayo_entre = entre;
    if (!sin_seccion) {
        c.entrar = entrar;
        c.salir  = salir;
    }
    bpvm_log_init(&c);

    pthread_t a, b;
    pthread_create(&a, NULL, escritor, (void*) (intptr_t) 1);
    pthread_create(&b, NULL, escritor, (void*) (intptr_t) 2);
    pthread_join(a, NULL);
    pthread_join(b, NULL);

    uint32_t pisados = 0;
    for (uint32_t k = REGION; k < REGION + GUARDA; k++)
        if (g_mem[k] != 0xA5) pisados++;
    uint32_t usado = log_used_bytes(), total = log_total_bytes();
    int bien = (pisados == 0) && (usado <= total);
    printf("[logmt] dos escritores, %d líneas cada uno, en %u B%s: usado %u de %u, guarda pisada en %u B %s\n",
           VUELTAS, (unsigned) REGION, sin_seccion ? " (SIN la sección: el log de antes)" : "", (unsigned) usado,
           (unsigned) total, (unsigned) pisados, bien ? "PASS" : "FAIL");
    if (bien && !sin_seccion) printf("OK: #653, el log con dos escritores no se sale de su región\n");
    return bien ? 0 : 1;
}
