/*
 * platform_freertos_cond.inc — V7/H12, etapa 2b (5-oct): LA VARIABLE DE CONDICIÓN sobre FreeRTOS, UNA para las tres
 * familias. La incluyen, en lugar de la suya, src/platform_freertos.c (la STM32, y la Pico con
 * BPVM_PICO_PLATFORM_COMUN), pico/platform_freertos.c y esp32/common/platform_esp32.c, detrás de sus includes de
 * FreeRTOS ("FreeRTOS.h", "task.h", "semphr.h"; en ESP-IDF, los "freertos/..."). Es un .inc y no un .c para no dar de
 * alta un fichero en cinco builds: es texto que cada familia mete en el suyo.
 *
 * ─── POR QUÉ SE REHIZO ───────────────────────────────────────────────────────────────────────────────────────────
 *
 * FreeRTOS no tiene condición. La de antes (las tres copias, la misma idea) era la clásica: UN semáforo BINARIO y una
 * cuenta de esperadores. Con un solo esperador va bien, y hasta la 2b casi nunca había dos. Con dos runs esperando en la
 * cola, dos fallos, los dos de la misma raíz —el aviso es UNA ficha en una caja de una, sin dueño—:
 *   1. SE PIERDE UN AVISO. `broadcast` con dos esperando daba la ficha dos veces seguidas; la segunda no cabe (la caja
 *      ya tiene una: el primero aún no ha corrido para llevársela) y se pierde, pero la cuenta ya dice 0. El segundo
 *      se queda dormido para siempre: un KILL, o un programa nuevo, despierta a un run y el otro no.
 *   2. UN TERCERO SE LLEVA LA FICHA DE OTRO. Un aviso para A deja la ficha en la caja; si antes de que A corra llega C a
 *      esperar, la encuentra y se la lleva, y A sigue dormido.
 * Un semáforo CONTADOR arregla el 1 y no el 2. Lo que no falla es que cada aviso vaya A UNA TAREA CONCRETA.
 *
 * ─── CÓMO ES ─────────────────────────────────────────────────────────────────────────────────────────────────────
 *
 * Una LISTA de esperadores, por orden de llegada. Cada uno es un nodo en SU pila (no se reserva nada al esperar) con su
 * tarea, y espera en SU notificación de tarea, en un índice propio (BPVM_FR_COND_NOTIF). Avisar es, con el guardia
 * cogido: sacar al primero de la lista, marcarlo `avisado` y mandarle su notificación. Así:
 *   - no se pierde ningún aviso: cada uno tiene su notificación, que no se pisa con la de otro;
 *   - nadie se lleva el de otro: el aviso es para una tarea, no una ficha en la caja;
 *   - no quedan fichas sueltas: quien vence su plazo, con el guardia cogido, o se saca él de la lista —y ya nadie le
 *     avisará— o ve que le avisaron mientras tanto (la notificación ya está dada: se da con el guardia) y la consume.
 *     Tras cada espera la notificación queda a cero, siempre.
 * Cuesta un mutex (el guardia) por condición —antes, uno y un semáforo— y una vuelta más por el guardia al despertar.
 *
 * Necesita `configTASK_NOTIFICATION_ARRAY_ENTRIES >= 2` (el índice 0 es el de siempre: lo usan los stream buffers del
 * kernel y drivers de las familias, y una notificación ajena despertaría a quien espera aquí). En la Pico y la STM32,
 * en su FreeRTOSConfig.h; en ESP-IDF, CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES en su sdkconfig.
 *
 * La prueba: test/test_frcond.c (make test-frcond), con el kernel de FreeRTOS de verdad en el PC (su port de Windows),
 * y la condición de antes como control: tiene que salir ROJA, o la prueba no prueba nada.
 */

#ifndef BPVM_FR_COND_NOTIF
#define BPVM_FR_COND_NOTIF 1
#endif
#if !defined(configTASK_NOTIFICATION_ARRAY_ENTRIES) || (configTASK_NOTIFICATION_ARRAY_ENTRIES <= BPVM_FR_COND_NOTIF)
#error "La condicion de la VM (platform_freertos_cond.inc) necesita su indice de notificacion: configTASK_NOTIFICATION_ARRAY_ENTRIES >= 2 (en ESP-IDF, CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES)"
#endif

/* Un esperador: en la pila de quien espera, mientras espera. */
typedef struct fr_esperador {
    TaskHandle_t         tarea;
    struct fr_esperador* sig;
    int                  avisado;   /* lo pone quien avisa, con el guardia cogido */
} fr_esperador_t;

typedef struct {
    SemaphoreHandle_t guardia;      /* protege la lista */
    fr_esperador_t*   primero;      /* los que esperan, por orden de llegada */
    fr_esperador_t*   ultimo;
} fr_cond_t;

int bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c) {
    if (!c) return -1;
    fr_cond_t* cv = (fr_cond_t*) pvPortMalloc(sizeof(fr_cond_t));
    if (!cv) return -1;
    cv->guardia = xSemaphoreCreateMutex();
    cv->primero = NULL;
    cv->ultimo  = NULL;
    if (!cv->guardia) {
        vPortFree(cv);
        return -1;
    }
    *c = (void*) cv;
    return 0;
}

void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    vSemaphoreDelete(cv->guardia);
    vPortFree(cv);
    *c = NULL;
}

/* Saca a `w` de la lista, si está. Con el guardia cogido. */
static void fr_cond_quitar(fr_cond_t* cv, fr_esperador_t* w) {
    fr_esperador_t* ant = NULL;
    for (fr_esperador_t* p = cv->primero; p != NULL; ant = p, p = p->sig) {
        if (p != w) continue;
        if (ant) ant->sig = p->sig; else cv->primero = p->sig;
        if (cv->ultimo == p) cv->ultimo = ant;
        p->sig = NULL;
        return;
    }
}

/* Avisa al primero: lo saca de la lista, lo marca y le da SU notificación. 0 si no esperaba nadie. Con el guardia
 * cogido: así quien vence su plazo y coge el guardia, si ve `avisado`, sabe que la notificación ya está dada. */
static int fr_cond_avisar_uno(fr_cond_t* cv) {
    fr_esperador_t* w = cv->primero;
    if (w == NULL) return 0;
    cv->primero = w->sig;
    if (cv->primero == NULL) cv->ultimo = NULL;
    w->sig = NULL;
    w->avisado = 1;
    xTaskNotifyGiveIndexed(w->tarea, BPVM_FR_COND_NOTIF);
    return 1;
}

/* Esperar, con `m` cogido (lo suelta mientras espera y lo vuelve a coger). 0, avisado; 1, venció el plazo. */
static int fr_cond_esperar(fr_cond_t* cv, SemaphoreHandle_t m, TickType_t ticks) {
    fr_esperador_t w;
    w.tarea   = xTaskGetCurrentTaskHandle();
    w.sig     = NULL;
    w.avisado = 0;
    xSemaphoreTake(cv->guardia, portMAX_DELAY);
    if (cv->ultimo) cv->ultimo->sig = &w; else cv->primero = &w;
    cv->ultimo = &w;
    xSemaphoreGive(cv->guardia);
    /* Ya en la lista: soltar `m` no pierde nada —un aviso de ahora en adelante encuentra a éste—. */
    xSemaphoreGive(m);
    (void) ulTaskNotifyTakeIndexed(BPVM_FR_COND_NOTIF, pdTRUE, ticks);
    xSemaphoreTake(cv->guardia, portMAX_DELAY);
    int avisado = w.avisado;
    if (!avisado)
        fr_cond_quitar(cv, &w);   /* venció el plazo: fuera de la lista; ya nadie le avisará */
    else
        (void) ulTaskNotifyTakeIndexed(BPVM_FR_COND_NOTIF, pdTRUE, 0);   /* si le avisaron tras vencer, la notificación
                                                                          * está dada: se consume (sin fichas sueltas) */
    xSemaphoreGive(cv->guardia);
    xSemaphoreTake(m, portMAX_DELAY);
    return avisado ? 0 : 1;
}

void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m) {
    if (!c || !*c || !m || !*m) return;
    (void) fr_cond_esperar((fr_cond_t*) *c, (SemaphoreHandle_t) *m, portMAX_DELAY);
}

int bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms) {
    if (!c || !*c || !m || !*m) return 1;
    /* V6/A1.2 — REDONDEAR HACIA ARRIBA, NUNCA A CERO: `pdMS_TO_TICKS` es una división entera, y con un tick de 10 ms
     * cualquier espera menor daba 0 ticks, que no espera: sondea (el `io` de la C6 giraba en vacío; fib(28), de
     * 23 400 a 140 600 ms). Un tick es lo mínimo que este reloj sabe esperar. */
    TickType_t ticks = 0;
    if (ms > 0) {
        ticks = pdMS_TO_TICKS(ms);
        if (ticks == 0) ticks = 1;
    }
    return fr_cond_esperar((fr_cond_t*) *c, (SemaphoreHandle_t) *m, ticks);
}

void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guardia, portMAX_DELAY);
    (void) fr_cond_avisar_uno(cv);
    xSemaphoreGive(cv->guardia);
}

void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guardia, portMAX_DELAY);
    while (fr_cond_avisar_uno(cv)) { }
    xSemaphoreGive(cv->guardia);
}
