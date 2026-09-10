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
#include "esp_random.h"    /* esp_random       -> random_u32 (#347) */

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

    /* V6/A1.2 - REDONDEAR HACIA ARRIBA, NUNCA A CERO.
     *
     * `pdMS_TO_TICKS(ms)` es una division entera: con el tick a 100 Hz, CUALQUIER
     * espera menor de 10 ms da 0 ticks, y `xSemaphoreTake(sem, 0)` no espera - es
     * un sondeo. Quien pidio "espera hasta N ms" se encuentra con que no espera
     * nada y gira en vacio.
     *
     * Lo destapo el hilo `io` en la C6 (5-sep): su lazo pide 5 ms entre trago y
     * trago, se los daban a cero, y el hilo consumia el 100 % de un nucleo que
     * compartia con la VM. Medido: fib(28) paso de 23 400 ms a 140 600. El
     * diseno no tenia la culpa; la tenia esta division.
     *
     * Un tick es el minimo que este reloj sabe esperar, asi que es lo correcto:
     * "hasta N ms" nunca puede significar "nada". */
    TickType_t ticks = 0;
    if (ms > 0) {
        ticks = pdMS_TO_TICKS(ms);
        if (ticks == 0) ticks = 1;
    }
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

static int es_thread_spawn_prio(bpvm_platform_thread_handle_t* t,
                                bpvm_thread_entry_t entry, void* arg,
                                BaseType_t core, UBaseType_t prio);

static int es_thread_spawn(bpvm_platform_thread_handle_t* t,
                           bpvm_thread_entry_t entry, void* arg,
                           BaseType_t core) {
    return es_thread_spawn_prio(t, entry, arg, core, tskIDLE_PRIORITY + 1);
}

static int es_thread_spawn_prio(bpvm_platform_thread_handle_t* t,
                                bpvm_thread_entry_t entry, void* arg,
                                BaseType_t core, UBaseType_t prio) {
    if (!t || !entry) return -1;
    es_thread_t* et = (es_thread_t*) pvPortMalloc(sizeof(es_thread_t));
    if (!et) return -1;
    et->exited = xSemaphoreCreateBinary();
    if (!et->exited) { vPortFree(et); return -1; }
    et->entry = entry;
    et->arg = arg;
    BaseType_t r = xTaskCreatePinnedToCore(es_thread_trampoline, "bpvm-thread",
                                           BPVM_ESP32_THREAD_STACK_BYTES,
                                           et, prio,
                                           &et->task, core);
    if (r != pdPASS) {
        vSemaphoreDelete(et->exited);
        vPortFree(et);
        return -1;
    }
    *t = (void*) et;
    return 0;
}

/* V6/A1 - el hilo `io`, a la MISMA prioridad que la tarea que ejecuta la VM (nunca
 * por debajo, y tampoco por encima: ver el porque en bpvm_platform.h). En ESP-IDF esa
 * es la tarea `main` (CONFIG_ESP_MAIN_TASK_PRIO, 1 por defecto), asi que aqui va
 * a 1: `io` atiende el wire aunque la VM este calculando, y como se bloquea en su
 * cola en cuanto no hay nada, no le quita tiempo (medido: fib(28) igual).
 * Sin afinidad a proposito: en el S3 y el P4 el planificador la pondra donde no
 * moleste, y fijarla al nucleo de I/O es cosa de A1.3 con las placas delante. */
int bpvm_platform_thread_create_io(bpvm_platform_thread_handle_t* t,
                                    bpvm_thread_entry_t entry, void* arg) {
    /* #485 - `io` NO LLEVA NUMERO: hereda el de quien lo arranca.
     *
     * El contrato dice «io a la MISMA prioridad que la tarea que ejecuta la VM,
     * nunca por debajo» (bpvm_platform.h), y hasta hoy eso se VIGILABA: habia
     * un literal aqui y otro donde cada familia crea su tarea de VM, y tenian
     * que coincidir a mano. En el P4 dejaron de coincidir -su VM corre en
     * wire_task a prioridad 5 y esto ponia io a 1- sin que nada lo dijera.
     *
     * bpvm_io_start() lo llama SIEMPRE la tarea que ejecuta la VM (los cuatro
     * repl lo hacen desde el camino del RUN), asi que uxTaskPriorityGet(NULL)
     * ES la prioridad de la VM. Con esto el contrato pasa de vigilado a
     * CONSTRUIDO: no hay dos numeros que puedan divergir. */
    return es_thread_spawn_prio(t, entry, arg, tskNO_AFFINITY,
                                (int) uxTaskPriorityGet(NULL));
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
    /* #473 (R2) - EL MISMO REDONDEO QUE cond_timed_wait, que aqui faltaba.
     * pdMS_TO_TICKS es una division entera: con el tick a 100 Hz -las CUATRO
     * variantes ESP lo tienen asi- CUALQUIER espera de 1..9 ms da 0 ticks, y
     * vTaskDelay(0) no duerme: hace un yield. O sea que `sleep(5)` desde BP no
     * esperaba. El consumidor es el planificador de hilos BP (scheduler.c:145).
     *
     * El arreglo estaba desde el 5-sep en cond_timed_wait de las TRES copias y
     * en el sleep del comun; a este no viajo. Cuarta vez que este fallo aparece
     * con la misma forma. */
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0) ticks = 1;
    vTaskDelay(ticks);
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

/* #347 — 32 bits al azar. El ESP32 trae RNG de hardware: esp_random(). Nada de
 * PRNG ni de semilla aquí.
 *
 * Matiz que conviene tener escrito: esp_random() es azar de VERDAD mientras el
 * subsistema de radio (WiFi/BT) esté arrancado, porque de ahí sale el ruido.
 * Sin radio degrada a pseudoaleatorio. Para lo que un programa BP pide a
 * random() da igual; si algún día se usara para algo criptográfico, NO vale —
 * y ahí habría que exigir la radio encendida o usar otra fuente. */
uint32_t bpvm_platform_random_u32(void) {
    return esp_random();
}
