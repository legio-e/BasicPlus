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
