/*
 * platform_esp32.c — backend de bpvm_platform.h sobre ESP-IDF (Xtensa).
 *
 * Hermano de platform_freertos.c (Pico) y platform_pthread.c (host).
 * Es ~idéntico al de Pico: ESP-IDF usa FreeRTOS, así que mutex/condvar/
 * thread son las mismas APIs. Sólo cambian:
 *   - now_ms      : esp_timer_get_time() (µs monotónicos, 64-bit) en vez
 *                   de time_us_64() del SDK Pico.
 *   - busy_wait_us: esp_rom_delay_us() en vez de busy_wait_us() del SDK.
 *   - sin flash_lock (#153 era RP2350-specific; ESP-IDF gestiona el SMP
 *     y el acceso a flash por su cuenta).
 *   - el stack de xTaskCreate* en ESP-IDF va en BYTES (en Pico/vanilla
 *     FreeRTOS va en WORDS) → usamos 4096 bytes.
 *
 * Cabeceras FreeRTOS bajo el prefijo "freertos/" (convención ESP-IDF).
 */

#include "bpvm_platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_timer.h"     /* esp_timer_get_time → now_ms */
#include "esp_rom_sys.h"   /* esp_rom_delay_us  → busy_wait_us */

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
 * Igual que el backend Pico: binary semaphore + contador de waiters
 * protegido por un guard mutex. */
typedef struct {
    SemaphoreHandle_t sem;
    SemaphoreHandle_t guard;
    int waiters;
} es_cond_t;

int bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c) {
    if (!c) return -1;
    es_cond_t* cv = (es_cond_t*) pvPortMalloc(sizeof(es_cond_t));
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
    es_cond_t* cv = (es_cond_t*) *c;
    vSemaphoreDelete(cv->sem);
    vSemaphoreDelete(cv->guard);
    vPortFree(cv);
    *c = NULL;
}

void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c,
                              bpvm_platform_mutex_handle_t* m) {
    if (!c || !*c || !m || !*m) return;
    es_cond_t* cv = (es_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);

    xSemaphoreGive((SemaphoreHandle_t) *m);
    xSemaphoreTake(cv->sem, portMAX_DELAY);
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}

int bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c,
                                   bpvm_platform_mutex_handle_t* m, int ms) {
    if (!c || !*c || !m || !*m) return 1;
    es_cond_t* cv = (es_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);

    xSemaphoreGive((SemaphoreHandle_t) *m);

    TickType_t ticks = (ms > 0) ? pdMS_TO_TICKS(ms) : 0;
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

void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    es_cond_t* cv = (es_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int wake = cv->waiters > 0;
    if (wake) cv->waiters--;
    xSemaphoreGive(cv->guard);
    if (wake) xSemaphoreGive(cv->sem);
}

void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    es_cond_t* cv = (es_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int n = cv->waiters;
    cv->waiters = 0;
    xSemaphoreGive(cv->guard);
    for (int i = 0; i < n; i++) xSemaphoreGive(cv->sem);
}

/* ============================== Thread ===========================
 * ESP-IDF FreeRTOS es SMP (2 cores en el S3). xTaskCreatePinnedToCore
 * con tskNO_AFFINITY = sin pin (cualquier core). El stack va en BYTES. */

#define BPVM_ESP32_THREAD_STACK_BYTES 4096

typedef struct {
    TaskHandle_t        task;
    SemaphoreHandle_t   exited;
    bpvm_thread_entry_t entry;
    void*               arg;
} es_thread_t;

static void es_thread_trampoline(void* pv) {
    es_thread_t* t = (es_thread_t*) pv;
    t->entry(t->arg);
    if (t->exited) xSemaphoreGive(t->exited);
    vTaskDelete(NULL);
}

static int es_thread_spawn(bpvm_platform_thread_handle_t* t,
                           bpvm_thread_entry_t entry, void* arg,
                           BaseType_t core) {
    if (!t || !entry) return -1;
    es_thread_t* et = (es_thread_t*) pvPortMalloc(sizeof(es_thread_t));
    if (!et) return -1;
    et->exited = xSemaphoreCreateBinary();
    if (!et->exited) { vPortFree(et); return -1; }
    et->entry = entry;
    et->arg = arg;
    BaseType_t r = xTaskCreatePinnedToCore(es_thread_trampoline, "bpvm-thread",
                                           BPVM_ESP32_THREAD_STACK_BYTES,
                                           et, tskIDLE_PRIORITY + 1,
                                           &et->task, core);
    if (r != pdPASS) {
        vSemaphoreDelete(et->exited);
        vPortFree(et);
        return -1;
    }
    *t = (void*) et;
    return 0;
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                 bpvm_thread_entry_t entry, void* arg) {
    return es_thread_spawn(t, entry, arg, tskNO_AFFINITY);
}

/* En ESP32-S3 el pin a core SÍ es posible (SMP nativo). core_id válido
 * → fija ahí; fuera de rango → sin afinidad. Aquí, a diferencia de la
 * Pico, el dual-core viene "gratis" (#153 no aplica). */
int bpvm_platform_thread_create_pinned(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        int core_id) {
    BaseType_t core = (core_id >= 0 && core_id < configNUMBER_OF_CORES)
                    ? (BaseType_t) core_id : tskNO_AFFINITY;
    return es_thread_spawn(t, entry, arg, core);
}

void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) {
    if (!t || !*t) return;
    es_thread_t* et = (es_thread_t*) *t;
    xSemaphoreTake(et->exited, portMAX_DELAY);
    vSemaphoreDelete(et->exited);
    vPortFree(et);
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
    /* esp_timer es un timer hardware de 64-bit en µs, monotónico e
     * inmune a cambios de frecuencia de CPU — análogo a time_us_64()
     * del SDK Pico. */
    return (int64_t) (esp_timer_get_time() / 1000LL);
}

void bpvm_platform_busy_wait_us(int us) {
    if (us <= 0) return;
    /* esp_rom_delay_us gira sin ceder al scheduler — para timings
     * críticos (setup/hold, bit-bang). */
    esp_rom_delay_us((uint32_t) us);
}
