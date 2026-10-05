/*
 * test_frcond.c — V7/H12, etapa 2b (5-oct): LA VARIABLE DE CONDICIÓN DE FREERTOS, con el kernel de FreeRTOS de verdad.
 *
 * POR QUÉ EXISTE ESTE FICHERO
 * ---------------------------
 * La condición de la VM sobre FreeRTOS (la de las placas) era UN semáforo binario y una cuenta de esperadores. Con un
 * esperador va bien; con dos runs esperando en la cola (la 2b) pierde avisos: un `broadcast` despierta a uno y el otro se
 * queda dormido para siempre, y un tercero que llega a esperar justo después de un aviso se lo lleva. Eso NO SE VE EN
 * EL PC: la condición de pthreads no pierde avisos. Y en la placa sale como un cuelgue de vez en cuando. Por eso esta
 * prueba corre el kernel de FreeRTOS DE VERDAD en el PC (su port de Windows, portable/MSVC-MingW: tareas, prioridades,
 * semáforos y notificaciones de FreeRTOS, con un hilo de Windows por tarea y una sola corriendo a la vez, como en un
 * núcleo) contra la condición de las familias (src/platform_freertos_cond.inc, tal cual).
 *
 * Y con la de ANTES como control (`--sabotaje`): tiene que salir ROJA —si no, la prueba no prueba nada—.
 *
 * Cubre:
 *   1. broadcast con DOS esperando: despiertan los dos (la de antes: uno);
 *   2. un aviso para uno no se lo lleva un tercero que llega a esperar después (la de antes: se lo lleva, y el primero
 *      se queda dormido);
 *   3. el plazo vence cuando debe, y un aviso sin nadie esperando no deja nada para el siguiente;
 *   4. el estrés: dos consumidores y un productor, 3000 avisos sueltos y en ráfaga: todo se consume y nadie se queda
 *      dormido. 4a, con esperas con y sin plazo mezcladas (un plazo tapa un aviso perdido: la de antes pasa); 4b, sólo
 *      sin plazo, como esperan los runs en la cola (ahí, un aviso perdido es un run dormido para siempre);
 *   5. y tras el estrés, ninguna ficha suelta: una espera de 30 ms dura 30 ms, en los dos.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "bpvm_platform.h"

/* ── El mutex de la plataforma: el de las tres familias, igual ─────────────────────────────────────────────────── */

int bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m) {
    if (!m) return -1;
    SemaphoreHandle_t s = xSemaphoreCreateMutex();
    if (!s) return -1;
    *m = (void*) s;
    return 0;
}
void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    vSemaphoreDelete((SemaphoreHandle_t) *m);
    *m = NULL;
}
void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}
void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    xSemaphoreGive((SemaphoreHandle_t) *m);
}

/* ── LA NUEVA: la de las familias, tal cual ───────────────────────────────────────────────────────────────────── */

#include "platform_freertos_cond.inc"

/* ── LA DE ANTES, el control: la de pico/platform_freertos.c hasta el 5-oct (las otras dos copias, la misma idea) ── */

typedef struct {
    SemaphoreHandle_t sem;     /* binario, a 0 */
    SemaphoreHandle_t guard;   /* protege `waiters` */
    int waiters;
} vieja_cond_t;

static int vieja_init(bpvm_platform_cond_handle_t* c) {
    vieja_cond_t* cv = (vieja_cond_t*) pvPortMalloc(sizeof(vieja_cond_t));
    if (!cv) return -1;
    cv->sem = xSemaphoreCreateBinary();
    cv->guard = xSemaphoreCreateMutex();
    cv->waiters = 0;
    *c = (void*) cv;
    return (cv->sem && cv->guard) ? 0 : -1;
}
static void vieja_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m) {
    vieja_cond_t* cv = (vieja_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);
    xSemaphoreGive((SemaphoreHandle_t) *m);
    xSemaphoreTake(cv->sem, portMAX_DELAY);
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}
static int vieja_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms) {
    vieja_cond_t* cv = (vieja_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);
    xSemaphoreGive((SemaphoreHandle_t) *m);
    TickType_t ticks = 0;
    if (ms > 0) { ticks = pdMS_TO_TICKS(ms); if (ticks == 0) ticks = 1; }
    BaseType_t r = xSemaphoreTake(cv->sem, ticks);
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
    if (r != pdTRUE) {
        xSemaphoreTake(cv->guard, portMAX_DELAY);
        if (cv->waiters > 0) cv->waiters--;
        xSemaphoreGive(cv->guard);
        return 1;
    }
    return 0;
}
static void vieja_signal(bpvm_platform_cond_handle_t* c) {
    vieja_cond_t* cv = (vieja_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int wake = cv->waiters > 0;
    if (wake) cv->waiters--;
    xSemaphoreGive(cv->guard);
    if (wake) xSemaphoreGive(cv->sem);
}
static void vieja_broadcast(bpvm_platform_cond_handle_t* c) {
    vieja_cond_t* cv = (vieja_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int n = cv->waiters;
    cv->waiters = 0;
    xSemaphoreGive(cv->guard);
    for (int i = 0; i < n; i++) xSemaphoreGive(cv->sem);
}

/* ── La que se prueba ─────────────────────────────────────────────────────────────────────────────────────────── */

typedef struct {
    int  (*init)(bpvm_platform_cond_handle_t* c);
    void (*wait)(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m);
    int  (*timed_wait)(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms);
    void (*signal)(bpvm_platform_cond_handle_t* c);
    void (*broadcast)(bpvm_platform_cond_handle_t* c);
} cond_ops_t;

static const cond_ops_t NUEVA = { bpvm_platform_cond_init, bpvm_platform_cond_wait, bpvm_platform_cond_timed_wait,
                                  bpvm_platform_cond_signal, bpvm_platform_cond_broadcast };
static const cond_ops_t VIEJA = { vieja_init, vieja_wait, vieja_timed_wait, vieja_signal, vieja_broadcast };
static const cond_ops_t* C = &NUEVA;
static int sabotaje = 0;
static int fallos = 0;

#define PRIO_DIRECTOR (tskIDLE_PRIORITY + 3)
#define PRIO_ESPERA   (tskIDLE_PRIORITY + 2)
#define PILA          1024

static void resultado(const char* que, int bien, const char* detalle) {
    printf("[frcond] %s%s: %s %s\n", que, sabotaje ? " (LA DE ANTES)" : "", detalle, bien ? "PASS" : "FAIL");
    fflush(stdout);
    if (!bien) fallos++;
}

void vAssertCalled(unsigned long ulLine, const char* const pcFileName) {
    printf("[frcond] configASSERT en %s:%lu\n", pcFileName, ulLine);
    fflush(stdout);
    exit(3);
}

/* ── 1. broadcast con dos esperando ───────────────────────────────────────────────────────────────────────────── */

static bpvm_platform_mutex_handle_t m1;
static bpvm_platform_cond_handle_t  c1;
static volatile int e1_listos, e1_salir, e1_despiertos;

static void e1_esperador(void* p) {
    (void) p;
    bpvm_platform_mutex_lock(&m1);
    e1_listos++;
    while (!e1_salir) C->wait(&c1, &m1);
    e1_despiertos++;
    bpvm_platform_mutex_unlock(&m1);
    vTaskSuspend(NULL);
}

static void prueba_1(void) {
    char d[96];
    C->init(&c1);
    bpvm_platform_mutex_init(&m1);
    xTaskCreate(e1_esperador, "e1a", PILA, NULL, PRIO_ESPERA, NULL);
    xTaskCreate(e1_esperador, "e1b", PILA, NULL, PRIO_ESPERA, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));               /* los dos, dormidos en la condición */
    bpvm_platform_mutex_lock(&m1);
    int listos = e1_listos;
    e1_salir = 1;
    C->broadcast(&c1);
    bpvm_platform_mutex_unlock(&m1);
    vTaskDelay(pdMS_TO_TICKS(200));
    bpvm_platform_mutex_lock(&m1);
    int despiertos = e1_despiertos;
    bpvm_platform_mutex_unlock(&m1);
    snprintf(d, sizeof d, "%d esperando, %d despiertos", listos, despiertos);
    resultado("1. broadcast con dos esperando: despiertan los dos", listos == 2 && despiertos == 2, d);
}

/* ── 2. el aviso de uno no se lo lleva otro ───────────────────────────────────────────────────────────────────── */

static bpvm_platform_mutex_handle_t m2;
static bpvm_platform_cond_handle_t  c2;
static volatile int e2_dato, e2_ok;

static void e2_esperador(void* p) {
    (void) p;
    bpvm_platform_mutex_lock(&m2);
    while (!e2_dato) C->wait(&c2, &m2);
    e2_dato = 0;
    e2_ok = 1;
    bpvm_platform_mutex_unlock(&m2);
    vTaskSuspend(NULL);
}

static void prueba_2(void) {
    char d[96];
    C->init(&c2);
    bpvm_platform_mutex_init(&m2);
    xTaskCreate(e2_esperador, "e2", PILA, NULL, PRIO_ESPERA, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));               /* dormido en la condición */
    bpvm_platform_mutex_lock(&m2);
    e2_dato = 1;
    C->signal(&c2);                              /* el aviso es para él... */
    int r = C->timed_wait(&c2, &m2, 100);        /* ...y el director llega a esperar después: no es para él */
    bpvm_platform_mutex_unlock(&m2);
    vTaskDelay(pdMS_TO_TICKS(100));
    bpvm_platform_mutex_lock(&m2);
    int ok = e2_ok;
    bpvm_platform_mutex_unlock(&m2);
    snprintf(d, sizeof d, "el avisado %s; el que llegó después, %s", ok ? "despierta" : "SIGUE DORMIDO",
             r == 1 ? "vence su plazo" : "SE LLEVA EL AVISO");
    resultado("2. un aviso para uno no se lo lleva otro que llega despues", ok == 1 && r == 1, d);
}

/* ── 3. el plazo, y nada suelto ───────────────────────────────────────────────────────────────────────────────── */

static void prueba_3(void) {
    char d[128];
    bpvm_platform_mutex_handle_t m3;
    bpvm_platform_cond_handle_t  c3;
    C->init(&c3);
    bpvm_platform_mutex_init(&m3);
    bpvm_platform_mutex_lock(&m3);
    TickType_t t0 = xTaskGetTickCount();
    int r1 = C->timed_wait(&c3, &m3, 20);
    TickType_t t1 = xTaskGetTickCount();
    bpvm_platform_mutex_unlock(&m3);
    C->signal(&c3);                              /* nadie espera: no tiene que quedar nada */
    C->broadcast(&c3);
    bpvm_platform_mutex_lock(&m3);
    TickType_t t2 = xTaskGetTickCount();
    int r2 = C->timed_wait(&c3, &m3, 20);
    TickType_t t3 = xTaskGetTickCount();
    bpvm_platform_mutex_unlock(&m3);
    snprintf(d, sizeof d, "sin aviso, %s a los %lu ms; tras avisos sin nadie, %s a los %lu ms",
             r1 == 1 ? "vence" : "AVISADO", (unsigned long) (t1 - t0), r2 == 1 ? "vence" : "AVISADO",
             (unsigned long) (t3 - t2));
    resultado("3. el plazo vence; un aviso sin nadie esperando no deja nada", r1 == 1 && (t1 - t0) >= 20 && r2 == 1
              && (t3 - t2) >= 20, d);
}

/* ── 4 y 5. el estrés, y ninguna ficha suelta ─────────────────────────────────────────────────────────────────── */

#define AVISOS 3000

static bpvm_platform_mutex_handle_t m4;
static bpvm_platform_cond_handle_t  c4;
static volatile int e4_cola, e4_fin, e4_sin_plazo;
static volatile int e4_consumidos[2], e4_terminado[2], e4_suelta[2];
static volatile unsigned long e4_espera_final[2];

static void e4_consumidor(void* p) {
    int id = (int) (intptr_t) p;
    uint32_t s = 2654435761u * (uint32_t) (id + 1);
    bpvm_platform_mutex_lock(&m4);
    for (;;) {
        while (e4_cola == 0 && !e4_fin) {
            s = s * 1103515245u + 12345u;
            /* 4a: esperas con y sin plazo mezcladas; 4b: sólo sin plazo, como esperan los runs en la cola (un aviso
             * perdido no lo arregla ningún plazo) */
            if (e4_sin_plazo || ((s >> 16) & 1u)) C->wait(&c4, &m4);
            else (void) C->timed_wait(&c4, &m4, 1 + (int) ((s >> 17) & 3u));
        }
        if (e4_cola == 0) break;                  /* e4_fin y nada que consumir */
        e4_cola--;
        e4_consumidos[id]++;
        if ((s >> 20) & 1u) {                     /* a veces suelta el cerrojo y cede: más cruces */
            bpvm_platform_mutex_unlock(&m4);
            taskYIELD();
            bpvm_platform_mutex_lock(&m4);
        }
    }
    /* 5. Nadie va a avisar ya: una espera de 30 ms tiene que durar 30 ms (una ficha suelta la acortaría). */
    TickType_t t0 = xTaskGetTickCount();
    int r = C->timed_wait(&c4, &m4, 30);
    e4_espera_final[id] = (unsigned long) (xTaskGetTickCount() - t0);
    e4_suelta[id] = (r != 1) || (e4_espera_final[id] < 30);
    e4_terminado[id] = 1;
    bpvm_platform_mutex_unlock(&m4);
    vTaskSuspend(NULL);
}

static void prueba_4_5(int sin_plazo) {
    char d[160];
    char q4[96], q5[64];
    snprintf(q4, sizeof q4, "4%s. estres: dos consumidores, avisos sueltos y en rafaga, %s", sin_plazo ? "b" : "a",
             sin_plazo ? "esperas sin plazo" : "esperas con y sin plazo");
    snprintf(q5, sizeof q5, "5%s. tras el estres, ninguna ficha suelta", sin_plazo ? "b" : "a");
    C->init(&c4);
    bpvm_platform_mutex_init(&m4);
    e4_cola = 0; e4_fin = 0; e4_sin_plazo = sin_plazo;
    for (int k = 0; k < 2; k++) { e4_consumidos[k] = 0; e4_terminado[k] = 0; e4_suelta[k] = 0; e4_espera_final[k] = 0; }
    /* A la prioridad del director: se reparten el núcleo por rodajas de un tick, y se cruzan en cualquier sitio. */
    xTaskCreate(e4_consumidor, "e4a", PILA, (void*) (intptr_t) 0, PRIO_DIRECTOR, NULL);
    xTaskCreate(e4_consumidor, "e4b", PILA, (void*) (intptr_t) 1, PRIO_DIRECTOR, NULL);
    for (int i = 0; i < AVISOS; i++) {
        bpvm_platform_mutex_lock(&m4);
        e4_cola++;
        if (i % 7 == 0) C->broadcast(&c4); else C->signal(&c4);
        bpvm_platform_mutex_unlock(&m4);
        if (i % 3 == 0) taskYIELD();
        if (i % 50 == 0) vTaskDelay(1);
    }
    bpvm_platform_mutex_lock(&m4);
    e4_fin = 1;
    C->broadcast(&c4);
    bpvm_platform_mutex_unlock(&m4);
    int espera = 0;
    while (!(e4_terminado[0] && e4_terminado[1]) && espera < 5000) { vTaskDelay(pdMS_TO_TICKS(10)); espera += 10; }
    int total = e4_consumidos[0] + e4_consumidos[1];
    snprintf(d, sizeof d, "consumidos %d de %d (%d y %d); terminados %d y %d%s", total, AVISOS, e4_consumidos[0],
             e4_consumidos[1], e4_terminado[0], e4_terminado[1],
             (e4_terminado[0] && e4_terminado[1]) ? "" : " (alguno, DORMIDO tras 5 s)");
    resultado(q4, total == AVISOS && e4_terminado[0] && e4_terminado[1], d);
    if (e4_terminado[0] && e4_terminado[1]) {
        snprintf(d, sizeof d, "la espera final de 30 ms duro %lu y %lu ms", e4_espera_final[0], e4_espera_final[1]);
        resultado(q5, !e4_suelta[0] && !e4_suelta[1], d);
    } else {
        resultado(q5, 0, "no se puede mirar: alguno no termino");
    }
}

static void director(void* p) {
    (void) p;
    prueba_1();
    prueba_2();
    prueba_3();
    prueba_4_5(0);
    prueba_4_5(1);
    printf("%s: la condicion de FreeRTOS (el kernel de verdad, su port de Windows)%s\n", fallos ? "FAIL" : "OK",
           sabotaje ? " — LA DE ANTES" : ": ni un aviso perdido, ni uno para otro, ni una ficha suelta");
    fflush(stdout);
    exit(fallos ? 1 : 0);
}

int main(int argc, char** argv) {
    sabotaje = (argc > 1 && strcmp(argv[1], "--sabotaje") == 0);
    if (sabotaje) C = &VIEJA;
    xTaskCreate(director, "director", 4 * PILA, NULL, PRIO_DIRECTOR, NULL);
    vTaskStartScheduler();
    printf("FAIL: el planificador de FreeRTOS no arranco\n");
    return 2;
}
