/*
 * platform_freertos.c — backend de bpvm_platform.h sobre FreeRTOS.
 *
 * Equivalente a platform_pthread.c para target embebido. Implementa
 * mutex, condvar, thread, sleep y now_ms.
 *
 * Notas:
 *  - F4 v1 (scheduler cooperativo dentro de un único task FreeRTOS) no
 *    necesita thread_create: la VM ejecuta los "threads BP" en su propio
 *    scheduler round-robin. Aún así implementamos la API por compleción
 *    (y para que F4 v2 = un task FreeRTOS por thread BP, futuro, no
 *    requiera reescribir).
 *  - Cada handle opaco es un puntero malloc'd que envuelve el objeto
 *    nativo FreeRTOS — mismo enfoque que platform_pthread.c.
 */

#include "bpvm_platform.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdlib.h>
#include <string.h>

/* ============================== Mutex ============================ */

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

/* ============================== Condvar ==========================
 * FreeRTOS no tiene condvar nativa. Implementación clásica con un
 * binary semaphore y un contador de waiters. El caller mantiene `m`
 * tomado al entrar; lo liberamos, esperamos a la señal, y lo
 * re-tomamos antes de salir.
 *
 * Esta impl es suficiente para "signal-one-of-many" estilo
 * condvar de pthread. Si N waiters hay y se hace broadcast,
 * desbloqueamos en cascada con signals encadenados.
 */
typedef struct {
    SemaphoreHandle_t sem;       /* binary, inicial 0 */
    SemaphoreHandle_t guard;     /* protege `waiters` */
    int waiters;
} fr_cond_t;

int bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c) {
    if (!c) return -1;
    fr_cond_t* cv = (fr_cond_t*) pvPortMalloc(sizeof(fr_cond_t));
    if (!cv) return -1;
    cv->sem = xSemaphoreCreateBinary();
    cv->guard = xSemaphoreCreateMutex();
    cv->waiters = 0;
    if (!cv->sem || !cv->guard) {
        if (cv->sem)   vSemaphoreDelete(cv->sem);
        if (cv->guard) vSemaphoreDelete(cv->guard);
        vPortFree(cv);
        return -1;
    }
    *c = (void*) cv;
    return 0;
}

void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    vSemaphoreDelete(cv->sem);
    vSemaphoreDelete(cv->guard);
    vPortFree(cv);
    *c = NULL;
}

void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c,
                              bpvm_platform_mutex_handle_t* m) {
    if (!c || !*c || !m || !*m) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);

    /* Suelta el mutex del caller. */
    xSemaphoreGive((SemaphoreHandle_t) *m);

    /* Espera la señal. */
    xSemaphoreTake(cv->sem, portMAX_DELAY);

    /* Re-toma el mutex. */
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}

int bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c,
                                   bpvm_platform_mutex_handle_t* m, int ms) {
    if (!c || !*c || !m || !*m) return 1;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);

    xSemaphoreGive((SemaphoreHandle_t) *m);

    TickType_t ticks = (ms > 0) ? pdMS_TO_TICKS(ms) : 0;
    BaseType_t r = xSemaphoreTake(cv->sem, ticks);

    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);

    if (r != pdTRUE) {
        /* Timeout: si nadie nos había contado todavía, decrementar. */
        xSemaphoreTake(cv->guard, portMAX_DELAY);
        if (cv->waiters > 0) cv->waiters--;
        xSemaphoreGive(cv->guard);
        return 1;
    }
    return 0;
}

void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int wake = cv->waiters > 0;
    if (wake) cv->waiters--;
    xSemaphoreGive(cv->guard);
    if (wake) xSemaphoreGive(cv->sem);
}

void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int n = cv->waiters;
    cv->waiters = 0;
    xSemaphoreGive(cv->guard);
    for (int i = 0; i < n; i++) xSemaphoreGive(cv->sem);
}

/* ============================== Thread ===========================
 * Wrapper para xTaskCreate. F4 v1 no llama a thread_create — la VM
 * corre todo en el task que invocó bpvm_run. Aquí estamos para
 * cuando portemos F4 v2 (un task por thread BP).
 */

typedef struct {
    TaskHandle_t        task;
    SemaphoreHandle_t   exited;     /* binary, 0 hasta que entry retorne */
    bpvm_thread_entry_t entry;
    void*               arg;
} fr_thread_t;

static void fr_thread_trampoline(void* pv) {
    fr_thread_t* t = (fr_thread_t*) pv;
    t->entry(t->arg);
    /* Marcar como terminado para join. */
    if (t->exited) xSemaphoreGive(t->exited);
    /* El task se borra a sí mismo. join libera la estructura. */
    vTaskDelete(NULL);
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                 bpvm_thread_entry_t entry, void* arg) {
    if (!t || !entry) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry = entry;
    ft->arg = arg;
    BaseType_t r = xTaskCreate(fr_thread_trampoline, "bpvm-thread",
                                1024,   /* words → 4 KB stack */
                                ft, tskIDLE_PRIORITY + 1, &ft->task);
    if (r != pdPASS) {
        vSemaphoreDelete(ft->exited);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
}

void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) {
    if (!t || !*t) return;
    fr_thread_t* ft = (fr_thread_t*) *t;
    xSemaphoreTake(ft->exited, portMAX_DELAY);
    vSemaphoreDelete(ft->exited);
    vPortFree(ft);
    *t = NULL;
}

void bpvm_platform_thread_yield(void) {
    taskYIELD();
}

void bpvm_platform_thread_sleep_ms(int ms) {
    if (ms <= 0) { taskYIELD(); return; }
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* ============================== Time =========================== */

int64_t bpvm_platform_now_ms(void) {
    TickType_t ticks = xTaskGetTickCount();
    /* 1 tick = 1 ms con configTICK_RATE_HZ=1000. Tipado explícito a
     * 64 bits para no envolver. */
    return (int64_t) ticks * (int64_t) portTICK_PERIOD_MS;
}
