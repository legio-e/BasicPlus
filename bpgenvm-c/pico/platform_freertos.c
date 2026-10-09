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

#include "pico/time.h"   /* busy_wait_us — bpvm_platform_busy_wait_us */
#include "pico/rand.h"   /* get_rand_32 — bpvm_platform_random_u32 (#347) */

#include "flash_lock.h"  /* #153 — registro de core víctima de lockout */
#include "nucleos.h"     /* V7/H12, etapa 3.1: `vm.cores=1`, cada tarea al núcleo 0 */
#include "bpvm_alloc.h"  /* V7/H5 P2b.3: la pila del ejecutor de natives, de malloc */

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
/* V7/H12 (N1) — EL CONTEXTO DE LAS NATIVES DE CADA TAREA (bpvm_internal.h, bpvm_aot_contexto_t), en su puntero por tarea
 * de FreeRTOS (el 0: FreeRTOSConfig.h): lo ponen cada run y cada ejecutor al empezar; las demás tareas, NULL, usan el
 * de siempre. Antes de arrancar el planificador no hay tarea a la que preguntar: NULL. */
struct bpvm_aot_contexto;
struct bpvm_aot_contexto* bpvm_platform_aot_contexto(void) {
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) return NULL;
    return (struct bpvm_aot_contexto*) pvTaskGetThreadLocalStoragePointer(NULL, 0);
}
void bpvm_platform_aot_contexto_poner(struct bpvm_aot_contexto* c) {
    vTaskSetThreadLocalStoragePointer(NULL, 0, c);
}
#endif

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
    int                 lockout_victim;  /* #153: registrar este core como
                                          * víctima de flash lockout antes
                                          * de correr entry (solo dual-core,
                                          * core != 0). */
    /* V7/H5 P2b.3 — sólo el ejecutor de una native: su TCB y su pila, de malloc (NULL en los demás). */
    void*               tcb;
    void*               pila;
} fr_thread_t;

static void fr_thread_trampoline(void* pv) {
    fr_thread_t* t = (fr_thread_t*) pv;
    /* #153 — Si este task corre en un core que NO escribe flash (core 1),
     * lo registramos como víctima de lockout AHORA, ya en su core, antes
     * de empezar a ejecutar el interp. Así, cuando el core 0 vaya a
     * escribir flash (SAVE / log), puede parquearlo en RAM. No-op en
     * single-core. Se ejecuta una vez, en el contexto del propio core. */
    if (t->lockout_victim) {
        pico_flash_lock_init_victim();
    }
    t->entry(t->arg);
    /* #635 (3-oct) — lo que se mira DESPUÉS del aviso se lee ANTES. En cuanto `exited` se da, join puede correr (con
     * el reparto por tiempo basta un tick en esta ventana) y hacer vPortFree(t): leer `t->tcb` después era leer
     * memoria liberada —y si el bloque se reutilizaba con algo no nulo ahí, la tarea se suspendía en vez de borrarse
     * y su pila no volvía—. Las ESP32 y la STM32 no leen nada tras el aviso. */
    int es_nativa = (t->tcb != NULL);
    /* Marcar como terminado para join. */
    if (t->exited) xSemaphoreGive(t->exited);
    /* P2b.3 — el ejecutor de una native tiene la memoria de su tarea en nuestras manos: no puede borrarse a sí
     * mismo (su TCB seguiría en la lista de terminadas hasta que corriera la ociosa, y join lo liberaría antes).
     * Se SUSPENDE y join lo borra: borrar OTRA tarea la saca de las listas del núcleo en el acto. */
    if (es_nativa) {
        vTaskSuspend(NULL);
        for (;;) { vTaskDelay(portMAX_DELAY); }   /* por si alguien la reanudara */
    }
    /* El task se borra a sí mismo. join libera la estructura. */
    vTaskDelete(NULL);
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                 bpvm_thread_entry_t entry, void* arg) {
    if (!t || !entry) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    memset(ft, 0, sizeof *ft);   /* P2b.3: join mira tcb/pila; y lockout_victim, que `io` no ponía */
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry = entry;
    ft->arg = arg;
    ft->lockout_victim = 0;     /* sin afinidad → no asumimos core víctima */
    BaseType_t r = xTaskCreate(fr_thread_trampoline, "bpvm-thread",
                                1024,   /* words → 4 KB stack */
                                ft, tskIDLE_PRIORITY + 1, &ft->task);
    if (r != pdPASS) {
        vSemaphoreDelete(ft->exited);
        vPortFree(ft);
        return -1;
    }
    pico_tarea_nucleos(ft->task);   /* V7/H12, etapa 3.1 */
    *t = (void*) ft;
    return 0;
}

/* Variante pinned. Bajo configNUMBER_OF_CORES=1 (el bring-up actual)
 * FreeRTOS-SMP no está activo, así que la afinidad NO se puede aplicar
 * — caemos a la variante normal y core_id se ignora.
 *
 * Cuando #153 P-smp-tx-exclusive cierre y configNUMBER_OF_CORES=2
 * se active, este wrapper hará xTaskCreateAffinitySet (o
 * vTaskCoreAffinitySet tras xTaskCreate) con la máscara correspondiente.
 * El call site (scheduler_smp.c, comm_pico.c) no cambia.
 */
/* V6/A1.4 - el hilo `io`, a la MISMA prioridad que `vm_task` (que corre a
 * tskIDLE_PRIORITY+2, ver pico/main.c). Con la creacion corriente nacia en +1, o
 * sea por debajo, y NO SE EJECUTABA mientras la VM calculaba: un KILL enviado a
 * mitad de un calculo no llegaba hasta que el programa acababa solo (medido el
 * 5-sep: 9,9 s y EXITED OK en vez de KILLED). Con +2 el reparto por tiempo le da
 * turno (KILL en 33 ms) sin expulsar a la VM en cada `print`: con +3 funcionaba
 * igual de bien pero PrintBench subia de 3 958 a 4 832 ms. Atiende el wire y
 * vuelve a bloquearse en su cola, asi que no le roba tiempo al interprete.
 * 4 KB de pila: el sink arma el evento OUTPUT en pila. */
int bpvm_platform_thread_create_io(bpvm_platform_thread_handle_t* t,
                                    bpvm_thread_entry_t entry, void* arg) {
    if (!t || !entry) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    memset(ft, 0, sizeof *ft);   /* P2b.3: join mira tcb/pila; y lockout_victim, que `io` no ponía */
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry = entry;
    ft->arg = arg;
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
    BaseType_t r = xTaskCreate(fr_thread_trampoline, "bpvm-io", 1024,
                               ft, uxTaskPriorityGet(NULL), &ft->task);
    if (r == pdPASS) pico_tarea_nucleos(ft->task);   /* V7/H12, etapa 3.1 */
    if (r != pdPASS) {
        vSemaphoreDelete(ft->exited);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
}

int bpvm_platform_thread_create_pinned(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        int core_id) {
#if (configNUMBER_OF_CORES > 1) && (configUSE_CORE_AFFINITY == 1)
    if (!t || !entry) return -1;
    if (core_id < 0 || core_id >= configNUMBER_OF_CORES) {
        return bpvm_platform_thread_create(t, entry, arg);
    }
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    memset(ft, 0, sizeof *ft);   /* P2b.3: join mira tcb/pila; y lockout_victim, que `io` no ponía */
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry = entry;
    ft->arg = arg;
    /* Core 0 escribe flash (REPL/comm/SAVE) y sirve USB; cualquier core
     * != 0 (worker) debe registrarse como víctima de lockout para que
     * core 0 pueda parquearlo durante un erase/program. */
    ft->lockout_victim = (core_id != 0);
    UBaseType_t mask = (UBaseType_t) 1U << core_id;
    BaseType_t r = xTaskCreateAffinitySet(fr_thread_trampoline,
                                           "bpvm-thr-pin",
                                           1024, ft,
                                           tskIDLE_PRIORITY + 1,
                                           mask, &ft->task);
    if (r != pdPASS) {
        vSemaphoreDelete(ft->exited);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
#else
    /* Single-core: nada que pinear. Delegamos a la API normal. */
    (void) core_id;
    return bpvm_platform_thread_create(t, entry, arg);
#endif
}

void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) {
    if (!t || !*t) return;
    fr_thread_t* ft = (fr_thread_t*) *t;
    xSemaphoreTake(ft->exited, portMAX_DELAY);
    vSemaphoreDelete(ft->exited);
    if (ft->tcb) {   /* P2b.3 — el ejecutor de una native: se borra aquí y se devuelve su memoria (ver el trampolín) */
#if ( configNUMBER_OF_CORES > 1 )
        /* V7/H12 3.1, paso 5 — con dos núcleos, el aviso llega con el ejecutor TODAVÍA corriendo en el otro (entre darlo
         * y suspenderse). Borrar una tarea que corre la deja a la tarea ociosa —su TCB, en la lista de las terminadas—,
         * y devolver su memoria sería pisar una pila en uso y una lista del núcleo. Se espera a que esté suspendida y
         * fuera de los dos núcleos: así ya no vuelve a correr, y borrarla es en el acto. (Sólo al acabar el programa:
         * esperar un tick no se nota.) */
        while (eTaskGetState(ft->task) != eSuspended
               || xTaskGetCurrentTaskHandleForCore(0) == ft->task
               || xTaskGetCurrentTaskHandleForCore(1) == ft->task)
            vTaskDelay(1);
#endif
        vTaskDelete(ft->task);
        bpvm_free(ft->tcb);
        bpvm_free(ft->pila);
    }
    vPortFree(ft);
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

/* Tarea ESTÁTICA con su TCB y su pila de malloc: el heap de FreeRTOS (32 KB) está dimensionado para `vm_task`
 * e `io`, y una pila de ejecutor no cabe en él; así sólo cuesta mientras existe. A la prioridad de quien la crea
 * —la tarea de la VM—, como `io`. */
int bpvm_platform_thread_create_nativa(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        uint32_t pila_bytes) {
    if (!t || !entry || pila_bytes < configMINIMAL_STACK_SIZE * sizeof(StackType_t)) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    memset(ft, 0, sizeof *ft);
    ft->exited = xSemaphoreCreateBinary();
    ft->entry  = entry;
    ft->arg    = arg;
    ft->tcb    = bpvm_malloc_os(sizeof(StaticTask_t));
    ft->pila   = bpvm_malloc_os(pila_bytes);
    if (ft->exited && ft->tcb && ft->pila)
        ft->task = xTaskCreateStatic(fr_thread_trampoline, "bpvm-nat",
                                     (configSTACK_DEPTH_TYPE) (pila_bytes / sizeof(StackType_t)), ft,
                                     uxTaskPriorityGet(NULL), (StackType_t*) ft->pila, (StaticTask_t*) ft->tcb);
    pico_tarea_nucleos(ft->task);   /* V7/H12, etapa 3.1 (NULL: nada) */
    if (ft->task == NULL) {
        if (ft->exited) vSemaphoreDelete(ft->exited);
        if (ft->tcb)  bpvm_free(ft->tcb);
        if (ft->pila) bpvm_free(ft->pila);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
}

int32_t bpvm_platform_pila_libre_actual(void) {
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
    /* IMPORTANTE: NO usamos xTaskGetTickCount() porque el SysTick de
     * FreeRTOS deriva de clk_sys. Si el código BP cambia clk_sys con
     * Pico.setCpuFreqMHz(), el tick FreeRTOS corre a una freq distinta
     * a la configurada (1000 Hz) y el "tiempo" reportado se desajusta
     * proporcionalmente — un fibo(30) a 200 MHz parecería tardar
     * 150/200 del tiempo real.
     *
     * En su lugar usamos `time_us_64()` del SDK Pico, que lee el timer
     * hardware del RP2350 (registros TIMER0). Ese timer corre a 1 MHz
     * fijos derivados de clk_ref/XOSC (12 MHz) — INMUNE al overclock
     * de clk_sys. Misma fuente que `busy_wait_us()` usa internamente,
     * así que sleep* y now() permanecen coherentes entre sí. */
    return (int64_t) (time_us_64() / 1000ULL);
}

int64_t bpvm_platform_now_us(void) { return (int64_t) time_us_64(); }

/* V7 #592 (9-oct) — EL 2º TEMPORIZADOR: el gancho del tick de FreeRTOS (configUSE_TICK_HOOK = 1, FreeRTOSConfig.h), que
 * cuenta hasta 5 ms y sube la marca de las natives; ya está en marcha, así que arrancarlo es no hacer nada. (Con el
 * reloj del CPU cambiado —Pico.setCpuFreqMHz— el tick corre a otro paso, como dice now_ms arriba: la marca sube algo
 * antes o después; quien decide si ceder, punto_control, mira el reloj de verdad.) */
void bpvm_platform_reloj_marca(void) { }
void vApplicationTickHook(void) { bpvm_marca_tiempo_tick((uint32_t) portTICK_PERIOD_MS); }

void bpvm_platform_busy_wait_us(int us) {
    if (us <= 0) return;
    /* busy_wait_us() del Pico SDK lee el timer hardware del RP2350 y
     * gira hasta que pasan los us pedidos. NO cede CPU al scheduler
     * de FreeRTOS — el resto de tasks BP no corren durante el wait.
     * Es lo deseado para timings críticos (setup/hold de chips,
     * bit-bang fino). */
    busy_wait_us((uint64_t) us);
}

/* #347 — 32 bits al azar. El RP2350 SÍ trae fuente de hardware: get_rand_32()
 * del SDK. En el RP2350 se alimenta del TRNG del chip (el RP2040 sólo tenía el
 * jitter del ROSC), así que aquí no hace falta PRNG ni semilla. */
uint32_t bpvm_platform_random_u32(void) {
    return get_rand_32();
}
