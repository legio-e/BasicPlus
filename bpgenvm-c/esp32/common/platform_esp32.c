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
#include "afinidad_esp32.h"   /* V7/H12: el núcleo de las tareas (`vm.cores=1`) */

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
 * V7/H12, etapa 2b (5-oct): la COMÚN a las tres familias (la Pico, la ESP32 y la STM32), en vez de una copia. La de
 * antes —un semáforo binario y una cuenta de esperadores— perdía avisos con dos esperando (un `broadcast` despertaba
 * a uno) y dejaba que un tercero se llevara el aviso de otro: con dos runs, un run dormido para siempre. El porqué y
 * el cómo, allí; la prueba, test/test_frcond.c (make test-frcond), con el kernel de FreeRTOS de verdad. */
#include "platform_freertos_cond.inc"

#if defined(BPVM_AOT_POR_TAREA)
/* V7/H12 (N1) — EL CONTEXTO DE LAS NATIVES DE CADA TAREA (bpvm_internal.h, bpvm_aot_contexto_t): un puntero en la zona
 * TLS de la tarea —el IDF le da una a cada una en su pila (uxInitialiseStackTLS), así que `__thread` es por tarea—; 4
 * bytes en cada tarea. Lo ponen cada run y cada ejecutor al empezar; las demás, NULL, usan el de siempre. */
struct bpvm_aot_contexto;
static __thread struct bpvm_aot_contexto* t_aot_contexto;
struct bpvm_aot_contexto* bpvm_platform_aot_contexto(void) { return t_aot_contexto; }
void bpvm_platform_aot_contexto_poner(struct bpvm_aot_contexto* c) { t_aot_contexto = c; }
#endif

/* ============================== Thread ===========================
 * ESP-IDF FreeRTOS es SMP (2 cores en el S3 y la P4, encendidos desde
 * V7/H12). xTaskCreatePinnedToCore con tskNO_AFFINITY = sin pin (cualquier
 * core); la de nuestras tareas la da esp32_afinidad() (afinidad_esp32.h:
 * con `vm.cores=1`, el 0). El stack va en BYTES. */

#define BPVM_ESP32_THREAD_STACK_BYTES 4096

/* V7/H8 #495 (paso 4) — LA PILA DE `io`, 6 KB y no la de un hilo cualquiera. Desde el paso 4, `io` corre durante un
 * RUN el GET y el LIST troceados —el recorrido del FS, el examen de E1, la lectura por el cursor— además de la salida
 * del programa. Medido en placa (1-oct, `ioStackFree` del INFO, con un GET de 40 KB, un STAT con CRC y un LIST
 * troceados durante un RUN): con 4 KB le sobraban 332 B en la S3 (los marcos de Xtensa) y 700 en la P4; en la Pico y
 * la DK2, con el mismo código en ARM, 1208. Aquí la RAM no aprieta; los 2 KB, sólo mientras dura el RUN. */
#define BPVM_ESP32_IO_STACK_BYTES 6144

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

/* V7/H5 P2b.3 — con su nombre y su tamaño de pila (el ejecutor de una native los pide suyos). */
static int es_thread_spawn_pila(bpvm_platform_thread_handle_t* t,
                                bpvm_thread_entry_t entry, void* arg,
                                BaseType_t core, UBaseType_t prio,
                                const char* nombre, uint32_t pila_bytes);

static int es_thread_spawn_prio(bpvm_platform_thread_handle_t* t,
                                bpvm_thread_entry_t entry, void* arg,
                                BaseType_t core, UBaseType_t prio) {
    return es_thread_spawn_pila(t, entry, arg, core, prio, "bpvm-thread",
                                BPVM_ESP32_THREAD_STACK_BYTES);
}

static int es_thread_spawn_pila(bpvm_platform_thread_handle_t* t,
                                bpvm_thread_entry_t entry, void* arg,
                                BaseType_t core, UBaseType_t prio,
                                const char* nombre, uint32_t pila_bytes) {
    if (!t || !entry) return -1;
    es_thread_t* et = (es_thread_t*) pvPortMalloc(sizeof(es_thread_t));
    if (!et) return -1;
    et->exited = xSemaphoreCreateBinary();
    if (!et->exited) { vPortFree(et); return -1; }
    et->entry = entry;
    et->arg = arg;
    BaseType_t r = xTaskCreatePinnedToCore(es_thread_trampoline, nombre,
                                           pila_bytes,
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
 * Sin afinidad: en el S3 y el P4 el planificador la pone donde no moleste (como
 * en la Pico, validada asi en H12: `io` en el otro nucleo apenas se nota); con
 * `vm.cores=1`, en el 0 con las demas (esp32_afinidad). */
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
    return es_thread_spawn_pila(t, entry, arg, esp32_afinidad(), (int) uxTaskPriorityGet(NULL),
                                "bpvm-io", BPVM_ESP32_IO_STACK_BYTES);   /* #495 (paso 4): su pila, medida */
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                 bpvm_thread_entry_t entry, void* arg) {
    return es_thread_spawn(t, entry, arg, esp32_afinidad());
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

/* ============================== V7/H5 P2b.3 ======================
 * EL TESTIGO Y EL HILO DEL EJECUTOR DE NATIVES (bpvm_platform.h). */

int bpvm_platform_sem_init(bpvm_platform_sem_handle_t* s) {
    if (!s) return -1;
    SemaphoreHandle_t x = xSemaphoreCreateBinary();   /* nace a 0 */
    if (!x) return -1;
    *s = (void*) x;
    return 0;
}

void bpvm_platform_sem_destroy(bpvm_platform_sem_handle_t* s) {
    if (!s || !*s) return;
    vSemaphoreDelete((SemaphoreHandle_t) *s);
    *s = NULL;
}

void bpvm_platform_sem_give(bpvm_platform_sem_handle_t* s) {
    xSemaphoreGive((SemaphoreHandle_t) *s);   /* ya dado: se queda dado (binario) */
}

void bpvm_platform_sem_take(bpvm_platform_sem_handle_t* s) {
    xSemaphoreTake((SemaphoreHandle_t) *s, portMAX_DELAY);
}

/* El IDF reserva la pila de una tarea en la RAM INTERNA (su heap de FreeRTOS es MALLOC_CAP_INTERNAL): justo lo
 * que hace falta, porque con la pila en la PSRAM no se puede escribir la flash (spi_flash lo comprueba con un
 * assert). A la prioridad de la tarea de la VM, como `io`, y con su afinidad, como `io`. */
int bpvm_platform_thread_create_nativa(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        uint32_t pila_bytes) {
    return es_thread_spawn_pila(t, entry, arg, esp32_afinidad(), uxTaskPriorityGet(NULL),
                                "bpvm-nat", pila_bytes);
}

int32_t bpvm_platform_pila_libre_actual(void) {
    /* En el IDF la pila se cuenta en BYTES (StackType_t es uint8_t): el producto no cambia nada, y vale igual si
     * algún día no lo fuera. */
    return (int32_t) (uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
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

int64_t bpvm_platform_now_us(void) { return (int64_t) esp_timer_get_time(); }

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
