/*
 * test_logmt.c — #653 (4-oct): EL LOG CON DOS ESCRITORES. Y #658 (5-oct): EL VOLCADO CON DOS A LA VEZ.
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
 * Con --volcado, #658: a los dos escritores se suman dos hilos que VUELCAN sin parar, contra una flash de mentira que
 * borra, cede el turno (la ventana entre borrar y grabar, que en la Pico son dos) y graba como una de verdad —sólo baja
 * bits: grabar encima de otra imagen deja el AND de las dos—, con lo que se añade parado mientras graba (en la placa,
 * las interrupciones apagadas y el otro núcleo aparcado). Cada imagen grabada se mira: la cabecera bien, y lo usado,
 * líneas enteras de texto (sin 0xFF, que es el relleno, ni bytes del AND). Verde: ningún volcado encima de otro, todas
 * las imágenes buenas, y un volcado al final deja en la flash lo mismo que en la RAM. Con --volcado --de-antes (el
 * sabotaje: el volcado de antes de #658, sin la bandera de uno a la vez y con el relleno fuera de la sección) —rojo—.
 *
 * Uso: test_logmt [--sin-seccion] | --volcado [--de-antes]. Sale 0 y escribe la línea «OK:» si todo está bien.
 */
#include "bpvm_log.h"
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REGION    512u
#define GUARDA    4096u
#define VUELTAS   50000
#define VUELTAS_V 20000      /* las líneas de cada escritor con --volcado (los volcados ceden el turno a menudo) */
#define LOG_MAGIC 0x4C4F4731u  /* la cabecera de src/bpvm_log.c: magic, versión, usado y el anillo */
#define LOG_VER   1u
#define LOG_HDR   16u

static uint8_t g_mem[REGION + GUARDA];
static pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;

static uint32_t entrar(void) { pthread_mutex_lock(&g_m); return 0u; }
static void     salir(uint32_t e) { (void) e; pthread_mutex_unlock(&g_m); }
/* La ventana ensanchada (sin ella dura unos nanosegundos y el sabotaje no se ve): ceder el turno entre «cabe» y
 * copiar. Con la sección, el otro hilo espera fuera; sin ella, entra en medio. */
static void entre(void) { sched_yield(); }

static int g_vueltas = VUELTAS;

static void* escritor(void* arg) {
    int id = (int) (intptr_t) arg;
    for (int i = 0; i < g_vueltas; i++) {
        int largo = (i * 7 + id * 13) % 90;   /* de 0 a 89 caracteres, más el prefijo */
        log_printf("h%d %d %.*s", id, i, largo,
                   "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst");
    }
    return NULL;
}

/* ─── #658: la flash de mentira ─── */
static uint8_t g_flash[REGION];
static int g_activos, g_solapes, g_malas, g_volcados;
static volatile int g_fin;

/* ¿Una imagen buena? La cabecera, y lo usado: líneas enteras de texto («[ms] h…»), sin 0xFF ni otro byte raro. */
static int imagen_buena(const uint8_t* img, uint32_t len) {
    uint32_t magic, ver, usado;
    memcpy(&magic, img + 0, 4);
    memcpy(&ver,   img + 4, 4);
    memcpy(&usado, img + 8, 4);
    if (magic != LOG_MAGIC || ver != LOG_VER || usado > len - LOG_HDR) return 0;
    const uint8_t* d = img + LOG_HDR;
    if (usado > 0 && d[usado - 1] != '\n') return 0;
    int principio = 1;
    for (uint32_t k = 0; k < usado; k++) {
        uint8_t b = d[k];
        if (principio && b != '[') return 0;
        if (b == '\n') { principio = 1; continue; }
        if (b < 0x20 || b > 0x7E) return 0;
        principio = 0;
    }
    return 1;
}

static int flash_write(const uint8_t* src, uint32_t len) {
    if (__atomic_add_fetch(&g_activos, 1, __ATOMIC_SEQ_CST) > 1) __atomic_add_fetch(&g_solapes, 1, __ATOMIC_SEQ_CST);
    memset(g_flash, 0xFF, len);                    /* borrar */
    sched_yield();                                 /* entre las dos ventanas */
    pthread_mutex_lock(&g_m);                      /* grabar: con lo que se añade, parado */
    for (uint32_t k = 0; k < len; k++) g_flash[k] &= src[k];   /* la flash sólo baja bits */
    int mala = !imagen_buena(g_flash, len);
    pthread_mutex_unlock(&g_m);
    if (mala) __atomic_add_fetch(&g_malas, 1, __ATOMIC_SEQ_CST);
    __atomic_add_fetch(&g_volcados, 1, __ATOMIC_SEQ_CST);
    __atomic_sub_fetch(&g_activos, 1, __ATOMIC_SEQ_CST);
    sched_yield();
    return 0;
}

static void* volcador(void* arg) {
    (void) arg;
    while (!__atomic_load_n(&g_fin, __ATOMIC_SEQ_CST)) { log_flush(); sched_yield(); }
    return NULL;
}

static int volcado(int de_antes) {
    bpvm_sabotaje_volcado = de_antes;
    g_vueltas = VUELTAS_V;
    pthread_t a, b, v1, v2;
    pthread_create(&v1, NULL, volcador, NULL);
    pthread_create(&v2, NULL, volcador, NULL);
    pthread_create(&a, NULL, escritor, (void*) (intptr_t) 1);
    pthread_create(&b, NULL, escritor, (void*) (intptr_t) 2);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    __atomic_store_n(&g_fin, 1, __ATOMIC_SEQ_CST);
    pthread_join(v1, NULL);
    pthread_join(v2, NULL);

    /* Lo último: un volcado a solas deja en la flash lo que hay en la RAM (la cabecera entera y lo usado). */
    log_flush();
    uint32_t usado = log_used_bytes();
    int igual = !memcmp(g_flash, g_mem, LOG_HDR + usado);
    int ram_buena = imagen_buena(g_mem, REGION);
    uint32_t pisados = 0;
    for (uint32_t k = REGION; k < REGION + GUARDA; k++)
        if (g_mem[k] != 0xA5) pisados++;

    int bien = g_solapes == 0 && g_malas == 0 && igual && ram_buena && pisados == 0 && g_volcados > 1;
    printf("[logmt] #658: dos escritores (%d líneas cada uno) y dos volcadores%s: %d volcados, %d encima de otro, "
           "%d imágenes malas; el último, %s; la RAM, %s; guarda pisada en %u B %s\n",
           VUELTAS_V, de_antes ? " (el volcado de ANTES)" : "", g_volcados, g_solapes, g_malas,
           igual ? "igual que la RAM" : "DISTINTO de la RAM", ram_buena ? "bien" : "MAL", (unsigned) pisados,
           bien ? "PASS" : "FAIL");
    if (bien && !de_antes) printf("OK: #658, el volcado del log, de uno en uno y sin pisar lo que se añade\n");
    return bien ? 0 : 1;
}

int main(int argc, char** argv) {
    int sin_seccion = (argc > 1 && !strcmp(argv[1], "--sin-seccion"));
    int con_volcado = (argc > 1 && !strcmp(argv[1], "--volcado"));
    int de_antes    = (argc > 2 && !strcmp(argv[2], "--de-antes"));
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
    if (con_volcado) c.flash_write = flash_write;
    bpvm_log_init(&c);
    if (con_volcado) return volcado(de_antes);

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
